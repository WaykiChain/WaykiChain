// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FCOIN_STAKE_H
#define FCOIN_STAKE_H

#include "tx.h"

#include "entities/stake.h"

class CFcoinStakeTx: public CBaseTx {
private:
    StakeType stakeType;
    uint64_t fcoinsToStake;  // when negative, it means staking revocation

public:
    CFcoinStakeTx(): CBaseTx(FCOIN_STAKE_TX), stakeType(StakeType::NULL_STAKE), fcoinsToStake(0) {}

    CFcoinStakeTx(const CBaseTx *pBaseTx): CBaseTx(FCOIN_STAKE_TX) {
        assert(FCOIN_STAKE_TX == pBaseTx->nTxType);
        *this = *(CFcoinStakeTx *) pBaseTx;
    }

    CFcoinStakeTx(const CUserID &txUidIn, int32_t validHeightIn, uint64_t feesIn, StakeType stakeTypeIn,
                  uint64_t fcoinsToStakeIn)
        : CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feesIn),
          stakeType(stakeTypeIn),
          fcoinsToStake(fcoinsToStakeIn) {}

    ~CFcoinStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(VARINT(llFees));
        READWRITE((uint8_t &)stakeType);
        READWRITE(VARINT(fcoinsToStake));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid
               << VARINT(llFees) << (uint8_t)stakeType << VARINT(fcoinsToStake);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual map<CoinType, uint64_t> GetValues() const {
        return map<CoinType, uint64_t>{{CoinType::WGRT, fcoinsToStake}};
    }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CFcoinStakeTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    bool CheckTx(int32_t nHeight, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
    bool UndoExecuteTx(int32_t nHeight, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif