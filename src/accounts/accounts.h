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

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "key.h"
#include "chainparams.h"
#include "crypto/hash.h"

using namespace json_spirit;

class CID;
class CNullID;
class CAccountViewCache;
class CAccountLog;

typedef vector<unsigned char> vector_unsigned_char;
typedef boost::variant<CNullID, CRegID, CKeyID, CPubKey> CUserID;

enum IDTypeEnum {
    NullType = 0,
    PubKey = 1,
    RegID = 2,
    KeyID = 3,
    NickID = 4,
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

    void SetRegID(string strRegID);
    void SetRegIDByCompact(const vector<unsigned char> &vIn);

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
    CKeyID GetKeyId(const CAccountViewCache &view) const;
    uint32_t GetHeight() const { return nHeight; }
    bool operator==(const CRegID &co) const { return (this->nHeight == co.nHeight && this->nIndex == co.nIndex); }
    bool operator!=(const CRegID &co) const { return (this->nHeight != co.nHeight || this->nIndex != co.nIndex); }
    static bool IsSimpleRegIdStr(const string &str);
    static bool IsRegIdStr(const string &str);
    static bool GetKeyId(const string &str, CKeyID &keyId);
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

class CID {
private:
    vector_unsigned_char vchData;
    IDTypeEnum idType;

public:
    CID() {}
    CID(const CUserID &dest) { Set(dest); }

    const vector_unsigned_char &GetID() { return vchData; }
    static const vector_unsigned_char &UserIDToVector(const CUserID &userid) { return CID(userid).GetID(); }
    bool Set(const CRegID &id);
    bool Set(const CKeyID &id);
    bool Set(const CPubKey &id);
    bool Set(const CNullID &id);
    bool Set(const CUserID &userid);

    CUserID GetUserId() const;
    vector_unsigned_char GetData() const { return vchData; };

    string GetIDName() const {
        switch(idType) {
            case IDTypeEnum::NullType: return "Null";
            case IDTypeEnum::RegID: return "RegID";
            case IDTypeEnum::KeyID: return "KeyID";
            case IDTypeEnum::PubKey: return "PubKey";
            default:
                return "UnknownType";
        }
    }

    void SetIDType(IDTypeEnum idTypeIn) { idType = idTypeIn; }
    IDTypeEnum GetIDType() { return idType; }

    friend bool operator==(const CID &id1, const CID &id2) {
        return (id1.GetData() == id2.GetData());
    }

    string ToString() const {
        CUserID uid = GetUserId();
         switch(idType) {
            case IDTypeEnum::NullType:
                return "Null";
            case IDTypeEnum::RegID:
                return boost::get<CRegID>(uid).ToString();
            case IDTypeEnum::KeyID:
                return boost::get<CKeyID>(uid).ToString();
            case IDTypeEnum::PubKey:
                return boost::get<CPubKey>(uid).ToString();
            default:
                return "Unknown";
        }
    }

    Object ToJson() const {
        Object obj;
        string id = ToString();
        obj.push_back(Pair("idType", idType));
        obj.push_back(Pair("id", id));
        return obj;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(vchData);)
};

class CIDVisitor : public boost::static_visitor<bool> {
private:
    CID *pId;

public:
    CIDVisitor(CID *pIdIn) : pId(pIdIn) {}

    bool operator()(const CRegID &id) const { pId->SetIDType(IDTypeEnum::RegID); return pId->Set(id); }
    bool operator()(const CKeyID &id) const { pId->SetIDType(IDTypeEnum::KeyID); return pId->Set(id); }
    bool operator()(const CPubKey &id) const { pId->SetIDType(IDTypeEnum::PubKey); return pId->Set(id); }
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

class CVoteFund {
private:
    CID voteId;             //!< candidate RegId or PubKey
    uint64_t voteCount;     //!< count of votes to the candidate

    uint256 sigHash;        //!< only in memory

public:
    CID GetVoteId() { return voteId; }
    uint64_t GetVoteCount() { return voteCount; }

    void SetVoteCount(uint64_t votes) { voteCount = votes; }
    void SetVoteId(CID voteIdIn) { voteId = voteIdIn; }

public:
    CVoteFund() {
        voteId = CID();
        voteCount  = 0;
    }
    CVoteFund(uint64_t voteCountIn) {
        voteId = CID();
        voteCount  = voteCountIn;
    }
    CVoteFund(CID voteIdIn, uint64_t voteCountIn) {
        voteId = voteIdIn;
        voteCount  = voteCountIn;
    }
    CVoteFund(const CVoteFund &fund) {
        voteId = fund.voteId;
        voteCount  = fund.voteCount;
    }
    CVoteFund &operator=(const CVoteFund &fund) {
        if (this == &fund)
            return *this;

        this->voteId = fund.voteId;
        this->voteCount  = fund.voteCount;
        return *this;
    }
    ~CVoteFund() {}

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(voteCount) << voteId;
            uint256 *hash = const_cast<uint256 *>(&sigHash);
            *hash         = ss.GetHash();  // Truly need to write the sigHash.
        }

