/*
 * configuration.h
 *
 *  Created on: 2016年9月8日
 *      Author: WaykiChain Core Developer
 */

#include "configuration.h"

#include <memory>
#include "commons/uint256.h"
#include "commons/arith_uint256.h"
#include "util.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <vector>

using namespace std;

#include <stdint.h>
#include <boost/assign/list_of.hpp> // for 'map_list_of()'

#include "main.h"
#include "commons/uint256.h"
#include "syncdatadb.h"

namespace Checkpoints
{
    typedef map<int, uint256> MapCheckpoints; // nHeight -> blockHash;
    CCriticalSection cs_checkPoint;

    // How many times we expect transactions after the last checkpoint to
    // be slower. This number is a compromise, as it can't be accurate for
    // every system. When reindexing from a fast disk with a slow CPU, it
    // can be up to 20, while when downloading from a slow network with a
    // fast multicore CPU, it won't be much higher than 1.
    static const double SIGCHECK_VERIFICATION_FACTOR = 5.0;

    struct CCheckpointData {
        MapCheckpoints *mapCheckpoints;
        int64_t nTimeLastCheckpoint;
        int64_t nTransactionsLastCheckpoint;
        double fTransactionsPerDay;
    };

    bool fEnabled = true;

    // What makes a good checkpoint block?
    // + Is surrounded by blocks with reasonable timestamps
    //   (no blocks before with a timestamp after, none after with
    //    timestamp before)
    // + Contains no strange transactions
    static MapCheckpoints mapCheckpoints =
        boost::assign::map_list_of
        ( 0, uint256S("1f0d05a703a917511558f046529c48ad53b55c5b16c5d432fab8773a4b5ed4f1"));
    static const CCheckpointData data = {
        &mapCheckpoints,
        0,      // * UNIX timestamp of last checkpoint block
        0,      // * total number of transactions between genesis and last checkpoint
                //   (the tx=... number in the SetBestChain debug.log lines)
        0       // * estimated number of transactions per day after checkpoint
    };

    static MapCheckpoints mapCheckpointsTestnet =
        boost::assign::map_list_of
        ( 0, uint256S("c28af610f0fb593e6194cef9195f154327577fc20b50018ccc822a7940d2b92d"));

    static const CCheckpointData dataTestnet = {
        &mapCheckpointsTestnet,
        0,
        0,
        0
    };

    static MapCheckpoints mapCheckpointsRegtest =
        boost::assign::map_list_of
        ( 0, uint256S("708d5c14424395963cd11bb3f2ff791f584efbeb59fe5922f2131bfc879cd1f7"))
        ;
    static const CCheckpointData dataRegtest = {
        &mapCheckpointsRegtest,
        0,
        0,
        0
    };

    const CCheckpointData &Checkpoints() {
        if (SysCfg().NetworkID() == TEST_NET)
            return dataTestnet;
        else if (SysCfg().NetworkID() == MAIN_NET)
            return data;
        else
            return dataRegtest;
    }

    // nHeight找不到 或 高度和hash都能找到，则返回true
    bool CheckBlock(int nHeight, const uint256& hash)
    {
        if (!fEnabled)
            return true;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        MapCheckpoints::const_iterator i = checkpoints.find(nHeight);
        if (i == checkpoints.end()) return true;
        return hash == i->second;
    }

