// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ACCOUNTS_H
#define ACCOUNTS_H

#include <boost/variant.hpp>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "crypto/hash.h"
#include "id.h"
#include "vote.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;

class CAccountLog;

enum CoinType: uint8_t {
    WICC = 1,
    WGRT = 2,
    WUSD = 3,
    WCNY = 4
};

// make compatibility with low GCC version(≤ 4.9.2)
struct CoinTypeHash {
    size_t operator()(const CoinType& type) const noexcept { return std::hash<uint8_t>{}(type); }
};

static const unordered_map<CoinType, string, CoinTypeHash> kCoinTypeMapName = {
    {WICC, "WICC"},
    {WGRT, "WGRT"},
    {WUSD, "WUSD"},
    {WCNY, "WCNY"}
};

static const unordered_map<string, CoinType> kCoinNameMapType = {
    {"WICC", WICC},
    {"WGRT", WGRT},
    {"WUSD", WUSD},
    {"WCNY", WCNY}
};

inline const string& GetCoinTypeName(CoinType coinType) {
    return kCoinTypeMapName.at(coinType);
}

inline bool ParseCoinType(const string& coinName, CoinType &coinType) {
    if (coinName != "") {
        auto it = kCoinNameMapType.find(coinName);
        if (it != kCoinNameMapType.end()) {
            coinType = it->second;
            return true;
        }
    }
    return false;
}

enum PriceType: uint8_t {
    USD     = 1,
    CNY     = 2,
    EUR     = 3,
    BTC     = 10,
    USDT    = 11,
    GOLD    = 20,
    KWH     = 100, // kilowatt hour
};

struct PriceTypeHash {
    size_t operator()(const PriceType& type) const noexcept { return std::hash<uint8_t>{}(type); }
};

static const unordered_map<PriceType, string, PriceTypeHash> kPriceTypeMapName = {
    { USD, "USD" },
    { CNY, "CNY" },
    { EUR, "EUR" },
    { BTC, "BTC" },
    {USDT, "USDT"},
    {GOLD, "GOLD"},
    { KWH, "KWH" }
};

static const unordered_map<string, PriceType> kPriceNameMapType = {
    { "USD", USD },
    { "CNY", CNY },
    { "EUR", EUR },
    { "BTC", BTC },
    {"USDT", USDT},
    {"GOLD", GOLD},
    { "KWH", KWH }
};

inline const string& GetPriceTypeName(PriceType priceType) {
    return kPriceTypeMapName.at(priceType);
}

inline bool ParsePriceType(const string& priceName, PriceType &priceType) {
    if (priceName != "") {
        auto it = kPriceNameMapType.find(priceName);
        if (it != kPriceNameMapType.end()) {
            priceType = it->second;
            return true;
        }
    }
    return false;
}

enum BalanceOpType : uint8_t {
    NULL_OP     = 0,  //!< invalid op
    ADD_VALUE   = 1,  //!< add operate
    MINUS_VALUE = 2,  //!< minus operate
};

class CAccount {
public:
    CKeyID keyId;         //!< KeyID of the account (interchangeable to address) : 20 Bytes
    CRegID regId;         //!< RegID of the account: 6 Bytes
    CNickID nickId;       //!< Nickname ID of the account (maxlen=32)
    CPubKey pubKey;       //!< account public key
    CPubKey minerPubKey;  //!< miner saving account public key

    uint64_t bcoins;        //!< BaseCoin balance
    uint64_t scoins;        //!< StableCoin balance
    uint64_t fcoins;        //!< FundCoin balance

    uint64_t frozenDEXBcoins;  //!< frozen bcoins in DEX
    uint64_t frozenDEXScoins;  //!< frozen scoins in DEX
    uint64_t frozenDEXFcoins;  //!< frozen fcoins in DEX

    uint64_t stakedBcoins;  //!< Staked/Collateralized BaseCoins (accumulated)
    uint64_t stakedFcoins;  //!< Staked FundCoins for pricefeed right (accumulated)

    uint64_t receivedVotes;   //!< received votes
    uint64_t lastVoteHeight;  //!< account's last vote block height used for computing interest

    mutable uint256 sigHash;  //!< in-memory only

public:
    /**
     * @brief operate account balance
     * @param coinType: balance coin Type
     * @param opType: balance operate type
     * @param value: balance value to add or minus
     * @return returns true if successful, otherwise false
     */
    bool OperateBalance(const CoinType coinType, const BalanceOpType opType, const uint64_t value);
    bool PayInterest(uint64_t scoinInterest, uint64_t fcoinsInterest);
    bool UndoOperateAccount(const CAccountLog& accountLog);
    bool FreezeDexCoin(CoinType coinType, uint64_t amount);
    bool FreezeDexAsset(CoinType assetType, uint64_t amount) {
        // asset always is coin, so can do the freeze as coin
        return FreezeDexCoin(assetType, amount);
    }
    bool UnFreezeDexCoin(CoinType coinType, uint64_t amount);
    bool MinusDEXFrozenCoin(CoinType coinType,  uint64_t coins);

