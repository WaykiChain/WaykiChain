// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pricefeedtx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/pricefeeddb.h"
#include "tx.h"
#include "commons/util/util.h"
#include "config/version.h"


string CoinPricePairToString(const PriceCoinPair &coinPricePair) {
    return strprintf("%s:%s", GetPriceBaseSymbol(coinPricePair), GetPriceQuoteSymbol(coinPricePair));
}

////////////////////////////////////////////////////////////////////////////////
// class CPriceFeedTx

bool CPriceFeedTx::CheckTx(CTxExecuteContext &context) {
    IMPLEMENT_DEFINE_CW_STATE;

    if (price_points.size() == 0 || price_points.size() > COIN_PRICE_PAIR_COUNT_MAX)
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, the count=%u of price_points is 0 or larger than %u",
                COIN_PRICE_PAIR_COUNT_MAX), REJECT_INVALID, "price-point-size-error");

    for (const auto &pricePoint : price_points) {
        const uint64_t &price = pricePoint.price;
        if (price == 0)
            return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, invalid price"), REJECT_INVALID, "bad-tx-invalid-price");

        const TokenSymbol &baseSymbol = pricePoint.coin_price_pair.first;
        const TokenSymbol &quoteSymbol = pricePoint.coin_price_pair.second;
        if (baseSymbol == quoteSymbol)
            return state.DoS(100,
                             ERRORMSG("baseSymbol=%s is same to quoteSymbol=%s", baseSymbol, quoteSymbol),
                             REJECT_INVALID, "same-base-quote-symbol");

        if (!cw.assetCache.CheckPriceFeedBaseSymbol(baseSymbol))
            return state.DoS(
                100, ERRORMSG("unsupported baseSymbol=%s for price feed coin pair", baseSymbol),
                REJECT_INVALID, "unsupported-base-symbol");

        if (!cw.assetCache.CheckPriceFeedQuoteSymbol(quoteSymbol))
            return state.DoS(100,
                             ERRORMSG("unsupported quote_symbol=%s of price feed coin pair",
                                      pricePoint.coin_price_pair.second),
                             REJECT_INVALID, "unsupported-quote-symbol");

        if (!cw.priceFeedCache.HasFeedCoinPair(pricePoint.coin_price_pair))
            return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, unsupported price coin pair={%s}",
                            CoinPricePairToString(pricePoint.coin_price_pair)), REJECT_INVALID, "unsupported-price-coin-pair");
    }

    return true;
}

bool CPriceFeedTx::ExecuteTx(CTxExecuteContext &context) {
    CCacheWrapper &cw = *context.pCw; CValidationState &state = *context.pState;

    CRegID sendRegId = txUid.get<CRegID>();
    if (!cw.delegateCache.IsActiveDelegate(sendRegId.ToString())) { // must be a delegate
        return state.DoS(100, ERRORMSG("txUid %s account is not a delegate error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-isn't-delegate");
    }

    uint64_t stakedAmountMin;
    if (!cw.sysParamCache.GetParam(PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN, stakedAmountMin)) {
        return state.DoS(100, ERRORMSG("read PRICE_FEED_BCOIN_STAKE_AMOUNT_MIN error",
                        txUid.ToString()), READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
    }

    CAccountToken accountToken = txAccount.GetToken(SYMB::WICC);
    if (accountToken.staked_amount < stakedAmountMin * COIN) // must stake enough bcoins to be a price feeder
        return state.DoS(100, ERRORMSG("Staked Bcoins insufficient(%llu vs %llu) by txUid %s account error",
                        accountToken.voted_amount, stakedAmountMin, txUid.ToString()),
                        PRICE_FEED_FAIL, "account-staked-boins-insufficient");

    // update the price feed cache accordingly
    if (!cw.ppCache.AddPrice(context.height, txUid.get<CRegID>(), price_points)) {
        return state.DoS(100, ERRORMSG("[%d] txUid %s account duplicated price feed exits", context.height,
                        txUid.ToString()), PRICE_FEED_FAIL, "duplicated-pricefeed");
    }

    return true;
}

string CPriceFeedTx::ToString(CAccountDBCache &accountCache) {
    string str;
    for (auto pp : price_points) {
        str += pp.ToString() + ", ";
    }

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, price_points=%s, valid_height=%d",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees, str, valid_height);
}

Object CPriceFeedTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result = CBaseTx::ToJson(accountCache);

    Array pricePointArray;
    for (const auto &pp : price_points) {
        pricePointArray.push_back(pp.ToJson());
    }
    result.push_back(Pair("price_points", pricePointArray));

    return result;
}
