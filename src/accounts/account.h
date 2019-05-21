// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "chainparams.h"
#include "crypto/hash.h"
#include "id.h"
#include "vote.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;

class CAccountLog;

class CAccount
{
public:
    enum BalanceOpType: unsigned char {
        ADD_VALUE   = 1,
        MINUS_VALUE = 2,
        NULL_OP,        //!< invalid OP type
    };

public:
    CKeyID keyID;                   //!< keyID of the account (interchangeable to address)
    CRegID regID;                   //!< regID of the account
    CNickID nickID;                 //!< Nickname ID of the account (maxlen=32)
    CPubKey pubKey;                 //!< account public key
    CPubKey minerPubKey;            //!< miner saving account public key

    uint64_t bcoins;                //!< BaseCoin balance
    uint64_t scoins;                //!< StableCoin balance
    uint64_t fcoins;                //!< FundCoin balance

    uint64_t stakedFcoins;          //!< Staked FundCoins for pricefeed right
    uint64_t receivedVotes;         //!< received votes (1:1 to bcoins)

    vector<CCandidateVote> voteFunds;    //!< account delegates votes sorted by vote amount

    bool hasOpenCdp;                //!< When true, its CDP exists in a map {cdp-$regid -> $cdp}
    uint256 sigHash;                //!< in-memory only

public:
    /**
     * @brief operate account balance
     * @param coinType: balance coin Type
     * @param opType: balance operate type
     * @param values: balance value to add or minus
     * @return returns true if successful, otherwise false
     */
    bool OperateBalance(CoinType coinType, BalanceOpType opType, const uint64_t& value);
    bool UndoOperateAccount(const CAccountLog& accountLog);
    bool ProcessDelegateVote(vector<CCandidateVote>& candidateVotes, const uint64_t nCurHeight);
    bool OperateVote(VoteType type, const uint64_t& values);

public:
    CAccount(CKeyID& keyId, CNickID& nickId, CPubKey& pubKey)
        : keyID(keyId),
          nickID(nickId),
          pubKey(pubKey),
          bcoins(0),
          scoins(0),
          fcoins(0),
          stakedFcoins(0),
          receivedVotes(0),
          hasOpenCdp(false) {
        minerPubKey = CPubKey();
        voteFunds.clear();
        regID.Clean();
    }

    CAccount()
        : keyID(uint160()),
          bcoins(0),
          scoins(0),
          fcoins(0),
          stakedFcoins(0),
          receivedVotes(0),
          hasOpenCdp(false) {
        pubKey      = CPubKey();
        minerPubKey = CPubKey();
        voteFunds.clear();
        regID.Clean();
    }

    CAccount(const CAccount& other) {
        this->keyID          = other.keyID;
        this->regID          = other.regID;
        this->nickID         = other.nickID;
        this->pubKey         = other.pubKey;
        this->minerPubKey    = other.minerPubKey;
        this->bcoins         = other.bcoins;
        this->scoins         = other.scoins;
        this->fcoins         = other.fcoins;
        this->stakedFcoins   = other.stakedFcoins;
        this->receivedVotes  = other.receivedVotes;
        this->voteFunds      = other.voteFunds;
        this->hasOpenCdp     = other.hasOpenCdp;
    }

    CAccount& operator=(const CAccount& other) {
        if (this == &other) return *this;

        this->keyID          = other.keyID;
        this->regID          = other.regID;
        this->nickID         = other.nickID;
        this->pubKey         = other.pubKey;
        this->minerPubKey    = other.minerPubKey;
        this->bcoins         = other.bcoins;
        this->scoins         = other.scoins;
        this->fcoins         = other.fcoins;
        this->receivedVotes  = other.receivedVotes;
        this->voteFunds      = other.voteFunds;
        this->hasOpenCdp     = other.hasOpenCdp;

        return *this;
    }

    std::shared_ptr<CAccount> GetNewInstance() const {
        return std::make_shared<CAccount>(*this);
    }

    bool IsRegistered() const {
        return (pubKey.IsFullyValid() && pubKey.GetKeyId() == keyID);
    }

