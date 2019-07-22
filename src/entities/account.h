// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_ACCOUNT_H
#define ENTITIES_ACCOUNT_H

#include <boost/variant.hpp>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "asset.h"
#include "crypto/hash.h"
#include "id.h"
#include "vote.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;

class CAccountInfo;
class CAccountDBCache;

enum CoinType: uint8_t {
    WICC = 0,
    WGRT = 1,
    WUSD = 2,
    WCNY = 3
};

typedef CoinType AssetType;

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

inline bool ParseAssetType(const string& assetName, AssetType &assetType) {
    return ParseCoinType(assetName, assetType);
}

enum PriceType: uint8_t {
    USD     = 0,
    CNY     = 1,
    EUR     = 2,
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
    { USDT, "USDT"},
    { GOLD, "GOLD"},
    { KWH,  "KWH" }
};

static const unordered_map<string, PriceType> kPriceNameMapType = {
    { "USD", USD },
    { "CNY", CNY },
    { "EUR", EUR },
    { "BTC", BTC },
    { "USDT", USDT},
    { "GOLD", GOLD},
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

class CAccountToken {
public:
    uint64_t free_tokens;
    uint64_t frozen_tokens; //held by open DEX orders

public:
    CAccountToken(uint64_t &freeTokens, uint64_t &frozenTokens) :
                    free_tokens(freeTokens), frozen_tokens(frozenTokens) { }

    CAccountToken& operator=(const CAccountToken& other) {
        if (this == &other) return *this;

        this->free_tokens       = other.free_tokens;
        this->frozen_tokens     = other.frozen_tokens;

        return *this;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(free_tokens));
        READWRITE(VARINT(frozen_tokens));)

};

class CAccountInfo {
public:
    CKeyID  keyid;                  //!< unique: keyId of the account (interchangeable to address) - 20 bytes
    CRegID  regid;                  //!< unique: regId - derived from 1st TxCord - 6 bytes
    CNickID nickid;                 //!< unique: Nickname ID of the account (sting maxlen=32) - 8 bytes

    CPubKey owner_pubkey;           //!< account public key
    CPubKey miner_pubkey;           //!< miner saving account public key

    uint64_t free_bcoins;           //!< baseCoin balance
    uint64_t free_fcoins;           //!< fundCoin balance

    uint64_t frozen_bcoins;         //!< frozen bcoins in DEX
    uint64_t frozen_fcoins;         //!< frozen fcoins in DEX

    uint64_t staked_bcoins;         //!< Staked/Collateralized BaseCoins
    uint64_t staked_fcoins;         //!< Staked FundCoins for pricefeed right

    uint64_t received_votes;        //!< votes received
    uint64_t last_vote_height;      //!< account's last vote block height used for computing interest

    map<TokenSymbol, CAccountToken> extended_tokens;
                                    //!< all other coins like miner-issued stablecoins WUSD|WCNY
                                    //!< as well as user-issued onchain tokens (WRC20 compilant)
                                    //!< persisted separately, in-memory only

    IMPLEMENT_SERIALIZE(
        READWRITE(keyid);
        READWRITE(regid);
        READWRITE(nickid);
        READWRITE(owner_pubkey);
        READWRITE(miner_pubkey);
        READWRITE(VARINT(free_bcoins));
        READWRITE(VARINT(free_fcoins));
        READWRITE(VARINT(frozen_bcoins));
        READWRITE(VARINT(frozen_fcoins));
        READWRITE(VARINT(staked_bcoins));
        READWRITE(VARINT(staked_fcoins));
        READWRITE(VARINT(received_votes));
        READWRITE(VARINT(last_vote_height));)

public:
    CAccountInfo(const CAccountInfo& acct) { SetValue(acct); }

    CAccountInfo(const CKeyID& keyIdIn):
        keyid(keyIdIn),
        regid(),
        nickid(),
        free_bcoins(0),
        free_fcoins(0),
        frozen_bcoins(0),
        frozen_fcoins(0),
        staked_bcoins(0),
        staked_fcoins(0),
        received_votes(0),
        last_vote_height(0) {}

    CAccountInfo(): CAccountInfo(CKeyID()) {}

    CAccountInfo& operator=(const CAccountInfo& other) {
        if (this == &other) return *this;

        this->keyid             = other.keyid;
        this->regid             = other.regid;
        this->nickid            = other.nickid;
        this->owner_pubkey      = other.owner_pubkey;
        this->miner_pubkey      = other.miner_pubkey;
        this->free_bcoins       = other.free_bcoins;
        this->free_fcoins       = other.free_fcoins;
        this->frozen_bcoins     = other.frozen_bcoins;
        this->frozen_fcoins     = other.frozen_fcoins;
        this->staked_bcoins     = other.staked_bcoins;
        this->staked_fcoins     = other.staked_fcoins;
        this->received_votes    = other.received_votes;
        this->last_vote_height  = other.last_vote_height;
        this->extended_tokens   = other.extended_tokens;

        return *this;
    }

    void SetValue(const CAccountInfo& acct) {
        keyid           = acct.keyid;
        regid           = acct.regid;
        nickid          = acct.nickid;
        owner_pubkey    = acct.owner_pubkey;
        miner_pubkey    = acct.miner_pubkey;
        free_bcoins     = acct.free_bcoins;
        free_fcoins     = acct.free_fcoins;
        frozen_bcoins   = acct.frozen_bcoins;
        frozen_fcoins   = acct.frozen_fcoins;
        staked_bcoins   = acct.staked_bcoins;
        staked_fcoins   = acct.staked_fcoins;
        received_votes  = acct.received_votes;
        last_vote_height= acct.last_vote_height;
        extended_tokens = acct.extended_tokens;
    }

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss  << keyid << regid << nickid << owner_pubkey << miner_pubkey
                << VARINT(free_bcoins) << VARINT(free_fcoins)
                << VARINT(frozen_bcoins) << VARINT(frozen_fcoins)
                << VARINT(staked_bcoins) << VARINT(staked_fcoins)
                << VARINT(received_votes) << VARINT(last_vote_height);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    string ToString() const {
        string str;
        str += strprintf(
            "AccountInfo: keyid=%d regid=%s nickid=%s owner_pubKey=%s miner_pubKey=%s "
            "free_bcoins=%lld free_fcoins=%lld staked_bcoins=%lld staked_fcoins=%lld "
            "extended_tokens: {%s}"
            " received_votes=%lld last_vote_height=%lld\n",
            keyid.GetHex(), regid.ToString(), nickid.ToString(), owner_pubkey.ToString(), miner_pubkey.ToString(),
            free_bcoins, free_fcoins, staked_bcoins, staked_fcoins, extended_tokens.ToString(),
            received_votes, last_vote_height);

        return str;
    }

private:
    mutable uint256 sigHash;    //!< in-memory only
};

class CAccount: public CAccountInfo {

public:
    bool OperateBalance(const CoinType coinType, const BalanceOpType opType, const uint64_t value);
    bool PayInterest(uint64_t scoinInterest, uint64_t fcoinsInterest);
    bool UndoOperateAccount(const CAccountInfo& accountInfo);
    bool FreezeDexCoin(CoinType coinType, uint64_t amount);
    bool FreezeDexAsset(AssetType assetType, uint64_t amount) {
        // asset always is coin, so can do the freeze as coin
        return FreezeDexCoin(assetType, amount);
    }
    bool UnFreezeDexCoin(CoinType coinType, uint64_t amount);
    bool MinusDEXFrozenCoin(CoinType coinType,  uint64_t coins);

    bool ProcessDelegateVotes(const vector<CCandidateVote>& candidateVotesIn,
                              vector<CCandidateVote>& candidateVotesInOut,
                              const uint64_t currHeight,
                              const CAccountDBCache* pAccountCache);
    bool StakeVoteBcoins(VoteType type, const uint64_t votes);
    bool StakeFcoins(const int64_t fcoinsToStake); //price feeder must stake fcoins
    bool StakeBcoinsToCdp(CoinType coinType, const int64_t bcoinsToStake, const int64_t mintedScoins);

public:
    CAccount(const CKeyID& keyidIn, const CNickID& nickidIn, const CPubKey& ownerPubkeyIn)
        : keyid(keyidIn),
          nickid(nickidIn),
          owner_pubkey(ownerPubkeyIn),
          free_bcoins(0),
          free_fcoins(0),
          frozen_bcoins(0),
          frozen_fcoins(0),
          staked_bcoins(0),
          staked_fcoins(0),
          received_votes(0),
          last_vote_height(0) {
        miner_pubkey = CPubKey();
        extended_tokens.Clear();
        regid.Clear();
    }

    CAccount() : CAccount(CKeyID(), CNickID(), CPubKey()) {}

    CAccount(const CAccount& other) {
        *this = other;
    }

    CAccount& operator=(const CAccount& other) {
        //FIXME
        return *this;
    }

    std::shared_ptr<CAccount> GetNewInstance() const {
        return std::make_shared<CAccount>(*this);
    }

    bool HaveOwnerPubKey() const {
        return owner_pubkey.IsFullyValid();
    }

    bool RegIDIsMature() const;

    bool SetRegId(const CRegID& regId) {
        this->regid = regId;
        return true;
    };

    bool GetRegId(CRegID& regid) const {
        regid = this->regid;
        return !regid.IsEmpty();
    };

    uint64_t GetTotalBcoins(const vector<CCandidateVote>& candidateVotes, const uint64_t currHeight);
    uint64_t GetVotedBCoins(const vector<CCandidateVote>& candidateVotes, const uint64_t currHeight);

    uint64_t ComputeVoteStakingInterest(const vector<CCandidateVote> &candidateVotes, const uint64_t currHeight);
    uint64_t ComputeBlockInflateInterest(const uint64_t currHeight) const;

    bool IsEmptyValue() const { return !(free_bcoins > 0); }
    bool IsEmpty() const { return keyid.IsEmpty(); }
    void SetEmpty() { keyid.SetEmpty(); }  // TODO: need set other fields to empty()??
    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;

private:
    bool IsBcoinWithinRange(uint64_t nAddMoney);
    bool IsFcoinWithinRange(uint64_t nAddMoney);
};

enum ACCOUNT_TYPE {
    // account type
    REGID      = 0x01,  //!< Registration account id
    BASE58ADDR = 0x02,  //!< Public key
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
		for (int i = 0; i < 34; i++)
			READWRITE(accountId[i]);
		READWRITE(opType);
		READWRITE(timeoutHeight);
		for (int i = 0; i < 8; i++)
			READWRITE(money[i]);
	)

	CVmOperate() {
		accountType = REGID;
		memset(accountId, 0, 34);
		opType = NULL_OP;
		timeoutHeight = 0;
		memset(money, 0, 8);
	}

	Object ToJson();
};


#endif //ENTITIES_ACCOUNT_H
