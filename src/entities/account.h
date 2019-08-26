// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_ACCOUNT_H
#define ENTITIES_ACCOUNT_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "asset.h"
#include "crypto/hash.h"
#include "entities/receipt.h"
#include "id.h"
#include "vote.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;

class CAccountDBCache;

enum BalanceType : uint8_t {
    NULL_TYPE    = 0,  //!< invalid type
    FREE_VALUE   = 1,
    STAKED_VALUE = 2,
    FROZEN_VALUE = 3
};

enum BalanceOpType : uint8_t {
    NULL_OP  = 0,  //!< invalid op
    ADD_FREE = 1,  //!< external send coins to this account
    SUB_FREE = 2,  //!< send coins to external account
    STAKE    = 3,  //!< free   -> staked
    UNSTAKE  = 4,  //!< staked -> free
    FREEZE   = 5,  //!< free   -> frozen
    UNFREEZE = 6   //!< frozen -> free
};

struct BalanceOpTypeHash {
    size_t operator()(const BalanceOpType& type) const noexcept { return std::hash<uint8_t>{}(type); }
};

static const unordered_map<BalanceOpType, string, BalanceOpTypeHash> kBalanceOpTypeTable = {
    { NULL_OP,  "NULL_OP"   },
    { ADD_FREE, "ADD_FREE"  },
    { SUB_FREE, "SUB_FREE"  },
    { STAKE,    "STAKE"     },
    { UNSTAKE,  "UNSTAKE"   },
    { FREEZE,   "FREEZE"    },
    { UNFREEZE, "UNFREEZE"  }
};

inline string GetBalanceOpTypeName(const BalanceOpType opType) {
    return kBalanceOpTypeTable.at(opType);
}

class CAccountToken {
public:
    uint64_t free_amount;
    uint64_t frozen_amount;  // held within open DEX orders
    uint64_t staked_amount;  // for staking purposes

public:
    CAccountToken() : free_amount(0), frozen_amount(0), staked_amount(0) { }

    CAccountToken(uint64_t& freeAmount, uint64_t& frozenAmount, uint64_t& stakedAmount)
        : free_amount(freeAmount), frozen_amount(frozenAmount), staked_amount(stakedAmount) {}

    CAccountToken& operator=(const CAccountToken& other) {
        if (this == &other)
            return *this;

        this->free_amount   = other.free_amount;
        this->frozen_amount = other.frozen_amount;
        this->staked_amount = other.staked_amount;

        return *this;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(free_amount));
        READWRITE(VARINT(frozen_amount));
        READWRITE(VARINT(staked_amount));
    )
};

typedef map<TokenSymbol, CAccountToken> AccountTokenMap;


/**
 * Common or Contract Account
 */
class CAccount {
public:
    CKeyID  keyid;                  //!< unique: keyId of the account (interchangeable to address) - 20 bytes
    CRegID  regid;                  //!< unique: regId - derived from 1st TxCord - 6 bytes
    CNickID nickid;                 //!< unique: Nickname ID of the account (sting maxlen=32) - 8 bytes

    CPubKey owner_pubkey;           //!< account public key
    CPubKey miner_pubkey;           //!< miner saving account public key

    AccountTokenMap tokens;         //!< In total, 3 types of coins/tokens:
                                    //!<    1) system-issued coins: WICC, WGRT
                                    //!<    2) miner-issued stablecoins WUSD|WCNY|...
                                    //!<    3) user-issued tokens (WRC20 compilant)

    uint64_t received_votes;        //!< votes received
    uint64_t last_vote_height;      //!< account's last vote block height used for computing interest

    mutable uint256 sigHash;        //!< in-memory only

public:
    CAccount() : CAccount(CKeyID(), CNickID(), CPubKey()) {}
    CAccount(const CAccount& other) { *this = other; }
    CAccount& operator=(const CAccount& other) {
        if (this == &other)
            return *this;

        this->keyid            = other.keyid;
        this->regid            = other.regid;
        this->nickid           = other.nickid;
        this->owner_pubkey     = other.owner_pubkey;
        this->miner_pubkey     = other.miner_pubkey;
        this->tokens           = other.tokens;
        this->received_votes   = other.received_votes;
        this->last_vote_height = other.last_vote_height;

        return *this;
    }
    CAccount(const CKeyID& keyIdIn): keyid(keyIdIn), regid(), nickid(), received_votes(0), last_vote_height(0) {}
    CAccount(const CKeyID& keyidIn, const CNickID& nickidIn, const CPubKey& ownerPubkeyIn)
        : keyid(keyidIn), nickid(nickidIn), owner_pubkey(ownerPubkeyIn), received_votes(0), last_vote_height(0) {
        miner_pubkey = CPubKey();
        tokens.clear();
        regid.Clear();
    }

