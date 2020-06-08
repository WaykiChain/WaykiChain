// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "configuration.h"

#include "commons/arith_uint256.h"
#include "commons/uint256.h"
#include "main.h"
#include "commons/util/util.h"

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

uint8_t G_CONFIG_TABLE::GetGenesisBlockNonce(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return GenesisBlockNonce[type];
}

uint256 G_CONFIG_TABLE::GetGenesisBlockHash(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return uint256S(genesisBlockHash[type]);
}

const string G_CONFIG_TABLE::GetAlertPkey(const NET_TYPE type) const {
    assert(type >= 0 && type < 2);
    return AlertPubKey[type];
}

const vector<string> G_CONFIG_TABLE::GetInitPubKey(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return initPubKey[type];
}

const vector<string> G_CONFIG_TABLE::GetDelegatePubKey(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return delegatePubKey[type];
}

string G_CONFIG_TABLE::GetDelegateSignature(const NET_TYPE type) const {
    assert(type >=0 && type < 3);
    return delegateSignature[type];
}

const string G_CONFIG_TABLE::GetInitFcoinOwnerPubKey(const NET_TYPE type) const {
    assert(type >=0 && type < 3);
    return initFcoinOwnerPubKey[type];
}

const string G_CONFIG_TABLE::GetDexMatchServicePubKey(const NET_TYPE type) const {
    return dexMatchPubKey[type];
}

const vector<string> G_CONFIG_TABLE::GetStableCoinGenesisTxid(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return stableCoinGenesisTxid[type];
}

uint32_t G_CONFIG_TABLE::GetVer2ForkHeight(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return nVer2ForkHeight[type];
}

uint32_t G_CONFIG_TABLE::GetVer2GenesisHeight(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return nStableScoinGenesisHeight[type];
}

uint32_t G_CONFIG_TABLE::GetVer3ForkHeight(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return nVer3ForkHeight[type];
}

vector<uint32_t> G_CONFIG_TABLE::GetSeedNodeIP() const { return pnSeed; }

uint8_t* G_CONFIG_TABLE::GetMagicNumber(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return MessageMagicNumber[type];
}

vector<uint8_t> G_CONFIG_TABLE::GetAddressPrefix(const NET_TYPE type, const Base58Type BaseType) const {
    assert(type >= 0 && type < 2);
    return AddrPrefix[type][BaseType];
}

uint32_t G_CONFIG_TABLE::GetDefaultPort(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return nP2PPort[type];
}

uint32_t G_CONFIG_TABLE::GetRPCPort(const NET_TYPE type) const {
    assert(type >=0 && type < 2);
    return nRPCPort[type];
}

uint32_t G_CONFIG_TABLE::GetStartTimeInit(const NET_TYPE type) const {
    assert(type >= 0 && type < 3);
    return StartTime[type];
}

uint8_t G_CONFIG_TABLE::GetTotalDelegateNum() const { return TotalDelegateNum; }

uint32_t G_CONFIG_TABLE::GetMaxVoteCandidateNum() const { return MaxVoteCandidateNum; }

// BaseCoin name
string G_CONFIG_TABLE::COIN_NAME = "WaykiChain";

vector<string> G_CONFIG_TABLE::initPubKey[3] = {
    { // Public Key for mainnet
        "037671de4799dbf919effa034bbcaadd78c8a942adeebe7d71155304979a02802a",
        "0226d8c242052560b3ec7c75d45ba3a8cb187ff2c21a9e96cb8755eeefd50bcdca"
    }, { // Public Key for testnet
        "037de11ea5def6393f45c2461c6f55e6e5cda831545324c63fc5c04409d459a5b3",
        "025fa44ce081c3b4f34982a86e85e474fca1d98bbb6da612e097c9e7041208f11a"
    }, { // Public Key for regtest
        "03b2299425981d6c2ec382cda999e604eb06b2b0f387f4b8500519c44d143cd2a8",
        "036c5397f3227a1e209952829d249b7ad0f615e43b763ac15e3a6f52627a10df21"
    }
};

// Initial batch of delegates' public keys
vector<string> G_CONFIG_TABLE::delegatePubKey[3] {
    {   //mainnet
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
    }, { //testnet
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
    }, { //regtest
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
    }
};

// Signature in genesis block
string G_CONFIG_TABLE::delegateSignature[3] = {
    "025e1310343d57f20740eeb32820a105a9372fb489028fea5471fa512168e75ce1",   //mainnet
    "02fc0033e19b9999997331c98652607299b0aaf20ed2dd6f0975d03cff3aecdeec",   //testnet
    "025e1310343d57f20740eeb32820a105a9372fb489028fea5471fa512168e75ce1"};  //regtest

// Pubkey
string G_CONFIG_TABLE::AlertPubKey[2] {
    "029e85b9822bb140d6934fe7e8cd82fb7fde49da8c96141d69884c7e53a57628cb",   //mainnet
    "0264afea20ebe6fe4c753f9c99bdce8293cf739efbc7543784873eb12f39469d46"};  //testnet

// Gensis Block Hash
string G_CONFIG_TABLE::genesisBlockHash[3] = {
    "0xa00d5d179450975237482f20f5cd688cac689eb83bc2151d561bfe720185dc13",   //mainnet
    "7d06f69186e0fe39b9c40417d448fd36b43f193a2cee1ccae7f99b181080ee40",     //testnet
    "0xab8d8b1d11784098108df399b247a0b80049de26af1b9c775d550228351c768d"};  //regtest

// Merkle Root Hash

// Public key for initial fund coin owner
string G_CONFIG_TABLE::initFcoinOwnerPubKey[3] = {
    "028593c9bf1fee77085f5164ba5a8c385e7c3710de2fd8fa1d00748a1469b2176f",   //mainnet
    "022b05bec85ac78054ba762ac8625e30c85f2a8f23fd62890236706d3ad29646d4",   //testnet
    "03307f4f5e59b89a8e0487ff01dd6c4e925a8c8bfc06091b2efb33f08c27e236c5"};  //regtest

// Public Key for DEX order-matching service
string G_CONFIG_TABLE::dexMatchPubKey[3] = {
    "03c89c66ee32e26ee2c1bf624dc01d6d3e8eb9a09d0a0c86383944871054c1fcc6",   //mainnet
    "021be050c7e67004dc494f52ca81ff7c100a7e8b527b1c5c18091c3ad7065c4d94",   //testnet
    "033f51c7ef38ee34d1fe436dbf6329821d1863f22cee69c281c58374dcb9c35569"};  //regtest

vector<string> G_CONFIG_TABLE::stableCoinGenesisTxid[3] = {
    {   //mainnet
        "cc587f5b0280863b09ecfc1e0c9e21615b1775de90aaac51ff02d5dc9a26bbda", //H-1: cdp global account
        "7bfcfa18384ed6af2e10b8abdd78977c232db970a9776e332f8cde21b30eb2ba", //H-2: wgrt genesis account
        "61c85164f428b3fbfd65e04a8b8528311808fee5d8e5ecc12a0c066ef2863812"  //H-3: dex order-matching account
    }, { //testnet
        "aafa4d424a032e6d87fd983e5f7ae27eb30d8ca0e9ff61c065125381ae3399cd",
        "5feb49f2f2af2858809dd235e7b4e3a633086f311ab1fca8a4a5180ab1a0fe10",
        "751cc6d03b7e17200056f788623d9ab349f297b008a02046f6993f135d38fd1d"
    }, { //regtest
        "bab7f09fae61d74275ac4434780e4698222c8229a7454fd475270e4b18a97777",
        "dfd6fe595c49653f7b8dd6513d7df77b8f7d8fa1e6b79d8cbff71cff650faea5",
        "88a9a2db20569d2253f6c079346288b6efd87714332780b6de491b9eeacaf0aa"
    }
};

// IP Address
vector<uint32_t> G_CONFIG_TABLE::pnSeed = {0xF6CF612F, 0xA4D80E6A, 0x35DD70C1, 0xDC36FB0D, 0x91A11C77, 0xFFFFE60D,
                                           0x3D304B2F, 0xB21A4E75, 0x0C2AFE2F, 0xC246FE2F, 0x0947FE2F};

//Genesis block nonce
uint8_t G_CONFIG_TABLE::GenesisBlockNonce[3] {108 /*mainnet*/, 100 /*testnet*/, 68 /*regtest*/};

// Network Magic No.
uint8_t G_CONFIG_TABLE::MessageMagicNumber[3][MESSAGE_START_SIZE] {
    {0xff, 0x42, 0x1d, 0x1a},  //mainnet
    {0xfd, 0x7d, 0x5c, 0xe1},  //testnet
    {0xfe, 0xfa, 0xd3, 0xc6}   //regtest
};

// Address Prefix
vector<uint8_t> G_CONFIG_TABLE::AddrPrefix[2][MAX_BASE58_TYPES] = {
    //pubkey,   script, seckey, ext_pub_key,                ext_sec_key
    { {73},     {51},   {153},  {0x4c, 0x1d, 0x3d, 0x5f},   {0x4c, 0x23, 0x3f, 0x4b}, {0} }, //mainnet
    { {135},    {88},   {210},  {0x7d, 0x57, 0x3a, 0x2c},   {0x7d, 0x5c, 0x5A, 0x26}, {0} } //testnet/regtest
};

// Default P2P Port
uint32_t G_CONFIG_TABLE::nP2PPort[3]    = {8920 /*main*/, 18920 /*test*/, 18921 /*regtest*/ };

// Default RPC Port
uint32_t G_CONFIG_TABLE::nRPCPort[2]    = { 18900 /*main*/, 18901 /*test*/};

// Blockchain Start Time
uint32_t G_CONFIG_TABLE::StartTime[3]   = { 1525404897 /*main*/, 1505401100 /*test*/, 1504305600 /*regtest*/};

// Initial Coin
uint64_t G_CONFIG_TABLE::InitialCoin    = INITIAL_BASE_COIN_AMOUNT;  // 210 million

// Total Delegate Number
uint8_t G_CONFIG_TABLE::TotalDelegateNum = 11;
// Max Number of Delegate Candidate to Vote for by a single account
uint32_t G_CONFIG_TABLE::MaxVoteCandidateNum = 22;

// Block height for stable coin genesis
uint32_t G_CONFIG_TABLE::nStableScoinGenesisHeight[3] {
    4109388,    // mainnet
    500,        // testnet
    8};         // regtest

// Block height to enable feature fork version
uint32_t G_CONFIG_TABLE::nVer2ForkHeight[3] {
    4109588,    // mainnet: Wed Oct 16 2019 10:16:00 GMT+0800
    520,        // testnet
    10};        // regtest

// Block height to enable feature fork version
uint32_t G_CONFIG_TABLE::nVer3ForkHeight[3] {
    10898479,   // mainnet, estimate block time: 2020-06-18 10:00:00
    5756220,    // testnet, estimate block time: 2020-06-04 10:00:00
    500};       // regtest
