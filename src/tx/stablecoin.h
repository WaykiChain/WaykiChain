// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef STABLE_COIN_H
#define STABLE_COIN_H

#include "tx/tx.h"

class CCdpOpenTx : public CBaseTx {
public:
    mutable CUserID userId;       // CDP owner's regid or pubkey
    uint64_t collateralAmount;    // CDP collateral amount
    vector_unsigned_char signature;

public:
    CCdpOpenTx(): CBaseTx(CDP_OPEN_TX) {}

    CCdpOpenTx(const CBaseTx *pBaseTx): CBaseTx(CDP_OPEN_TX) {
        assert(CDP_OPEN_TX == pBaseTx->nTxType);
        *this = *(CCdpOpenTx *) pBaseTx;
    }

    CCdpOpenTx(const CUserID &userIdIn, int validHeightIn, uint64_t feeIn, uint64_t collateralAmountIn):
        CBaseTx(CDP_OPEN_TX, validHeightIn, feeIn) {
        userId = userIdIn;
        collateralAmount = collateralAmountIn;
    }

    ~CCdpOpenTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(userId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(collateralAmount));
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << userId
               << VARINT(llFees) << VARINT(collateralAmount);

            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    uint64_t GetValue() const { return 0; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CCdpOpenTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &AccountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view,
                          CScriptDBViewCache &scriptDB);
};

#endif