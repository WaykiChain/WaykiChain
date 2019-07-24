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
            const UnsignedCharArray &accountIn,
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

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
                << candidateVotes << VARINT(llFees);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual map<CoinType, uint64_t> GetValues() const { return map<CoinType, uint64_t>{{CoinType::WICC, 0}}; }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CDelegateVoteTx>(this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);
};

#endif