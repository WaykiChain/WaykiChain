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

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
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
    const string GetAlertPkey(NET_TYPE type) const;

    const vector<string> GetInitPubKey(NET_TYPE type) const;
    const uint256 GetGenesisBlockHash(NET_TYPE type) const;
    string GetDelegateSignature(NET_TYPE type) const;
    const vector<string> GetDelegatePubKey(NET_TYPE type) const;
    const uint256 GetMerkleRootHash() const;

    const string GetDexMatchServicePubKey(NET_TYPE type) const;
    const string GetInitFcoinOwnerPubKey(NET_TYPE type) const;

    vector<unsigned int> GetSeedNodeIP() const;
    unsigned char* GetMagicNumber(NET_TYPE type) const;
    vector<unsigned char> GetAddressPrefix(NET_TYPE type, Base58Type BaseType) const;
    unsigned int GetDefaultPort(NET_TYPE type) const;
    unsigned int GetRPCPort(NET_TYPE type) const;
    unsigned int GetStartTimeInit(NET_TYPE type) const;
    unsigned int GetHalvingInterval(NET_TYPE type) const;
    uint64_t GetBlockSubsidyCfg(int nHeight) const;
    int GetBlockSubsidyJumpHeight(uint64_t nSubsidyValue) const;
    uint32_t GetTotalDelegateNum() const;
    uint32_t GetMaxVoteCandidateNum() const;
    uint64_t GetCoinInitValue() const { return InitialCoin; };
	uint32_t GetFeatureForkHeight(NET_TYPE) const;
    uint32_t GetStableCoinGenesisHeight(NET_TYPE) const;
    const vector<string> GetStableCoinGenesisTxid(NET_TYPE type) const;

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
    static vector<unsigned int> pnSeed;

    /* Network Magic Number */
    static unsigned char Message_mainNet[MESSAGE_START_SIZE];
    static unsigned char Message_testNet[MESSAGE_START_SIZE];
    static unsigned char Message_regTest[MESSAGE_START_SIZE];

    /* Address Prefix */
    static vector<unsigned char> AddrPrefix_mainNet[MAX_BASE58_TYPES];
    static vector<unsigned char> AddrPrefix_testNet[MAX_BASE58_TYPES];

    /* P2P Port */
    static unsigned int nDefaultPort_mainNet;
    static unsigned int nDefaultPort_testNet;
    static unsigned int nDefaultPort_regTest;

    /* RPC Port */
    static unsigned int nRPCPort_mainNet;
    static unsigned int nRPCPort_testNet;

    /* UI Port */
    static unsigned int nUIPort_mainNet;
    static unsigned int nUIPort_testNet;

    /* Start Time */
    static unsigned int StartTime_mainNet;
    static unsigned int StartTime_testNet;
    static unsigned int StartTime_regTest;

    /* Subsidy Halving Interval*/
    static unsigned int nSubsidyHalvingInterval_mainNet;
    static unsigned int nSubsidyHalvingInterval_testNet;
    static unsigned int nSubsidyHalvingInterval_regNet;

    /* Initial Coin */
    static uint64_t InitialCoin;

    /* Default Miner Fee */
    static uint64_t DefaultFee;

    /* Total Delegate Number */
    static uint32_t TotalDelegateNum;

    /* Max Number of Delegate Candidate to Vote for by a single account */
    static uint32_t MaxVoteCandidateNum;

    /* Initial subsidy rate upon vote casting */
    static uint64_t nInitialSubsidy;
    /* Eventual/lasting subsidy rate for vote casting */
    static uint64_t nFixedSubsidy;

    /* Block height to enable feature fork version */
	static uint32_t nFeatureForkHeight_mainNet;
    static uint32_t nFeatureForkHeight_testNet;
    static uint32_t nFeatureForkHeight_regNet;

    /* Block height for stable coin genesis */
    static uint32_t nStableScoinGenesisHeight_mainNet;
    static uint32_t nStableScoinGenesisHeight_testNet;
    static uint32_t nStableScoinGenesisHeight_regNet;
};

inline FeatureForkVersionEnum GetFeatureForkVersion(int32_t blockHeight) {
	if (blockHeight >= (int32_t)SysCfg().GetFeatureForkHeight())
		return MAJOR_VER_R2;
	else
		return MAJOR_VER_R1;
}

static const int g_BlockVersion = 1;

/* No amount larger than this (in sawi) is valid */
static const int64_t BASECOIN_MAX_MONEY = IniCfg().GetCoinInitValue() * COIN;	// 210 million
static const int64_t FUNDCOIN_MAX_MONEY = BASECOIN_MAX_MONEY / 10;				// 21 million
static const int64_t INIT_FUEL_RATES    = 100;  	// 100 unit / 100 step
static const int64_t MIN_FUEL_RATES     = 1;    	// 1 unit / 100 step

inline int64_t GetBaseCoinMaxMoney() { return BASECOIN_MAX_MONEY; }
inline bool CheckBaseCoinRange(int64_t nValue) { return (nValue >= 0 && nValue <= BASECOIN_MAX_MONEY); }
inline bool CheckFundCoinRange(int64_t nValue) { return (nValue >= 0 && nValue <= FUNDCOIN_MAX_MONEY); }

#endif /* CONFIGURATION_H_ */
