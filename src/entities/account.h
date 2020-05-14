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
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"

using namespace json_spirit;

class CAccountDBCache;


// perms for an account
enum AccountPermType : uint64_t {
    NULL_ACCOUNT_PERM   = 0,         // no perm at all w/ the account, even for smart contract account
    PERM_SEND_COIN      = (1 << 0 ), //recv_coin is always allowed
    PERM_STAKE_COIN     = (1 << 1 ), //when input is negative, it means unstake
    PERM_SEND_VOTE      = (1 << 2 ), //send or revoke votes to others
    PERM_SEND_UTXO      = (1 << 3 ), //recv utox is always allowed
    PERM_DEPLOY_SC      = (1 << 4 ), //Deploy smart contract
    PERM_UPGRADE_SC     = (1 << 5 ), //Upgrade smart contract
    PERM_INVOKE_SC      = (1 << 6 ), //Invoke smart contract
    PERM_PROPOSE        = (1 << 7 ), //DeGov propose
    PERM_MINE_BLOCK     = (1 << 8 ), //elected BP can mine blocks, mostly used to disable the perm when set zero
    PERM_FEED_PRICE     = (1 << 9 ), //Feed price, mostly used to disable the perm when set zero
    PERM_DEX            = (1 << 10 ), //freeze | unfreeze
    PERM_CDP            = (1 << 11), //pledge | unpledge
};

static const unordered_map<uint64_t, string> kAccountPermTitleMap = {
    { PERM_SEND_COIN,    "PERM_SEND_COIN"   },
    { PERM_STAKE_COIN,   "PERM_STAKE_COIN"  },
    { PERM_SEND_VOTE,    "PERM_SEND_VOTE"   },
    { PERM_SEND_UTXO,    "PERM_SEND_UTXO"   },
    { PERM_DEPLOY_SC,    "PERM_DEPLOY_SC"   },
    { PERM_UPGRADE_SC,   "PERM_UPGRADE_SC"  },
    { PERM_INVOKE_SC,    "PERM_INVOKE_SC"   },
    { PERM_PROPOSE,      "PERM_PROPOSE"     },
    { PERM_MINE_BLOCK,   "PERM_MINE_BLOCK"  },
    { PERM_FEED_PRICE,   "PERM_FEED_PRICE"  },
    { PERM_DEX,          "PERM_DEX"         },
    { PERM_CDP,          "PERM_CDP"         },
};

const uint64_t kAccountAllPerms = uint64_t(-1); // == 0xFFFFFFFFFFFFFFFF enable all perms

enum BalanceType : uint8_t {
    NULL_TYPE    = 0,   //!< invalid type
    FREE_VALUE   = 1,   //!< free & available
    STAKED_VALUE = 2,   //!< staked for pricefeed qualification etc
    FROZEN_VALUE = 3,   //!< frozen in DEX
    VOTED_VALUE  = 4,   //!< voted for other account(s)
    PLEDGED_VALUE= 5    //!< collateralized in CDP
};


class CAccountToken {
public:
    uint64_t free_amount;
    uint64_t frozen_amount;     // for coins held in DEX buy/sell orders
    uint64_t staked_amount;     // for staking
    uint64_t voted_amount;      // for voting
    uint64_t pledged_amount;    // for CDP collateral amount

public:
    CAccountToken() : free_amount(0), frozen_amount(0), staked_amount(0), voted_amount(0), pledged_amount(0) {}

    CAccountToken(uint64_t& freeAmount, uint64_t& frozenAmount, uint64_t& stakedAmount,
                uint64_t& votedAmount, uint64_t& pledgedAmount)
        : free_amount(freeAmount), frozen_amount(frozenAmount), staked_amount(stakedAmount),
          voted_amount(votedAmount), pledged_amount(pledgedAmount) {}

    CAccountToken& operator=(const CAccountToken& other) {
        if (this == &other)
            return *this;

        this->free_amount   = other.free_amount;
        this->frozen_amount = other.frozen_amount;
        this->staked_amount = other.staked_amount;
        this->voted_amount  = other.voted_amount;
        this->pledged_amount= other.pledged_amount;

        return *this;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(free_amount));
        READWRITE(VARINT(frozen_amount));
        READWRITE(VARINT(staked_amount));
        READWRITE(VARINT(voted_amount));
        READWRITE(VARINT(pledged_amount));
    )
};

typedef map<TokenSymbol, CAccountToken> AccountTokenMap;


/**
 * Common or Contract Account
 */
struct CAccount {
    CKeyID  keyid;                  //!< unique: keyId of the account (interchangeable to address) - 20 bytes
    CRegID  regid;                  //!< unique: regId - derived from 1st TxCord - 6 bytes

