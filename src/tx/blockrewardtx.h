// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_BLOCK_REWARD_H
#define TX_BLOCK_REWARD_H

#include "tx.h"

class CBlockRewardTx : public CBaseTx {
public:
    uint64_t reward;

public:
    CBlockRewardTx(): CBaseTx(BLOCK_REWARD_TX), reward(0) {}

    CBlockRewardTx(const UnsignedCharArray &accountIn, const uint64_t rewardIn, const int32_t nValidHeightIn):
        CBaseTx(BLOCK_REWARD_TX) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        reward  = rewardIn;
        valid_height = nValidHeightIn;
    }
    ~CBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(txUid);

        // Do NOT change the order.
        READWRITE(VARINT(reward));
        READWRITE(VARINT(valid_height));)

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << txUid << VARINT(reward) << VARINT(valid_height);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CBlockRewardTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) { return true; }

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

class CUCoinBlockRewardTx : public CBaseTx {
public:
    map<TokenSymbol, uint64_t> rewards;
    uint64_t profits;  // Profits as delegate according to received votes.

public:
    CUCoinBlockRewardTx(): CBaseTx(UCOIN_BLOCK_REWARD_TX), profits(0) {}

    CUCoinBlockRewardTx(const CUserID &txUidIn, const map<TokenSymbol, uint64_t> rewardValuesIn,
                            const int32_t validHeightIn)
        : CBaseTx(UCOIN_BLOCK_REWARD_TX) {
        txUid = txUidIn;

        for (const auto &item : rewardValuesIn) {
            rewards.emplace(item.first, item.second);
        }

        valid_height = validHeightIn;
    }
    ~CUCoinBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(rewards);
        READWRITE(VARINT(profits));
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << rewards << VARINT(profits);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    uint64_t GetProfits() const { return profits; }
    std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUCoinBlockRewardTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) { return true; }

    virtual bool CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state);
};

#endif // TX_BLOCK_REWARD_H
