// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CDP_H
#define ENTITIES_CDP_H

#include "id.h"

#include <string>

/**
 * CDP Cache Item: stake in BaseCoin to get StableCoins
 *
 * Ij =  TNj * (Hj+1 - Hj)/Y * 0.2a/Log10(1+b*TNj)
 *
 * Persisted in LDB as:
 *      cdp{$RegID}{$CTxCord} --> { blockHeight, totalStakedBcoins, totalOwedScoins }
 *
 */
struct CUserCDP {
    CRegID ownerRegId;              // CDP Owner RegId
    uint256 cdpTxId;                // CDP TxID
    int32_t blockHeight;            // persisted: Hj (Hj+1 refer to current height) - last op block height
    uint64_t totalStakedBcoins;     // persisted: total staked bcoins
    uint64_t totalOwedScoins;       // persisted: TNj = last + minted = total minted - total redempted

    mutable double collateralRatioBase;  // ratioBase = totalStakedBcoins / totalOwedScoins, mem-only

    CUserCDP() : blockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    CUserCDP(const CRegID &regId, const uint256 &cdpTxIdIn)
        : ownerRegId(regId), cdpTxId(cdpTxIdIn), blockHeight(0), totalStakedBcoins(0), totalOwedScoins(0) {}

    bool operator<(const CUserCDP &cdp) const {
        if (collateralRatioBase == cdp.collateralRatioBase) {
            if (ownerRegId == cdp.ownerRegId)
                return cdpTxId < cdp.cdpTxId;
            else
                return ownerRegId < cdp.ownerRegId;
        } else {
            return collateralRatioBase < cdp.collateralRatioBase;
        }
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(ownerRegId);
        READWRITE(cdpTxId);
        READWRITE(VARINT(blockHeight));
        READWRITE(VARINT(totalStakedBcoins));
        READWRITE(VARINT(totalOwedScoins));
        if (fRead) {
            collateralRatioBase = double(totalStakedBcoins) / totalOwedScoins;
        }
    )

    string ToString() {
        return strprintf(
            "ownerRegId=%s, cdpTxId=%s, blockHeight=%d, totalStakedBcoins=%d, tatalOwedScoins=%d, "
            "collateralRatioBase=%f",
            ownerRegId.ToString(), cdpTxId.ToString(), blockHeight, totalStakedBcoins, totalOwedScoins,
            collateralRatioBase);
    }

    Object ToJson() {
        Object result;
        result.push_back(Pair("regid",          ownerRegId.ToString()));
        result.push_back(Pair("cdp_id",         cdpTxId.GetHex()));
        result.push_back(Pair("height",         blockHeight));
        result.push_back(Pair("total_bcoin",    totalStakedBcoins));
        result.push_back(Pair("total_scoin",    totalOwedScoins));
        result.push_back(Pair("ratio",          collateralRatioBase));
        return result;
    }

    bool IsEmpty() const {
        return cdpTxId.IsEmpty();
    }

    void SetEmpty() {
        cdpTxId             = uint256();
        blockHeight         = 0;
        totalStakedBcoins   = 0;
        totalOwedScoins     = 0;
    }
};

#endif //ENTITIES_CDP_H