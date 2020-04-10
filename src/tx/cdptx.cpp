// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "cdptx.h"

#include "config/const.h"
#include "main.h"
#include "persistence/cdpdb.h"

#include <cmath>

using namespace std;

static bool ReadCdpParam(CBaseTx &tx, CTxExecuteContext &context, const CCdpCoinPair &cdpCoinPair,
    CdpParamType paramType, uint64_t &value) {
    if (!context.pCw->sysParamCache.GetCdpParam(cdpCoinPair, paramType, value)) {
        return context.pState->DoS(100, ERRORMSG("%s, read cdp param %s error! cdpCoinPair=%s",
            TX_OBJ_ERR_TITLE(tx), GetCdpParamName(paramType), cdpCoinPair.ToString()),
                    READ_SYS_PARAM_FAIL, "read-cdp-param-error");
    }
    return true;
}

static bool GetBcoinMedianPrice(CBaseTx &tx, CTxExecuteContext &context, const CCdpCoinPair &cdpCoinPair,
        uint64_t &bcoinPrice) {
    const TokenSymbol &quoteSymbol = GetQuoteSymbolByCdpScoin(cdpCoinPair.scoin_symbol);
    if (quoteSymbol.empty()) {
        return context.pState->DoS(100, ERRORMSG("get price quote by cdp scoin=%s failed!",
                TX_OBJ_ERR_TITLE(tx), cdpCoinPair.scoin_symbol),
                REJECT_INVALID, "get-price-quote-by-cdp-scoin-failed");
    }

    uint64_t priceTimeoutBlocks = 0;
    if (!context.pCw->sysParamCache.GetParam(SysParamType::PRICE_FEED_TIMEOUT_BLOCKS, priceTimeoutBlocks)) {
        return context.pState->DoS(100, ERRORMSG("read sys param PRICE_FEED_TIMEOUT_BLOCKS error"),
                REJECT_INVALID, "read-sysparam-error");
    }
    CMedianPriceDetail priceDetail;
    PriceCoinPair priceCoinPair(cdpCoinPair.bcoin_symbol, quoteSymbol);
    context.pCw->priceFeedCache.GetMedianPriceDetail(priceCoinPair, priceDetail);
    if (priceDetail.price == 0 || !priceDetail.IsActive(context.height, priceTimeoutBlocks)) {
        return context.pState->DoS(100, ERRORMSG("[%d] the price of %s is empty or inactive! "
                "price={%s}, tip_height=%u", context.height, CoinPairToString(priceCoinPair),
                priceDetail.ToString()), REJECT_INVALID, "invalid-bcoin-price");
    }
    bcoinPrice = priceDetail.price;
    return true;
}

namespace cdp_util {

    static string ToString(const CDPStakeAssetMap& assetMap) {
        string ret = "";
        for (auto item : assetMap) {
            ret = strprintf("{%s=%llu}", item.first, item.second.get());
            if (!ret.empty()) ret += ",";
        }
        return "{" + ret + "}";
    }

    static Object ToJson(const CDPStakeAssetMap& assetMap) {
        Object ret;
        for (auto item : assetMap) {
            ret.push_back(Pair(item.first, item.second.get()));
        }
        return ret;
    }

    bool CdpNeedSettleInterest(HeightType lastHeight, HeightType curHeight, uint64_t cycleDays) {
        uint64_t cycleBlocks = cycleDays * GetDayBlockCount(curHeight);
        return (curHeight > lastHeight) && ((curHeight - lastHeight) >= cycleBlocks);
    }

    bool SellInterestForFcoins(CBaseTx &tx, CTxExecuteContext &context, const CUserCDP &cdp,
                               const uint64_t scoinsInterest, vector<CReceipt> &receipts) {
        if (scoinsInterest == 0)
            return true;

        CCacheWrapper &cw = *context.pCw;
        CValidationState &state = *context.pState;
        CAccount fcoinGenesisAccount;
        cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
        // send interest to fcoin genesis account
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, scoinsInterest,
                                                ReceiptType::CDP_INTEREST_BUY_DEFLATE_FCOINS, receipts)) {
            return state.DoS(100, ERRORMSG("%s, operate fcoin genesis account failed", TX_OBJ_ERR_TITLE(tx)),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }

        // should freeze user's coin for buying the asset
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::FREEZE, scoinsInterest,
                                                ReceiptType::CDP_INTEREST_BUY_DEFLATE_FCOINS, receipts)) {
            return state.DoS(100, ERRORMSG("%s, fcoin genesis account has insufficient funds", TX_OBJ_ERR_TITLE(tx)),
                            UPDATE_ACCOUNT_FAIL, "fcoin-genesis-account-insufficient");
        }

        CTxCord txCord(context.height, context.index);
        auto pSysBuyMarketOrder = dex::CSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol,
                                                                    SYMB::WGRT, scoinsInterest);

        if (!cw.dexCache.CreateActiveOrder(tx.GetHash(), *pSysBuyMarketOrder)) {
            return state.DoS(100, ERRORMSG("%s, create system buy order failed", TX_OBJ_ERR_TITLE(tx)),
                            CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
        }

        if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
            return state.DoS(100, ERRORMSG("%s, set account info error", TX_OBJ_ERR_TITLE(tx)),
                            WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
        return true;
    }


    bool SellInterestForFcoins(CBaseTx &tx, const CUserCDP &cdp, CTxExecuteContext &context,
                               const uint64_t scoinsInterest, vector<CReceipt> &receipts) {
        if (scoinsInterest == 0)
            return true;

        CCacheWrapper &cw = *context.pCw;
        CValidationState &state = *context.pState;
        CAccount fcoinGenesisAccount;
        cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
        // send interest to fcoin genesis account
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, scoinsInterest,
                                                ReceiptType::CDP_INTEREST_BUY_DEFLATE_FCOINS, receipts)) {
            return state.DoS(100, ERRORMSG("%s, operate fcoin genesis account failed", TX_OBJ_ERR_TITLE(tx)),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }

        // should freeze user's coin for buying the asset
        if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::FREEZE, scoinsInterest,
                                                ReceiptType::CDP_INTEREST_BUY_DEFLATE_FCOINS, receipts)) {
            return state.DoS(100, ERRORMSG("%s, fcoin genesis account has insufficient funds", TX_OBJ_ERR_TITLE(tx)),
                            UPDATE_ACCOUNT_FAIL, "fcoin-genesis-account-insufficient");
        }

        CTxCord txCord(context.height, context.index);
        auto pSysBuyMarketOrder = dex::CSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol,
                                                                    SYMB::WGRT, scoinsInterest);

        if (!cw.dexCache.CreateActiveOrder(tx.GetHash(), *pSysBuyMarketOrder)) {
            return state.DoS(100, ERRORMSG("%s, create system buy order failed", TX_OBJ_ERR_TITLE(tx)),
                            CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
        }

        if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
            return state.DoS(100, ERRORMSG("%s, set account info error", TX_OBJ_ERR_TITLE(tx)),
                            WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
        return true;
    }
}

static uint64_t CalcCollateralRatio(uint64_t assetAmount, uint64_t scoinAmount, uint64_t price) {

    return scoinAmount == 0 ? UINT64_MAX :
        uint64_t(double(assetAmount) * price / PRICE_BOOST / scoinAmount * RATIO_BOOST);
}

/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *
 *  ==> ratio = a / Log10 (b+N)
 */
