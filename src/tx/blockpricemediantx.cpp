// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockpricemediantx.h"
#include "main.h"

bool CBlockPriceMedianTx::CheckTx(CTxExecuteContext &context) { return true; }

/**
 *  force settle/liquidate any under-collateralized CDP (collateral ratio <= 104%)
 */
bool CBlockPriceMedianTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    PriceMap medianPrices;
    if (!cw.ppCache.CalcBlockMedianPrices(cw, context.height, medianPrices)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, failed to get block median price points"),
                         READ_PRICE_POINT_FAIL, "bad-read-price-points");
    }

    if (medianPrices != median_prices) {
        string pricePoints;
        for (const auto item : medianPrices) {
            pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first,
                                     item.first.second, item.second);
        }

        LogPrint(BCLog::ERROR, "CBlockPriceMedianTx::ExecuteTx, from cache, height: %d, price points: %s\n", context.height, pricePoints);

        pricePoints.clear();
        for (const auto item : median_prices) {
            pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first,
                                     item.first.second, item.second);
        }

        LogPrint(BCLog::ERROR, "CBlockPriceMedianTx::ExecuteTx, from median tx, height: %d, price points: %s\n", context.height, pricePoints);

        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, invalid median price points"), REJECT_INVALID,
                         "bad-median-price-points");
    }

    if (!cw.blockCache.SetMedianPrices(medianPrices)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, save median prices to db failed"), REJECT_INVALID,
                         "save-median-prices-failed");
    }

    do {

        // 0. acquire median prices
        // TODO: multi stable coin
        uint64_t bcoinMedianPrice = medianPrices[CoinPricePair(SYMB::WICC, SYMB::USD)];
        if (bcoinMedianPrice == 0) {
            LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, failed to acquire bcoin median price\n");
            break;
        }

        uint64_t fcoinMedianPrice = medianPrices[CoinPricePair(SYMB::WGRT, SYMB::USD)];
        if (fcoinMedianPrice == 0) {
            LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, failed to acquire fcoin median price\n");
            break;
        }

        // 1. Check Global Collateral Ratio floor & Collateral Ceiling if reached
        uint64_t globalCollateralRatioFloor = 0;
        if (!cw.sysParamCache.GetParam(SysParamType::CDP_GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read global collateral ratio floor error"),
                            READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
        }

        // check global collateral ratio
        if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(bcoinMedianPrice, globalCollateralRatioFloor)) {
            LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, GlobalCollateralFloorReached!!\n");
            break;
        }

        // 2. get all CDPs to be force settled
        RatioCDPIdCache::Map cdpMap;
        uint64_t forceLiquidateRatio = 0;
        if (!cw.sysParamCache.GetParam(SysParamType::CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read force liquidate ratio error"),
                            READ_SYS_PARAM_FAIL, "read-force-liquidate-ratio-error");
        }

        cw.cdpCache.GetCdpListByCollateralRatio(forceLiquidateRatio, bcoinMedianPrice, cdpMap);

        LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, tx_cord=%d-%d, globalCollateralRatioFloor: %llu, bcoinMedianPrice: %llu, "
                "forceLiquidateRatio: %llu, cdpMap: %llu\n", context.height, context.index,
                globalCollateralRatioFloor, bcoinMedianPrice, forceLiquidateRatio, cdpMap.size());

        // 3. force settle each cdp
        if (cdpMap.size() == 0) {
            break;
        } else {
            // TODO: remove me.
            LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, have %llu cdps to force settle, in detail:\n",
                     cdpMap.size());
            for (const auto &cdpKey : cdpMap) {
                LogPrint(BCLog::CDP, "%s\n", cdpKey.second.ToString());
            }
        }

        NET_TYPE netType = SysCfg().NetworkID();
        if (netType == TEST_NET && context.height < 1800000) { // soft fork to compat old data of testnet
            // TODO: remove me if reset testnet.
            return ForceLiquidateCDPCompat(context, bcoinMedianPrice, fcoinMedianPrice, cdpMap);
        }

        int32_t cdpIndex             = 0;
        uint64_t totalCloseoutScoins = 0;
        uint64_t totalSelloutBcoins  = 0;
        uint64_t totalInflateFcoins  = 0;
        vector<CReceipt> receipts;
        CAccount fcoinGenesisAccount;
        cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
        uint64_t currRiskReserveScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
        for (auto &cdpPair : cdpMap) {
            auto &cdp = cdpPair.second;
            if (++cdpIndex > FORCE_SETTLE_CDP_MAX_COUNT_PER_BLOCK)
                break;

            LogPrint(BCLog::CDP,
                     "CBlockPriceMedianTx::ExecuteTx, begin to force settle CDP (%s), currRiskReserveScoins: %llu, "
                     "index: %u\n",
                     cdp.ToString(), currRiskReserveScoins, cdpIndex);

            // Suppose we have 120 (owed scoins' amount), 30, 50 three cdps, but current risk reserve scoins is 100,
            // then skip the 120 cdp and settle the 30 and 50 cdp.
            if (currRiskReserveScoins < cdp.total_owed_scoins) {
                LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, currRiskReserveScoins(%lu) < cdp.total_owed_scoins(%lu) !!\n",
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
            auto pBcoinSellMarketOrder = dex::CSysOrder::CreateSellMarketOrder(
                CTxCord(context.height, context.index), SYMB::WUSD, SYMB::WICC, cdp.total_staked_bcoins);
            uint256 bcoinSellMarketOrderId = GenOrderId(context, cdp, SYMB::WICC);
            if (!cw.dexCache.CreateActiveOrder(bcoinSellMarketOrderId, *pBcoinSellMarketOrder)) {
                return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, create sys order for SellBcoinForScoin (%s) failed",
                                pBcoinSellMarketOrder->ToString()), CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
            }

            totalSelloutBcoins += cdp.total_staked_bcoins;

            // b) inflate WGRT coins and sell them for WUSD to return to risk reserve pool if necessary
            uint64_t bcoinsValueInScoin = uint64_t(double(cdp.total_staked_bcoins) * bcoinMedianPrice / PRICE_BOOST);
            if (bcoinsValueInScoin >= cdp.total_owed_scoins) {  // 1 ~ 1.04
                LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, Force settled CDP: "
                    "Placed BcoinSellMarketOrder: %s, orderId: %s\n"
                    "No need to infate WGRT coins: %llu vs %llu\n"
                    "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                    pBcoinSellMarketOrder->ToString(), bcoinSellMarketOrderId.GetHex(),
                    bcoinsValueInScoin, cdp.total_owed_scoins,
                    currRiskReserveScoins, currRiskReserveScoins - cdp.total_owed_scoins);
            } else {  // 0 ~ 1
                uint64_t fcoinsValueToInflate = cdp.total_owed_scoins - bcoinsValueInScoin;
                assert(fcoinMedianPrice != 0);
                uint64_t fcoinsToInflate = uint64_t(double(fcoinsValueToInflate) * PRICE_BOOST / fcoinMedianPrice);
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

                auto pFcoinSellMarketOrder = dex::CSysOrder::CreateSellMarketOrder(
                    CTxCord(context.height, context.index), SYMB::WUSD, SYMB::WGRT, fcoinsToInflate);
                uint256 fcoinSellMarketOrderId = GenOrderId(context, cdp, SYMB::WGRT);
                if (!cw.dexCache.CreateActiveOrder(fcoinSellMarketOrderId, *pFcoinSellMarketOrder)) {
                    return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, create sys order for SellFcoinForScoin (%s) failed",
                            pFcoinSellMarketOrder->ToString()), CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
                }

                totalInflateFcoins += fcoinsToInflate;

                LogPrint(BCLog::CDP, "CBlockPriceMedianTx::ExecuteTx, Force settled CDP: "
                    "Placed BcoinSellMarketOrder:  %s, orderId: %s\n"
                    "Placed FcoinSellMarketOrder:  %s, orderId: %s\n"
                    "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                    pBcoinSellMarketOrder->ToString(), bcoinSellMarketOrderId.GetHex(),
                    pFcoinSellMarketOrder->ToString(), fcoinSellMarketOrderId.GetHex(),
                    currRiskReserveScoins, currRiskReserveScoins - cdp.total_owed_scoins);
            }

            // c) Close the CDP
            const CUserCDP &oldCDP = cdp;
            cw.cdpCache.EraseCDP(oldCDP, cdp);
            if (SysCfg().GetArg("-persistclosedcdp", false)) {
                if (!cw.closedCdpCache.AddClosedCdpIndex(oldCDP.cdpid, uint256(), CDPCloseType::BY_FORCE_LIQUIDATE)) {
                    LogPrint(BCLog::ERROR, "persistclosedcdp add failed for force-liquidated cdpid (%s)", oldCDP.cdpid.GetHex());
                }
            }

            // d) minus scoins from the risk reserve pool to repay CDP scoins
            currRiskReserveScoins -= cdp.total_owed_scoins;
            totalCloseoutScoins += cdp.total_owed_scoins;
        }

        // 4. update fcoin genesis account
        uint64_t prevScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
        assert(prevScoins >= currRiskReserveScoins);
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, SUB_FREE, prevScoins - currRiskReserveScoins)) {
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, opeate fcoin genesis account failed"),
                             UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }
        cw.accountCache.SaveAccount(fcoinGenesisAccount);

        if (totalCloseoutScoins > 0) {
            receipts.emplace_back(fcoinGenesisAccount.regid, nullId, SYMB::WUSD, totalCloseoutScoins,
                                  ReceiptCode::CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE);
        }

        if (totalSelloutBcoins > 0) {
            receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WICC, totalSelloutBcoins,
                                  ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE);
        }

        if (totalInflateFcoins > 0) {
            receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WGRT, totalInflateFcoins,
                                  ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE);
        }

        if (!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
            return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, set tx receipts failed!! txid=%s",
                            GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");

    } while (false);

    return true;
}

