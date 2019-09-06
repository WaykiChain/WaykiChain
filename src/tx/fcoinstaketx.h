// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef FCOIN_STAKE_H
#define FCOIN_STAKE_H

#include "tx.h"

class CFcoinStakeTx: public CBaseTx {
private:
    BalanceOpType stakeType;
    uint64_t fcoinsToStake;  // when negative, it means staking revocation

public:
    CFcoinStakeTx()
        : CBaseTx(FCOIN_STAKE_TX), stakeType(BalanceOpType::NULL_OP), fcoinsToStake(0) {}

    CFcoinStakeTx(const CUserID &txUidIn, const int32_t validHeightIn, const TokenSymbol &feeSymbol,
                  const uint64_t feesIn, const BalanceOpType stakeTypeIn, const uint64_t fcoinsToStakeIn)
        : CBaseTx(FCOIN_STAKE_TX, txUidIn, validHeightIn, feeSymbol, feesIn),
          stakeType(stakeTypeIn),
          fcoinsToStake(fcoinsToStakeIn) {}

    ~CFcoinStakeTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE((uint8_t &)stakeType);
        READWRITE(VARINT(fcoinsToStake));

        READWRITE(signature);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol
               << VARINT(llFees) << (uint8_t)stakeType << VARINT(fcoinsToStake);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CFcoinStakeTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

#endif