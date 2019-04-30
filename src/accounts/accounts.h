// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <boost/variant.hpp>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "chainparams.h"
#include "crypto/hash.h"
#include "id.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "vote.h"

using namespace json_spirit;

class CAccountLog;

class CAccount {
public:
    CKeyID keyID;                   //!< keyID of the account (interchangeable to address)
    CRegID regID;                   //!< regID of the account
    CNickID nickID;                 //!< Nickname ID of the account (maxlen=32)
    CPubKey pubKey;                 //!< account public key
    CPubKey minerPubKey;            //!< miner saving account public key

    uint64_t bcoins;                //!< Free Base Coins
    uint64_t scoins;                //!< Stable Coins
    uint64_t fcoins;                //!< FundCoin balance

    uint64_t receivedVotes;         //!< votes received

    uint64_t lastVoteHeight;        //!< account's last vote block height used for computing interest
    vector<CVoteFund> voteFunds;    //!< account delegates votes sorted by vote amount
    
    bool hasOpenCdp;                //!< When true, its CDP exists in a map {cdp-$regid -> $cdp}

    uint256 sigHash;                //!< in-memory only

public:
    /**
     * @brief operate account
     * @param type: operate type
     * @param values
     * @param nCurHeight:  the tip block height
     * @return returns true if successful, otherwise false
     */
    bool OperateAccount(OperType type, const uint64_t &values, const uint64_t nCurHeight);
    bool UndoOperateAccount(const CAccountLog &accountLog);
    bool ProcessDelegateVote(vector<COperVoteFund> &operVoteFunds, const uint64_t nCurHeight);
    bool OperateVote(VoteOperType type, const uint64_t &values);

public:
    CAccount(CKeyID &keyId, CPubKey &pubKey, CNickID &nickId)
        : keyID(keyId),
          pubKey(pubKey),
          nickID(nickId),
          bcoins(0),
          scoins(0),
          fcoins(0),
          lastVoteHeight(0),
          receivedVotes(0) {
        minerPubKey = CPubKey();
        voteFunds.clear();
        regID.Clean();
    }

    CAccount()
        : keyID(uint160()),
          bcoins(0),
          scoins(0),
          fcoins(0),
          lastVoteHeight(0),
          receivedVotes(0) {
        pubKey      = CPubKey();
        minerPubKey = CPubKey();
        voteFunds.clear();
        regID.Clean();
    }

    CAccount(const CAccount &other) {
        this->keyID         = other.keyID;
        this->regID         = other.regID;
        this->nickID        = other.nickID;
        this->pubKey        = other.pubKey;
        this->minerPubKey   = other.minerPubKey;
        this->bcoins  = other.bcoins;
        this->scoins  = other.scoins;
        this->fcoins  = other.fcoins;
        this->receivedVotes = other.receivedVotes;
        this->lastVoteHeight= other.lastVoteHeight;
        this->voteFunds     = other.voteFunds;
    }

    CAccount &operator=(const CAccount &other) {
        if (this == &other)
            return *this;
        
        this->keyID         = other.keyID;
        this->regID         = other.regID;
        this->nickID        = other.nickID;
        this->pubKey        = other.pubKey;
        this->minerPubKey   = other.minerPubKey;
        this->bcoins  = other.bcoins;
        this->scoins  = other.scoins;
        this->fcoins  = other.fcoins;
        this->receivedVotes = other.receivedVotes;
        this->lastVoteHeight= other.lastVoteHeight;
        this->voteFunds     = other.voteFunds;
        
        return *this;
    }

    std::shared_ptr<CAccount> GetNewInstance() const { return std::make_shared<CAccount>(*this); }
    bool IsRegistered() const { return (pubKey.IsFullyValid() && pubKey.GetKeyId() == keyID); }
    bool SetRegId(const CRegID &regID) {
        this->regID = regID;
        return true;
    };

    bool GetRegId(CRegID &regID) const {
        regID = this->regID;
        return !regID.IsEmpty();
    };

    uint64_t GetFreeBCoins();
    uint64_t GetTotalBcoins();
    uint64_t GetVotedBCoins();
    uint64_t GetAccountProfit(uint64_t prevBlockHeight);
    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;
    bool IsEmptyValue() const { return !(bcoins > 0); }

    uint256 GetHash(bool recalculate = false) {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            //FIXME: need to check block soft-fork height here
            ss << regID << keyID << pubKey << minerPubKey
                << VARINT(bcoins) << VARINT(scoins) << VARINT(fcoins)
                << VARINT(lastVoteHeight)
                << voteFunds << receivedVotes;

            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    bool UpDateAccountPos(int nCurHeight);

    IMPLEMENT_SERIALIZE(
        READWRITE(regID);
        READWRITE(keyID);
        READWRITE(pubKey);
        READWRITE(minerPubKey);
        READWRITE(nickID);
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(scoins));
        READWRITE(VARINT(fcoins));
        READWRITE(VARINT(lastVoteHeight));
        READWRITE(voteFunds);
        READWRITE(receivedVotes);)

    uint64_t GetReceiveVotes() const { return receivedVotes; }

private:
    bool IsMoneyOverflow(uint64_t nAddMoney);
};

class CAccountLog {
public:
    CKeyID keyID;
    uint64_t bcoins;          //!< baseCoin balance
    uint64_t scoins;          //!< stableCoin balance
    uint64_t fcoins;          //!< fundCoin balance
    uint64_t lastVoteHeight;        //!< account's last vote height
    vector<CVoteFund> voteFunds;    //!< delegate votes
    uint64_t receivedVotes;         //!< votes received

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(lastVoteHeight));
        READWRITE(voteFunds);
        READWRITE(receivedVotes);)

public:
    CAccountLog(const CAccount &acct) {
        keyID        = acct.keyID;
        bcoins = acct.bcoins;
        lastVoteHeight  = acct.lastVoteHeight;
        voteFunds    = acct.voteFunds;
        receivedVotes= acct.receivedVotes;
    }
    CAccountLog(CKeyID &keyId) {
        keyID        = keyId;
        bcoins = 0;
        lastVoteHeight  = 0;
        receivedVotes= 0;
    }
    CAccountLog() {
        keyID        = uint160();
        bcoins = 0;
        lastVoteHeight  = 0;
        voteFunds.clear();
        receivedVotes = 0;
    }
    void SetValue(const CAccount &acct) {
        keyID           = acct.keyID;
        regID           = acct.regID;
        nickID          = acct.nickID;
        pubKey          = acct.pubKey;
        minerPubKey     = acct.minerPubKey;

        bcoins    = acct.bcoins;
        scoins    = acct.scoins;
        fcoins    = acct.fcoins;
        lastVoteHeight  = acct.lastVoteHeight;
        receivedVotes   = acct.receivedVotes;
        voteFunds       = acct.voteFunds;
        hasOpenCdp      = acct.hasOpenCdp;
    }
    string ToString() const;
};

#endif