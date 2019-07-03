// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockpricemediantx.h"
#include "main.h"


bool CBlockPriceMedianTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    IMPLEMENT_CHECK_TX_REGID(txUid.type());
    return true;
}

bool CBlockPriceMedianTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    // Nothing to do here.
    return true;
}

bool CBlockPriceMedianTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    // Nothing to do here.
    return true;
}

string CBlockPriceMedianTx::ToString(CAccountDBCache &accountCache) {
    string pricePoints;
    for (auto it = mapMedianPricePoints.begin(); it != mapMedianPricePoints.end(); ++it) {
        pricePoints += strprintf("{coin_type:%u, price_type:%u, price:%lld}",
                        it->first.coinType, it->first.priceType, it->second);
    };

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, nValidHeight=%d, txUid=%s, llFees=%ld,"
        "median_price_points=%s\n",
        GetTxType(nTxType), GetHash().GetHex(), nVersion, nValidHeight, txUid.ToString(), llFees,
        pricePoints);

    return str;
}

Object CBlockPriceMedianTx::ToJson(const CAccountDBCache &accountCache) const {
    Object result;

    CKeyID keyId;
    accountCache.GetKeyId(txUid, keyId);

    Array pricePointArray;
    for (auto it = mapMedianPricePoints.begin(); it != mapMedianPricePoints.end(); ++it) {
        Object subItem;
        subItem.push_back(Pair("coin_type",     it->first.coinType));
        subItem.push_back(Pair("price_type",    it->first.priceType));
        subItem.push_back(Pair("price",         it->second));
        pricePointArray.push_back(subItem);
    };

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("tx_type",        GetTxType(nTxType)));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("tx_uid",         txUid.ToString()));
    result.push_back(Pair("addr",           keyId.ToAddress()));
    result.push_back(Pair("valid_height",   nValidHeight));
    result.push_back(Pair("fees",           llFees));

    result.push_back(Pair("median_price_points",   pricePointArray));

    return result;
}

bool CBlockPriceMedianTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

uint64_t CBlockPriceMedianTx::GetMedianPriceByType(const CoinType coinType, const PriceType priceType) {
    return mapMedianPricePoints.count(CCoinPriceType(coinType, priceType))
               ? mapMedianPricePoints[CCoinPriceType(coinType, priceType)]
               : 0;
}