    CPubKey owner_pubkey;           //!< account public key
    CPubKey miner_pubkey;           //!< miner saving account public key

    AccountTokenMap tokens;         //!< In total, 3 types of coins/tokens:
                                    //!<    1) system-issued coins: WICC, WGRT
                                    //!<    2) miner-issued stablecoins WUSD|WCNY|...
                                    //!<    3) user-issued tokens (WRC20 compliant)

    uint64_t received_votes;        //!< votes received
    uint64_t last_vote_height;      //!< account's last vote block height used for computing interest
    uint64_t last_vote_epoch;       //!< account's last vote epoch used for computing interest

    uint64_t perms_sum
                = kAccountAllPerms; //! a sum of granted perms to the account

    mutable uint256 sigHash;        //!< in-memory only

    CAccount() : CAccount(CKeyID(), CPubKey()) {}
    CAccount(const CAccount& other) { *this = other; }
    CAccount& operator=(const CAccount& other) {
        if (this == &other)
            return *this;

        this->keyid             = other.keyid;
        this->regid             = other.regid;
        this->owner_pubkey      = other.owner_pubkey;
        this->miner_pubkey      = other.miner_pubkey;
        this->tokens            = other.tokens;
        this->received_votes    = other.received_votes;
        this->last_vote_height  = other.last_vote_height;
        this->last_vote_epoch   = other.last_vote_epoch;
        this->perms_sum         = other.perms_sum;

        return *this;
    }
    CAccount(const CKeyID& keyIdIn): keyid(keyIdIn), regid(), received_votes(0), last_vote_height(0), last_vote_epoch(0) {}
    CAccount(const CKeyID& keyidIn, const CPubKey& ownerPubkeyIn)
        : keyid(keyidIn), owner_pubkey(ownerPubkeyIn), received_votes(0), last_vote_height(0), last_vote_epoch(0) {
        miner_pubkey = CPubKey();
        tokens.clear();
        regid.Clear();
    }

    std::shared_ptr<CAccount> GetNewInstance() const { return std::make_shared<CAccount>(*this); }

    bool CheckPerms(const uint64_t permsSum) {
        if (permsSum > perms_sum)
            return false;

        return (permsSum == (perms_sum & permsSum));
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(keyid);
        READWRITE(regid);
        READWRITE(owner_pubkey);
        READWRITE(miner_pubkey);
        READWRITE(tokens);
        READWRITE(VARINT(received_votes));
        READWRITE(VARINT(last_vote_height));
        READWRITE(VARINT(last_vote_epoch));
        READWRITE(VARINT(perms_sum));
    )

    CAccountToken GetToken(const TokenSymbol &tokenSymbol) const;
    bool SetToken(const TokenSymbol &tokenSymbol, const CAccountToken &accountToken);

    bool GetBalance(const TokenSymbol &tokenSymbol, const BalanceType balanceType, uint64_t &value);
    bool OperateBalance(const TokenSymbol &tokenSymbol, const BalanceOpType opType, const uint64_t &value,
                        ReceiptType receiptType, ReceiptList &receipts, CAccount *pOtherAccount = nullptr);

    bool StakeVoteBcoins(VoteType type, const uint64_t votes);
    bool ProcessCandidateVotes(const vector<CCandidateVote>& candidateVotesIn,
                               vector<CCandidateReceivedVote>& candidateVotesInOut, const uint32_t currHeight,
                               const uint32_t currBlockTime, const CAccountDBCache& accountCache,
                               vector<CReceipt>& receipts);

    uint64_t ComputeVoteBcoinInterest(const uint64_t lastVotedBcoins, const uint32_t currHeight);
    uint64_t ComputeVoteFcoinInterest(const uint64_t lastVotedBcoins, const uint32_t currBlockTime);
    uint64_t ComputeBlockInflateInterest(const uint32_t currHeight, const VoteDelegate &curDelegate, const uint32_t totalDelegateNum) const;

    bool HasOwnerPubKey() const { return owner_pubkey.IsFullyValid(); }
    bool IsRegistered() const { return !regid.IsEmpty(); }

    bool IsEmptyValue() const { return (tokens.size() == 0); }
    bool IsEmpty() const { return keyid.IsEmpty(); }
    void SetEmpty() { keyid.SetEmpty(); }  // TODO: need set other fields to empty()??
    string ToString() const;
    Object ToJsonObj() const;

    void SetRegId(CRegID & regIdIn) { regid = regIdIn; }

    bool IsSelfUid(const CUserID &uid);

    bool IsBcoinWithinRange(uint64_t nAddMoney);
    bool IsFcoinWithinRange(uint64_t nAddMoney);
    bool CheckBalance(const TokenSymbol& symbol,  const BalanceType& balanceType, const uint64_t& value);
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