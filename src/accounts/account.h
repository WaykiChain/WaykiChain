// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


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

enum CoinType: uint8_t {
    WICC = 1,
    MICC = 2,
    WUSD = 3,
};

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
    CKeyID keyID;         //!< keyID of the account (interchangeable to address)
    CRegID regID;         //!< regID of the account
    CNickID nickID;       //!< Nickname ID of the account (maxlen=32)
    CPubKey pubKey;       //!< account public key
    CPubKey minerPubKey;  //!< miner saving account public key

    uint64_t bcoins;        //!< BaseCoin balance
    uint64_t scoins;        //!< StableCoin balance
    uint64_t fcoins;        //!< FundCoin balance
    uint64_t stakedBcoins;  //!< Staked/Collateralized BaseCoins (accumulated)
    uint64_t stakedFcoins;  //!< Staked FundCoins for pricefeed right (accumulated)

    uint64_t receivedVotes;                 //!< received votes
    uint64_t lastVoteHeight;                //!< account's last vote block height used for computing interest

    vector<CCandidateVote> candidateVotes;  //!< account delegates votes sorted by vote amount
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
    bool ProcessDelegateVote(vector<CCandidateVote>& candidateVotesIn, const uint64_t curHeight);
    bool StakeVoteBcoins(VoteType type, const uint64_t votes);
    bool StakeFcoins(const int64_t fcoinsToStake); //price feeder must stake fcoins
    bool OperateFcoinStaking(const int64_t fcoinsToStake) { return false; } // TODO: ...

    bool StakeBcoinsToCdp(CoinType coinType, const int64_t bcoinsToStake, const int64_t mintedScoins);
    bool RedeemScoinsToCdp(const int64_t bcoinsToStake);
    bool LiquidateCdp(const int64_t bcoinsToStake);

public:
    CAccount(CKeyID& keyId, CNickID& nickId, CPubKey& pubKey)
        : keyID(keyId),
          nickID(nickId),
          pubKey(pubKey),
          bcoins(0),
          scoins(0),
          fcoins(0),
          stakedBcoins(0),
          stakedFcoins(0),
          receivedVotes(0),
          lastVoteHeight(0),
          hasOpenCdp(false) {
        minerPubKey = CPubKey();
        candidateVotes.clear();
        regID.Clean();
    }

    CAccount()
        : keyID(uint160()),
          bcoins(0),
          scoins(0),
          fcoins(0),
          stakedBcoins(0),
          stakedFcoins(0),
          receivedVotes(0),
          lastVoteHeight(0),
          hasOpenCdp(false) {
        pubKey      = CPubKey();
        minerPubKey = CPubKey();
        candidateVotes.clear();
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
        this->stakedBcoins   = other.stakedBcoins;
        this->stakedFcoins   = other.stakedFcoins;
        this->receivedVotes  = other.receivedVotes;
        this->lastVoteHeight = other.lastVoteHeight;
        this->candidateVotes = other.candidateVotes;
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
        this->stakedBcoins   = other.stakedBcoins;
        this->stakedFcoins   = other.stakedFcoins;
        this->receivedVotes  = other.receivedVotes;
        this->lastVoteHeight = other.lastVoteHeight;
        this->candidateVotes = other.candidateVotes;
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

    uint64_t GetTotalBcoins(const uint64_t curHeight);
    uint64_t GetVotedBCoins(const uint64_t curHeight);

    // Get profits for voting.
    uint64_t GetAccountProfit(const uint64_t curHeight);
    // Calculate profits for voted.
    uint64_t CalculateAccountProfit(const uint64_t curHeight) const;

    string ToString(bool isAddress = false) const;
    Object ToJsonObj(bool isAddress = false) const;
    bool IsEmptyValue() const { return !(bcoins > 0); }

    uint256 GetHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << keyID << regID << nickID << pubKey << minerPubKey << VARINT(bcoins) << VARINT(scoins)
               << VARINT(fcoins) << VARINT(stakedFcoins) << VARINT(receivedVotes) << VARINT(lastVoteHeight)
               << candidateVotes << hasOpenCdp;

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
        READWRITE(VARINT(stakedBcoins));
        READWRITE(VARINT(stakedFcoins));
        READWRITE(VARINT(receivedVotes));
        READWRITE(VARINT(lastVoteHeight));
        READWRITE(candidateVotes);
        READWRITE(hasOpenCdp);)