    // Guess how far we are in the verification process at the given block index
    double GuessVerificationProgress(CBlockIndex *pindex, bool fSigchecks)
    {
        if (pindex==NULL)
            return 0.0;

        int64_t nNow = time(NULL);

        double fSigcheckVerificationFactor = fSigchecks ? SIGCHECK_VERIFICATION_FACTOR : 1.0;
        double fWorkBefore = 0.0; // Amount of work done before pindex
        double fWorkAfter = 0.0;  // Amount of work left after pindex (estimated)
        // Work is defined as: 1.0 per transaction before the last checkpoint, and
        // fSigcheckVerificationFactor per transaction after.

        const CCheckpointData &data = Checkpoints();

        if (pindex->nChainTx <= data.nTransactionsLastCheckpoint) {
            double nCheapBefore = pindex->nChainTx;
            double nCheapAfter = data.nTransactionsLastCheckpoint - pindex->nChainTx;
            double nExpensiveAfter = (nNow - data.nTimeLastCheckpoint)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore;
            fWorkAfter = nCheapAfter + nExpensiveAfter*fSigcheckVerificationFactor;
        } else {
            double nCheapBefore = data.nTransactionsLastCheckpoint;
            double nExpensiveBefore = pindex->nChainTx - data.nTransactionsLastCheckpoint;
            double nExpensiveAfter = (nNow - pindex->nTime)/86400.0*data.fTransactionsPerDay;
            fWorkBefore = nCheapBefore + nExpensiveBefore*fSigcheckVerificationFactor;
            fWorkAfter = nExpensiveAfter*fSigcheckVerificationFactor;
        }

        return fWorkBefore / (fWorkBefore + fWorkAfter);
    }

    // 获取mapCheckpoints 中保存最后一个checkpoint 的高度
    int GetTotalBlocksEstimate()
    {
        if (!fEnabled) return 0;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;
        return checkpoints.rbegin()->first;
    }

    CBlockIndex* GetLastCheckpoint(const map<uint256, CBlockIndex*>& mapBlockIndex)
    {
        if (!fEnabled)
            return NULL;

        const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;

        BOOST_REVERSE_FOREACH(const MapCheckpoints::value_type& i, checkpoints)
        {
            const uint256& hash = i.second;
            map<uint256, CBlockIndex*>::const_iterator t = mapBlockIndex.find(hash);
            if (t != mapBlockIndex.end())
                return t->second;
        }
        return NULL;
    }

	bool LoadCheckpoint() {
		LOCK(cs_checkPoint);
		SyncData::CSyncDataDb db;
		return db.LoadCheckPoint(*Checkpoints().mapCheckpoints);
	}

	bool GetCheckpointByHeight(const int nHeight, std::vector<int> &vCheckpoints) {
		LOCK(cs_checkPoint);
		MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;
		std::map<int, uint256>::iterator iterMap = checkpoints.upper_bound(nHeight);
		while (iterMap != checkpoints.end()) {
			vCheckpoints.push_back(iterMap->first);
			++iterMap;
		}
		return !vCheckpoints.empty();
	}

	bool AddCheckpoint(int nHeight, uint256 hash) {
		LOCK(cs_checkPoint);
		MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;
		checkpoints.insert(checkpoints.end(), make_pair(nHeight, hash));
		return true;
	}

	void GetCheckpointMap(std::map<int, uint256> &mapCheckpoints) {
		LOCK(cs_checkPoint);
		const MapCheckpoints& checkpoints = *Checkpoints().mapCheckpoints;
		mapCheckpoints = checkpoints;
	}

}


//=========================================================================
//========以下是静态成员初始化的值=====================================================

const G_CONFIG_TABLE& IniCfg() {
    static G_CONFIG_TABLE* psCfg = NULL;
    if (psCfg == NULL) {
        psCfg = new G_CONFIG_TABLE();
    }
    assert(psCfg != NULL);
    return *psCfg;
}

const uint256 G_CONFIG_TABLE::GetIntHash(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return (uint256S((hashGenesisBlock_mainNet)));
        }
        case TEST_NET: {
            return (uint256S((hashGenesisBlock_testNet)));
        }
        case REGTEST_NET: {
            return (uint256S((hashGenesisBlock_regTest)));
        }
        default:
            assert(0);
    }
    return uint256S("");
}

const string G_CONFIG_TABLE::GetCheckPointPkey(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return CheckPointPK_MainNet;
        }
        case TEST_NET: {
            return CheckPointPK_TestNet;
        }
        default:
            assert(0);
    }
    return "";
}

const vector<string> G_CONFIG_TABLE::GetIntPubKey(NET_TYPE type) const {
	switch (type) {
	    case MAIN_NET: {
	        return (intPubKey_mainNet);
		}
		case TEST_NET: {
			return (initPubKey_testNet);
		}
		case REGTEST_NET: {
			return (initPubkey_regTest);
		}
		default:
			assert(0);
		}
		return vector<string>();
	}

const vector<string> G_CONFIG_TABLE::GetDelegatePubKey(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return (delegatePubKey_mainNet);
        }
        case TEST_NET: {
            return (delegatePubKey_testNet);
        }
        case REGTEST_NET: {
            return (delegatePubKey_regTest);
        }
        default:
            assert(0);
        }
    return vector<string>();
}

const uint256 G_CONFIG_TABLE::GetHashMerkleRoot() const{
    return (uint256S((HashMerkleRoot)));
}

vector<unsigned int> G_CONFIG_TABLE::GetSeedNodeIP() const
{
    return pnSeed;
}

unsigned char* G_CONFIG_TABLE::GetMagicNumber(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return Message_mainNet;
        }
        case TEST_NET: {
            return Message_testNet;
        }
        case REGTEST_NET: {
            return Message_regTest;
        }
        default:
            assert(0);
    }
    return NULL;
}

vector<unsigned char> G_CONFIG_TABLE::GetAddressPrefix(NET_TYPE type, Base58Type BaseType) const {
    switch (type) {
        case MAIN_NET: {
            return AddrPrefix_mainNet[BaseType];
        }
        case TEST_NET: {
            return AddrPrefix_testNet[BaseType];
        }
        // case REGTEST_NET: {
        //     return Message_regTest;
        // }
        default:
            assert(0);
    }
    return vector<unsigned char>();
}

unsigned int G_CONFIG_TABLE::GetnDefaultPort(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return nDefaultPort_mainNet;
        }
        case TEST_NET: {
            return nDefaultPort_testNet;
        }
        case REGTEST_NET: {
            return nDefaultPort_regTest;
        }
        default:
            assert(0);
    }
    return 0;
}

unsigned int G_CONFIG_TABLE::GetnRPCPort(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return nRPCPort_mainNet;
        }
        case TEST_NET: {
            return nRPCPort_testNet;
        }
        // case REGTEST_NET: {
        //     return Message_regTest;
        // }
        default:
            assert(0);
    }
    return 0;
}

unsigned int G_CONFIG_TABLE::GetnUIPort(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return nUIPort_mainNet;
        }
        case TEST_NET: {
            return nUIPort_testNet;
        }
        case REGTEST_NET: {
            return nUIPort_testNet;
        }
        default:
            assert(0);
    }
    return 0;
}

unsigned int G_CONFIG_TABLE::GetStartTimeInit(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return StartTime_mainNet;
        }
        case TEST_NET: {
            return StartTime_testNet;
        }
        case REGTEST_NET: {
            return StartTime_regTest;
        }
        default:
            assert(0);
    }
    return 0;
}

unsigned int G_CONFIG_TABLE::GetHalvingInterval(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return nSubsidyHalvingInterval_mainNet;
        }
        case TEST_NET: {
            return nSubsidyHalvingInterval_testNet;
        }
        case REGTEST_NET: {
            return nSubsidyHalvingInterval_regNet;
        }
        default:
            assert(0);
    }
    return 0;
}

uint64_t G_CONFIG_TABLE::GetCoinInitValue() const {
    return InitialCoin;
}

uint64_t G_CONFIG_TABLE::GetBlockSubsidyCfg(int nHeight) const {
    uint64_t nSubsidy = nInitialSubsidy;
    int nHalvings     = nHeight / SysCfg().GetSubsidyHalvingInterval();
    // Force block reward to a fixed value when right shift is more than 3.
    if (nHalvings > 4) {
        return nFixedSubsidy;
    } else {
        // Subsidy is cut by 1% every 3,153,600 blocks which will occur approximately every 1 year and the profit will be 1 percent
        nSubsidy -= nHalvings;
    }
    return nSubsidy;
}