    bool RegIDIsMature() const;

    bool SetRegId(const CRegID& regID) {
        this->regID = regID;
        return true;
    };

    bool GetRegId(CRegID& regID) const {
        regID = this->regID;
        return !regID.IsEmpty();
    };

    uint64_t GetFreeBCoins() const { return bcoins; }
    uint64_t GetFreeScoins() const { return scoins; }
    uint64_t GetFreeFcoins() const { return fcoins; }
    uint64_t GetReceiveVotes() const { return receivedVotes; }

    uint64_t GetTotalBcoins();
    uint64_t GetVotedBCoins();

    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;
    bool IsEmptyValue() const { return !(bcoins > 0); }

    uint256 GetHash(bool recalculate = false)
    {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << keyID << regID << nickID << pubKey << minerPubKey
                << VARINT(bcoins) << VARINT(scoins) << VARINT(fcoins)
                << VARINT(receivedVotes)
                << voteFunds << hasOpenCdp;

            uint256* hash = const_cast<uint256*>(&sigHash);
            *hash = ss.GetHash();
        }

        return sigHash;
    }

    bool UpDateAccountPos(int nCurHeight);

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(regID);
        READWRITE(nickID);
        READWRITE(pubKey);
        READWRITE(minerPubKey);
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(scoins));
        READWRITE(VARINT(fcoins));
        READWRITE(VARINT(receivedVotes));
        READWRITE(voteFunds);
        READWRITE(hasOpenCdp);)

private:
    bool IsMoneyOverflow(uint64_t nAddMoney);
};

class CAccountLog
{
public:
    CKeyID keyID;        //!< keyID of the account (interchangeable to address)
    CRegID regID;        //!< regID of the account
    CNickID nickID;      //!< Nickname ID of the account (maxlen=32)
    CPubKey pubKey;      //!< account public key
    CPubKey minerPubKey; //!< miner saving account public key
    bool hasOpenCdp;

    uint64_t bcoins;             //!< baseCoin balance
    uint64_t scoins;             //!< stableCoin balance
    uint64_t fcoins;             //!< fundCoin balance
    vector<CCandidateVote> voteFunds; //!< casted delegate votes
    uint64_t receivedVotes;      //!< votes received

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(regID);
        READWRITE(nickID);
        READWRITE(pubKey);
        READWRITE(minerPubKey);
        READWRITE(hasOpenCdp);
        READWRITE(VARINT(bcoins));
        READWRITE(voteFunds);
        READWRITE(VARINT(receivedVotes));)

public:
    CAccountLog(const CAccount& acct) {
        keyID          = acct.keyID;
        regID          = acct.regID;
        nickID         = acct.nickID;
        pubKey         = acct.pubKey;
        minerPubKey    = acct.minerPubKey;
        bcoins         = acct.bcoins;
        hasOpenCdp     = acct.hasOpenCdp;
        voteFunds      = acct.voteFunds;
        receivedVotes  = acct.receivedVotes;
    }

    CAccountLog(CKeyID& keyId) {
        keyID = keyId;
        regID.Clean();
        nickID.Clean();
        bcoins = 0;
        hasOpenCdp = false;
        voteFunds.clear();
        receivedVotes = 0;
    }

    CAccountLog() {
        keyID = uint160();
        regID.Clean();
        nickID.Clean();
        bcoins = 0;
        hasOpenCdp = false;
        voteFunds.clear();
        receivedVotes = 0;
    }

    void SetValue(const CAccount& acct) {
        keyID          = acct.keyID;
        regID          = acct.regID;
        nickID         = acct.nickID;
        pubKey         = acct.pubKey;
        minerPubKey    = acct.minerPubKey;
        bcoins         = acct.bcoins;
        scoins         = acct.scoins;
        fcoins         = acct.fcoins;
        receivedVotes  = acct.receivedVotes;
        voteFunds      = acct.voteFunds;
        hasOpenCdp     = acct.hasOpenCdp;
    }

    string ToString() const;
};

#endif