        return sigHash;
    }

    friend bool operator<(const CVoteFund &fa, const CVoteFund &fb) {
        return (fa.voteCount <= fb.voteCount);
    }
    friend bool operator>(const CVoteFund &fa, const CVoteFund &fb) {
        return !operator<(fa, fb);
    }
    friend bool operator==(const CVoteFund &fa, const CVoteFund &fb) {
        return (fa.voteId == fb.voteId && fa.voteCount == fb.voteCount);
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(voteId);
        READWRITE(VARINT(voteCount));
    )

    string ToString() const {
        string str("");
        string idType = voteId.GetIDName();
        str = idType + voteId.ToString();

        str += " votes:";
        str += strprintf("%s", voteCount);
        str += "\n";
        return str;
    }

    Object ToJson() const {
        Object obj;
        obj.push_back(Pair("voteId", this->voteId.ToJson()));
        obj.push_back(Pair("votes", voteCount));
        return obj;
    }
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
    string ToString() const;
    Object ToJson() const;
};

class CCdp {
public:
    uint64_t collateralBCoinAmount;
    uint64_t mintedSCoinAmount;
};

class CAccountNickID  {
private:
    vector_unsigned_char nickId;

public:
    CAccountNickID() {}
    CAccountNickID(vector_unsigned_char nickIdIn) {
        if (nickIdIn.size() > 32)
            throw ios_base::failure("Nickname ID length > 32 not allowed!");

        nickId = nickIdIn;
     }

    vector_unsigned_char GetNickId() const { return nickId; }

    string ToString() const { return std::string(nickId.begin(), nickId.end()); }

    IMPLEMENT_SERIALIZE(
        READWRITE(nickId);)
};

class CAccount {
public:
    CRegID regID;                   //!< regID of the account
    CKeyID keyID;                   //!< keyID of the account (interchangeable to address)
    CPubKey pubKey;                 //!< public key of the account
    CPubKey minerPubKey;            //!< miner public key of the account
    CAccountNickID nickID;          //!< Nickname ID of the account (maxlen=32)

    uint64_t bcoinBalance;          //!< BaseCoin balance
    uint64_t scoinBalance;          //!< StableCoin balance
    uint64_t fcoinBalance;          //!< FundCoin balance

    uint64_t nVoteHeight;           //!< account vote block height
    vector<CVoteFund> vVoteFunds;   //!< account delegates votes sorted by vote amount
    uint64_t receivedVotes;         //!< votes received

    bool hasOpenCdp;                //!< When true, its CDP exists in a map {cdp-$regid -> $cdp}

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
    CAccount(CKeyID &keyId, CPubKey &pubKey, CAccountNickID &nickId)
        : keyID(keyId),
          pubKey(pubKey),
          nickID(nickId),
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
        this->regID         = other.regID;
        this->keyID         = other.keyID;
        this->nickID        = other.nickID;
        this->pubKey        = other.pubKey;
        this->minerPubKey   = other.minerPubKey;
        this->bcoinBalance  = other.bcoinBalance;
        this->scoinBalance  = other.scoinBalance;
        this->fcoinBalance  = other.fcoinBalance;
        this->nVoteHeight   = other.nVoteHeight;
        this->vVoteFunds    = other.vVoteFunds;
        this->receivedVotes = other.receivedVotes;
    }

    CAccount &operator=(const CAccount &other) {
        if (this == &other)
            return *this;

        this->regID         = other.regID;
        this->keyID         = other.keyID;
        this->nickID        = other.nickID;
        this->pubKey        = other.pubKey;
        this->minerPubKey   = other.minerPubKey;
        this->bcoinBalance  = other.bcoinBalance;
        this->scoinBalance  = other.scoinBalance;
        this->fcoinBalance  = other.fcoinBalance;
        this->nVoteHeight   = other.nVoteHeight;
        this->vVoteFunds    = other.vVoteFunds;
        this->receivedVotes = other.receivedVotes;

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
    uint64_t receivedVotes;        //!< votes received

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
        receivedVotes= acct.receivedVotes;
    }
    CAccountLog(CKeyID &keyId) {
        keyID        = keyId;
        bcoinBalance = 0;
        nVoteHeight  = 0;
        receivedVotes= 0;
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
        receivedVotes= acct.receivedVotes;
        vVoteFunds   = acct.vVoteFunds;
    }
    string ToString() const;
};

#endif