int G_CONFIG_TABLE::GetBlockSubsidyJumpHeight(uint64_t nSubsidyValue) const {
    assert(nSubsidyValue >= nFixedSubsidy && nSubsidyValue <= nInitialSubsidy);
    uint64_t nSubsidy = nInitialSubsidy;
    int nHalvings = 0;
    map<uint64_t/*subsidy*/, int/*height*/> mSubsidyHeight;
    while (nSubsidy >= nFixedSubsidy) {
        mSubsidyHeight[nSubsidy] = nHalvings * SysCfg().GetSubsidyHalvingInterval();
        nHalvings += 1;
        nSubsidy -= 1;
    }

    return mSubsidyHeight[nSubsidyValue];
}

uint64_t G_CONFIG_TABLE::GetDelegatesNum() const {
    return nDelegates;
}

string G_CONFIG_TABLE::GetDelegateSignature(NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: {
            return delegateSignature_mainNet;
        }
        case TEST_NET: {
            return delegateSignature_testNet;
        }
        case REGTEST_NET: {
            return delegateSignature_regNet;
        }
        default:
            assert(0);
    }
    return 0;
}

//=========================================================================
//========以下是静态成员初始化的值=====================================================
//=========================================================================

//BaseCoin name
string G_CONFIG_TABLE::COIN_NAME = "WaykiChain";


/** Public Key for mainnet */
vector<string> G_CONFIG_TABLE::intPubKey_mainNet =
{
	"037671de4799dbf919effa034bbcaadd78c8a942adeebe7d71155304979a02802a",
	"0226d8c242052560b3ec7c75d45ba3a8cb187ff2c21a9e96cb8755eeefd50bcdca"
};
/** Public Key for testnet */
 vector<string> G_CONFIG_TABLE::initPubKey_testNet =
{
	"037de11ea5def6393f45c2461c6f55e6e5cda831545324c63fc5c04409d459a5b3",
	"025fa44ce081c3b4f34982a86e85e474fca1d98bbb6da612e097c9e7041208f11a"
};
/** Public Key for RegTestNet */
 vector<string> G_CONFIG_TABLE::initPubkey_regTest =
{
	"03b2299425981d6c2ec382cda999e604eb06b2b0f387f4b8500519c44d143cd2a8",
	"036c5397f3227a1e209952829d249b7ad0f615e43b763ac15e3a6f52627a10df21"
};