uint64_t ComputeCDPInterest(const uint64_t total_owed_scoins, const int32_t beginHeight, const uint32_t endHeight,
                            uint64_t A, uint64_t B) {

    int32_t blockInterval = endHeight - beginHeight;
    int32_t loanedDays    = std::max<int32_t>(1, ceil((double)blockInterval / ::GetDayBlockCount(endHeight)));

    uint64_t N                = total_owed_scoins;
    double annualInterestRate = 0.1 * A / log10(1.0 + B * N / (double)COIN);
    uint64_t interest         = (uint64_t)(((double)N / 365) * loanedDays * annualInterestRate);

    LogPrint(BCLog::CDP, "beginHeight=%d, endHeight=%d, loanedDays=%d, A=%llu, B=%llu, N="
             "%llu, annualInterestRate=%f, interest=%llu\n",
             beginHeight, endHeight, loanedDays, A, B, N, annualInterestRate, interest);

    return interest;
}

/**
 *  Interest Ratio Formula: ( a / Log10(b + N) )
 *
 *  ==> ratio = a / Log10 (b+N)
 */
bool ComputeCDPInterest(CTxExecuteContext &context, const CCdpCoinPair& coinPair, uint64_t total_owed_scoins,
        int32_t beginHeight, int32_t endHeight, uint64_t &interestOut) {
    if (total_owed_scoins == 0) {
        interestOut = 0;
        return true;
    }

    list<CCdpInterestParamChange> changes;
    if (!context.pCw->sysParamCache.GetCdpInterestParamChanges(coinPair, beginHeight, endHeight, changes)) {
        return context.pState->DoS(100, ERRORMSG("get cdp interest param changes error! coinPiar=%s",
                                    coinPair.ToString()), REJECT_INVALID, "get-cdp-interest-param-changes-error");
    }

    interestOut = 0;
    for (auto &change : changes) {
        interestOut += ComputeCDPInterest(total_owed_scoins, change.begin_height, change.end_height,
                                        change.param_a, change.param_b);
    }

    LogPrint(BCLog::CDP, "beginHeight: %d, endHeight: %d, totalInterest: %llu\n",
             beginHeight, endHeight, interestOut);

    return true;
}

// CDP owner can redeem his or her CDP that are in liquidation list
bool CCDPStakeTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (assets_to_stake.size() != 1)
        return state.DoS(100, ERRORMSG("only support to stake one asset!"),
                        REJECT_INVALID, "invalid-stake-asset");

    const TokenSymbol &assetSymbol = assets_to_stake.begin()->first;
    if (!kCdpScoinSymbolSet.count(scoin_symbol))
        return state.DoS(100, ERRORMSG("invalid scoin=%s", scoin_symbol),
                        REJECT_INVALID, "invalid-CDP-SCoin-Symbol");

    if (assetSymbol == SYMB::WGRT || kCdpScoinSymbolSet.count(assetSymbol) > 0 ||
        !cw.assetCache.CheckAsset(assetSymbol, AssetPermType::PERM_CDP_BCOIN))
        return state.DoS(100, ERRORMSG("asset=%s can not be a bcoin", assetSymbol),
                        REJECT_INVALID, "invalid-CDP-BCoin-Symbol");

    return true;
}

