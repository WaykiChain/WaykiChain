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

#include "../json/json_spirit_utils.h"
#include "../json/json_spirit_value.h"
#include "crypto/hash.h"
#include "accounts/key.h"
#include "chainparams.h"

using namespace json_spirit;

class CID;
class CAccountViewCache;
class CAccountLog;

typedef vector<unsigned char> vector_unsigned_char;

class CVoteFund {
public:
    CPubKey pubKey;   //!< delegates public key
    uint64_t value;   //!< amount of vote
    uint256 sigHash;  //!< only in memory

public:
    CVoteFund() {
        value  = 0;
        pubKey = CPubKey();
    }
    CVoteFund(uint64_t valueIn) {
        value  = valueIn;
        pubKey = CPubKey();
    }
    CVoteFund(uint64_t valueIn, CPubKey pubKeyIn) {
        value  = valueIn;
        pubKey = pubKeyIn;
    }
    CVoteFund(const CVoteFund &fund) {
        value  = fund.value;
        pubKey = fund.pubKey;
    }
    CVoteFund &operator=(const CVoteFund &fund) {
        if (this == &fund) {
            return *this;
        }
        this->value  = fund.value;
        this->pubKey = fund.pubKey;
        return *this;
    }
    ~CVoteFund() {}

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(value) << pubKey;
            // Truly need to write the sigHash.
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();
        }

        return sigHash;
    }

    friend bool operator<(const CVoteFund &fa, const CVoteFund &fb) {
        if (fa.value <= fb.value)
            return true;
        else
            return false;
    }
    friend bool operator>(const CVoteFund &fa, const CVoteFund &fb) {
        return !operator<(fa, fb);
    }
    friend bool operator==(const CVoteFund &fa, const CVoteFund &fb) {
        if (fa.pubKey != fb.pubKey)
            return false;
        if (fa.value != fb.value)
            return false;
        return true;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(pubKey);
        READWRITE(VARINT(value));
    )

    string ToString(bool isAddress = false) const {
        string str("");
        str += "pubKey:";
        if (isAddress) {
            str += pubKey.GetKeyID().ToAddress();
        } else {
            str += pubKey.ToString();
        }
        str += " value:";
        str += strprintf("%s", value);
        str += "\n";
        return str;
    }
    Object ToJson(bool isAddress = false) const;
};

class CNullID {
public:
    friend bool operator==(const CNullID &a, const CNullID &b) { return true; }
    friend bool operator<(const CNullID &a, const CNullID &b) { return true; }
};

class CRegID {
private:
    uint32_t nHeight;
    uint16_t nIndex;
    mutable vector<unsigned char> vRegID;
    void SetRegIDByCompact(const vector<unsigned char> &vIn);
    void SetRegID(string strRegID);

public:
    friend class CID;
    CRegID(string strRegID);
    CRegID(const vector<unsigned char> &vIn);
    CRegID(uint32_t nHeight = 0, uint16_t nIndex = 0);

    const vector<unsigned char> &GetVec6() const {
        assert(vRegID.size() == 6);
        return vRegID;
    }
    void SetRegID(const vector<unsigned char> &vIn);
    CKeyID GetKeyID(const CAccountViewCache &view) const;
    uint32_t GetHeight() const { return nHeight; }
    bool operator==(const CRegID &co) const { return (this->nHeight == co.nHeight && this->nIndex == co.nIndex); }
    bool operator!=(const CRegID &co) const { return (this->nHeight != co.nHeight || this->nIndex != co.nIndex); }
    static bool IsSimpleRegIdStr(const string &str);
    static bool IsRegIdStr(const string &str);
    static bool GetKeyID(const string &str, CKeyID &keyId);
    bool IsEmpty() const { return (nHeight == 0 && nIndex == 0); };
    bool Clean();
    string ToString() const;

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(nHeight));
        READWRITE(VARINT(nIndex));
        if (fRead) {
            vRegID.clear();
            vRegID.insert(vRegID.end(), BEGIN(nHeight), END(nHeight));
            vRegID.insert(vRegID.end(), BEGIN(nIndex), END(nIndex));
        })
};

