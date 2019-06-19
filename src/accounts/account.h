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

#include "chainparams.h"
#include "crypto/hash.h"
#include "id.h"
#include "vote.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;

class CAccountLog;

enum CoinType: uint8_t {
    WICC = 1,
    MICC = 2,
    WUSD = 3,
};

struct CoinTypeHash {
    size_t operator()(const CoinType& type) const noexcept { return std::hash<int>{}(type); }
};

static const unordered_set<CoinType, CoinTypeHash> COINT_TYPE_SET = { WICC, MICC, WUSD};

enum PriceType: uint8_t {
    USD     = 1,
    CNY     = 2,
    EUR     = 3,
    BTC     = 10,
    USDT    = 11,
    GOLD    = 20,
    KWH     = 100, // kilowatt hour
};

enum BalanceOpType : uint8_t {
    NULL_OP     = 0,  //!< invalid op
    ADD_VALUE   = 1,  //!< add operate
    MINUS_VALUE = 2,  //!< minus operate
};

class CAccount {
public:
    CKeyID keyID;         //!< KeyID of the account (interchangeable to address)
    CRegID regID;         //!< RegID of the account
    CNickID nickID;       //!< Nickname ID of the account (maxlen=32)
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

    uint64_t receivedVotes;    //!< received votes
    uint64_t lastVoteHeight;   //!< account's last vote block height used for computing interest

    bool hasOpenCdp;
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
    bool OperateDEXSettle(CoinType coinType, uint64_t amount);
    bool ProcessDelegateVote(const vector<CCandidateVote>& candidateVotesIn,
                             vector<CCandidateVote>& candidateVotesInOut, const uint64_t curHeight);
    bool StakeVoteBcoins(VoteType type, const uint64_t votes);
    bool StakeFcoins(const int64_t fcoinsToStake); //price feeder must stake fcoins
    bool OperateFcoinStaking(const int64_t fcoinsToStake) { return false; } // TODO: ...

    bool StakeBcoinsToCdp(CoinType coinType, const int64_t bcoinsToStake, const int64_t mintedScoins);
    bool RedeemScoinsToCdp(const int64_t bcoinsToStake);
    bool LiquidateCdp(const int64_t bcoinsToStake);

public:
    CAccount(const CKeyID& keyId, const CNickID& nickId, const CPubKey& pubKey)
        : keyID(keyId),
          nickID(nickId),
          pubKey(pubKey),
          bcoins(0),
          scoins(0),
          fcoins(0),
          frozenDEXBcoins(0),
          frozenDEXScoins(0),
          frozenDEXFcoins(0),
          stakedBcoins(0),
          stakedFcoins(0),
          receivedVotes(0),
          lastVoteHeight(0),
          hasOpenCdp(false) {
        minerPubKey = CPubKey();
        regID.Clean();
    }

    CAccount() : CAccount(CKeyID(), CNickID(), CPubKey()) {}

    CAccount(const CAccount& other) {
        *this = other;
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
        this->frozenDEXBcoins = other.frozenDEXBcoins;
        this->frozenDEXScoins = other.frozenDEXScoins;
        this->frozenDEXFcoins = other.frozenDEXFcoins;
        this->stakedBcoins   = other.stakedBcoins;
        this->stakedFcoins   = other.stakedFcoins;
        this->receivedVotes  = other.receivedVotes;
        this->lastVoteHeight = other.lastVoteHeight;
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
            ss << keyID << regID << nickID << pubKey << minerPubKey << VARINT(bcoins) << VARINT(scoins)
               << VARINT(fcoins) << VARINT(frozenDEXBcoins) << VARINT(frozenDEXScoins) << VARINT(frozenDEXFcoins)
               << VARINT(stakedFcoins) << VARINT(receivedVotes) << VARINT(lastVoteHeight)
               << hasOpenCdp;

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    bool UpDateAccountPos(int nCurHeight);

    bool IsEmpty() const {
        return keyID.IsEmpty();
    }

    void SetEmpty() {
        keyID.SetEmpty();
        // TODO: need set other fields to empty()??
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(regID);
        READWRITE(nickID);
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
        READWRITE(VARINT(lastVoteHeight));
        READWRITE(hasOpenCdp);)

private:
    bool IsMoneyOverflow(uint64_t nAddMoney);
};

//TODO: add frozenDEXBCons etc below
class CAccountLog {
public:
    CKeyID keyID;         //!< keyID of the account (interchangeable to address)
    CRegID regID;         //!< regID of the account
    CNickID nickID;       //!< Nickname ID of the account (maxlen=32)
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

    bool hasOpenCdp;

    IMPLEMENT_SERIALIZE(
        READWRITE(keyID);
        READWRITE(regID);
        READWRITE(nickID);
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
        READWRITE(VARINT(lastVoteHeight));
        READWRITE(hasOpenCdp);)

public:
    CAccountLog(const CAccount& acct) {
        SetValue(acct);
    }

    CAccountLog(const CKeyID& keyIdIn):
        keyID(keyIdIn),
        regID(),
        nickID(),
        bcoins(0),
        scoins(0),
        fcoins(0),

        frozenDEXBcoins(0),
        frozenDEXScoins(0),
        frozenDEXFcoins(0),

        stakedBcoins(0),
        stakedFcoins(0),
        receivedVotes(0),
        lastVoteHeight(0),
        hasOpenCdp(false) {}

    CAccountLog(): CAccountLog(CKeyID()) {}

    void SetValue(const CAccount& acct) {
        keyID           = acct.keyID;
        regID           = acct.regID;
        nickID          = acct.nickID;
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
        hasOpenCdp      = acct.hasOpenCdp;
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