bool CCDPStakeTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    //0. check preconditions
    assert(assets_to_stake.size() == 1);
    const TokenSymbol &assetSymbol = assets_to_stake.begin()->first;
    uint64_t assetAmount = assets_to_stake.begin()->second.get();
    CCdpCoinPair cdpCoinPair(assetSymbol, scoin_symbol);

    const TokenSymbol &quoteSymbol = GetQuoteSymbolByCdpScoin(scoin_symbol);
    if (quoteSymbol.empty())
        return state.DoS(100, ERRORMSG("get price quote by cdp scoin=%s failed!", scoin_symbol),
                        REJECT_INVALID, "get-price-quote-by-cdp-scoin-failed");

    uint64_t bcoinMedianPrice = 0;
    if (!GetBcoinMedianPrice(*this, context, cdpCoinPair, bcoinMedianPrice))
        return false;

    uint64_t globalCollateralRatioMin;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CdpParamType::CDP_GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioMin))
        return false;

    CCdpGlobalData cdpGlobalData = cw.cdpCache.GetCdpGlobalData(cdpCoinPair);
    uint64_t globalCollateralRatio = cdpGlobalData.GetCollateralRatio(bcoinMedianPrice);

    // FIXME :: remove test net compatible
    if ( SysCfg().NetworkID() != NET_TYPE::TEST_NET  && globalCollateralRatio < globalCollateralRatioMin) {
        return state.DoS(100, ERRORMSG("GlobalCollateralFloorReached! ratio=%llu,"
                " min=%llu", globalCollateralRatio, globalCollateralRatioMin),
                REJECT_INVALID, "global-collateral-floor-reached");
    }

    uint64_t globalCollateralCeiling;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CdpParamType::CDP_GLOBAL_COLLATERAL_CEILING_AMOUNT,
            globalCollateralCeiling)) {
        return false;
    }

    if (cdpGlobalData.CheckGlobalCollateralCeilingReached(assetAmount, globalCollateralCeiling)) {
        return state.DoS(100, ERRORMSG("GlobalCollateralCeilingReached!"),
                        REJECT_INVALID, "global-collateral-ceiling-reached");
    }

    LogPrint(BCLog::CDP,
             "CCDPStakeTx::ExecuteTx, globalCollateralRatioMin: %llu, bcoinMedianPrice: %llu, globalCollateralCeiling: %llu\n",
             globalCollateralRatioMin, bcoinMedianPrice, globalCollateralCeiling);

    //2. check collateral ratio: parital or total >= 200%
    uint64_t startingCdpCollateralRatio;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CdpParamType::CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio))
        return state.DoS(100, ERRORMSG("read CDP_START_COLLATERAL_RATIO error!!"),
                        READ_SYS_PARAM_FAIL, "read-sysparamdb-error");

    uint64_t newMintScoins = scoins_to_mint;

    if (cdp_txid.IsEmpty()) { // 1st-time CDP creation
        if (assetAmount == 0 || scoins_to_mint == 0)
            return state.DoS(100, ERRORMSG("invalid amount"), REJECT_INVALID, "invalid-amount");

        vector<CUserCDP> userCdps;
        if (cw.cdpCache.UserHaveCdp(txAccount.regid, assetSymbol, scoin_symbol)) {
            return state.DoS(100, ERRORMSG("the user (regid=%s) has existing CDP (txid=%s)!"
                            "asset_symbol=%s, scoin_symbol=%s",
                             GetHash().GetHex(), txAccount.regid.ToString(), assetSymbol, scoin_symbol),
                             REJECT_INVALID, "user-cdp-created");
        }

        uint64_t collateralRatio = CalcCollateralRatio(assetAmount, scoins_to_mint, bcoinMedianPrice);
        if (collateralRatio < startingCdpCollateralRatio)
            return state.DoS(100,
                             ERRORMSG("1st-time CDP creation, collateral ratio (%.2f%%) is "
                                      "smaller than the minimal (%.2f%%), price: %llu",
                                      100.0 * collateralRatio / RATIO_BOOST,
                                      100.0 * startingCdpCollateralRatio / RATIO_BOOST, bcoinMedianPrice),
                             REJECT_INVALID, "CDP-collateral-ratio-toosmall");

        CUserCDP cdp(txAccount.regid, GetHash(), context.height, assetSymbol, scoin_symbol, assetAmount, scoins_to_mint);

        if (!cw.cdpCache.NewCDP(context.height, cdp)) {
            return state.DoS(100, ERRORMSG("save new cdp to db failed"),
                            READ_SYS_PARAM_FAIL, "save-new-cdp-failed");
        }

        uint64_t bcoinsToStakeAmountMinInScoin;
        if (!ReadCdpParam(*this, context, cdpCoinPair, CdpParamType::CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN,
                        bcoinsToStakeAmountMinInScoin)) {
            return false;
        }

        uint64_t bcoinsToStakeAmountMin = bcoinsToStakeAmountMinInScoin / (double(bcoinMedianPrice) / PRICE_BOOST);
        if (cdp.total_staked_bcoins < bcoinsToStakeAmountMin) {
            return state.DoS(100, ERRORMSG("total staked bcoins (%llu vs %llu) is too small, price: %llu",
                            cdp.total_staked_bcoins, bcoinsToStakeAmountMin, bcoinMedianPrice), REJECT_INVALID,
                            "total-staked-bcoins-too-small");
        }
    } else { // further staking on one's existing CDP
        CUserCDP cdp;
        if (!cw.cdpCache.GetCDP(cdp_txid, cdp))
            return state.DoS(100, ERRORMSG("the cdp not exist! cdp_txid=%s", cdp_txid.ToString()),
                             REJECT_INVALID, "cdp-not-exist");

        if (assetSymbol != cdp.bcoin_symbol)
            return state.DoS(100, ERRORMSG("the asset symbol=%s does not match with the current CDP's=%s",
                            assetSymbol, cdp.bcoin_symbol), REJECT_INVALID, "invalid-asset-symbol");

        if (txAccount.regid != cdp.owner_regid)
            return state.DoS(100, ERRORMSG("permission denied! cdp_txid=%s, owner(%s) vs operator(%s)",
                            cdp_txid.ToString(), cdp.owner_regid.ToString(), txUid.ToString()), REJECT_INVALID, "permission-denied");

        CUserCDP oldCDP = cdp; // copy before modify.

        if (context.height < cdp.block_height) {
            return state.DoS(100, ERRORMSG("height: %d < cdp.block_height: %d",
                            context.height, cdp.block_height), UPDATE_ACCOUNT_FAIL, "height-error");
        }

        uint64_t scoinsInterestToRepay = 0;
        if (!ComputeCDPInterest(context, cdpCoinPair, cdp.total_owed_scoins, cdp.block_height, context.height,
                                scoinsInterestToRepay)) {
            return false;
        }

        uint64_t ownerScoins = txAccount.GetToken(scoin_symbol).free_amount;
        uint64_t mintScoinForInterest = 0;
        if (scoinsInterestToRepay > ownerScoins) {
            mintScoinForInterest = scoinsInterestToRepay - ownerScoins;
            txAccount.OperateBalance(scoin_symbol, BalanceOpType::ADD_FREE, mintScoinForInterest,
                                   ReceiptType::CDP_MINTED_SCOIN_TO_OWNER, receipts);
            LogPrint(BCLog::CDP, "Mint scoins=%llu for interest!\n", mintScoinForInterest);
        }

        newMintScoins          = scoins_to_mint + mintScoinForInterest;
        uint64_t totalBcoinsToStake     = cdp.total_staked_bcoins + assetAmount;
        uint64_t totalScoinsToOwe       = cdp.total_owed_scoins + newMintScoins;
        uint64_t partialCollateralRatio = CalcCollateralRatio(assetAmount, newMintScoins, bcoinMedianPrice);
        uint64_t totalCollateralRatio   = CalcCollateralRatio(totalBcoinsToStake, totalScoinsToOwe, bcoinMedianPrice);

        if (partialCollateralRatio < startingCdpCollateralRatio && totalCollateralRatio < startingCdpCollateralRatio) {
            return state.DoS(100,
                             ERRORMSG("further staking CDP, collateral ratio (partial=%.2f%%, "
                                      "total=%.2f%%) is smaller than the minimal, price: %llu",
                                      100.0 * partialCollateralRatio / RATIO_BOOST,
                                      100.0 * totalCollateralRatio / RATIO_BOOST, bcoinMedianPrice),
                             REJECT_INVALID, "CDP-collateral-ratio-toosmall");
        }

        if (!SellInterestForFcoins(CTxCord(context.height, context.index), cdp, scoinsInterestToRepay, cw, state, receipts))
            return false;

        if (!txAccount.OperateBalance(scoin_symbol, BalanceOpType::SUB_FREE, scoinsInterestToRepay, ReceiptType::CDP_REPAY_INTEREST, receipts))
            return state.DoS(100, ERRORMSG("scoins balance < scoinsInterestToRepay: %llu",
                            scoinsInterestToRepay), UPDATE_ACCOUNT_FAIL,
                            strprintf("deduct-interest(%llu)-error", scoinsInterestToRepay));

        // settle cdp state & persist
        cdp.AddStake(context.height, assetAmount, scoins_to_mint);
        if (!cw.cdpCache.UpdateCDP(oldCDP, cdp))
            return state.DoS(100, ERRORMSG("save changed cdp to db failed"),
                            READ_SYS_PARAM_FAIL, "save-changed-cdp-failed");
    }

    // update account accordingly
    if (!txAccount.OperateBalance(assetSymbol, BalanceOpType::PLEDGE, assetAmount, ReceiptType::CDP_PLEDGED_ASSET_FROM_OWNER, receipts))
        return state.DoS(100, ERRORMSG("bcoins insufficient to pledge"), UPDATE_ACCOUNT_FAIL,
                         "bcoins-insufficient-error");

    if (!txAccount.OperateBalance(scoin_symbol, BalanceOpType::ADD_FREE, scoins_to_mint, ReceiptType::CDP_MINTED_SCOIN_TO_OWNER, receipts))
        return state.DoS(100, ERRORMSG("add scoins failed"), UPDATE_ACCOUNT_FAIL,
                         "add-scoins-error");

    return true;
}

string CCDPStakeTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, valid_height=%llu, cdp_txid=%s, assets_to_stake=%s, "
        "scoin_symbol=%s, scoins_to_mint=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), valid_height,
        cdp_txid.ToString(), cdp_util::ToString(assets_to_stake), scoin_symbol, scoins_to_mint);
}

Object CCDPStakeTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    TxID cdpId = cdp_txid;
    if (cdpId.IsEmpty()) { // this is new cdp tx
        cdpId = GetHash();
    }

    result.push_back(Pair("cdp_txid",           cdpId.ToString()));
    result.push_back(Pair("assets_to_stake",    cdp_util::ToJson(assets_to_stake)));
    result.push_back(Pair("scoin_symbol",       scoin_symbol));
    result.push_back(Pair("scoins_to_mint",     scoins_to_mint));

    return result;
}

bool CCDPStakeTx::SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp,
                                        const uint64_t scoinsInterestToRepay, CCacheWrapper &cw,
                                        CValidationState &state, vector<CReceipt> &receipts) {
    if (scoinsInterestToRepay == 0)
        return true;

    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    // send interest to fcoin genesis account
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, scoinsInterestToRepay,
                                            ReceiptType::CDP_REPAY_INTEREST_TO_FUND, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, operate balance failed"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    // should freeze user's coin for buying the asset
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::FREEZE, scoinsInterestToRepay,
                                            ReceiptType::CDP_REPAY_INTEREST_TO_FUND, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, account has insufficient funds"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, set account info error"),
                        WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    auto pSysBuyMarketOrder = dex::CSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol,
        SYMB::WGRT, scoinsInterestToRepay);
    if (!cw.dexCache.CreateActiveOrder(GetHash(), *pSysBuyMarketOrder)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }
    assert(!fcoinGenesisAccount.regid.IsEmpty());
    receipts.emplace_back(ReceiptType::CDP_INTEREST_BUY_DEFLATE_FCOINS, BalanceOpType::FREEZE,
                        txUid, fcoinGenesisAccount.regid, cdp.scoin_symbol, scoinsInterestToRepay);

    return true;
}

/************************************<< CCDPRedeemTx >>***********************************************/
bool CCDPRedeemTx::CheckTx(CTxExecuteContext &context) {
    CValidationState &state = *context.pState;

    if (cdp_txid.IsEmpty())
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::CheckTx, cdp_txid is empty"),
                        REJECT_INVALID, "empty-cdpid");

    return true;
}

bool CCDPRedeemTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    //0. check preconditions
    CUserCDP cdp;
    if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, cdp (%s) not exist", cdp_txid.ToString()),
                         REJECT_INVALID, "cdp-not-exist");
    }

    if (assets_to_redeem.size() != 1) {
        return state.DoS(100, ERRORMSG("only support to redeem one asset!"),
                        REJECT_INVALID, "invalid-stake-asset");
    }
    const TokenSymbol &assetSymbol = assets_to_redeem.begin()->first;
    uint64_t assetAmount = assets_to_redeem.begin()->second.get();
    if (assetSymbol != cdp.bcoin_symbol)
        return state.DoS(100, ERRORMSG("asset symbol to redeem is not match!"),
                        REJECT_INVALID, "invalid-stake-asset");

    if (txAccount.regid != cdp.owner_regid) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, permission denied! cdp_txid=%s, owner(%s) vs operator(%s)",
                        cdp_txid.ToString(), cdp.owner_regid.ToString(), txUid.ToString()), REJECT_INVALID, "permission-denied");
    }

    CCdpCoinPair cdpCoinPair(cdp.bcoin_symbol, cdp.scoin_symbol);
    CUserCDP oldCDP = cdp; // copy before modify.

    uint64_t globalCollateralRatioFloor;

    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor)) {
        return false;
    }

    uint64_t bcoinMedianPrice = 0;
    if (!GetBcoinMedianPrice(*this, context, cdpCoinPair, bcoinMedianPrice)) return false;

    CCdpGlobalData cdpGlobalData = cw.cdpCache.GetCdpGlobalData(cdpCoinPair);
    if (cdpGlobalData.CheckGlobalCollateralRatioFloorReached(bcoinMedianPrice, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, GlobalCollateralFloorReached!!"), REJECT_INVALID,
                         "global-cdp-lock-is-on");
    }

    //1. pay interest fees in wusd
    if (context.height < cdp.block_height) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, height: %d < cdp.block_height: %d",
                        context.height, cdp.block_height), UPDATE_ACCOUNT_FAIL, "height-error");
    }

    uint64_t scoinsInterestToRepay = 0;
    if (!ComputeCDPInterest(context, cdpCoinPair, cdp.total_owed_scoins, cdp.block_height, context.height,
            scoinsInterestToRepay)) {
        return false;
    }

    if (!txAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::SUB_FREE, scoinsInterestToRepay,
                                ReceiptType::CDP_REPAY_INTEREST, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, Deduct interest error!"),
                        REJECT_INVALID, "deduct-interest-error");
    }

    if (!SellInterestForFcoins(CTxCord(context.height, context.index), cdp, scoinsInterestToRepay, cw, state, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, SellInterestForFcoins error!"),
                        REJECT_INVALID, "sell-interest-for-fcoins-error");
    }

    uint64_t startingCdpCollateralRatio;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_START_COLLATERAL_RATIO, startingCdpCollateralRatio))
        return false;

    //2. redeem in scoins and update cdp
    if (assetAmount > cdp.total_staked_bcoins) {
        LogPrint(BCLog::CDP, "CCDPRedeemTx::ExecuteTx, the redeemed bcoins=%llu is bigger than total_staked_bcoins=%llu, use the min one",
                assetAmount, cdp.total_staked_bcoins);

        assetAmount = cdp.total_staked_bcoins;
    }
    uint64_t actualScoinsToRepay = scoins_to_repay;
    if (actualScoinsToRepay > cdp.total_owed_scoins) {
        LogPrint(BCLog::CDP, "CCDPRedeemTx::ExecuteTx, the repay scoins=%llu is bigger than total_owed_scoins=%llu, use the min one",
                actualScoinsToRepay, cdp.total_staked_bcoins);

        actualScoinsToRepay = cdp.total_owed_scoins;
    }

    // check account balance vs scoins_to_repay
    if (txAccount.GetToken(cdp.scoin_symbol).free_amount < scoins_to_repay) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, account balance insufficient"), REJECT_INVALID,
                         "account-balance-insufficient");
    }

    cdp.Redeem(context.height, assetAmount, actualScoinsToRepay);

    // check and save CDP to db
    if (cdp.IsFinished()) {
        if (!cw.cdpCache.EraseCDP(oldCDP, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, erase the finished CDP %s failed",
                            cdp.cdpid.ToString()), UPDATE_CDP_FAIL, "erase-cdp-failed");

        } else {
            if (SysCfg().GetArg("-persistclosedcdp", false)) {
                if (!cw.closedCdpCache.AddClosedCdpIndex(oldCDP.cdpid, GetHash(), CDPCloseType::BY_REDEEM)) {
                    LogPrint(BCLog::ERROR, "persistclosedcdp AddClosedCdpIndex failed for redeemed cdpid (%s)", oldCDP.cdpid.GetHex());
                }

                if (!cw.closedCdpCache.AddClosedCdpTxIndex(GetHash(), oldCDP.cdpid, CDPCloseType::BY_REDEEM)) {
                    LogPrint(BCLog::ERROR, "persistclosedcdp AddClosedCdpTxIndex failed for redeemed cdpid (%s)", oldCDP.cdpid.GetHex());
                }
            }
        }
    } else { // partial redeem
        if (assetAmount != 0) {
            uint64_t collateralRatio  = cdp.GetCollateralRatio(bcoinMedianPrice);
            if (collateralRatio < startingCdpCollateralRatio) {
                return state.DoS(100,
                                 ERRORMSG("CCDPRedeemTx::ExecuteTx, CDP collatera ratio=%.2f%% < %.2f%% error"
                                          "after redeem, price: %llu",
                                          100.0 * collateralRatio / RATIO_BOOST,
                                          100.0 * startingCdpCollateralRatio / RATIO_BOOST, bcoinMedianPrice),
                                 UPDATE_CDP_FAIL, "invalid-collatera-ratio");
            }

            uint64_t bcoinsToStakeAmountMinInScoin;

           if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN,
                            bcoinsToStakeAmountMinInScoin))
                return false;

            uint64_t bcoinsToStakeAmountMin = bcoinsToStakeAmountMinInScoin / (double(bcoinMedianPrice) / PRICE_BOOST);
            if (cdp.total_staked_bcoins < bcoinsToStakeAmountMin) {
                return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, total staked bcoins (%llu vs %llu) is too small",
                                cdp.total_staked_bcoins, bcoinsToStakeAmountMin), REJECT_INVALID, "total-staked-bcoins-too-small");
            }
        }

        if (!cw.cdpCache.UpdateCDP(oldCDP, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update CDP %s failed", cdp.cdpid.ToString()),
                            UPDATE_CDP_FAIL, "bad-save-cdp");
        }
    }

    if (!txAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::SUB_FREE, actualScoinsToRepay,
                                ReceiptType::CDP_REPAID_SCOIN_FROM_OWNER, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update account(%s) SUB WUSD(%lu) failed",
                        txAccount.regid.ToString(), actualScoinsToRepay), UPDATE_ACCOUNT_FAIL, "bad-operate-account");
    }
    if (!txAccount.OperateBalance(cdp.bcoin_symbol, BalanceOpType::UNPLEDGE, assetAmount,
                                ReceiptType::CDP_REDEEMED_ASSET_TO_OWNER, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::ExecuteTx, update account(%s) ADD WICC(%lu) failed",
                        txAccount.regid.ToString(), assetAmount), UPDATE_ACCOUNT_FAIL, "bad-operate-account");
    }

    return true;
}

string CCDPRedeemTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, valid_height=%llu, cdp_txid=%s, scoins_to_repay=%d, "
        "assets_to_redeem=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), valid_height,
        cdp_txid.ToString(), scoins_to_repay, cdp_util::ToString(assets_to_redeem));
}

Object CCDPRedeemTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("cdp_txid",           cdp_txid.ToString()));
    result.push_back(Pair("scoins_to_repay",    scoins_to_repay));
    result.push_back(Pair("assets_to_redeem",   cdp_util::ToJson(assets_to_redeem)));

    return result;
}

bool CCDPRedeemTx::SellInterestForFcoins(const CTxCord &txCord, const CUserCDP &cdp,
                                        const uint64_t scoinsInterestToRepay, CCacheWrapper &cw,
                                        CValidationState &state, vector<CReceipt> &receipts) {
    if (scoinsInterestToRepay == 0)
        return true;

    CAccount fcoinGenesisAccount;
    cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount);
    // send interest to fcoin genesis account
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::ADD_FREE, scoinsInterestToRepay,
                                            ReceiptType::CDP_INTEREST_BUY_DEFLATE_FCOINS, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, operate balance failed"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    // should freeze user's coin for buying the asset
    if (!fcoinGenesisAccount.OperateBalance(SYMB::WUSD, BalanceOpType::FREEZE, scoinsInterestToRepay,
                                            ReceiptType::CDP_INTEREST_BUY_DEFLATE_FCOINS, receipts)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, account has insufficient funds"),
                        UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, set account info error"),
                        WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    auto pSysBuyMarketOrder = dex::CSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol,
                                                                SYMB::WGRT, scoinsInterestToRepay);

    if (!cw.dexCache.CreateActiveOrder(GetHash(), *pSysBuyMarketOrder)) {
        return state.DoS(100, ERRORMSG("CCDPRedeemTx::SellInterestForFcoins, create system buy order failed"),
                        CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
    }

    assert(!fcoinGenesisAccount.regid.IsEmpty());

    return true;
}

 /************************************<< CdpLiquidateTx >>***********************************************/
 bool CCDPLiquidateTx::CheckTx(CTxExecuteContext &context) {
     CValidationState &state = *context.pState;

    if (scoins_to_liquidate == 0)
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, invalid liquidate amount(0)"), REJECT_INVALID,
                         "invalid-liquidate-amount");

    if (cdp_txid.IsEmpty())
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::CheckTx, cdp_txid is empty"), REJECT_INVALID, "empty-cdpid");

    return true;
}

/**
  * total_staked_bcoinsInScoins : total_owed_scoins = M : N
  *
  * Liquidator paid         1.13lN          (0 < l ≤ 100%)
  *   Liquidate Amount:     l * N       = lN
  *   Penalty Fees:         l * N * 13% = 0.13lN
  * Liquidator received:    Bcoins only
  *   Bcoins:               1.13lN ~ 1.16lN (WICC)
  *   Net Profit:           0 ~ 0.03lN (WICC)
  *
  * CDP Owner returned
  *   Bcoins:               lM - 1.16lN = l(M - 1.16N)
  *
  *  when M is 1.16 N and below, there'll be no return to the CDP owner
  *  when M is 1.13 N and below, there'll be no profit for the liquidator, hence requiring force settlement
  */