typedef boost::variant<CNullID, CRegID, CKeyID, CPubKey> CUserID;

class CID {
private:
    vector_unsigned_char vchData;

public:
    const vector_unsigned_char &GetID() { return vchData; }
    static const vector_unsigned_char &UserIDToVector(const CUserID &userid) { return CID(userid).GetID(); }
    bool Set(const CRegID &id);
    bool Set(const CKeyID &id);
    bool Set(const CPubKey &id);
    bool Set(const CNullID &id);
    bool Set(const CUserID &userid);
    CID() {}
    CID(const CUserID &dest) { Set(dest); }
    CUserID GetUserId();
    IMPLEMENT_SERIALIZE(
        READWRITE(vchData);)
};

class CIDVisitor : public boost::static_visitor<bool> {
private:
    CID *pId;

public:
    CIDVisitor(CID *pIdIn) : pId(pIdIn) {}
    bool operator()(const CRegID &id) const { return pId->Set(id); }
    bool operator()(const CKeyID &id) const { return pId->Set(id); }
    bool operator()(const CPubKey &id) const { return pId->Set(id); }
    bool operator()(const CNullID &no) const { return true; }
};

/**
 * brief:   kinds of fund type
 */
enum FundType: unsigned char {
    FREEDOM = 1,    //!< FREEDOM
    REWARD_FUND,    //!< REWARD_FUND
    NULL_FUNDTYPE,  //!< NULL_FUNDTYPE
};

enum OperType: unsigned char {
    ADD_FREE   = 1,  //!< add money to freedom
    MINUS_FREE = 2,  //!< minus money from freedom
    NULL_OPERTYPE,   //!< invalid operate type
};

enum VoteOperType: unsigned char {
    ADD_FUND   = 1,  //!< add operate
    MINUS_FUND = 2,  //!< minus operate
    NULL_OPER,       //!< invalid
};

class COperVoteFund {
public:
    static const string voteOperTypeArray[3];

public:
    unsigned char operType;  //!<1:ADD_FUND 2:MINUS_FUND
    CVoteFund fund;

    IMPLEMENT_SERIALIZE(
        READWRITE(operType);
        READWRITE(fund);)
public:
    COperVoteFund() {
        operType = NULL_OPER;
    }
    COperVoteFund(unsigned char nType, const CVoteFund &operFund) {
        operType = nType;
        fund     = operFund;
    }
    string ToString(bool isAddress = false) const;
    Object ToJson(bool isAddress = false) const;
};

class CCdp {
public:
    uint64_t collateralBCoinAmount;
    uint64_t mintedSCoinAmount;
};

class CAccount {
public:
    CRegID regID;                   //!< regID of the account
    CKeyID keyID;                   //!< keyID of the account
    CPubKey pubKey;                 //!< public key of the account
    CPubKey minerPubKey;            //!< miner public key of the account
    uint64_t bcoinBalance;          //!< BaseCoin balance
    uint64_t scoinBalance;          //!< StableCoin balance
    uint64_t fcoinBalance;          //!< FundCoin balance
    uint64_t nVoteHeight;           //!< account vote block height
    vector<CVoteFund> vVoteFunds;   //!< account delegate votes order by vote value
    uint64_t receivedVotes;         //!< votes received
    bool hasOpenCdp;                //!< Whether the account has open CDP or not. If true, it exists in {cdp-$regid : $cdp}

    uint256 sigHash;                //!< memory only

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
    CAccount(CKeyID &keyId, CPubKey &pubKey)
        : keyID(keyId),
          pubKey(pubKey),
          bcoinBalance(0),
          scoinBalance(0),
          fcoinBalance(0),
          nVoteHeight(0),
          receivedVotes(0) {
        minerPubKey = CPubKey();
        vVoteFunds.clear();
        regID.Clean();
    }

    CAccount()
        : keyID(uint160()),
          bcoinBalance(0),
          scoinBalance(0),
          fcoinBalance(0),
          nVoteHeight(0),
          receivedVotes(0) {
        pubKey      = CPubKey();
        minerPubKey = CPubKey();
        vVoteFunds.clear();
        regID.Clean();
    }