uint256 CBlockPriceMedianTx::GenOrderId(CTxExecuteContext &context, const CUserCDP &cdp,
        TokenSymbol assetSymbol) {

    CHashWriter ss(SER_GETHASH, 0);
    ss << cdp.cdpid << assetSymbol;
    return ss.GetHash();
}

bool CBlockPriceMedianTx::ForceLiquidateCDPCompat(CTxExecuteContext &context, uint64_t bcoinMedianPrice,
    uint64_t fcoinMedianPrice, RatioCDPIdCache::Map &cdps) {

    int32_t cdpIndex             = 0;
    uint64_t totalCloseoutScoins = 0;
    uint64_t totalSelloutBcoins  = 0;
    uint64_t totalInflateFcoins  = 0;
    vector<CReceipt> receipts;
    CAccount fcoinGenesisAccount;
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    const uint256 &txid = GetHash();

    // sort by CUserCDP::operator<()
    set<CUserCDP> cdpSet;
    for (auto &cdpPair : cdps) {
        cdpSet.insert(cdpPair.second);
    }

    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    uint64_t currRiskReserveScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
    uint32_t orderIndex            = 0;
    for (auto &cdp : cdpSet) {
        if (++cdpIndex > FORCE_SETTLE_CDP_MAX_COUNT_PER_BLOCK)
            break;

        LogPrint(BCLog::CDP,
                    "%s(), begin to force settle CDP (%s), currRiskReserveScoins: %llu, "
                    "index: %u\n",
                    __FUNCTION__, cdp.ToString(), currRiskReserveScoins, cdpIndex);

        // Suppose we have 120 (owed scoins' amount), 30, 50 three cdps, but current risk reserve scoins is 100,
        // then skip the 120 cdp and settle the 30 and 50 cdp.
        if (currRiskReserveScoins < cdp.total_owed_scoins) {
            LogPrint(BCLog::CDP, "%s, currRiskReserveScoins(%lu) < cdp.total_owed_scoins(%lu) !!\n",
                    __FUNCTION__, currRiskReserveScoins, cdp.total_owed_scoins);
            continue;
        }

        // a) sell WICC for WUSD to return to risk reserve pool
        // send bcoin from cdp to fcoin genesis account
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, cdp.total_staked_bcoins)) {
            return state.DoS(100, ERRORMSG("%s(), operate balance failed", __FUNCTION__),
                                UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }
        // should freeze user's asset for selling
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::FREEZE, cdp.total_staked_bcoins)) {
            return state.DoS(100, ERRORMSG("%s(), account has insufficient funds", __FUNCTION__),
                                UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }
        auto pBcoinSellMarketOrder = dex::CSysOrder::CreateSellMarketOrder(
            CTxCord(context.height, context.index), SYMB::WUSD, SYMB::WICC, cdp.total_staked_bcoins);
        uint256 bcoinSellMarketOrderId = GenOrderIdCompat(txid, orderIndex++);
        if (!cw.dexCache.CreateActiveOrder(bcoinSellMarketOrderId, *pBcoinSellMarketOrder)) {
            return state.DoS(100, ERRORMSG("%s(), create sys order for SellBcoinForScoin (%s) failed",
                __FUNCTION__, pBcoinSellMarketOrder->ToString()), CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
        }

        totalSelloutBcoins += cdp.total_staked_bcoins;

        // b) inflate WGRT coins and sell them for WUSD to return to risk reserve pool if necessary
        uint64_t bcoinsValueInScoin = uint64_t(double(cdp.total_staked_bcoins) * bcoinMedianPrice / PRICE_BOOST);
        if (bcoinsValueInScoin >= cdp.total_owed_scoins) {  // 1 ~ 1.04
            LogPrint(BCLog::CDP, "%s(), Force settled CDP: "
                "Placed BcoinSellMarketOrder: %s, orderId: %s\n"
                "No need to infate WGRT coins: %llu vs %llu\n"
                "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                __FUNCTION__, pBcoinSellMarketOrder->ToString(), bcoinSellMarketOrderId.GetHex(),
                bcoinsValueInScoin, cdp.total_owed_scoins,
                currRiskReserveScoins, currRiskReserveScoins - cdp.total_owed_scoins);
        } else {  // 0 ~ 1
            uint64_t fcoinsValueToInflate = cdp.total_owed_scoins - bcoinsValueInScoin;
            assert(fcoinMedianPrice != 0);
            uint64_t fcoinsToInflate = uint64_t(double(fcoinsValueToInflate) * PRICE_BOOST / fcoinMedianPrice);
            // inflate fcoin to fcoin genesis account
            if (!fcoinGenesisAccount.OperateBalance(SYMB::WGRT, BalanceOpType::ADD_FREE, fcoinsToInflate)) {
                return state.DoS(100, ERRORMSG("%s(), operate balance failed", __FUNCTION__),
                                    UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
            }
            // should freeze user's asset for selling
            if (!fcoinGenesisAccount.OperateBalance(SYMB::WGRT, BalanceOpType::FREEZE, fcoinsToInflate)) {
                return state.DoS(100, ERRORMSG("%s(), account has insufficient funds", __FUNCTION__),
                                    UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
            }

            auto pFcoinSellMarketOrder = dex::CSysOrder::CreateSellMarketOrder(
                CTxCord(context.height, context.index), SYMB::WUSD, SYMB::WGRT, fcoinsToInflate);
            uint256 fcoinSellMarketOrderId = GenOrderIdCompat(txid, orderIndex++);
            if (!cw.dexCache.CreateActiveOrder(fcoinSellMarketOrderId, *pFcoinSellMarketOrder)) {
                return state.DoS(100, ERRORMSG("%s(), create sys order for SellFcoinForScoin (%s) failed",
                        __FUNCTION__, pFcoinSellMarketOrder->ToString()), CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
            }

            totalInflateFcoins += fcoinsToInflate;

            LogPrint(BCLog::CDP, "%s(), Force settled CDP: "
                "Placed BcoinSellMarketOrder:  %s, orderId: %s\n"
                "Placed FcoinSellMarketOrder:  %s, orderId: %s\n"
                "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                __FUNCTION__, pBcoinSellMarketOrder->ToString(), bcoinSellMarketOrderId.GetHex(),
                pFcoinSellMarketOrder->ToString(), fcoinSellMarketOrderId.GetHex(),
                currRiskReserveScoins, currRiskReserveScoins - cdp.total_owed_scoins);
        }

        // c) Close the CDP
        const CUserCDP &oldCDP = cdp;
        cw.cdpCache.EraseCDP(oldCDP, cdp);
        if (SysCfg().GetArg("-persistclosedcdp", false)) {
            if (!cw.closedCdpCache.AddClosedCdpIndex(oldCDP.cdpid, uint256(), CDPCloseType::BY_FORCE_LIQUIDATE)) {
                LogPrint(BCLog::ERROR, "persistclosedcdp add failed for force-liquidated cdpid (%s)", oldCDP.cdpid.GetHex());
            }
        }

        // d) minus scoins from the risk reserve pool to repay CDP scoins
        currRiskReserveScoins -= cdp.total_owed_scoins;
        totalCloseoutScoins += cdp.total_owed_scoins;
    }

    // 4. update fcoin genesis account
    uint64_t prevScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
    assert(prevScoins >= currRiskReserveScoins);
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, SUB_FREE, prevScoins - currRiskReserveScoins)) {
        return state.DoS(100, ERRORMSG("%s(), opeate fcoin genesis account failed", __FUNCTION__),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }
    cw.accountCache.SaveAccount(fcoinGenesisAccount);

    if (totalCloseoutScoins > 0) {
        receipts.emplace_back(fcoinGenesisAccount.regid, nullId, SYMB::WUSD, totalCloseoutScoins,
                                ReceiptCode::CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE);
    }

    if (totalSelloutBcoins > 0) {
        receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WICC, totalSelloutBcoins,
                                ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE);
    }

    if (totalInflateFcoins > 0) {
        receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WGRT, totalInflateFcoins,
                                ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE);
    }

    if (!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("%s(), set tx receipts failed!! txid=%s",
                        __FUNCTION__, GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");
    return true;
}

// gen orderid compat with testnet old data
// Generally, index is an auto increment variable.
// TODO: remove me if reset testnet.
uint256 CBlockPriceMedianTx::GenOrderIdCompat(const uint256 &txid, uint32_t index) {

    CHashWriter ss(SER_GETHASH, 0);
    ss << txid << VARINT(index);
    return ss.GetHash();
}

string CBlockPriceMedianTx::ToString(CAccountDBCache &accountCache) {
    string pricePoints;
    for (const auto item : median_prices) {
        pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first, item.first.second,
                                 item.second);
    }

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, median_prices=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, txUid.ToString(), llFees, pricePoints,
                     valid_height);
}

Object CBlockPriceMedianTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    Array pricePointArray;
    for (const auto &item : median_prices) {
        Object subItem;
        subItem.push_back(Pair("coin_symbol",     item.first.first));
        subItem.push_back(Pair("price_symbol",    item.first.second));
        subItem.push_back(Pair("price",           item.second));
        pricePointArray.push_back(subItem);
    }
    result.push_back(Pair("median_price_points",   pricePointArray));

    return result;
}

PriceMap CBlockPriceMedianTx::GetMedianPrice() const { return median_prices; }
