// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef TX_BLOCK_REWARD_H
#define TX_BLOCK_REWARD_H

#include "tx.h"

class CBlockRewardTx : public CBaseTx {
public:
    uint64_t reward_fees;

public:
    CBlockRewardTx(): CBaseTx(BLOCK_REWARD_TX), reward_fees(0) {}

    CBlockRewardTx(const UnsignedCharArray &accountIn, const uint64_t rewardFeesIn, const int32_t nValidHeightIn):
        CBaseTx(BLOCK_REWARD_TX) {
        if (accountIn.size() > 6) {
            txUid = CPubKey(accountIn);
        } else {
            txUid = CRegID(accountIn);
        }
        reward_fees  = rewardFeesIn;
        valid_height = nValidHeightIn;
    }
    ~CBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(txUid);

        // Do NOT change the order.
        READWRITE(VARINT(reward_fees));
        READWRITE(VARINT(valid_height));)

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << txUid << VARINT(reward_fees) << VARINT(valid_height);
    }

    std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CBlockRewardTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(CCacheWrapper &cw) const;

    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) { return true; }

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

class CUCoinBlockRewardTx : public CBaseTx {
public:
    map<TokenSymbol, uint64_t> reward_fees;
    uint64_t inflated_bcoins;  // inflated bcoin amount computed against received votes.

public:
    CUCoinBlockRewardTx(): CBaseTx(UCOIN_BLOCK_REWARD_TX), inflated_bcoins(0) {}

    CUCoinBlockRewardTx(const CUserID &txUidIn, const map<TokenSymbol, uint64_t> rewardValuesIn,
                            const int32_t validHeightIn)
        : CBaseTx(UCOIN_BLOCK_REWARD_TX) {
        txUid = txUidIn;

        for (const auto &item : rewardValuesIn) {
            reward_fees.emplace(item.first, item.second);
        }

        valid_height = validHeightIn;
    }
    ~CUCoinBlockRewardTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);

        READWRITE(reward_fees);
        READWRITE(VARINT(inflated_bcoins));
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid
                   << reward_fees << VARINT(inflated_bcoins);
    }

    uint64_t GetInflatedBcoins() const { return inflated_bcoins; }
    std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CUCoinBlockRewardTx>(*this); }

    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(CCacheWrapper &cw) const;

    bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) { return true; }

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);
};

#endif // TX_BLOCK_REWARD_H