//初始记账人公钥-主网络
vector<string> G_CONFIG_TABLE::delegatePubKey_mainNet =
{
    "02d5b0c28802250ff0e48ba1961b79337b1ed4c2a7e695f5b0b41c6777b1bd2bcf",
    "02e829790a0bcfc5b62547a38ef2880dd653df61d52d1523d22e0d5431fc0bae3b",
    "03145134d18bbcb1da64adb201d1234f57b3daee8bb7d0bcbe7c27b53edadcab59",
    "033425e0f00180897f7e7a1756f74257d3214b36f960cb087861068735c485d403",
    "021a8f295c89311c2b9f949cca49d12d83321e373f74c8d7d8bf011f5e2adc1668",
    "026e75eecd92704426391312c52748c4b643d5ac34b49e9381d4878a0217f6d18a",
    "02bf65df9f193ac3e9f2353ea03b51b499156c228d4d285bb7e91bca28e7108554",
    "0295b340f643042e6d41cf248e6bc3135f83a69426a094d80bc7c9a96178d08550",
    "036ca3212b053f168c98e96456b1bce42ade96af674a3e3faa4e8cfd18c422a65a",
    "024000ff8aee862f3a5768065c06f3d0d87229099cde8abdb89c391fcf0bae8f83",
    "0260b67e343d21a039d2f46eb1c51e6ad46bd83c58af962ab06af06e5487e4d005"
};
//初始记账人公钥-测试网络
vector<string> G_CONFIG_TABLE::delegatePubKey_testNet =
{
    "0389e1bdfeab629107631fdc27f75c2d0bd47d2a09930d9c95935fe7b15a14506c",
    "0393f920474be2babf0a4679e3e1341c4eb8e31b22e19fc341ef5c0a74102b1b62",
    "03c2511372f6d68b1b7f46e2d5426efdb1c32eb7826f23f012acfee6176e072f0d",
    "021de59471052acba560185e9e8f0d8029fe0214180afe8a750204c44e5c385da1",
    "0267a77cfab55a3cedf0576393b90d307da1c6745970f286d40c356853443df9e6",
    "035eaee0cce88f4d3b8af6e71797f36750608b9425607e02ce03e8f08cad5b19ae",
    "036c670e382df168387083152e257f528bb7a7136900ea684550843a5347d89a04",
    "02f06edeb3d0a0cd01c44999ccbdc2126f65c1e0dcb09742e03336cfae2175d8bd",
    "0236f8aae8a5e4d4daab49e0b48723258a74dade6380c104a7759ec5d4a45aa186",
    "022a55aac2432590f1111f151cbb27c7a4417d0d85e5e4c24805943b90842b8710",
    "02e45a86ca60b7d0a53e9612228d5d9bee83056b6b57c1d58f7216a5060e6a3752"
};
//初始记账人公钥-局域网络
vector<string> G_CONFIG_TABLE::delegatePubKey_regTest =
{
 "0376de6a21f63c35a053c849a339598016a0261d6bdc5567adeda0af78b750c4cc",
 "025a37cb6ec9f63bb17e562865e006f0bafa9afbd8a846bd87fc8ff9e35db1252e",
 "03f52925f191c77bb1d16b19387bcfcb83380f1622d643a11038cf4867c4578696",
 "0378f1d7ce11bace8bbf28e124cd15f1bc82d7e8a25f62713f812201e1cf8060d8",
 "03fb1fe453bf830843fd90f0d2ae1b67011c168a0a5f2160e41f1d86a64c86c25c",
 "0282464694b94780b88c5e88ff64a7e24800590d4061b90f334067d15643604057",
 "02361884b20bdb2f751c56783a5fdc01ec64e25d77b7bf5a30409aef4b5b3b44ce",
 "029699e9b4d679d04d8961bf64ffc0b5ec6f9d46e88bbdcbc6d0a02ce1e4991c0c",
 "024af74c1cc6b1d729b038427ce9011627f6b816e77699421a85265c4ae0b74b5b",
 "03e44dcf2df8ec33a17c63a894f3697ed863b2d1fcc7b5fd37cc4fc2edf8e7ed71",
 "03ff9fb0c58b6097bc944592faee68fbdb2d1c5cd901f6eae9198bd8b31a1e6f5e"
};

//创世纪块中投票交易签名
string G_CONFIG_TABLE::delegateSignature_mainNet = "025e1310343d57f20740eeb32820a105a9372fb489028fea5471fa512168e75ce1";
string G_CONFIG_TABLE::delegateSignature_testNet = "02fc0033e19b9999997331c98652607299b0aaf20ed2dd6f0975d03cff3aecdeec";
string G_CONFIG_TABLE::delegateSignature_regNet = "025e1310343d57f20740eeb32820a105a9372fb489028fea5471fa512168e75ce1";

//Pubkey
string G_CONFIG_TABLE::CheckPointPK_MainNet = "029e85b9822bb140d6934fe7e8cd82fb7fde49da8c96141d69884c7e53a57628cb";
string G_CONFIG_TABLE::CheckPointPK_TestNet = "0264afea20ebe6fe4c753f9c99bdce8293cf739efbc7543784873eb12f39469d46";

