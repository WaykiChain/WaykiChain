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
 *  force settle/liquidate any under-collateralized CDP (collateral ratio <= 100%)
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

    // 0. Check Global Collateral Ratio floor & Collateral Ceiling if reached
    uint64_t globalCollateralRatioFloor = 0;
    if (!cw.sysParamCache.GetParam(SysParamType::GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read global collateral ratio floor error"),
                         READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
    }

    uint64_t bcoinMedianPrice = cw.ppCache.GetBcoinMedianPrice(height, slideWindowBlockCount);
    if (cw.cdpCache.CheckGlobalCollateralRatioFloorReached(bcoinMedianPrice, globalCollateralRatioFloor)) {
        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, GlobalCollateralFloorReached!!\n");
        return true;
    }

    // 1. get fcoin median price
    uint64_t fcoinMedianPrice = cw.ppCache.GetFcoinMedianPrice(height, slideWindowBlockCount);
    if (fcoinMedianPrice == 0) {
        LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, failed to acquire fcoin median price\n");
        return true;
    }

    // 2. get all CDPs to be force settled
    set<CUserCDP> forceLiquidateCdps;
    uint64_t forceLiquidateRatio = 0;
    if (!cw.sysParamCache.GetParam(SysParamType::CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, read force liquidate ratio error"),
                         READ_SYS_PARAM_FAIL, "read-force-liquidate-ratio-error");
    }
    cw.cdpCache.cdpMemCache.GetCdpListByCollateralRatio(forceLiquidateRatio, bcoinMedianPrice, forceLiquidateCdps);

    LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, globalCollateralRatioFloor: %llu, bcoinMedianPrice: %llu, "
             "forceLiquidateRatio: %llu, forceLiquidateCdps: %llu\n",
             globalCollateralRatioFloor, bcoinMedianPrice, forceLiquidateRatio, forceLiquidateCdps.size());

    // 3. force settle each cdp
    int32_t cdpIndex = 0;
    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    uint64_t currRiskReserveScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
    for (auto cdp : forceLiquidateCdps) {
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

        // a) minus scoins from the risk reserve pool to repay CDP scoins
        uint64_t prevRiskReserveScoins = currRiskReserveScoins;
        currRiskReserveScoins -= cdp.total_owed_scoins;

        // b) sell WICC for WUSD to return to risk reserve pool
        auto pBcoinSellMarketOrder = CDEXSysOrder::CreateSellMarketOrder(
            CTxCord(height, index), SYMB::WUSD, SYMB::WICC, cdp.total_staked_bcoins);
        if (!cw.dexCache.CreateActiveOrder(GetHash(), *pBcoinSellMarketOrder)) {
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, create sys order for SellBcoinForScoin (%s) failed!!",
                    pBcoinSellMarketOrder->ToString());
            break;
        }

        // c) inflate WGRT coins and sell them for WUSD to return to risk reserve pool if necessary
        uint64_t bcoinsValueInScoin = uint64_t(double(cdp.total_staked_bcoins) * bcoinMedianPrice / kPercentBoost);
        if (bcoinsValueInScoin >= cdp.total_owed_scoins) {  // 1 ~ 1.04
            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, Force settled CDP: "
                "Placed BcoinSellMarketOrder: %s\n"
                "No need to infate WGRT coins: %llu vs %llu\n"
                "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                pBcoinSellMarketOrder->ToString(),
                bcoinsValueInScoin, cdp.total_owed_scoins,
                prevRiskReserveScoins,
                currRiskReserveScoins);
        } else {  // 0 ~ 1
            uint64_t fcoinsValueToInflate = cdp.total_owed_scoins - bcoinsValueInScoin;
            assert(fcoinMedianPrice != 0);
            uint64_t fcoinsToInflate = fcoinsValueToInflate * kPercentBoost / fcoinMedianPrice;
            auto pFcoinSellMarketOrder =
                CDEXSysOrder::CreateSellMarketOrder(CTxCord(height, index), SYMB::WUSD, SYMB::WGRT, fcoinsToInflate);
            if (!cw.dexCache.CreateActiveOrder(GetHash(), *pFcoinSellMarketOrder)) {
                LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, create sys order for SellFcoinForScoin (%s) failed!!",
                        pFcoinSellMarketOrder->ToString());
                break;
            }

            LogPrint("CDP", "CBlockPriceMedianTx::ExecuteTx, Force settled CDP: "
                "Placed BcoinSellMarketOrder:  %s\n"
                "Placed FcoinSellMarketOrder:  %s\n"
                "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n",
                pBcoinSellMarketOrder->ToString(),
                pFcoinSellMarketOrder->ToString(),
                prevRiskReserveScoins,
                currRiskReserveScoins);
        }

        // d) Close the CDP
        cw.cdpCache.EraseCDP(cdp);
    }

    uint64_t prevScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
    fcoinGenesisAccount.OperateBalance(SYMB::WUSD, ADD_FREE, currRiskReserveScoins - prevScoins);
    cw.accountCache.SaveAccount(fcoinGenesisAccount);

    return SaveTxAddresses(height, index, cw, state, {txUid});
}

string CBlockPriceMedianTx::ToString(CAccountDBCache &accountCache) {
    string pricePoints;
    for (const auto item : median_price_points) {
        pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}",
                                item.first.first, item.first.second, item.second);
    };

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, median_price_points=%s, nValidHeight=%d\n",
                     GetTxType(nTxType), GetHash().GetHex(), nVersion, txUid.ToString(), llFees, pricePoints,
                     nValidHeight);
}

Object CBlockPriceMedianTx::ToJson(const CAccountDBCache &accountCache) const {
    Array pricePointArray;
    for (const auto &item : median_price_points) {
        Object subItem;
        subItem.push_back(Pair("coin_symbol",     item.first.first));
        subItem.push_back(Pair("price_symbol",    item.first.second));
        subItem.push_back(Pair("price",           item.second));
        pricePointArray.push_back(subItem);
    }

    Object result;
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache);
    result.push_back(Pair("median_price_points",   pricePointArray));

    return result;
}

bool CBlockPriceMedianTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

map<CoinPricePair, uint64_t> CBlockPriceMedianTx::GetMedianPrice() const {
    return median_price_points;
}
