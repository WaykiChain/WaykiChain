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
        const TokenSymbol &assetSymbolIn, const TokenSymbol &scoinSymbolIn, uint64_t bcoinPriceIn,
        uint64_t fcoindUsdPriceIn)
    : tx(txIn),
      context(contextIn),
      receipts(receiptsIn),
      fcoinGenesisAccount(fcoinGenesisAccountIn),
      assetSymbol(assetSymbolIn),
      scoinSymbol(scoinSymbolIn),
      bcoin_price(bcoinPriceIn),
      fcoin_usd_price(fcoindUsdPriceIn) {}

    bool Execute();

private:
    // input params
    CBlockPriceMedianTx &tx;
    CTxExecuteContext &context;
    vector<CReceipt> &receipts;
    CAccount &fcoinGenesisAccount;
    const TokenSymbol &assetSymbol;
    const TokenSymbol &scoinSymbol;
    uint64_t bcoin_price;
    uint64_t fcoin_usd_price;

    bool SellAssetToRiskRevervePool(const CUserCDP &cdp, const TokenSymbol &assetSymbol,
        uint64_t amount, const TokenSymbol &coinSymbol, uint256 &orderId, shared_ptr<CDEXOrderDetail> &pOrderOut,
        ReceiptList &receipts);

    uint256 GenOrderId(const CUserCDP &cdp, TokenSymbol assetSymbol);


    bool ForceLiquidateCDPCompat(CdpRatioSortedCache::Map &cdps, ReceiptList &receipts);

    uint256 GenOrderIdCompat(const uint256 &txid, uint32_t index);
};

////////////////////////////////////////////////////////////////////////////////
// class CBlockPriceMedianTx
bool CBlockPriceMedianTx::CheckTx(CTxExecuteContext &context) { return true; }

/**
 *  force settle/liquidate any under-collateralized CDP (collateral ratio <= 104%)
 */
bool CBlockPriceMedianTx::ExecuteTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    PriceDetailMap priceDetails;
    if (!cw.ppCache.CalcMedianPriceDetails(cw, context.height, priceDetails))
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, calc block median price points failed"),
                         READ_PRICE_POINT_FAIL, "calc-median-prices-failed");

    if (!EqualToCalculatedPrices(priceDetails)) {
        string str;
        for (const auto &item : priceDetails) {
            str += strprintf("{coin_pair=%s, price:%llu},", CoinPairToString(item.first), item.second.price);
        }

        LogPrint(BCLog::ERROR, "CBlockPriceMedianTx::ExecuteTx, calc from cache, height=%d, price map={%s}\n",
                context.height, str);

        str.clear();
        for (const auto &item : median_prices) {
            str += strprintf("{coin_pair=%s, price=%llu}", CoinPairToString(item.first), item.second);
        }

        LogPrint(BCLog::ERROR, "CBlockPriceMedianTx::ExecuteTx, from median tx, height: %d, price map: %s\n",
                context.height, str);

        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, invalid median price points"), REJECT_INVALID,
                         "bad-median-price-points");
    }

    if (!cw.priceFeedCache.SetMedianPrices(priceDetails))
        return state.DoS(100, ERRORMSG("CBlockPriceMedianTx::ExecuteTx, save median prices to db failed"), REJECT_INVALID,
                         "save-median-prices-failed");

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("%s(), get fcoin genesis account failed", __func__), REJECT_INVALID,
                         "get-fcoin-genesis-account-failed");

    if (!ForceLiquidateCdps(context, priceDetails))
        return false; // error msg has been processed

    return true;
}

