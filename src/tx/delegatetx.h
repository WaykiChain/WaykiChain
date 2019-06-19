// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef DELEGATE_H
#define DELEGATE_H

#include "tx.h"

class CDelegateVoteTx : public CBaseTx {
public:
    vector<CCandidateVote> candidateVotes;  //!< candidate-delegate votes, max size is 22

public:
    CDelegateVoteTx(const CBaseTx *pBaseTx): CBaseTx(DELEGATE_VOTE_TX) {
        assert(DELEGATE_VOTE_TX == pBaseTx->nTxType);
        *this = *(CDelegateVoteTx *)pBaseTx;
    }
    CDelegateVoteTx(
            const vector_unsigned_char &accountIn,
            const vector<CCandidateVote> &candidateVotesIn,
            const uint64_t feesIn,
            const int validHeightIn)
        : CBaseTx(DELEGATE_VOTE_TX, CNullID(), validHeightIn, feesIn) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        candidateVotes = candidateVotesIn;
    }
    CDelegateVoteTx(
            const CUserID &txUidIn,
            const uint64_t feesIn,
            const vector<CCandidateVote> &candidateVotesIn,
            const int validHeightIn)
        : CBaseTx(DELEGATE_VOTE_TX, txUidIn, validHeightIn, feesIn) {

        if (txUidIn.type() == typeid(CRegID))
            assert(!txUidIn.get<CRegID>().IsEmpty());

        candidateVotes = candidateVotesIn;
    }
    CDelegateVoteTx(): CBaseTx(DELEGATE_VOTE_TX) {}
    ~CDelegateVoteTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(candidateVotes);
        READWRITE(VARINT(llFees));
        READWRITE(signature);
    )

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << txUid
                << candidateVotes << VARINT(llFees);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual uint64_t GetFee() const { return llFees; }
    virtual uint64_t GetValue() const { return 0; }
    virtual double GetPriority() const { return llFees / GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDelegateVoteTx>(this); }
    virtual string ToString(CAccountCache &view);
    virtual Object ToJson(const CAccountCache &accountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
    virtual bool UndoExecuteTx(int nHeight, int nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif