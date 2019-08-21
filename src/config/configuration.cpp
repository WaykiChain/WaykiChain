// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "configuration.h"

#include "commons/arith_uint256.h"
#include "commons/uint256.h"
#include "main.h"
#include "commons/util.h"

#include <stdint.h>
#include <boost/assign/list_of.hpp>  // for 'map_list_of()'
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <memory>
#include <vector>

using namespace std;

const G_CONFIG_TABLE& IniCfg() {
    static G_CONFIG_TABLE* psCfg = nullptr;
    if (psCfg == nullptr) {
        psCfg = new G_CONFIG_TABLE();
    }
    assert(psCfg != nullptr);

    return *psCfg;
}

const uint256 G_CONFIG_TABLE::GetGenesisBlockHash(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return uint256S(genesisBlockHash_mainNet);
        case TEST_NET: return uint256S(genesisBlockHash_testNet);
        case REGTEST_NET: return uint256S(genesisBlockHash_regNet);
        default: assert(0);
    }

    return uint256S("");
}

const string G_CONFIG_TABLE::GetAlertPkey(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return AlertPK_MainNet;
        case TEST_NET: return AlertPK_TestNet;
        default: assert(0);
    }

    return "";
}

const vector<string> G_CONFIG_TABLE::GetInitPubKey(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return initPubKey_mainNet;
        case TEST_NET: return initPubKey_testNet;
        case REGTEST_NET: return initPubkey_regTest;
        default: assert(0);
    }

    return vector<string>();
}

const vector<string> G_CONFIG_TABLE::GetDelegatePubKey(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return delegatePubKey_mainNet;
        case TEST_NET: return delegatePubKey_testNet;
        case REGTEST_NET: return delegatePubKey_regTest;
        default: assert(0);
    }

    return vector<string>();
}

const uint256 G_CONFIG_TABLE::GetMerkleRootHash() const { return (uint256S((MerkleRootHash))); }

string G_CONFIG_TABLE::GetDelegateSignature(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return delegateSignature_mainNet;
        case TEST_NET: return delegateSignature_testNet;
        case REGTEST_NET: return delegateSignature_regNet;
        default: assert(0);
    }

    return "";
}

const string G_CONFIG_TABLE::GetInitFcoinOwnerPubKey(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return initFcoinOwnerPubKey_mainNet;
        case TEST_NET: return initFcoinOwnerPubKey_testNet;
        case REGTEST_NET: return initFcoinOwnerPubkey_regNet;
        default: assert(0);
    }

    return "";
}

const string G_CONFIG_TABLE::GetDexMatchServicePubKey(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return dexMatchPubKey_mainNet;
        case TEST_NET: return dexMatchPubKey_testNet;
        case REGTEST_NET: return dexMatchPubKey_regTest;
        default: assert(0);
    }

    return "";
}

const vector<string> G_CONFIG_TABLE::GetStableCoinGenesisTxid(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return stableCoinGenesisTxid_mainNet;
        case TEST_NET: return stableCoinGenesisTxid_testNet;
        case REGTEST_NET: return stableCoinGenesisTxid_regNet;
        default: assert(0);
    }

    return vector<string>();
}

uint32_t G_CONFIG_TABLE::GetFeatureForkHeight(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return nFeatureForkHeight_mainNet;
        case TEST_NET: return nFeatureForkHeight_testNet;
        case REGTEST_NET: return nFeatureForkHeight_regNet;
        default: assert(0);
    }

    return 0;
}

uint32_t G_CONFIG_TABLE::GetStableCoinGenesisHeight(const NET_TYPE type) const {
   switch (type) {
        case MAIN_NET: return nStableScoinGenesisHeight_mainNet;
        case TEST_NET: return nStableScoinGenesisHeight_testNet;
        case REGTEST_NET: return nStableScoinGenesisHeight_regNet;
        default: assert(0);
    }

    return 0;
}

vector<uint32_t> G_CONFIG_TABLE::GetSeedNodeIP() const { return pnSeed; }

uint8_t* G_CONFIG_TABLE::GetMagicNumber(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return Message_mainNet;
        case TEST_NET: return Message_testNet;
        case REGTEST_NET: return Message_regTest;
        default: assert(0);
    }
    return NULL;
}

vector<uint8_t> G_CONFIG_TABLE::GetAddressPrefix(const NET_TYPE type, const Base58Type BaseType) const {
    switch (type) {
        case MAIN_NET: return AddrPrefix_mainNet[BaseType];
        case TEST_NET: return AddrPrefix_testNet[BaseType];
        default: assert(0);
    }
    return vector<uint8_t>();
}

uint32_t G_CONFIG_TABLE::GetDefaultPort(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return nDefaultPort_mainNet;
        case TEST_NET: return nDefaultPort_testNet;
        case REGTEST_NET: return nDefaultPort_regTest;
        default: assert(0);
    }

    return 0;
}

uint32_t G_CONFIG_TABLE::GetRPCPort(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return nRPCPort_mainNet;
        case TEST_NET: return nRPCPort_testNet;
        default: assert(0);
    }

    return 0;
}

uint32_t G_CONFIG_TABLE::GetStartTimeInit(const NET_TYPE type) const {
    switch (type) {
        case MAIN_NET: return StartTime_mainNet;
        case TEST_NET: return StartTime_testNet;
        case REGTEST_NET: return StartTime_regTest;
        default: assert(0);
    }

    return 0;
}

uint32_t G_CONFIG_TABLE::GetTotalDelegateNum() const { return TotalDelegateNum; }

uint32_t G_CONFIG_TABLE::GetMaxVoteCandidateNum() const { return MaxVoteCandidateNum; }

// BaseCoin name
string G_CONFIG_TABLE::COIN_NAME = "WaykiChain";

// Public Key for mainnet
vector<string> G_CONFIG_TABLE::initPubKey_mainNet = {
    "037671de4799dbf919effa034bbcaadd78c8a942adeebe7d71155304979a02802a",
    "0226d8c242052560b3ec7c75d45ba3a8cb187ff2c21a9e96cb8755eeefd50bcdca"};
// Public Key for testnet
vector<string> G_CONFIG_TABLE::initPubKey_testNet = {
    "037de11ea5def6393f45c2461c6f55e6e5cda831545324c63fc5c04409d459a5b3",
    "025fa44ce081c3b4f34982a86e85e474fca1d98bbb6da612e097c9e7041208f11a"};
// Public Key for regtest
vector<string> G_CONFIG_TABLE::initPubkey_regTest = {
    "03b2299425981d6c2ec382cda999e604eb06b2b0f387f4b8500519c44d143cd2a8",
    "036c5397f3227a1e209952829d249b7ad0f615e43b763ac15e3a6f52627a10df21"};

// Initial delegates' public keys for mainnet
vector<string> G_CONFIG_TABLE::delegatePubKey_mainNet = {
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
    "0260b67e343d21a039d2f46eb1c51e6ad46bd83c58af962ab06af06e5487e4d005"};
// Initial delegates' public keys for testnet
vector<string> G_CONFIG_TABLE::delegatePubKey_testNet = {
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
    "02e45a86ca60b7d0a53e9612228d5d9bee83056b6b57c1d58f7216a5060e6a3752"};
// Initial delegates' public keys for regtest
vector<string> G_CONFIG_TABLE::delegatePubKey_regTest = {
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
    "03ff9fb0c58b6097bc944592faee68fbdb2d1c5cd901f6eae9198bd8b31a1e6f5e"};

// Signature in genesis block
string G_CONFIG_TABLE::delegateSignature_mainNet = "025e1310343d57f20740eeb32820a105a9372fb489028fea5471fa512168e75ce1";
string G_CONFIG_TABLE::delegateSignature_testNet = "02fc0033e19b9999997331c98652607299b0aaf20ed2dd6f0975d03cff3aecdeec";
string G_CONFIG_TABLE::delegateSignature_regNet  = "025e1310343d57f20740eeb32820a105a9372fb489028fea5471fa512168e75ce1";

// Pubkey
string G_CONFIG_TABLE::AlertPK_MainNet = "029e85b9822bb140d6934fe7e8cd82fb7fde49da8c96141d69884c7e53a57628cb";
string G_CONFIG_TABLE::AlertPK_TestNet = "0264afea20ebe6fe4c753f9c99bdce8293cf739efbc7543784873eb12f39469d46";

// Gensis Block Hash
string G_CONFIG_TABLE::genesisBlockHash_mainNet = "0xa00d5d179450975237482f20f5cd688cac689eb83bc2151d561bfe720185dc13";
string G_CONFIG_TABLE::genesisBlockHash_testNet = "0xf8aea423c73890eb982c77793cf2fff1dcc1c4d141f42a4c6841b1ffe87ac594";
string G_CONFIG_TABLE::genesisBlockHash_regNet  = "0xab8d8b1d11784098108df399b247a0b80049de26af1b9c775d550228351c768d";

// Merkle Root Hash
string G_CONFIG_TABLE::MerkleRootHash = "0x16b211137976871bb062e211f08b2f70a60fa8651b609823f298d1a3d3f3e05d";

// TODO: replace public key.
// Public key for initial fund coin owner
string G_CONFIG_TABLE::initFcoinOwnerPubKey_mainNet = "03307f4f5e59b89a8e0487ff01dd6c4e925a8c8bfc06091b2efb33f08c27e236c5";
string G_CONFIG_TABLE::initFcoinOwnerPubKey_testNet = "03307f4f5e59b89a8e0487ff01dd6c4e925a8c8bfc06091b2efb33f08c27e236c5";
string G_CONFIG_TABLE::initFcoinOwnerPubkey_regNet  = "03307f4f5e59b89a8e0487ff01dd6c4e925a8c8bfc06091b2efb33f08c27e236c5";

// Public Key for DEX order-matching service
string G_CONFIG_TABLE::dexMatchPubKey_mainNet = "033f51c7ef38ee34d1fe436dbf6329821d1863f22cee69c281c58374dcb9c35569";
string G_CONFIG_TABLE::dexMatchPubKey_testNet = "033f51c7ef38ee34d1fe436dbf6329821d1863f22cee69c281c58374dcb9c35569";
string G_CONFIG_TABLE::dexMatchPubKey_regTest = "033f51c7ef38ee34d1fe436dbf6329821d1863f22cee69c281c58374dcb9c35569";

vector<string> G_CONFIG_TABLE::stableCoinGenesisTxid_mainNet = {
    "578cbf63fb95f9e8d00fb83d712f94e57c98f0da7972a0736a8962277cd40f47",
    "ecd82e2ebd8415f23e9fb44342aaf99a781304314ecc2b1cd237d48b3ae0a1ff",
    "88a9a2db20569d2253f6c079346288b6efd87714332780b6de491b9eeacaf0aa"};

vector<string> G_CONFIG_TABLE::stableCoinGenesisTxid_testNet = {
    "578cbf63fb95f9e8d00fb83d712f94e57c98f0da7972a0736a8962277cd40f47",
    "ecd82e2ebd8415f23e9fb44342aaf99a781304314ecc2b1cd237d48b3ae0a1ff",
    "88a9a2db20569d2253f6c079346288b6efd87714332780b6de491b9eeacaf0aa"};

vector<string> G_CONFIG_TABLE::stableCoinGenesisTxid_regNet = {
    "578cbf63fb95f9e8d00fb83d712f94e57c98f0da7972a0736a8962277cd40f47",
    "ecd82e2ebd8415f23e9fb44342aaf99a781304314ecc2b1cd237d48b3ae0a1ff",
    "88a9a2db20569d2253f6c079346288b6efd87714332780b6de491b9eeacaf0aa"};

// IP Address
vector<uint32_t> G_CONFIG_TABLE::pnSeed = {0xF6CF612F, 0xA4D80E6A, 0x35DD70C1, 0xDC36FB0D, 0x91A11C77, 0xFFFFE60D,
                                               0x3D304B2F, 0xB21A4E75, 0x0C2AFE2F, 0xC246FE2F, 0x0947FE2F};

// Network Magic No.
uint8_t G_CONFIG_TABLE::Message_mainNet[MESSAGE_START_SIZE] = {0xff, 0x42, 0x1d, 0x1a};
uint8_t G_CONFIG_TABLE::Message_testNet[MESSAGE_START_SIZE] = {0xfd, 0x7d, 0x5c, 0xd0};
uint8_t G_CONFIG_TABLE::Message_regTest[MESSAGE_START_SIZE] = {0xfe, 0xfa, 0xd3, 0xc6};

// Address Prefix
vector<uint8_t> G_CONFIG_TABLE::AddrPrefix_mainNet[MAX_BASE58_TYPES] = {
    {73}, {51}, {153}, {0x4c, 0x1d, 0x3d, 0x5f}, {0x4c, 0x23, 0x3f, 0x4b}, {0}};
vector<uint8_t> G_CONFIG_TABLE::AddrPrefix_testNet[MAX_BASE58_TYPES] = {
    {135}, {88}, {210}, {0x7d, 0x57, 0x3a, 0x2c}, {0x7d, 0x5c, 0x5A, 0x26}, {0}};

// Default P2P Port
uint32_t G_CONFIG_TABLE::nDefaultPort_mainNet = 8920;
uint32_t G_CONFIG_TABLE::nDefaultPort_testNet = 18920;
uint32_t G_CONFIG_TABLE::nDefaultPort_regTest = 18921;

// Default RPC Port
uint32_t G_CONFIG_TABLE::nRPCPort_mainNet = 18900;
uint32_t G_CONFIG_TABLE::nRPCPort_testNet = 18901;

// Default UI Port
uint32_t G_CONFIG_TABLE::nUIPort_mainNet = 4245;
uint32_t G_CONFIG_TABLE::nUIPort_testNet = 4246;

// Blockchain Start Time
uint32_t G_CONFIG_TABLE::StartTime_mainNet = 1525404897;
uint32_t G_CONFIG_TABLE::StartTime_testNet = 1505401100;
uint32_t G_CONFIG_TABLE::StartTime_regTest = 1504305600;

// Initial Coin
uint64_t G_CONFIG_TABLE::InitialCoin = INITIAL_BASE_COIN_AMOUNT;  // 210 million

// Default Miner Fee
uint64_t G_CONFIG_TABLE::DefaultFee = 15;

// Total Delegate Number
uint32_t G_CONFIG_TABLE::TotalDelegateNum = 11;
// Max Number of Delegate Candidate to Vote for by a single account
uint32_t G_CONFIG_TABLE::MaxVoteCandidateNum = 22;

// Block height to enable feature fork version
uint32_t G_CONFIG_TABLE::nFeatureForkHeight_mainNet = 6000000;
uint32_t G_CONFIG_TABLE::nFeatureForkHeight_testNet = 1000000;
uint32_t G_CONFIG_TABLE::nFeatureForkHeight_regNet  = 10;

// Block height for stable coin genesis
uint32_t G_CONFIG_TABLE::nStableScoinGenesisHeight_mainNet = 5880000;
uint32_t G_CONFIG_TABLE::nStableScoinGenesisHeight_testNet = 588000;
uint32_t G_CONFIG_TABLE::nStableScoinGenesisHeight_regNet  = 8;