    std::shared_ptr<CAccount> GetNewInstance() const { return std::make_shared<CAccount>(*this); }

public:

    IMPLEMENT_SERIALIZE(
        READWRITE(keyid);
        READWRITE(regid);
        READWRITE(nickid);
        READWRITE(owner_pubkey);
        READWRITE(miner_pubkey);
        READWRITE(tokens);
        READWRITE(VARINT(received_votes));
        READWRITE(VARINT(last_vote_height));
    )

    CAccountToken GetToken(const TokenSymbol &tokenSymbol) const;
    bool SetToken(const TokenSymbol &tokenSymbol, const CAccountToken &accountToken);

    bool GetBalance(const TokenSymbol &tokenSymbol, const BalanceType balanceType, uint64_t &value);
    bool OperateBalance(const TokenSymbol &tokenSymbol, const BalanceOpType opType, const uint64_t &value);

    bool StakeVoteBcoins(VoteType type, const uint64_t votes);
    bool ProcessDelegateVotes(const vector<CCandidateVote>& candidateVotesIn,
                              vector<CCandidateReceivedVote>& candidateVotesInOut, const uint32_t currHeight,
                              const CAccountDBCache &accountCache, vector<CReceipt> &receipts);

    uint64_t GetVotedBcoins(const vector<CCandidateReceivedVote>& candidateVotes, const uint64_t currHeight);

    uint64_t ComputeVoteStakingInterest(const vector<CCandidateReceivedVote>& candidateVotes, const uint32_t currHeight,
                                        const FeatureForkVersionEnum& featureForkVersion);
    uint64_t ComputeBlockInflateInterest(const uint32_t currHeight) const;

    bool HaveOwnerPubKey() const { return owner_pubkey.IsFullyValid(); }
    bool IsRegistered() const { return owner_pubkey.IsValid(); }

    bool IsEmptyValue() const { return (tokens.size() == 0); }
    bool IsEmpty() const { return keyid.IsEmpty(); }
    void SetEmpty() { keyid.SetEmpty(); }  // TODO: need set other fields to empty()??
    string ToString() const;
    Object ToJsonObj() const;

    void SetRegId(CRegID & regIdIn) { regid = regIdIn; }

    bool IsMyUid(const CUserID &uid);

private:
    bool IsBcoinWithinRange(uint64_t nAddMoney);
    bool IsFcoinWithinRange(uint64_t nAddMoney);
};

enum AccountType {
    REGID      = 0x01,  //!< Registration account id
    BASE58ADDR = 0x02,  //!< Public key
};

/**
 * account operate produced in contract
 * TODO: rename CVmOperate
 */
class CVmOperate{
public:
    AccountType accountType;   //!< regid or base58addr
    uint8_t accountId[34];      //!< accountId: address
    BalanceOpType opType;       //!< OperType
    uint32_t timeoutHeight;     //!< the transacion timeout height
    uint8_t money[8];           //!< The transfer amount

    IMPLEMENT_SERIALIZE(
        READWRITE((uint8_t&)accountType);
        for (int8_t i = 0; i < 34; ++i)
            READWRITE(accountId[i]);
        READWRITE((uint8_t&)opType);
        READWRITE(timeoutHeight);
        for (int8_t i = 0; i < 8; ++i)
            READWRITE(money[i]);
    )

    CVmOperate() {
        accountType = AccountType::REGID;
        memset(accountId, 0, 34);
        opType        = BalanceOpType::NULL_OP;
        timeoutHeight = 0;
        memset(money, 0, 8);
    }

    Object ToJson();
};


#endif //ENTITIES_ACCOUNT_H