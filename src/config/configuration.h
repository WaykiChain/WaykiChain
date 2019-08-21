// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIGURATION_H_
#define CONFIGURATION_H_

#include "chainparams.h"
#include "commons/arith_uint256.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include "version.h"

#include <map>
#include <memory>
#include <vector>

using namespace std;

class CBlockIndex;
class uint256;
class G_CONFIG_TABLE;

const G_CONFIG_TABLE& IniCfg();

class G_CONFIG_TABLE {
public:
    string GetCoinName() const { return COIN_NAME; }
    const string GetAlertPkey(const NET_TYPE type) const;

    const vector<string> GetInitPubKey(const NET_TYPE type) const;
    const uint256 GetGenesisBlockHash(const NET_TYPE type) const;
    string GetDelegateSignature(const NET_TYPE type) const;
    const vector<string> GetDelegatePubKey(const NET_TYPE type) const;
    const uint256 GetMerkleRootHash() const;

    const string GetDexMatchServicePubKey(const NET_TYPE type) const;
    const string GetInitFcoinOwnerPubKey(const NET_TYPE type) const;

    vector<uint32_t> GetSeedNodeIP() const;
    uint8_t* GetMagicNumber(const NET_TYPE type) const;
    vector<uint8_t> GetAddressPrefix(const NET_TYPE type, const Base58Type BaseType) const;
    uint32_t GetDefaultPort(const NET_TYPE type) const;
    uint32_t GetRPCPort(const NET_TYPE type) const;
    uint32_t GetStartTimeInit(const NET_TYPE type) const;
    uint32_t GetTotalDelegateNum() const;
    uint32_t GetMaxVoteCandidateNum() const;
    uint64_t GetCoinInitValue() const { return InitialCoin; };
	uint32_t GetFeatureForkHeight(const NET_TYPE type) const;
    uint32_t GetStableCoinGenesisHeight(const NET_TYPE type) const;
    const vector<string> GetStableCoinGenesisTxid(const NET_TYPE type) const;

private:
    static string COIN_NAME; /* basecoin name */

    /* initial public key */
    static vector<string> initPubKey_mainNet;
    static vector<string> initPubKey_testNet;
    static vector<string> initPubkey_regTest;

    /* delegate public key */
    static vector<string> delegatePubKey_mainNet;
    static vector<string> delegatePubKey_testNet;
    static vector<string> delegatePubKey_regTest;

    /* delegate signature */
    static string delegateSignature_mainNet;
    static string delegateSignature_testNet;
    static string delegateSignature_regNet;

    /* gensis block hash */
    static string genesisBlockHash_mainNet;
    static string genesisBlockHash_testNet;
    static string genesisBlockHash_regNet;

    /* alert public key */
    static string AlertPK_MainNet;
    static string AlertPK_TestNet;

    /* merkle root hash */
    static string MerkleRootHash;

    /* fund coin initial owner public key */
    static string initFcoinOwnerPubKey_mainNet;
    static string initFcoinOwnerPubKey_testNet;
    static string initFcoinOwnerPubkey_regNet;

    /* DEX order-matching service's public key */
    static string dexMatchPubKey_mainNet;
    static string dexMatchPubKey_testNet;
    static string dexMatchPubKey_regTest;

    /* txids in stable coin genesis */
    static vector<string> stableCoinGenesisTxid_mainNet;
    static vector<string> stableCoinGenesisTxid_testNet;
    static vector<string> stableCoinGenesisTxid_regNet;

    /* Peer IP seeds */
    static vector<uint32_t> pnSeed;

    /* Network Magic Number */
    static uint8_t Message_mainNet[MESSAGE_START_SIZE];
    static uint8_t Message_testNet[MESSAGE_START_SIZE];
    static uint8_t Message_regTest[MESSAGE_START_SIZE];

    /* Address Prefix */
    static vector<uint8_t> AddrPrefix_mainNet[MAX_BASE58_TYPES];
    static vector<uint8_t> AddrPrefix_testNet[MAX_BASE58_TYPES];

    /* P2P Port */
    static uint32_t nDefaultPort_mainNet;
    static uint32_t nDefaultPort_testNet;
    static uint32_t nDefaultPort_regTest;

    /* RPC Port */
    static uint32_t nRPCPort_mainNet;
    static uint32_t nRPCPort_testNet;

    /* UI Port */
    static uint32_t nUIPort_mainNet;
    static uint32_t nUIPort_testNet;

    /* Start Time */
    static uint32_t StartTime_mainNet;
    static uint32_t StartTime_testNet;
    static uint32_t StartTime_regTest;

    /* Initial Coin */
    static uint64_t InitialCoin;

