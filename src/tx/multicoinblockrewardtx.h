// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_MULTI_COIN_BLOCK_REWARD_H
#define TX_MULTI_COIN_BLOCK_REWARD_H

#include "tx.h"

#include "entities/account.h"

#include <map>

using namespace std;

class CMultiCoinBlockRewardTx : public CBaseTx {
public:
    map<uint8_t /* CoinType */, uint64_t /* reward value */> rewardValues;
    uint64_t profits;  // Profits as delegate according to received votes.

public:
    CMultiCoinBlockRewardTx(): CBaseTx(UCOIN_BLOCK_REWARD_TX), profits(0) {}
    CMultiCoinBlockRewardTx(const CBaseTx *pBaseTx) : CBaseTx(UCOIN_BLOCK_REWARD_TX), profits(0) {
        assert(UCOIN_BLOCK_REWARD_TX == pBaseTx->nTxType);
        *this = *(CMultiCoinBlockRewardTx *)pBaseTx;
    }
    CMultiCoinBlockRewardTx(const CUserID &txUidIn, const map<CoinType, uint64_t> rewardValuesIn,
                            const int32_t validHeightIn)
        : CBaseTx(UCOIN_BLOCK_REWARD_TX) {
        txUid = txUidIn;

        for (const auto &item : rewardValuesIn) {
            rewardValues.emplace(uint8_t(item.first), item.second);
        }

        nValidHeight = validHeightIn;
    }
    ~CMultiCoinBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(rewardValues);
        READWRITE(VARINT(profits));
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << rewardValues << VARINT(profits);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    map<CoinType, uint64_t> GetValues() const;
    uint64_t GetProfits() const { return profits; }
    std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CMultiCoinBlockRewardTx>(this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t nIndex, CCacheWrapper &cw, CValidationState &state);
};

#endif // TX_MULTI_COIN_BLOCK_REWARD_H