bool CBlockPriceMedianTx::ForceLiquidateCdps(CTxExecuteContext &context, PriceDetailMap &priceDetails) {
    CCacheWrapper &cw = *context.pCw;  CValidationState &state = *context.pState;

    auto fcoinIt = priceDetails.find(kFcoinPriceCoinPair);
    if (fcoinIt == priceDetails.end() || fcoinIt->second.price == 0) {
        LogPrint(BCLog::CDP, "%s(), price of fcoin(%s) is 0, ignore\n",
            __func__, CoinPairToString(kFcoinPriceCoinPair));
        return true;

    }
    uint64_t priceTimeoutBlocks = 0;
    if (!pCdMan->pSysParamCache->GetParam(SysParamType::PRICE_FEED_TIMEOUT_BLOCKS, priceTimeoutBlocks)) {
        return state.DoS(100, ERRORMSG("%s, read sys param PRICE_FEED_TIMEOUT_BLOCKS error", __func__),
                REJECT_INVALID, "read-sysparam-error");
    }
    if (!fcoinIt->second.IsActive(context.height, priceTimeoutBlocks)) {
        LogPrint(BCLog::CDP,
                 "%s(), price of fcoin(%s) is inactive, ignore, "
                 "last_update_height=%u, cur_height=%u\n",
                 __func__, CoinPairToString(kFcoinPriceCoinPair), fcoinIt->second.last_feed_height,
                 context.height);
        return true;
    }

    uint64_t fcoinUsdPrice = fcoinIt->second.price;

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("%s(), get fcoin genesis account failed", __func__), REJECT_INVALID,
                         "save-median-prices-failed");
    }

    for (const auto& item : priceDetails) {
        if (item.first == kFcoinPriceCoinPair) continue;

        CAsset asset;
        const TokenSymbol &bcoinSymbol = item.first.first;
        const TokenSymbol &quoteSymbol = item.first.second;

        TokenSymbol scoinSymbol = GetCdpScoinByQuoteSymbol(quoteSymbol);
        if (scoinSymbol.empty()) {
            LogPrint(BCLog::CDP, "%s(), quote_symbol=%s not have a corresponding scoin , ignore",
                     __func__, bcoinSymbol);
            continue;
        }

        // TODO: remove me if need to support multi scoin and improve the force liquidate process
        if (scoinSymbol != SYMB::WUSD)
            throw runtime_error(strprintf("%s(), only support to force liquidate scoin=WUSD, actual_scoin=%s",
                    __func__, scoinSymbol));

        if (!cw.cdpCache.IsBcoinActivated(bcoinSymbol)) {
            LogPrint(BCLog::CDP, "%s(), asset=%s does not be activated, ignore", __func__, bcoinSymbol);
            continue;
        }

        if (!item.second.IsActive(context.height, priceTimeoutBlocks)) {
            LogPrint(BCLog::CDP,
                    "%s(), price of coin_pair(%s) is inactive, ignore, "
                    "last_update_height=%u, cur_height=%u\n",
                    __func__, CoinPairToString(item.first), item.second.last_feed_height,
                    context.height);
            continue;
        }

        CCdpForceLiquidator forceLiquidator(*this, context, receipts, fcoinGenesisAccount,
                                            SYMB::WICC, SYMB::WUSD, item.second.price,
                                            fcoinUsdPrice);
        if (!forceLiquidator.Execute())
            return false; // the forceLiquidator.Execute() has processed the error
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("%s(), save fcoin genesis account failed! addr=%s", __func__,
                        fcoinGenesisAccount.keyid.ToAddress()), REJECT_INVALID, "set-tx-receipt-failed");

    return true;
}

bool CBlockPriceMedianTx::EqualToCalculatedPrices(const PriceDetailMap &calcPrices) {

    PriceMap medianPrices;
    for (auto &item : median_prices) {
        if (item.second != 0)
            medianPrices.insert(item);
    }
    // the calcPrices must not contain 0 price item
    if (medianPrices.size() != calcPrices.size()) return false;

    auto priceIt = medianPrices.begin();
    auto detailIt = calcPrices.begin();
    for(; priceIt != medianPrices.end() && detailIt != calcPrices.end(); priceIt++, detailIt++) {
        if (priceIt->first != detailIt->first || priceIt->second != detailIt->second.price)
            return false;
    }
    return priceIt == medianPrices.end() && detailIt == calcPrices.end();
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
    if (cdpGlobalData.CheckGlobalCollateralRatioFloorReached(bcoin_price, globalCollateralRatioFloor)) {
        LogPrint(BCLog::CDP, "%s(), GlobalCollateralFloorReached!!\n", __func__);
        return true;
    }

    // 2. get all CDPs to be force settled
    CdpRatioSortedCache::Map cdpMap;
    uint64_t forceLiquidateRatio = 0;
    if (!cw.sysParamCache.GetCdpParam(cdpCoinPair, CdpParamType::CDP_FORCE_LIQUIDATE_RATIO, forceLiquidateRatio)) {
        return state.DoS(100, ERRORMSG("%s(), read force liquidate ratio param error! cdpCoinPair=%s",
                __func__, cdpCoinPair.ToString()),
                READ_SYS_PARAM_FAIL, "read-force-liquidate-ratio-error");
    }

    cw.cdpCache.GetCdpListByCollateralRatio(cdpCoinPair, forceLiquidateRatio, bcoin_price, cdpMap);

    LogPrint(BCLog::CDP, "%s(), tx_cord=%d-%d, globalCollateralRatioFloor=%llu, bcoin_price: %llu, "
            "forceLiquidateRatio: %llu, cdpMap: %llu\n", __func__, context.height, context.index,
            globalCollateralRatioFloor, bcoin_price, forceLiquidateRatio, cdpMap.size());

    // 3. force settle each cdp
    if (cdpMap.size() == 0) {
        return true;
    }

    {
        // TODO: remove me.
        // print all force liquidating cdps
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
        return ForceLiquidateCDPCompat(cdpMap, receipts);
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
        ReceiptCode code = ReceiptCode::CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE;
        fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::SUB_FREE, cdp.total_owed_scoins, code, receipts);

        // b) sell bcoins for risk reserve pool
        // b.1) clean up cdp owner's pledged_amount
        code = ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE;
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, cdp.total_staked_bcoins, code, receipts)) {
            return state.DoS(100, ERRORMSG("%s(), unpledge bcoins failed! cdp={%s}", __func__, cdp.ToString()),
                    UPDATE_ACCOUNT_FAIL, "unpledge-bcoins-failed");
        }

        code = ReceiptCode::CDP_LIQUIDATED_ASSET_TO_OWNER;
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, SUB_FREE, cdp.total_staked_bcoins, code, receipts)) {
            return state.DoS(100, ERRORMSG("%s(), sub unpledged bcoins failed! cdp={%s}", __func__, cdp.ToString()),
                    UPDATE_ACCOUNT_FAIL, "deduct-bcoins-failed");
        }

        // b.2) sell bcoins to get scoins and put them to risk reserve pool
        uint256 assetSellOrderId;
        shared_ptr<CDEXOrderDetail> pAssetSellOrder;
        if (!SellAssetToRiskRevervePool(cdp, assetSymbol, cdp.total_staked_bcoins, scoinSymbol, assetSellOrderId,
                                        pAssetSellOrder, receipts))
            return false;

        totalSelloutBcoins += cdp.total_staked_bcoins;

        // c) inflate WGRT coins to risk reserve pool and sell them to get WUSD  if necessary
        uint64_t bcoinsValueInScoin = uint64_t(double(cdp.total_staked_bcoins) * bcoin_price / PRICE_BOOST);
        if (bcoinsValueInScoin < cdp.total_owed_scoins) {  // 0 ~ 1
            uint64_t fcoinsValueToInflate = cdp.total_owed_scoins - bcoinsValueInScoin;
            assert(fcoin_usd_price != 0);
            uint64_t fcoinsToInflate = uint64_t(double(fcoinsValueToInflate) * PRICE_BOOST / fcoin_usd_price);
            // inflate fcoin to fcoin genesis account
            uint256 fcoinSellOrderId;
            shared_ptr<CDEXOrderDetail> pFcoinSellOrder;
            if (!SellAssetToRiskRevervePool(cdp, SYMB::WGRT, fcoinsToInflate, scoinSymbol, fcoinSellOrderId,
                                            pFcoinSellOrder, receipts))
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

    return true;
}