bool CCDPLiquidateTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    //0. check preconditions
    CUserCDP cdp;
    if (!cw.cdpCache.GetCDP(cdp_txid, cdp)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp (%s) not exist!",
                        txUid.ToString()), REJECT_INVALID, "cdp-not-exist");
    }

    if (!liquidate_asset_symbol.empty() && liquidate_asset_symbol != cdp.bcoin_symbol)
        return state.DoS(100, ERRORMSG("the liquidate_asset_symbol=%s must be empty of match with the asset symbols of CDP",
            liquidate_asset_symbol), REJECT_INVALID, "invalid-asset-symbol");

    CCdpCoinPair cdpCoinPair(cdp.bcoin_symbol, cdp.scoin_symbol);
    CUserCDP oldCDP = cdp; // copy before modify.

    uint64_t free_scoins = txAccount.GetToken(cdp.scoin_symbol).free_amount;
    if (free_scoins < scoins_to_liquidate) {  // more applicable when scoinPenalty is omitted
        return state.DoS(100, ERRORMSG("CdpLiquidateTx::ExecuteTx, account scoins %d < scoins_to_liquidate: %d", free_scoins,
                        scoins_to_liquidate), CDP_LIQUIDATE_FAIL, "account-scoins-insufficient");
    }

    uint64_t bcoinMedianPrice = 0;
    if (!GetBcoinMedianPrice(*this, context, cdpCoinPair, bcoinMedianPrice)) return false;

    uint64_t globalCollateralRatioFloor;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_GLOBAL_COLLATERAL_RATIO_MIN, globalCollateralRatioFloor))
        return false;

    CCdpGlobalData cdpGlobalData = cw.cdpCache.GetCdpGlobalData(cdpCoinPair);
    if (cdpGlobalData.CheckGlobalCollateralRatioFloorReached(bcoinMedianPrice, globalCollateralRatioFloor)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, GlobalCollateralFloorReached!!"), REJECT_INVALID,
                         "global-cdp-lock-is-on");
    }

    //1. pay penalty fees: 0.13lN --> 50% burn, 50% to Risk Reserve
    CAccount cdpOwnerAccount;
    if (!cw.accountCache.GetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount)) {
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, read CDP Owner txUid %s account info error",
                        txUid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    uint64_t startingCdpLiquidateRatio;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_START_LIQUIDATE_RATIO, startingCdpLiquidateRatio))
        return false;

    uint64_t nonReturnCdpLiquidateRatio;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_NONRETURN_LIQUIDATE_RATIO, nonReturnCdpLiquidateRatio))
        return false;

    uint64_t cdpLiquidateDiscountRate;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_LIQUIDATE_DISCOUNT_RATIO, cdpLiquidateDiscountRate))
        return false;

    uint64_t forcedCdpLiquidateRatio;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_FORCE_LIQUIDATE_RATIO, forcedCdpLiquidateRatio))
        return false;

    uint64_t totalBcoinsToReturnLiquidator = 0;
    uint64_t totalScoinsToLiquidate        = 0;
    uint64_t totalScoinsToReturnSysFund    = 0;
    uint64_t totalBcoinsToCdpOwner         = 0;

    uint64_t collateralRatio = cdp.GetCollateralRatio(bcoinMedianPrice);
    if (collateralRatio > startingCdpLiquidateRatio) {  // 1.5++
        return state.DoS(100,
                         ERRORMSG("CCDPLiquidateTx::ExecuteTx, cdp collateralRatio(%.2f%%) > %.2f%%, price: %llu",
                                  100.0 * collateralRatio / RATIO_BOOST,
                                  100.0 * startingCdpLiquidateRatio / RATIO_BOOST, bcoinMedianPrice),
                         REJECT_INVALID, "cdp-not-liquidate-ready");

    } else if (collateralRatio > nonReturnCdpLiquidateRatio) { // 1.13 ~ 1.5
        totalBcoinsToReturnLiquidator = cdp.total_owed_scoins * (double)nonReturnCdpLiquidateRatio / RATIO_BOOST /
                                        ((double)bcoinMedianPrice / PRICE_BOOST);  // 1.13N
        assert(cdp.total_staked_bcoins >= totalBcoinsToReturnLiquidator);

        totalBcoinsToCdpOwner = cdp.total_staked_bcoins - totalBcoinsToReturnLiquidator;

        totalScoinsToLiquidate = ( cdp.total_owed_scoins * (double)nonReturnCdpLiquidateRatio / RATIO_BOOST )
                                * cdpLiquidateDiscountRate / RATIO_BOOST; //1.096N

        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.total_owed_scoins;

    } else if (collateralRatio > forcedCdpLiquidateRatio) {    // 1.04 ~ 1.13
        totalBcoinsToReturnLiquidator = cdp.total_staked_bcoins; //M
        totalBcoinsToCdpOwner = 0;
        totalScoinsToLiquidate = totalBcoinsToReturnLiquidator * ((double) bcoinMedianPrice / PRICE_BOOST)
                                * cdpLiquidateDiscountRate / RATIO_BOOST; //M * 97%

        totalScoinsToReturnSysFund = totalScoinsToLiquidate - cdp.total_owed_scoins; // M * 97% - N

    } else {                                                    // 0 ~ 1.04
        // Although not likely to happen, but when it does, execute it accordingly.
        totalBcoinsToReturnLiquidator = cdp.total_staked_bcoins;
        totalBcoinsToCdpOwner         = 0;
        totalScoinsToLiquidate        = cdp.total_owed_scoins;  // N
        totalScoinsToReturnSysFund    = 0;
    }

    if (scoins_to_liquidate >= totalScoinsToLiquidate) {
        if (!txAccount.OperateBalance(cdp.scoin_symbol, SUB_FREE, totalScoinsToLiquidate,
                                    ReceiptType::CDP_SCOIN_FROM_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, deduct scoins from regId=%s failed",
                            txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-scoins-failed");
        }
        if (!txAccount.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToReturnLiquidator,
                                    ReceiptType::CDP_ASSET_TO_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, add bcoins failed"), UPDATE_ACCOUNT_FAIL,
                             "add-bcoins-failed");
        }

        // clean up cdp owner's pledged_amount
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, totalBcoinsToReturnLiquidator,
                                            ReceiptType::CDP_ASSET_TO_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, unpledge bcoins failed"), UPDATE_ACCOUNT_FAIL,
                             "unpledge-bcoins-failed");
        }
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, SUB_FREE, totalBcoinsToReturnLiquidator,
                                            ReceiptType::CDP_ASSET_TO_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, sub unpledged bcoins failed"), UPDATE_ACCOUNT_FAIL,
                             "deduct-bcoins-failed");
        }

        if (txAccount.regid != cdpOwnerAccount.regid) { //liquidate by others
            if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, totalBcoinsToCdpOwner,
                                                ReceiptType::CDP_LIQUIDATED_ASSET_TO_OWNER, receipts)) {
                return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, unpledge bcoins failed"), UPDATE_ACCOUNT_FAIL,
                                 "unpledge-bcoins-failed");
            }
            if (!cw.accountCache.SetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount))
                return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, write cdp owner account info error! owner_regid=%s",
                                cdp.owner_regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        } else {  // liquidate by oneself
            if (!txAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, totalBcoinsToCdpOwner,
                                        ReceiptType::CDP_LIQUIDATED_ASSET_TO_OWNER, receipts)) {
                return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, unpledge bcoins failed"), UPDATE_ACCOUNT_FAIL,
                                 "unpledge-bcoins-failed");
            }
        }

        if (!ProcessPenaltyFees(context, cdp, (uint64_t)totalScoinsToReturnSysFund, receipts))
            return false;

        // close CDP
        if (!cw.cdpCache.EraseCDP(oldCDP, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, erase CDP failed! cdpid=%s",
                        cdp.cdpid.ToString()), UPDATE_CDP_FAIL, "erase-cdp-failed");

        } else if (SysCfg().GetArg("-persistclosedcdp", false)) {
            if (!cw.closedCdpCache.AddClosedCdpIndex(oldCDP.cdpid, GetHash(), CDPCloseType::BY_MANUAL_LIQUIDATE)) {
                LogPrint(BCLog::ERROR, "persistclosedcdp AddClosedCdpIndex failed for redeemed cdpid (%s)", oldCDP.cdpid.GetHex());
            }

            if (!cw.closedCdpCache.AddClosedCdpTxIndex(GetHash(), oldCDP.cdpid, CDPCloseType::BY_MANUAL_LIQUIDATE)) {
                LogPrint(BCLog::ERROR, "persistclosedcdp AddClosedCdpTxIndex failed for redeemed cdpid (%s)", oldCDP.cdpid.GetHex());
            }
        }

    } else {    // partial liquidation
        double liquidateRate = (double)scoins_to_liquidate / totalScoinsToLiquidate;  // unboosted on purpose
        assert(liquidateRate < 1);
        totalBcoinsToReturnLiquidator *= liquidateRate;

        if (!txAccount.OperateBalance(cdp.scoin_symbol, SUB_FREE, scoins_to_liquidate,
                                    ReceiptType::CDP_SCOIN_FROM_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, sub scoins to liquidator failed"),
                             UPDATE_ACCOUNT_FAIL, "sub-scoins-to-liquidator-failed");
        }
        if (!txAccount.OperateBalance(cdp.bcoin_symbol, ADD_FREE, totalBcoinsToReturnLiquidator,
                                    ReceiptType::CDP_ASSET_TO_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, add bcoins to liquidator failed"),
                             UPDATE_ACCOUNT_FAIL, "add-bcoins-to-liquidator-failed");
        }

        // clean up cdp owner's pledged_amount
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, totalBcoinsToReturnLiquidator,
                                            ReceiptType::CDP_ASSET_TO_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, unpledge bcoins failed"), UPDATE_ACCOUNT_FAIL,
                             "unpledge-bcoins-failed");
        }
        if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, SUB_FREE, totalBcoinsToReturnLiquidator,
                                            ReceiptType::CDP_ASSET_TO_LIQUIDATOR, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, sub unpledged bcoins failed"), UPDATE_ACCOUNT_FAIL,
                             "deduct-bcoins-failed");
        }

        uint64_t bcoinsToCDPOwner = totalBcoinsToCdpOwner * liquidateRate;
        if (txAccount.regid != cdpOwnerAccount.regid) {
            if (!cdpOwnerAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, bcoinsToCDPOwner,
                                                ReceiptType::CDP_LIQUIDATED_ASSET_TO_OWNER, receipts)) {
                return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, unpledge bcoins to cdp owner failed"),
                                 UPDATE_ACCOUNT_FAIL, "unpledge-bcoins-to-cdp-owner-failed");
            }
            if (!cw.accountCache.SetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount))
                return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, write cdp owner account info error! owner_regid=%s",
                                cdp.owner_regid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
        } else {  // liquidate by oneself
            if (!txAccount.OperateBalance(cdp.bcoin_symbol, UNPLEDGE, bcoinsToCDPOwner,
                                        ReceiptType::CDP_LIQUIDATED_ASSET_TO_OWNER, receipts)) {
                return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, unpledge bcoins to cdp owner failed"),
                                 UPDATE_ACCOUNT_FAIL, "unpledge-bcoins-to-cdp-owner-failed");
            }
        }

        uint64_t scoinsToCloseout = cdp.total_owed_scoins * liquidateRate;
        uint64_t bcoinsToLiquidate = totalBcoinsToReturnLiquidator + bcoinsToCDPOwner;

        assert(cdp.total_owed_scoins > scoinsToCloseout);
        assert(cdp.total_staked_bcoins > bcoinsToLiquidate);

        cdp.PartialLiquidate(context.height, bcoinsToLiquidate, scoinsToCloseout);

        uint64_t bcoinsToStakeAmountMinInScoin;
        if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_BCOINSTOSTAKE_AMOUNT_MIN_IN_SCOIN,
                        bcoinsToStakeAmountMinInScoin))
            return false;

        uint64_t bcoinsToStakeAmountMin = bcoinsToStakeAmountMinInScoin / (double(bcoinMedianPrice) / PRICE_BOOST);
        if (cdp.total_staked_bcoins < bcoinsToStakeAmountMin) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, total staked bcoins (%llu vs %llu) is too small, "
                            "txid=%s, cdp=%s, height=%d, price=%llu", cdp.total_staked_bcoins, bcoinsToStakeAmountMin,
                            GetHash().GetHex(), cdp.ToString(), context.height, bcoinMedianPrice),
                            REJECT_INVALID, "total-staked-bcoins-too-small");
        }

        CCdpCoinPair cdpCoinPair(cdp.bcoin_symbol, cdp.scoin_symbol);
        uint64_t scoinsToReturnSysFund = scoins_to_liquidate -  scoinsToCloseout;
        if (!ProcessPenaltyFees(context, cdp, scoinsToReturnSysFund, receipts))
            return false;

        if (!cw.cdpCache.UpdateCDP(oldCDP, cdp)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, update CDP failed! cdpid=%s",
                        cdp.cdpid.ToString()), UPDATE_CDP_FAIL, "bad-save-cdp");
        }
    }

    return true;
}

