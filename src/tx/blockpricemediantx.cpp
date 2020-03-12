// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockpricemediantx.h"
#include "main.h"

using namespace dex;

class CCdpForceLiquidator {
public:
    // result:

    uint64_t totalCloseoutScoins = 0;
    uint64_t totalSelloutBcoins  = 0;
    uint64_t totalInflateFcoins  = 0;
public:
    CCdpForceLiquidator(CBlockPriceMedianTx &txIn, CTxExecuteContext &contextIn,
        vector<CReceipt> &receiptsIn, CAccount &fcoinGenesisAccountIn,
        const TokenSymbol &assetSymbolIn, const TokenSymbol &scoinSymbolIn)
    : tx(txIn),
      context(contextIn),
      receipts(receiptsIn),
      fcoinGenesisAccount(fcoinGenesisAccountIn),
      assetSymbol(assetSymbolIn),
      scoinSymbol(scoinSymbolIn) {}

    bool Execute();

private:
    CBlockPriceMedianTx &tx;
    CTxExecuteContext &context;
    vector<CReceipt> &receipts;
    CAccount &fcoinGenesisAccount;
    const TokenSymbol &assetSymbol;
    const TokenSymbol &scoinSymbol;

    bool SellAssetToRiskRevervePool(const CUserCDP &cdp, const TokenSymbol &assetSymbol,
        uint64_t amount, const TokenSymbol &coinSymbol, uint256 &orderId, shared_ptr<CDEXOrderDetail> &pOrderOut);

    uint256 GenOrderId(const CUserCDP &cdp, TokenSymbol assetSymbol);


    bool ForceLiquidateCDPCompat(uint64_t bcoinMedianPrice, uint64_t fcoinMedianPrice, CdpRatioSortedCache::Map &cdps);

    uint256 GenOrderIdCompat(const uint256 &txid, uint32_t index);
};


bool CBlockPriceMedianTx::CheckTx(CTxExecuteContext &context) { return true; }

/**
 *  force settle/liquidate any under-collateralized CDP (collateral ratio <= 104%)
 */
bool CBlockPriceMedianTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

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

        LogPrint(BCLog::ERROR, "CBlockPriceMedianTx::ExecuteTx, from cache, height: %d, price points: %s\n",
                context.height, pricePoints);

        pricePoints.clear();
        for (const auto item : median_prices) {
            pricePoints += strprintf("{coin_symbol:%s, price_symbol:%s, price:%lld}", item.first.first,
                                     item.first.second, item.second);
        }

        LogPrint(BCLog::ERROR, "CBlockPriceMedianTx::ExecuteTx, from median tx, height: %d, price points: %s\n",
                context.height, pricePoints);

        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, invalid median price points"), REJECT_INVALID,
                         "bad-median-price-points");
    }

    if (!cw.priceFeedCache.SetMedianPrices(median_prices)) {
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, save median prices to db failed"), REJECT_INVALID,
                         "save-median-prices-failed");
    }

    vector<CReceipt> receipts;
    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("%s(), get fcoin genesis account failed", __func__), REJECT_INVALID,
                         "save-median-prices-failed");

    }

    // TODO: support multi asset/scoin cdp
    CCdpForceLiquidator forceLiquidator(*this, context, receipts, fcoinGenesisAccount, SYMB::WICC, SYMB::WUSD);
    if (!forceLiquidator.Execute())
        return false;

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("%s(), save fcoin genesis account failed! addr=%s", __func__,
                        fcoinGenesisAccount.keyid.ToAddress()), REJECT_INVALID, "set-tx-receipt-failed");

    if (!cw.txReceiptCache.SetTxReceipts(GetHash(), receipts))
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, set tx receipts failed!! txid=%s",
                        GetHash().ToString()), REJECT_INVALID, "set-tx-receipt-failed");

    return true;
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

///////////////////////////////////////////////////////////////////////////////
// class CCdpForceLiquidator