    bool ProcessDelegateVote(const vector<CCandidateVote>& candidateVotesIn,
                             vector<CCandidateVote>& candidateVotesInOut, const uint64_t curHeight);
    bool StakeVoteBcoins(VoteType type, const uint64_t votes);
    bool StakeFcoins(const int64_t fcoinsToStake); //price feeder must stake fcoins
    bool OperateFcoinStaking(const int64_t fcoinsToStake) { return false; } // TODO: ...

    bool StakeBcoinsToCdp(CoinType coinType, const int64_t bcoinsToStake, const int64_t mintedScoins);

public:
    CAccount(const CKeyID& keyId, const CNickID& nickId, const CPubKey& pubKey)
        : keyId(keyId),
          regId(),
          nickId(nickId),
          pubKey(pubKey),
          minerPubKey(),
          bcoins(0),
          scoins(0),
          fcoins(0),
          frozenDEXBcoins(0),
          frozenDEXScoins(0),
          frozenDEXFcoins(0),
          stakedBcoins(0),
          stakedFcoins(0),
          receivedVotes(0),
          lastVoteHeight(0) {
        minerPubKey = CPubKey();
        regId.Clean();
    }

    CAccount() : CAccount(CKeyID(), CNickID(), CPubKey()) {}

    CAccount(const CAccount& other) {
        *this = other;
    }

    CAccount& operator=(const CAccount& other) {
        if (this == &other) return *this;

        this->keyId          = other.keyId;
        this->regId          = other.regId;
        this->nickId         = other.nickId;
        this->pubKey         = other.pubKey;
        this->minerPubKey    = other.minerPubKey;
        this->bcoins         = other.bcoins;
        this->scoins         = other.scoins;
        this->fcoins         = other.fcoins;
        this->frozenDEXBcoins = other.frozenDEXBcoins;
        this->frozenDEXScoins = other.frozenDEXScoins;
        this->frozenDEXFcoins = other.frozenDEXFcoins;
        this->stakedBcoins   = other.stakedBcoins;
        this->stakedFcoins   = other.stakedFcoins;
        this->receivedVotes  = other.receivedVotes;
        this->lastVoteHeight = other.lastVoteHeight;

        return *this;
    }

    std::shared_ptr<CAccount> GetNewInstance() const {
        return std::make_shared<CAccount>(*this);
    }

    bool IsRegistered() const {
        return pubKey.IsFullyValid();
    }

    bool RegIDIsMature() const;

    bool SetRegId(const CRegID& regId) {
        this->regId = regId;
        return true;
    };

    bool GetRegId(CRegID& regId) const {
        regId = this->regId;
        return !regId.IsEmpty();
    };

    uint64_t GetFreeBcoins() const { return bcoins; }
    uint64_t GetFreeScoins() const { return scoins; }
    uint64_t GetFreeFcoins() const { return fcoins; }

    uint64_t GetFrozenBCoins() const { return frozenDEXBcoins; }
    uint64_t GetFrozenScoins() const { return frozenDEXScoins; }
    uint64_t GetFrozenFcoins() const { return frozenDEXFcoins; }

    uint64_t GetReceiveVotes() const { return receivedVotes; }

    uint64_t GetTotalBcoins(const vector<CCandidateVote>& candidateVotes, const uint64_t curHeight);
    uint64_t GetVotedBCoins(const vector<CCandidateVote>& candidateVotes, const uint64_t curHeight);

    // Get profits for voting.
    uint64_t GetAccountProfit(const vector<CCandidateVote> &candidateVotes, const uint64_t curHeight);
    // Calculate profits for voted.
    uint64_t CalculateAccountProfit(const uint64_t curHeight) const;

    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;
    bool IsEmptyValue() const { return !(bcoins > 0); }

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << keyId << regId << nickId << pubKey << minerPubKey << VARINT(bcoins) << VARINT(scoins)
               << VARINT(fcoins) << VARINT(frozenDEXBcoins) << VARINT(frozenDEXScoins) << VARINT(frozenDEXFcoins)
               << VARINT(stakedFcoins) << VARINT(receivedVotes) << VARINT(lastVoteHeight);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    bool UpDateAccountPos(int nCurHeight);

    bool IsEmpty() const {
        return keyId.IsEmpty();
    }