string CCDPLiquidateTx::ToString(CAccountDBCache &accountCache) {
    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    return strprintf(
        "txType=%s, hash=%s, ver=%d, txUid=%s, addr=%s, valid_height=%llu, cdp_txid=%s, liquidate_asset_symbol=%s, "
        "scoins_to_liquidate=%d",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), keyId.ToAddress(), valid_height,
        cdp_txid.ToString(), liquidate_asset_symbol, scoins_to_liquidate);
}

Object CCDPLiquidateTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("cdp_txid",               cdp_txid.ToString()));
    result.push_back(Pair("liquidate_asset_symbol", liquidate_asset_symbol));
    result.push_back(Pair("scoins_to_liquidate",    scoins_to_liquidate));

    return result;
}

bool CCDPLiquidateTx::ProcessPenaltyFees(CTxExecuteContext &context, const CUserCDP &cdp, uint64_t scoinPenaltyFees,
                                        vector<CReceipt> &receipts) {

    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;
    CTxCord txCord = CTxCord(context.height, context.index);

    if (scoinPenaltyFees == 0)
        return true;

    CAccount fcoinGenesisAccount;
    if (!cw.accountCache.GetFcoinGenesisAccount(fcoinGenesisAccount)) {
        return state.DoS(100, ERRORMSG("CCDPStakeTx::ProcessPenaltyFees, read fcoinGenesisUid %s account info error"),
                        READ_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    CCdpCoinPair cdpCoinPair(cdp.bcoin_symbol, cdp.scoin_symbol);
    uint64_t minSysOrderPenaltyFee;
    if (!ReadCdpParam(*this, context, cdpCoinPair, CDP_SYSORDER_PENALTY_FEE_MIN, minSysOrderPenaltyFee))
        return false;

    if (scoinPenaltyFees > minSysOrderPenaltyFee ) { //10+ WUSD
        uint64_t halfScoinsPenalty = scoinPenaltyFees / 2;
        uint64_t leftScoinPenalty  = scoinPenaltyFees - halfScoinsPenalty;  // handle odd amount

        // 1) save 50% penalty fees into risk reserve
        if (!fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, halfScoinsPenalty,
                                                ReceiptType::CDP_PENALTY_TO_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, add scoins to fcoin genesis account failed"),
                             UPDATE_ACCOUNT_FAIL, "add-scoins-to-fcoin-genesis-account-failed");
        }

        // 2) sell 50% penalty fees for Fcoins and burn
        // send half scoin penalty to fcoin genesis account
        if (!fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, leftScoinPenalty,
                                                ReceiptType::CDP_PENALTY_TO_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, add scoins to fcoin genesis account failed"),
                             UPDATE_ACCOUNT_FAIL, "add-scoins-to-fcoin-genesis-account-failed");
        }

        // should freeze user's coin for buying the asset
        if (!fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::FREEZE, leftScoinPenalty,
                                                ReceiptType::CDP_PENALTY_TO_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("CdpLiquidateTx::ProcessPenaltyFees, account has insufficient funds"),
                            UPDATE_ACCOUNT_FAIL, "operate-fcoin-genesis-account-failed");
        }

        auto pSysBuyMarketOrder = dex::CSysOrder::CreateBuyMarketOrder(txCord, cdp.scoin_symbol, SYMB::WGRT, leftScoinPenalty);
        if (!cw.dexCache.CreateActiveOrder(GetHash(), *pSysBuyMarketOrder)) {
            return state.DoS(100, ERRORMSG("CdpLiquidateTx::ProcessPenaltyFees, create system buy order failed"),
                            CREATE_SYS_ORDER_FAILED, "create-sys-order-failed");
        }

        CUserID fcoinGenesisUid(fcoinGenesisAccount.regid);
        receipts.emplace_back(ReceiptType::CDP_PENALTY_TO_RESERVE, BalanceOpType::FREEZE, nullId, fcoinGenesisUid, cdp.scoin_symbol, halfScoinsPenalty);
        receipts.emplace_back(ReceiptType::CDP_PENALTY_BUY_DEFLATE_FCOINS, BalanceOpType::FREEZE, nullId, fcoinGenesisUid, cdp.scoin_symbol, leftScoinPenalty);
    } else {
        // send penalty fees into risk reserve
        if (!fcoinGenesisAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, scoinPenaltyFees,
                                                ReceiptType::CDP_PENALTY_TO_RESERVE, receipts)) {
            return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ExecuteTx, add scoins to fcoin genesis account failed"),
                             UPDATE_ACCOUNT_FAIL, "add-scoins-to-fcoin-genesis-account-failed");
        }
    }

    if (!cw.accountCache.SetAccount(fcoinGenesisAccount.keyid, fcoinGenesisAccount))
        return state.DoS(100, ERRORMSG("CCDPLiquidateTx::ProcessPenaltyFees, write fcoin genesis account info error!"),
                UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");

    return true;
}


 /************************************<< CCDPInterestForceSettleTx >>***********************************************/
 bool CCDPInterestForceSettleTx::CheckTx(CTxExecuteContext &context) {
     CValidationState &state = *context.pState;
    auto sz = cdp_list.size();
    if ( sz == 0 || sz > CDP_SETTLE_INTEREST_MAX_COUNT)
        return state.DoS(100, ERRORMSG("%s, cdp_list size=%u is out of range[1, %u]", TX_ERR_TITLE, sz, CDP_SETTLE_INTEREST_MAX_COUNT),
            REJECT_INVALID, "invalid-cdp-list-size");
    if (!txUid.IsEmpty()) // txUid is reserved
        return state.DoS(100, ERRORMSG("%s, txUid must be empty", TX_ERR_TITLE),
            REJECT_INVALID, "invalid-txUid");
    if (!signature.empty()) // signature is reserved
        return state.DoS(100, ERRORMSG("%s, signature must be empty", TX_ERR_TITLE),
            REJECT_INVALID, "invalid-signature");
    return true;
}

bool CCDPInterestForceSettleTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    set<uint256> cdpidSet; // for check duplication
    for (auto cdpid : cdp_list) {
        // check duplication
        auto retIt = cdpidSet.emplace(cdpid);
        if (!retIt.second)
            return state.DoS(100, ERRORMSG("%s, duplicated cdp=%s in list!", TX_ERR_TITLE, cdpid.ToString()),
                    REJECT_INVALID, "duplicated-cdp");
        // get cdp info
        CUserCDP cdp;
        if (!cw.cdpCache.GetCDP(cdpid, cdp))
            return state.DoS(100, ERRORMSG("%s, cdp=%s not exist!", TX_ERR_TITLE, cdpid.ToString()),
                    REJECT_INVALID, "cdp-not-exist");
        const auto &cdpCoinPair = cdp.GetCoinPair();
        uint64_t cycleDays;
        if (!ReadCdpParam(*this, context, cdpCoinPair, CdpParamType::CDP_CONVERT_INTEREST_TO_DEBT_DAYS, cycleDays))
            return false;

        if (!cdp_util::CdpNeedSettleInterest(cdp.block_height, context.height, cycleDays)) {
            return state.DoS(100, ERRORMSG("%s, CDP does not reach the settlement cycle!"
                    " last_height=%u, cur_height=%u, cycleDays=%u", TX_ERR_TITLE,
                    cdp.block_height, context.height, cycleDays),
                    UPDATE_ACCOUNT_FAIL, "not-reach-sttlement-cycle");
        }

        CAccount cdpOwnerAccount;
        if (!cw.accountCache.GetAccount(CUserID(cdp.owner_regid), cdpOwnerAccount)) {
            return state.DoS(100, ERRORMSG("%s, read CDP Owner account info error! owner_regid=%s",
                        TX_ERR_TITLE, cdp.owner_regid.ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }

        CUserCDP oldCDP = cdp; // copy before modify.

        uint64_t mintScoinForInterest = 0;
        if (!ComputeCDPInterest(context, cdpCoinPair, cdp.total_owed_scoins, cdp.block_height, context.height,
                                mintScoinForInterest)) {
            return false;
        }

        txAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::ADD_FREE, mintScoinForInterest,
                                ReceiptType::CDP_MINTED_SCOIN_TO_OWNER, receipts);

        if (!cdp_util::SellInterestForFcoins(*this, context, cdp, mintScoinForInterest, receipts))
            return false; // error msg has been processed

        if (!txAccount.OperateBalance(cdp.scoin_symbol, BalanceOpType::SUB_FREE,
                                      mintScoinForInterest, ReceiptType::CDP_REPAY_INTEREST,
                                      receipts))
            return state.DoS(100, ERRORMSG("scoins balance < scoinsInterestToRepay: %llu",
                            mintScoinForInterest), UPDATE_ACCOUNT_FAIL,
                            strprintf("deduct-interest(%llu)-error", mintScoinForInterest));

        // settle cdp state & persist
        cdp.AddStake(context.height, 0, mintScoinForInterest);
        if (!cw.cdpCache.UpdateCDP(oldCDP, cdp))
            return state.DoS(100, ERRORMSG("save changed cdp to db failed"),
                            READ_SYS_PARAM_FAIL, "save-changed-cdp-failed");

        LogPrint(BCLog::CDP, "%s, settle interest for cdp! cdpid=%s, cdp={%s}, interest=%llu\n", TX_ERR_TITLE,
            cdpid.ToString(), cdp.ToString(), mintScoinForInterest);
    }

    return true;
}

string CCDPInterestForceSettleTx::ToString(CAccountDBCache &accountCache) {
    string cdpListStr;
    for (const auto &cdpid : cdp_list) {
        cdpListStr += cdpid.ToString() + ",";
    }

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, valid_height=%llu, cdp_list={%s}",
        GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), valid_height,
        cdpListStr);
}

Object CCDPInterestForceSettleTx::ToJson(const CAccountDBCache &accountCache) const {
    Array cdpArray;
    for (const auto &cdpid : cdp_list) {
        cdpArray.push_back(cdpid.ToString());
    }
    Object result = CBaseTx::ToJson(accountCache);
    result.push_back(Pair("txid",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("valid_height",   valid_height));
    result.push_back(Pair("signature",      HexStr(signature)));

    result.push_back(Pair("cdp_list",       cdpArray));
    return result;
}

bool GetSettledInterestCdps(CCacheWrapper &cw, HeightType height, const CCdpCoinPair &cdpCoinPair,
                            vector<uint256> &cdpList, uint32_t &count) {
    auto pIt = cw.cdpCache.CreateCdpHeightIndexIt();
    uint64_t cycleDays;
    if (!cw.sysParamCache.GetCdpParam(cdpCoinPair, CDP_CONVERT_INTEREST_TO_DEBT_DAYS, cycleDays))
        return ERRORMSG("read cdp param CDP_CONVERT_INTEREST_TO_DEBT_DAYS error! cdpCoinPair=%s", cdpCoinPair.ToString());

    for (pIt->First(); pIt->IsValid(); pIt->Next()) {
        if (!cdp_util::CdpNeedSettleInterest(pIt->GetHeight(), height, cycleDays)) {
            break;
        }
        count--;
        if (count == 0)
            break;

        cdpList.push_back(pIt->GetCdpId());
    }
    return true;
}

bool GetSettledInterestCdps(CCacheWrapper &cw, HeightType height, vector<uint256> &cdpList) {

    PriceDetailMap medianPrices = cw.priceFeedCache.GetMedianPrices();

    uint64_t priceTimeoutBlocks = 0;
    if (!cw.sysParamCache.GetParam(SysParamType::PRICE_FEED_TIMEOUT_BLOCKS, priceTimeoutBlocks)) {
        return ERRORMSG("%s, read sys param PRICE_FEED_TIMEOUT_BLOCKS error", __func__);
    }

    Array cdpInfoArray;
    uint32_t count = CDP_SETTLE_INTEREST_MAX_COUNT;

    for (const auto& item : medianPrices) {
        if (item.first == kFcoinPriceCoinPair) continue;

        CAsset asset;
        const TokenSymbol &bcoinSymbol = item.first.first;
        const TokenSymbol &quoteSymbol = item.first.second;

        TokenSymbol scoinSymbol = GetCdpScoinByQuoteSymbol(quoteSymbol);
        if (scoinSymbol.empty()) {
            LogPrint(BCLog::CDP, "quote_symbol=%s not have a corresponding scoin , ignore", bcoinSymbol);
            continue;
        }

        // TODO: remove me if need to support multi scoin and improve the force liquidate process
        if (scoinSymbol != SYMB::WUSD)
            throw runtime_error(strprintf("only support to force liquidate scoin=WUSD, actual_scoin=%s", scoinSymbol));

        if (!cw.assetCache.CheckAsset(bcoinSymbol, AssetPermType::PERM_CDP_BCOIN)) {
            LogPrint(BCLog::CDP, "base_symbol=%s not have cdp bcoin permission, ignore", bcoinSymbol);
            continue;
        }

        if (!cw.cdpCache.IsBcoinActivated(bcoinSymbol)) {
            LogPrint(BCLog::CDP, "bcoin=%s does not be activated, ignore", bcoinSymbol);
            continue;
        }

        if (item.second.price == 0) {
            LogPrint(BCLog::CDP, "coin_pair(%s) price=0, ignore\n", CoinPairToString(item.first));
            continue;
        }
        CCdpCoinPair cdpCoinPair(bcoinSymbol, scoinSymbol);
        if (!GetSettledInterestCdps(cw, height, cdpCoinPair, cdpList, count)) {
            return ERRORMSG("get settled interest cdps error! coin_pair=%s", cdpCoinPair.ToString());
        }
    }
    return true;
}