//Gensis Block Hash
string G_CONFIG_TABLE::hashGenesisBlock_mainNet = "0xa00d5d179450975237482f20f5cd688cac689eb83bc2151d561bfe720185dc13";
string G_CONFIG_TABLE::hashGenesisBlock_testNet = "0xf8aea423c73890eb982c77793cf2fff1dcc1c4d141f42a4c6841b1ffe87ac594";
string G_CONFIG_TABLE::hashGenesisBlock_regTest = "0xab8d8b1d11784098108df399b247a0b80049de26af1b9c775d550228351c768d";

//Merkle Hash Root
string G_CONFIG_TABLE::HashMerkleRoot = "0x16b211137976871bb062e211f08b2f70a60fa8651b609823f298d1a3d3f3e05d";

//IP Address
vector<unsigned int> G_CONFIG_TABLE::pnSeed =
    {0xF6CF612F, 0xA4D80E6A, 0x35DD70C1, 0xDC36FB0D, 0x91A11C77, 0xFFFFE60D, 0x3D304B2F, 0xB21A4E75, 0x0C2AFE2F, 0xC246FE2F, 0x0947FE2F};

//Network Magic No.
unsigned char G_CONFIG_TABLE::Message_mainNet[MESSAGE_START_SIZE] = {0xff, 0x42, 0x1d, 0x1a};
unsigned char G_CONFIG_TABLE::Message_testNet[MESSAGE_START_SIZE] = {0xfd, 0x7d, 0x5c, 0xd0};
unsigned char G_CONFIG_TABLE::Message_regTest[MESSAGE_START_SIZE] = {0xfe, 0xfa, 0xd3, 0xc6};

//Address Prefix
vector<unsigned char> G_CONFIG_TABLE::AddrPrefix_mainNet[MAX_BASE58_TYPES] =
    {{73}, {51}, {153}, {0x4c, 0x1d, 0x3d, 0x5f}, {0x4c, 0x23, 0x3f, 0x4b}, {0}};
vector<unsigned char> G_CONFIG_TABLE::AddrPrefix_testNet[MAX_BASE58_TYPES] =
    {{135}, {88}, {210}, {0x7d, 0x57, 0x3a, 0x2c}, {0x7d, 0x5c, 0x5A, 0x26}, {0}};

//Default P2P Port
unsigned int G_CONFIG_TABLE::nDefaultPort_mainNet = 8920;
unsigned int G_CONFIG_TABLE::nDefaultPort_testNet = 18920;
unsigned int G_CONFIG_TABLE::nDefaultPort_regTest = 18921;

// Default RPC Port
unsigned int G_CONFIG_TABLE::nRPCPort_mainNet = 18900;
unsigned int G_CONFIG_TABLE::nRPCPort_testNet = 18901;

// Default UI Port
unsigned int G_CONFIG_TABLE::nUIPort_mainNet = 4245;
unsigned int G_CONFIG_TABLE::nUIPort_testNet = 4246;

//Blockchain Start Time
unsigned int G_CONFIG_TABLE::StartTime_mainNet = 1525404897;
unsigned int G_CONFIG_TABLE::StartTime_testNet = 1505401100;
unsigned int G_CONFIG_TABLE::StartTime_regTest = 1504305600;

//半衰期 (half-life)
unsigned int G_CONFIG_TABLE::nSubsidyHalvingInterval_mainNet = 3153600; // one year = 365 * 24 * 60 * 60 / 10
unsigned int G_CONFIG_TABLE::nSubsidyHalvingInterval_testNet = 3153600; // ditto
unsigned int G_CONFIG_TABLE::nSubsidyHalvingInterval_regNet  = 500;

//修改发币初始值
uint64_t G_CONFIG_TABLE::InitialCoin = 210000000;

//矿工费用
uint64_t G_CONFIG_TABLE::DefaultFee = 15;

unsigned int G_CONFIG_TABLE::nDelegates = 11;

//投票初始分红率
uint64_t G_CONFIG_TABLE::nInitialSubsidy = 5;
//投票固定分红率
uint64_t G_CONFIG_TABLE::nFixedSubsidy = 1;