bool CCdpForceLiquidator::Execute() {

    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);


    const TokenSymbol &quoteSymbol = GetQuoteSymbolByCdpScoin(scoinSymbol);
    if (quoteSymbol.empty()) {
        return state.DoS(100, ERRORMSG("%s(), get price quote by cdp scoin=%s failed!", __func__, scoinSymbol),
                        REJECT_INVALID, "get-price-quote-by-cdp-scoin-failed");
    }

    // 0. acquire median prices
    // TODO: multi stable coin
    uint64_t bcoinMedianPrice = tx.median_prices[CoinPricePair(assetSymbol, quoteSymbol)];
    if (bcoinMedianPrice == 0) {
        LogPrint(BCLog::CDP, "%s(), price of %s:%s is 0, ignore\n", __func__, assetSymbol, quoteSymbol);
        return true;
    }

    uint64_t fcoinMedianPrice = tx.median_prices[CoinPricePair(SYMB::WGRT, quoteSymbol)];
    if (fcoinMedianPrice == 0) {
        LogPrint(BCLog::CDP, "%s(), price of fcoin(WGRT:USD) is 0, ignore\n", __func__);
        return true;
    }

    CCdpCoinPair cdpCoinPair(assetSymbol, scoinSymbol);
    // 1. Check Global Collateral Ratio floor & Collateral Ceiling if reached
    uint64_t globalCollateralRatioFloor = 0;

    if (!cw.sysParamCache.GetCdpParam(cdpCoinPair, CdpParamType::CDP_GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("%s(), read global collateral ratio floor param error! cdpCoinPair=%s",
                __func__, cdpCoinPair.ToString()),
                READ_SYS_PARAM_FAIL, "read-global-collateral-ratio-floor-error");
    }

    CCdpGlobalData cdpGlobalData = cw.cdpCache.GetCdpGlobalData(cdpCoinPair);
    // check global collateral ratio
    if (cdpGlobalData.CheckGlobalCollateralRatioFloorReached(bcoinMedianPrice, globalCollateralRatioFloor)) {
        LogPrint(BCLog::CDP, "%s(), GlobalCollateralFloorReached!!\n", __func__);
        return true;
    }

    // 2. get all CDPs to be force settled
    CdpRatioSortedCache::Map cdpMap;
    uint64_t forceLiquidateRatio = 0;
    // TODO: get cdp CDP_FORCE_LIQUIDATE_RATIO
    if (!cw.sysParamCache.GetCdpParam(cdpCoinPair, CdpParamType::CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("%s(), read force liquidate ratio param error! cdpCoinPair=%s",
                __func__, cdpCoinPair.ToString()),
                READ_SYS_PARAM_FAIL, "read-force-liquidate-ratio-error");
    }

    // TODO: get liquidating cdp map
    cw.cdpCache.GetCdpListByCollateralRatio(cdpCoinPair, forceLiquidateRatio, bcoinMedianPrice, cdpMap);

    LogPrint(BCLog::CDP, "%s(), tx_cord=%d-%d, globalCollateralRatioFloor: %llu, bcoinMedianPrice: %llu, "
            "forceLiquidateRatio: %llu, cdpMap: %llu\n", __func__, context.height, context.index,
            globalCollateralRatioFloor, bcoinMedianPrice, forceLiquidateRatio, cdpMap.size());

    // 3. force settle each cdp
    if (cdpMap.size() == 0) {
        return true;
    }

    {
        // TODO: remove me.
        LogPrint(BCLog::CDP, "%s(), have %llu cdps to force settle, in detail:\n",
                    __func__, cdpMap.size());
        for (const auto &cdpKey : cdpMap) {
            LogPrint(BCLog::CDP, "%s\n", cdpKey.second.ToString());
        }
    }

    NET_TYPE netType = SysCfg().NetworkID();
    if (netType == TEST_NET && context.height < 1800000  && assetSymbol == SYMB::WICC && scoinSymbol == SYMB::WUSD) {
        // soft fork to compat old data of testnet
        // TODO: remove me if reset testnet.
        return ForceLiquidateCDPCompat(bcoinMedianPrice, fcoinMedianPrice, cdpMap);
    }

    int32_t count             = 0;
    uint64_t totalCloseoutScoins = 0;
    uint64_t totalSelloutBcoins  = 0;
    uint64_t totalInflateFcoins  = 0;
    for (auto &cdpPair : cdpMap) {
        auto &cdp = cdpPair.second;
        if (count + 1 > FORCE_SETTLE_CDP_MAX_COUNT_PER_BLOCK)
            break;

        // Suppose we have 120 (owed scoins' amount), 30, 50 three cdps, but current risk reserve scoins is 100,
        // then skip the 120 cdp and settle the 30 and 50 cdp.
        uint64_t currRiskReserveScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
        if (currRiskReserveScoins < cdp.total_owed_scoins) {
            LogPrint(BCLog::CDP, "%s(), currRiskReserveScoins(%lu) < cdp.total_owed_scoins(%lu) !!\n",
                    __func__, currRiskReserveScoins, cdp.total_owed_scoins);
            continue;
        }

        CAccount cdpOwnerAccount;
        if (!cw.accountCache.GetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount)) {
            return state.DoS(100, ERRORMSG("%s(), read CDP Owner account info error! uid=%s",
                    __func__, cdp.owner_regid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        count++;
        LogPrint(BCLog::CDP,
                    "%s(), begin to force settle CDP {%s}, currRiskReserveScoins: %llu, "
                    "index: %u\n", __func__,
                    cdp.ToString(), currRiskReserveScoins, count - 1);
        // a) get scoins from risk reserve pool for closeout
        fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::SUB_FREE, cdp.total_owed_scoins);

        // b) sell bcoins for risk reserve pool
        // b.1) clean up cdp owner's pledged_amount
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, cdp.total_staked_bcoins)) {
            return state.DoS(100, ERRORMSG("%s(), unpledge bcoins failed! cdp={%s}", __func__, cdp.ToString()),
                    UPDATE_ACCOUNT_FAIL, "unpledge-bcoins-failed");
        }
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, SUB_FREE, cdp.total_staked_bcoins)) {
            return state.DoS(100, ERRORMSG("%s(), sub unpledged bcoins failed! cdp={%s}", __func__, cdp.ToString()),
                    UPDATE_ACCOUNT_FAIL, "deduct-bcoins-failed");
        }

        // b.2) sell bcoins to get scoins and put them to risk reserve pool
        uint256 assetSellOrderId;
        shared_ptr<CDEXOrderDetail> pAssetSellOrder;
        if (!SellAssetToRiskRevervePool(cdp, assetSymbol, cdp.total_staked_bcoins, scoinSymbol, assetSellOrderId, pAssetSellOrder))
            return false;

        totalSelloutBcoins += cdp.total_staked_bcoins;

        // c) inflate WGRT coins to risk reserve pool and sell them to get WUSD  if necessary
        uint64_t bcoinsValueInScoin = uint64_t(double(cdp.total_staked_bcoins) * bcoinMedianPrice / PRICE_BOOST);
        if (bcoinsValueInScoin < cdp.total_owed_scoins) {  // 0 ~ 1
            uint64_t fcoinsValueToInflate = cdp.total_owed_scoins - bcoinsValueInScoin;
            assert(fcoinMedianPrice != 0);
            uint64_t fcoinsToInflate = uint64_t(double(fcoinsValueToInflate) * PRICE_BOOST / fcoinMedianPrice);
            // inflate fcoin to fcoin genesis account
            uint256 fcoinSellOrderId;
            shared_ptr<CDEXOrderDetail> pFcoinSellOrder;
            if (!SellAssetToRiskRevervePool(cdp, SYMB::WGRT, fcoinsToInflate, scoinSymbol, fcoinSellOrderId, pFcoinSellOrder))
                return false;
            totalInflateFcoins += fcoinsToInflate;

            LogPrint(BCLog::CDP, "%s(), Force settled CDP: "
                "Placed BcoinSellMarketOrder:  %s, orderId: %s\n"
                "Placed FcoinSellMarketOrder:  %s, orderId: %s\n"
                "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n", __func__,
                pAssetSellOrder->ToString(), assetSellOrderId.GetHex(),
                pFcoinSellOrder->ToString(), fcoinSellOrderId.GetHex(),
                currRiskReserveScoins, currRiskReserveScoins - cdp.total_owed_scoins);
        } else  {  // 1 ~ 1.04
            // The sold assets are sufficient to pay off the debt
            LogPrint(BCLog::CDP, "%s(), Force settled CDP: "
                "Placed BcoinSellMarketOrder: %s, orderId: %s\n"
                "No need to infate WGRT coins: %llu vs %llu\n"
                "prevRiskReserveScoins: %lu -> currRiskReserveScoins: %lu\n", __func__,
                pAssetSellOrder->ToString(), assetSellOrderId.GetHex(),
                bcoinsValueInScoin, cdp.total_owed_scoins,
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

        totalCloseoutScoins += cdp.total_owed_scoins;
    }

    if (count > 0) {
        receipts.emplace_back(fcoinGenesisAccount.regid, nullId, scoinSymbol, totalCloseoutScoins,
                                ReceiptCode::CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE);

        receipts.emplace_back(nullId, fcoinGenesisAccount.regid, assetSymbol, totalSelloutBcoins,
                                ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE);

        receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WGRT, totalInflateFcoins,
                                ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE);
    }

    return true;
}


