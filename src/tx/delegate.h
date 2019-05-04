// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef DELEGATE_H
#define DELEGATE_H

#include "tx.h"

class CDelegateVoteTx : public CBaseTx {
public:
    vector<COperVoteFund> operVoteFunds;  //!< oper delegate votes, max length is Delegates number

public:
    CDelegateVoteTx(const CBaseTx *pBaseTx): CBaseTx(DELEGATE_VOTE_TX) {
        assert(DELEGATE_VOTE_TX == pBaseTx->nTxType);
        *this = *(CDelegateVoteTx *)pBaseTx;
    }
    CDelegateVoteTx(const vector_unsigned_char &accountIn, const vector<COperVoteFund> &operVoteFundsIn,
                const uint64_t feeIn, const int validHeightIn)
        : CBaseTx(DELEGATE_VOTE_TX, validHeightIn, feeIn) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateVoteTx(const CUserID &userIdIn, const uint64_t feeIn,
                const vector<COperVoteFund> &operVoteFundsIn, const int validHeightIn)
        : CBaseTx(DELEGATE_VOTE_TX, validHeightIn, feeIn) {
        if (userIdIn.type() == typeid(CRegID)) assert(!userIdIn.get<CRegID>().IsEmpty());

        txUid        = userIdIn;
        operVoteFunds = operVoteFundsIn;
    }
    CDelegateVoteTx(): CBaseTx(DELEGATE_VOTE_TX) {}
    ~CDelegateVoteTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(operVoteFunds);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
    )

    uint256 SignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid << operVoteFunds
               << VARINT(llFees);
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    uint256 GetHash() const { return SignatureHash(); }
    uint64_t GetFee() const { return llFees; }
    uint64_t GetValue() const { return 0; }
    double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDelegateVoteTx>(this); }
    string ToString(CAccountViewCache &view) const;
    Object ToJson(const CAccountViewCache &accountView) const;
    bool GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
    bool ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
                   int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB);
    bool UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                       CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                       CScriptDBViewCache &scriptDB);
    bool CheckTx(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB);
};

#endif