bool CCdpForceLiquidator::SellAssetToRiskRevervePool(const CUserCDP &cdp, const TokenSymbol &assetSymbol,
    uint64_t amount, const TokenSymbol &coinSymbol, uint256 &orderId, shared_ptr<CDEXOrderDetail> &pOrderOut,
    ReceiptList &receipts) {

    ReceiptCode code = ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE;
    if (!fcoinGenesisAccount.OperateBalance(assetSymbol, BalanceOpType::ADD_FREE, amount, code, receipts)) {
        return context.pState->DoS(100, ERRORMSG("%s(), add account balance failed", __func__),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    // freeze account asset for selling
    code = ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE;
    if (!fcoinGenesisAccount.OperateBalance(assetSymbol, BalanceOpType::FREEZE, amount, code, receipts)) {
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


bool CCdpForceLiquidator::ForceLiquidateCDPCompat(CdpRatioSortedCache::Map &cdps, ReceiptList &receipts) {

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
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::ADD_FREE, cdp.total_staked_bcoins,
                                                ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("%s(), operate balance failed", __FUNCTION__),
                                UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }

        // should freeze user's asset for selling
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WICC, BalanceOpType::FREEZE, cdp.total_staked_bcoins,
                                                ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE, receipts)) {
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
        uint64_t bcoinsValueInScoin = uint64_t(double(cdp.total_staked_bcoins) * bcoin_price / PRICE_BOOST);
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
            assert(fcoin_usd_price != 0);
            uint64_t fcoinsToInflate = uint64_t(double(fcoinsValueToInflate) * PRICE_BOOST / fcoin_usd_price);
            // inflate fcoin to fcoin genesis account
            if (!fcoinGenesisAccount.OperateBalance(SYMB::WGRT, BalanceOpType::ADD_FREE, fcoinsToInflate,
                                                    ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE, receipts)) {
                return state.DoS(100, ERRORMSG("%s(), operate balance failed", __FUNCTION__),
                                    UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
            }

            // should freeze user's asset for selling
            if (!fcoinGenesisAccount.OperateBalance(SYMB::WGRT, BalanceOpType::FREEZE, fcoinsToInflate,
                                                    ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE, receipts)) {
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

    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, SUB_FREE, prevScoins - currRiskReserveScoins,
                                            ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE, receipts)) {
        return state.DoS(100, ERRORMSG("%s(), opeate fcoin genesis account failed", __FUNCTION__),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    // receipts.emplace_back(fcoinGenesisAccount.regid, nullId, SYMB::WUSD, totalCloseoutScoins,
    //                         ReceiptCode::CDP_TOTAL_CLOSEOUT_SCOIN_FROM_RESERVE);
    // receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WICC, totalSelloutBcoins,
    //                         ReceiptCode::CDP_TOTAL_ASSET_TO_RESERVE);
    // receipts.emplace_back(nullId, fcoinGenesisAccount.regid, SYMB::WGRT, totalInflateFcoins,
    //                         ReceiptCode::CDP_TOTAL_INFLATE_FCOIN_TO_RESERVE);

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