    CAccount(const CAccount &other) {
        this->regID        = other.regID;
        this->keyID        = other.keyID;
        this->pubKey       = other.pubKey;
        this->minerPubKey  = other.minerPubKey;
        this->bcoinBalance = other.bcoinBalance;
        this->scoinBalance = other.scoinBalance;
        this->fcoinBalance = other.fcoinBalance;
        this->nVoteHeight  = other.nVoteHeight;
        this->vVoteFunds   = other.vVoteFunds;
        this->receivedVotes      = other.receivedVotes;
    }

    CAccount &operator=(const CAccount &other) {
        if (this == &other)
            return *this;

        this->regID        = other.regID;
        this->keyID        = other.keyID;
        this->pubKey       = other.pubKey;
        this->minerPubKey  = other.minerPubKey;
        this->bcoinBalance = other.bcoinBalance;
        this->scoinBalance = other.scoinBalance;
        this->fcoinBalance = other.fcoinBalance;
        this->nVoteHeight  = other.nVoteHeight;
        this->vVoteFunds   = other.vVoteFunds;
        this->receivedVotes      = other.receivedVotes;

        return *this;
    }

    std::shared_ptr<CAccount> GetNewInstance() const { return std::make_shared<CAccount>(*this); }
    bool IsRegistered() const { return (pubKey.IsFullyValid() && pubKey.GetKeyID() == keyID); }
    bool SetRegId(const CRegID &regID) {
        this->regID = regID;
        return true;
    };
    bool GetRegId(CRegID &regID) const {
        regID = this->regID;
        return !regID.IsEmpty();
    };
    uint64_t GetRawBalance();
    uint64_t GetTotalBalance();
    uint64_t GetFrozenBalance();
    uint64_t GetAccountProfit(uint64_t prevBlockHeight);
    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;
    bool IsEmptyValue() const { return !(bcoinBalance > 0); }

    uint256 GetHash(bool recalculate = false) {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            //FIXME: need to check block soft-fork height here
            ss << regID << keyID << pubKey << minerPubKey
                << VARINT(bcoinBalance) << VARINT(scoinBalance) << VARINT(fcoinBalance)
                << VARINT(nVoteHeight)
                << vVoteFunds << receivedVotes;

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
        READWRITE(VARINT(bcoinBalance));
        READWRITE(VARINT(nVoteHeight));
        READWRITE(vVoteFunds);
        READWRITE(receivedVotes);)

    uint64_t GetReceiveVotes() const { return receivedVotes; }

private:
    bool IsMoneyOverflow(uint64_t nAddMoney);
};

class CAccountLog {
public:
    CKeyID keyID;
    uint64_t bcoinBalance;         //!< freedom money which coinage greater than 30 days
    uint64_t nVoteHeight;          //!< account vote height
    vector<CVoteFund> vVoteFunds;  //!< delegate votes
    uint64_t receivedVotes;              //!< votes received

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(VARINT(bcoinBalance));
        READWRITE(VARINT(nVoteHeight));
        READWRITE(vVoteFunds);
        READWRITE(receivedVotes);)

public:
    CAccountLog(const CAccount &acct) {
        keyID        = acct.keyID;
        bcoinBalance = acct.bcoinBalance;
        nVoteHeight  = acct.nVoteHeight;
        vVoteFunds   = acct.vVoteFunds;
        receivedVotes      = acct.receivedVotes;
    }
    CAccountLog(CKeyID &keyId) {
        keyID        = keyId;
        bcoinBalance = 0;
        nVoteHeight  = 0;
        receivedVotes      = 0;
    }
    CAccountLog() {
        keyID        = uint160();
        bcoinBalance = 0;
        nVoteHeight  = 0;
        vVoteFunds.clear();
        receivedVotes = 0;
    }
    void SetValue(const CAccount &acct) {
        keyID        = acct.keyID;
        bcoinBalance = acct.bcoinBalance;
        nVoteHeight  = acct.nVoteHeight;
        receivedVotes      = acct.receivedVotes;
        vVoteFunds   = acct.vVoteFunds;
    }
    string ToString() const;
};

#endif