private:
    bool IsMoneyOverflow(uint64_t nAddMoney);
};

class CAccountLog {
public:
    CKeyID keyID;         //!< keyID of the account (interchangeable to address)
    CRegID regID;         //!< regID of the account
    CNickID nickID;       //!< Nickname ID of the account (maxlen=32)
    CPubKey pubKey;       //!< account public key
    CPubKey minerPubKey;  //!< miner saving account public key

    uint64_t bcoins;                        //!< baseCoin balance
    uint64_t scoins;                        //!< stableCoin balance
    uint64_t fcoins;                        //!< fundCoin balance
    uint64_t stakedBcoins;                  //!< Staked/Collateralized BaseCoins
    uint64_t stakedFcoins;                  //!< Staked FundCoins for pricefeed right

    uint64_t receivedVotes;                 //!< votes received
    uint64_t lastVoteHeight;                //!< account's last vote block height used for computing interest

    vector<CCandidateVote> candidateVotes;  //!< casted delegate votes
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
        READWRITE(VARINT(stakedBcoins));
        READWRITE(VARINT(stakedFcoins));
        READWRITE(VARINT(receivedVotes));
        READWRITE(VARINT(lastVoteHeight));
        READWRITE(candidateVotes);
        READWRITE(hasOpenCdp);)

public:
    CAccountLog(const CAccount& acct) {
        keyID          = acct.keyID;
        regID          = acct.regID;
        nickID         = acct.nickID;
        pubKey         = acct.pubKey;
        minerPubKey    = acct.minerPubKey;
        bcoins         = acct.bcoins;
        scoins         = acct.scoins;
        fcoins         = acct.fcoins;
        stakedBcoins   = acct.stakedBcoins;
        stakedFcoins   = acct.stakedFcoins;
        receivedVotes  = acct.receivedVotes;
        lastVoteHeight = acct.lastVoteHeight;
        candidateVotes = acct.candidateVotes;
        hasOpenCdp     = acct.hasOpenCdp;
    }

    CAccountLog(CKeyID& keyId) {
        keyID = keyId;
        regID.Clean();
        nickID.Clean();
        bcoins         = 0;
        scoins         = 0;
        fcoins         = 0;
        stakedBcoins   = 0;
        stakedFcoins   = 0;
        receivedVotes  = 0;
        lastVoteHeight = 0;
        candidateVotes.clear();
        hasOpenCdp = false;
    }

    CAccountLog() {
        keyID = uint160();
        regID.Clean();
        nickID.Clean();
        bcoins         = 0;
        scoins         = 0;
        fcoins         = 0;
        stakedBcoins   = 0;
        stakedFcoins   = 0;
        receivedVotes  = 0;
        lastVoteHeight = 0;
        candidateVotes.clear();
        hasOpenCdp = false;
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
        stakedBcoins   = acct.stakedBcoins;
        stakedFcoins   = acct.stakedFcoins;
        receivedVotes  = acct.receivedVotes;
        lastVoteHeight = acct.lastVoteHeight;
        candidateVotes = acct.candidateVotes;
        hasOpenCdp     = acct.hasOpenCdp;
    }

    string ToString() const;
};


enum ACCOUNT_TYPE {
    // account type
    regid      = 0x01,  //!< Registration accountId
    base58addr = 0x02,  //!< pulickey
};
/**
 * account operate produced in contract
 * TODO: rename CVmOperate
 */
class CVmOperate{
public:
	unsigned char accountType;      //regid or base58addr
	unsigned char accountId[34];	//!< accountId: address
	unsigned char opType;		    //!OperType
	unsigned int  timeoutHeight;    //!< the transacion Timeout height
	unsigned char money[8];			//!<The transfer amount

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