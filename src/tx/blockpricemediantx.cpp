// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockpricemediantx.h"
#include "main.h"

bool CBlockPriceMedianTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {
    // TODO: txUid == miner?
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    return true;
}

/**
 *  force settle/liquidate any under-collateralized CDP (collateral ratio <= 104%)
 */
bool CBlockPriceMedianTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    uint64_t slideWindowBlockCount;
    if (!cw.sysParamCache.GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::CheckTx, read MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT error"),
                         READ_SYS_PARAM_FAIL, "read-sysparamdb-err");
    }

    map<CoinPricePair, uint64_t> mapMedianPricePoints;
    if (!cw.ppCache.GetBlockMedianPricePoints(height, slideWindowBlockCount, mapMedianPricePoints)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, failed to get block median price points"),
                         READ_PRICE_POINT_FAIL, "bad-read-price-points");
    }

    if (mapMedianPricePoints != median_price_points) {
        string pricePoints;
        for (const auto item : mapMedianPricePoints) {
            pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first,
                                     item.first.second, item.second);
        };

        LogPrint("ERROR", "CBlockPriceMedianTx::ExecuteTx, from cache, height: %d, price points: %s\n", height, pricePoints);

        pricePoints.clear();
        for (const auto item : median_price_points) {
            pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first,
                                     item.first.second, item.second);
        };

        LogPrint("ERROR", "CBlockPriceMedianTx::ExecuteTx, from median tx, height: %d, price points: %s\n", height, pricePoints);

        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, invalid median price points"), REJECT_INVALID,
                         "bad-median-price-points");
    }

    do {

        // 0. acquire median prices
        uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount);
        if (bcoinMedianPrice == 0) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, failed to acquire bcoin median price\n");
            break;
        }

        uint64_t fcoinMedianPrice = cw.ppCache.GetFcoinMedianPrice(height, slideWindowBlockCount);
        if (fcoinMedianPrice == 0) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, failed to acquire fcoin median price\n");
            break;
        }

        // 1. Check Global Collateral Ratio floor & Collateral Ceiling if reached
        uint64_t globalCollateralRatioFloor = 0;
        if (!cw.sysParamCache.GetParam(SysParamType::GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read global collateral ratio floor error"),
                            READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
        }

        // check global collateral ratio
        if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(bcoinMedianPrice, globalCollateralRatioFloor)) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, GlobalCollateralFloorReached!!\n");
            break;
        }

        // 2. get all CDPs to be force settled
        set<CUserCDP> forceLiquidateCDPList;
        uint64_t forceLiquidateRatio = 0;
        if (!cw.sysParamCache.GetParam(SysParamType::CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read force liquidate ratio error"),
                            READ_SYS_PARAM_FAIL, "read-force-liquidate-ratio-error");
        }
        cw.cdpCache.cdpMemCache.GetCdpListByCollateralRatio(forceLiquidateRatio, bcoinMedianPrice, forceLiquidateCDPList);

        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, globalCollateralRatioFloor: %llu, bcoinMedianPrice: %llu, "
                "forceLiquidateRatio: %llu, forceLiquidateCDPList: %llu\n",
                globalCollateralRatioFloor, bcoinMedianPrice, forceLiquidateRatio, forceLiquidateCDPList.size());

        // 3. force settle each cdp
        if (forceLiquidateCDPList.size() == 0) {
            break;
        }

        int32_t cdpIndex = 0;
        CAccount fcoinGenesisAccount;
        cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
        uint64_t currRiskReserveScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
        string orderIdFactor           = GetHash().GetHex();
        uint32_t orderIndex            = 0;
        for (auto cdp : forceLiquidateCDPList) {
            if (++cdpIndex > kForceSettleCDPMaxCountPerBlock)
                break;

            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, begin to force settle CDP (%s), currRiskReserveScoins: %llu\n",
                    cdp.ToString(), currRiskReserveScoins);

            // Suppose we have 120 (owed scoins' amount), 30, 50 three cdps, but current risk reserve scoins is 100,
            // then skip the 120 cdp and settle the 30 and 50 cdp.
            if (currRiskReserveScoins < cdp.total_owed_scoins) {
                LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, currRiskReserveScoins(%lu) < cdp.total_owed_scoins(%lu) !!\n",
                        currRiskReserveScoins, cdp.total_owed_scoins);
                continue;
            }

            // a) sell WICC for WUSD to return to risk reserve pool
            // send bcoin from cdp to fcoin genesis account
            if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, cdp.total_staked_bcoins)) {
                return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, operate balance failed"),
                                 UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
            }
            // should freeze user's asset for selling
            if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::FREEZE, cdp.total_staked_bcoins)) {
                return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, account has insufficient funds"),
                                    UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
            }
            auto pBcoinSellMarketOrder = CDEXSysOrder::CreateSellMarketOrder(
                CTxCord(height, index), SYMB::WUSD, SYMB::WICC, cdp.total_staked_bcoins);
            string bcoinSellMarketOrderId = orderIdFactor + std::to_string(orderIndex ++);
            if (!cw.dexCache.CreateActiveOrder(uint256S(bcoinSellMarketOrderId), *pBcoinSellMarketOrder)) {
                return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, create sys order for SellBcoinForScoin (%s) failed",
                                pBcoinSellMarketOrder->ToString()), CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
            }

            // b) inflate WGRT coins and sell them for WUSD to return to risk reserve pool if necessary
            uint64_t bcoinsValueInScoin = uint64_t(double(cdp.total_staked_bcoins) * bcoinMedianPrice / kPercentBoost);
            if (bcoinsValueInScoin >= cdp.total_owed_scoins) {  // 1 ~ 1.04
                LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, Force settled CDP: "
                    "Placed BcoinSellMarketOrder: %s, orderId: %s\n"
                    "No need to infate WGRT coins: %llu vs %llu\n"
                    "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                    pBcoinSellMarketOrder->ToString(), uint256S(bcoinSellMarketOrderId).GetHex(),
                    bcoinsValueInScoin, cdp.total_owed_scoins,
                    currRiskReserveScoins,
                    currRiskReserveScoins - cdp.total_owed_scoins);
            } else {  // 0 ~ 1
                uint64_t fcoinsValueToInflate = cdp.total_owed_scoins - bcoinsValueInScoin;
                assert(fcoinMedianPrice != 0);
                uint64_t fcoinsToInflate = fcoinsValueToInflate * kPercentBoost / fcoinMedianPrice;
                // inflate fcoin to fcoin genesis account
                if (!fcoinGenesisAccount.OperateBalance(SYMB::WGRT, BalanceOpType::ADD_FREE, fcoinsToInflate)) {
                    return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, operate balance failed"),
                                     UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
                }
                // should freeze user's asset for selling
                if (!fcoinGenesisAccount.OperateBalance(SYMB::WGRT, BalanceOpType::FREEZE, fcoinsToInflate)) {
                    return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, account has insufficient funds"),
                                     UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
                }

                auto pFcoinSellMarketOrder =
                    CDEXSysOrder::CreateSellMarketOrder(CTxCord(height, index), SYMB::WUSD, SYMB::WGRT, fcoinsToInflate);
                string fcoinSellMarketOrderId = orderIdFactor + std::to_string(orderIndex ++);
                if (!cw.dexCache.CreateActiveOrder(uint256S(fcoinSellMarketOrderId), *pFcoinSellMarketOrder)) {
                    return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, create sys order for SellFcoinForScoin (%s) failed",
                            pFcoinSellMarketOrder->ToString()), CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
                }

                LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, Force settled CDP: "
                    "Placed BcoinSellMarketOrder:  %s, orderId: %s\n"
                    "Placed FcoinSellMarketOrder:  %s, orderId: %s\n"
                    "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                    pBcoinSellMarketOrder->ToString(), uint256S(bcoinSellMarketOrderId).GetHex(),
                    pFcoinSellMarketOrder->ToString(), uint256S(fcoinSellMarketOrderId).GetHex(),
                    currRiskReserveScoins,
                    currRiskReserveScoins - cdp.total_owed_scoins);
            }

            // c) Close the CDP
            const CUserCDP &oldCDP = cdp;
            cw.cdpCache.EraseCDP(oldCDP, cdp);

            // d) minus scoins from the risk reserve pool to repay CDP scoins
            currRiskReserveScoins -= cdp.total_owed_scoins;
        }

        // 4. update fcoin genesis account
        uint64_t prevScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
        assert(prevScoins >= currRiskReserveScoins);
        fcoinGenesisAccount.OperateBalance(SYMB::WUSD, SUB_FREE, prevScoins - currRiskReserveScoins);
        cw.accountCache.SaveAccount(fcoinGenesisAccount);
    } while (false);

    return true;
}

string CBlockPriceMedianTx::ToString(CAccountDBCache &accountCache) {
    string pricePoints;
    for (const auto item : median_price_points) {
        pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first, item.first.second,
                                 item.second);
    }

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, median_price_points=%s, valid_height=%d\n",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, txUid.ToString(), llFees, pricePoints,
                     valid_height);
}

Object CBlockPriceMedianTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    Array pricePointArray;
    for (const auto &item : median_price_points) {
        Object subItem;
        subItem.push_back(Pair("coin_symbol",     item.first.first));
        subItem.push_back(Pair("price_symbol",    item.first.second));
        subItem.push_back(Pair("price",           item.second));
        pricePointArray.push_back(subItem);
    }
    result.push_back(Pair("median_price_points",   pricePointArray));

    return result;
}

map<CoinPricePair, uint64_t> CBlockPriceMedianTx::GetMedianPrice() const { return median_price_points; }