    /* Default Miner Fee */
    static uint64_t DefaultFee;

    /* Total Delegate Number */
    static uint32_t TotalDelegateNum;

    /* Max Number of Delegate Candidate to Vote for by a single account */
    static uint32_t MaxVoteCandidateNum;

    /* Block height to enable feature fork version */
	static uint32_t nFeatureForkHeight_mainNet;
    static uint32_t nFeatureForkHeight_testNet;
    static uint32_t nFeatureForkHeight_regNet;

    /* Block height for stable coin genesis */
    static uint32_t nStableScoinGenesisHeight_mainNet;
    static uint32_t nStableScoinGenesisHeight_testNet;
    static uint32_t nStableScoinGenesisHeight_regNet;
};

inline FeatureForkVersionEnum GetFeatureForkVersion(const int32_t currBlockHeight) {
    if (currBlockHeight >= (int32_t)SysCfg().GetFeatureForkHeight())
        return MAJOR_VER_R2;
    else
        return MAJOR_VER_R1;
}

inline uint32_t GetBlockInterval(const int32_t currBlockHeight) {
    FeatureForkVersionEnum featureForkVersion = GetFeatureForkVersion(currBlockHeight);
    switch (featureForkVersion) {
        case MAJOR_VER_R1:
            return SysCfg().GetBlockIntervalPreStableCoinRelease();
        case MAJOR_VER_R2:
            return SysCfg().GetBlockIntervalStableCoinRelease();
        default:
            assert(false && "unknown feature fork version");
    }

    return 0;
}

inline uint32_t GetYearBlockCount(const int32_t currBlockHeight) {
    return 365 /* days/year */ * 24 /* hours/day */ * 60 * 60 / GetBlockInterval(currBlockHeight);
}

inline uint32_t GetDayBlockCount(const int32_t currBlockHeight) {
    return 24 /* hours/day */ * 60 * 60 / GetBlockInterval(currBlockHeight);
}

inline uint32_t GetSubsidyHalvingInterval(const int32_t currBlockHeight) {
    if (SysCfg().NetworkID() == REGTEST_NET) {
        return SysCfg().GetArg("-subsidyhalvinginterval", 500);
    }

    return GetYearBlockCount(currBlockHeight);
}

inline uint8_t GetSubsidyRate(const int32_t currBlockHeight) {
    uint32_t halvingTimes = currBlockHeight / GetSubsidyHalvingInterval(currBlockHeight);

    // Force block reward to a fixed value when right shift is more than 3.
    return halvingTimes > 4 ? FIXED_SUBSIDY_RATE : INITIAL_SUBSIDY_RATE - halvingTimes;
}

inline uint32_t GetJumpHeightBySubsidy(const int32_t currBlockHeight, const uint8_t targetSubsidyRate) {
    assert(targetSubsidyRate >= FIXED_SUBSIDY_RATE && targetSubsidyRate <= INITIAL_SUBSIDY_RATE);
    uint8_t subsidyRate                   = INITIAL_SUBSIDY_RATE;
    uint32_t halvingTimes                 = 0;
    const uint32_t subsidyHalvingInterval = GetSubsidyHalvingInterval(currBlockHeight);
    map<uint8_t, uint32_t> subsidyRate2BlockHeight;

    while (subsidyRate >= FIXED_SUBSIDY_RATE) {
        subsidyRate2BlockHeight[subsidyRate] = halvingTimes * subsidyHalvingInterval;
        halvingTimes += 1;
        subsidyRate -= 1;
    }

    return subsidyRate2BlockHeight.at(targetSubsidyRate);
}

static const int32_t INIT_BLOCK_VERSION = 1;

/* No amount larger than this (in sawi) is valid */
static const int64_t BASECOIN_MAX_MONEY   = IniCfg().GetCoinInitValue() * COIN;  // 210 million
static const int64_t FUNDCOIN_MAX_MONEY   = BASECOIN_MAX_MONEY / 10;             // 21 million
static const int64_t STABLECOIN_MAX_MONEY = BASECOIN_MAX_MONEY * 10;             // 2100 million

inline int64_t GetBaseCoinMaxMoney() { return BASECOIN_MAX_MONEY; }
inline bool CheckBaseCoinRange(int64_t nValue) { return (nValue >= 0 && nValue <= BASECOIN_MAX_MONEY); }
inline bool CheckFundCoinRange(int64_t nValue) { return (nValue >= 0 && nValue <= FUNDCOIN_MAX_MONEY); }
inline bool CheckStableCoinRange(int64_t nValue) { return (nValue >= 0 && nValue <= STABLECOIN_MAX_MONEY); }

#endif /* CONFIGURATION_H_ */