    void SetEmpty() {
        keyId.SetEmpty();
        // TODO: need set other fields to empty()??
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(keyId);
        READWRITE(regId);
        READWRITE(nickId);
        READWRITE(pubKey);
        READWRITE(minerPubKey);
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(scoins));
        READWRITE(VARINT(fcoins));
        READWRITE(VARINT(frozenDEXBcoins));
        READWRITE(VARINT(frozenDEXScoins));
        READWRITE(VARINT(frozenDEXFcoins));
        READWRITE(VARINT(stakedBcoins));
        READWRITE(VARINT(stakedFcoins));
        READWRITE(VARINT(receivedVotes));
        READWRITE(VARINT(lastVoteHeight));)
private:
    bool IsMoneyValid(uint64_t nAddMoney);
};

//TODO: add frozenDEXBCons etc below
class CAccountLog {
public:
    CKeyID keyId;         //!< keyId of the account (interchangeable to address)
    CRegID regId;         //!< regId of the account
    CNickID nickId;       //!< Nickname ID of the account (maxlen=32)
    CPubKey pubKey;       //!< account public key
    CPubKey minerPubKey;  //!< miner saving account public key

    uint64_t bcoins;  //!< baseCoin balance
    uint64_t scoins;  //!< stableCoin balance
    uint64_t fcoins;  //!< fundCoin balance

    uint64_t frozenDEXBcoins;  //!< frozen bcoins in DEX
    uint64_t frozenDEXScoins;  //!< frozen scoins in DEX
    uint64_t frozenDEXFcoins;  //!< frozen fcoins in DEX

    uint64_t stakedBcoins;  //!< Staked/Collateralized BaseCoins
    uint64_t stakedFcoins;  //!< Staked FundCoins for pricefeed right

    uint64_t receivedVotes;   //!< votes received
    uint64_t lastVoteHeight;  //!< account's last vote block height used for computing interest

    IMPLEMENT_SERIALIZE(
        READWRITE(keyId);
        READWRITE(regId);
        READWRITE(nickId);
        READWRITE(pubKey);
        READWRITE(minerPubKey);
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(scoins));
        READWRITE(VARINT(fcoins));
        READWRITE(VARINT(frozenDEXBcoins));
        READWRITE(VARINT(frozenDEXScoins));
        READWRITE(VARINT(frozenDEXFcoins));
        READWRITE(VARINT(stakedBcoins));
        READWRITE(VARINT(stakedFcoins));
        READWRITE(VARINT(receivedVotes));
        READWRITE(VARINT(lastVoteHeight));)

public:
    CAccountLog(const CAccount& acct) {
        SetValue(acct);
    }

    CAccountLog(const CKeyID& keyIdIn):
        keyId(keyIdIn),
        regId(),
        nickId(),
        bcoins(0),
        scoins(0),
        fcoins(0),

        frozenDEXBcoins(0),
        frozenDEXScoins(0),
        frozenDEXFcoins(0),

        stakedBcoins(0),
        stakedFcoins(0),
        receivedVotes(0),
        lastVoteHeight(0) {}

    CAccountLog(): CAccountLog(CKeyID()) {}

    void SetValue(const CAccount& acct) {
        keyId           = acct.keyId;
        regId           = acct.regId;
        nickId          = acct.nickId;
        pubKey          = acct.pubKey;
        minerPubKey     = acct.minerPubKey;
        bcoins          = acct.bcoins;
        scoins          = acct.scoins;
        fcoins          = acct.fcoins;
        frozenDEXBcoins = acct.frozenDEXBcoins;
        frozenDEXScoins = acct.frozenDEXScoins;
        frozenDEXFcoins = acct.frozenDEXFcoins;
        stakedBcoins    = acct.stakedBcoins;
        stakedFcoins    = acct.stakedFcoins;
        receivedVotes   = acct.receivedVotes;
        lastVoteHeight  = acct.lastVoteHeight;
    }

    string ToString() const;
};


enum ACCOUNT_TYPE {
    // account type
    regid      = 0x01,  //!< Registration account id
    base58addr = 0x02,  //!< Public key
};
/**
 * account operate produced in contract
 * TODO: rename CVmOperate
 */
class CVmOperate{
public:
	unsigned char accountType;      //!< regid or base58addr
	unsigned char accountId[34];	//!< accountId: address
	unsigned char opType;		    //!< OperType
	unsigned int  timeoutHeight;    //!< the transacion Timeout height
	unsigned char money[8];			//!< The transfer amount

	IMPLEMENT_SERIALIZE
	(
		READWRITE(accountType);
		for (int i = 0;i < 34;i++)
			READWRITE(accountId[i]);
		READWRITE(opType);
		READWRITE(timeoutHeight);
		for (int i = 0;i < 8;i++)
			READWRITE(money[i]);
	)

	CVmOperate() {
		accountType = regid;
		memset(accountId, 0, 34);
		opType = NULL_OP;
		timeoutHeight = 0;
		memset(money, 0, 8);
	}

	Object ToJson();
};


#endif
