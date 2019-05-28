// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "blockpricemediantx.h"


bool CBlockPriceMedianTx::CheckTx(CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CBlockPriceMedianTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

bool CBlockPriceMedianTx::UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state) {
    return true;
}

string CBlockPriceMedianTx::ToString(CAccountCache &view) {
    //TODO
    return "";
}

Object CBlockPriceMedianTx::ToJson(const CAccountCache &AccountView) const {
    //TODO
    return Object();
}

bool CBlockPriceMedianTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    //TODO
    return true;
}

inline uint64_t GetMedianPriceByType(const CoinType coinType, const PriceType priceType) {
    // return mapMedianPricePoints[make_tuple<CoinType, PriceType>(coinType, priceType)];

    return 0;
}