bool CCdpForceLiquidator::SellAssetToRiskRevervePool(const CUserCDP &cdp, const TokenSymbol &assetSymbol,
    uint64_t amount, const TokenSymbol &coinSymbol, uint256 &orderId, shared_ptr<CDEXOrderDetail> &pOrderOut) {

    if (!fcoinGenesisAccount.OperateBalance(assetSymbol, BalanceOpType::ADD_FREE, amount)) {
        return context.pState->DoS(100, ERRORMSG("%s(), add account balance failed", __func__),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }
    // freeze account asset for selling
    if (!fcoinGenesisAccount.OperateBalance(assetSymbol, BalanceOpType::FREEZE, amount)) {
        return context.pState->DoS(100, ERRORMSG("%s(), account has insufficient funds", __func__),
                            UPDATE_ACCOUNT_FAIL, "account-insufficient");
    }

    pOrderOut = dex::CSysOrder::CreateSellMarketOrder(
        CTxCord(context.height, context.index), coinSymbol, assetSymbol, amount);
    orderId = GenOrderId(cdp, assetSymbol);
    if (!context.pCw->dexCache.CreateActiveOrder(orderId, *pOrderOut)) {
        return context.pState->DoS(100, ERRORMSG("%s(), create sys sell market order failed, cdpid=%s, "
                "assetSymbol=%s, coinSymbol=%s, amount=%llu",
                __func__, cdp.cdpid.ToString(), assetSymbol, coinSymbol, amount),
                CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }
    LogPrint(BCLog::DEX, "%s(), create sys sell market order OK! cdpid=%s, order_detail={%s}",
                __func__, cdp.cdpid.ToString(), pOrderOut->ToString());
    return true;
}


uint256 CCdpForceLiquidator::GenOrderId(const CUserCDP &cdp, TokenSymbol assetSymbol) {

    CHashWriter ss(SER_GETHASH, 0);
    ss << cdp.cdpid << assetSymbol;
    return ss.GetHash();
}


bool CCdpForceLiquidator::ForceLiquidateCDPCompat(uint64_t bcoinMedianPrice, uint64_t fcoinMedianPrice,
    CdpRatioSortedCache::Map &cdps) {

    int32_t cdpIndex             = 0;
    uint64_t totalCloseoutScoins = 0;
    uint64_t totalSelloutBcoins  = 0;
    uint64_t totalInflateFcoins  = 0;
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    const uint256 &txid = tx.GetHash();

    // sort by CUserCDP::operator<()
    set<CUserCDP> cdpSet;
    for (auto &cdpPair : cdps) {
        cdpSet.insert(cdpPair.second);
    }

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

    // 4. operate fcoin genesis account
    uint64_t prevScoins = fcoinGenesisAccount.GetToken(SYMB::WUSD).free_amount;
    assert(prevScoins >= currRiskReserveScoins);
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, SUB_FREE, prevScoins - currRiskReserveScoins)) {
        return state.DoS(100, ERRORMSG("%s(), opeate fcoin genesis account failed", __FUNCTION__),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    receipts.emplace_back(fcoinGenesisAccount.regid, nullId, SYMB::WUSD, totalCloseoutScoins,
                            ReceiptCode::CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE);
    receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WICC, totalSelloutBcoins,
                            ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE);
    receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WGRT, totalInflateFcoins,
                            ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE);

    return true;
}


// gen orderid compat with testnet old data
// Generally, index is an auto increase variable.
// TODO: remove me if reset testnet.
uint256 CCdpForceLiquidator::GenOrderIdCompat(const uint256 &txid, uint32_t index) {

    CHashWriter ss(SER_GETHASH, 0);
    ss << txid << VARINT(index);
    return ss.GetHash();
}
