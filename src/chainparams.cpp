// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "configuration.h"
#include "chainparams.h"

#include "assert.h"
#include "core.h"
#include "protocol.h"
#include "util.h"
#include "accounts/key.h"
#include "tx/tx.h"
#include "main.h"

#include <boost/assign/list_of.hpp>
#include <boost/algorithm/string/case_conv.hpp> // for to_lower()
#include <boost/algorithm/string/predicate.hpp> // for startswith() and endswith()
#include <boost/filesystem.hpp>
using namespace boost::assign;
using namespace std;

map<string, string> CBaseParams::m_mapArgs;
map<string, vector<string> > CBaseParams::m_mapMultiArgs;

class CMainParams: public CBaseParams {
public:
    CMainParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        memcpy(pchMessageStart, IniCfg().GetMagicNumber(MAIN_NET), sizeof(pchMessageStart));
        vAlertPubKey =  ParseHex(IniCfg().GetCheckPointPkey(MAIN_NET));
        nDefaultPort = IniCfg().GetnDefaultPort(MAIN_NET) ;
        nRPCPort = IniCfg().GetnRPCPort(MAIN_NET);
        nUIPort = IniCfg().GetnUIPort(MAIN_NET);
        strDataDir = "main";
        bnProofOfStakeLimit =~arith_uint256(0) >> 10;        //00 3f ff ff
        nSubsidyHalvingInterval = IniCfg().GetHalvingInterval(MAIN_NET);
        assert(CreateGenesisRewardTx(genesis.vptx, MAIN_NET));
        assert(CreateGenesisDelegateTx(genesis.vptx, MAIN_NET));
        genesis.SetHashPrevBlock(uint256());
        genesis.SetHashMerkleRoot(genesis.BuildMerkleTree());

        genesis.SetVersion(g_BlockVersion);
        genesis.SetTime(IniCfg().GetStartTimeInit(MAIN_NET));
        genesis.SetNonce(108);
        genesis.SetFuelRate(INIT_FUEL_RATES);
        genesis.SetHeight(0);
        genesis.ClearSignature();
        hashGenesisBlock = genesis.GetHash();
        CheckPointPKey=IniCfg().GetCheckPointPkey(MAIN_NET);
        assert(hashGenesisBlock == IniCfg().GetIntHash(MAIN_NET));
        assert(genesis.GetHashMerkleRoot() == IniCfg().GetHashMerkleRoot());

        vSeeds.push_back(CDNSSeedData("seed1.waykichain.net", "n1.waykichain.net"));
        vSeeds.push_back(CDNSSeedData("seed2.waykichain.net", "n2.waykichain.net"));

        base58Prefixes[PUBKEY_ADDRESS]  = IniCfg().GetAddressPrefix(MAIN_NET,PUBKEY_ADDRESS);
        base58Prefixes[SCRIPT_ADDRESS]  = IniCfg().GetAddressPrefix(MAIN_NET,SCRIPT_ADDRESS);
        base58Prefixes[SECRET_KEY]      = IniCfg().GetAddressPrefix(MAIN_NET,SECRET_KEY);
        base58Prefixes[EXT_PUBLIC_KEY]  = IniCfg().GetAddressPrefix(MAIN_NET,EXT_PUBLIC_KEY);
        base58Prefixes[EXT_SECRET_KEY]  = IniCfg().GetAddressPrefix(MAIN_NET,EXT_SECRET_KEY);

        // Convert the pnSeeds array into usable address objects.
        for (unsigned int i = 0; i < IniCfg().GetSeedNodeIP().size(); i++) {
            // It'll only connect to one or two seed nodes because once it connects,
            // it'll get a pile of addresses with newer timestamps.
            // Seed nodes are given a random 'last seen time' of between one and two
            // weeks ago.
            const int64_t nOneWeek = 7 * 24 * 60 * 60;
            struct in_addr ip;
            memcpy(&ip, &IniCfg().GetSeedNodeIP()[i], sizeof(ip));
            CAddress addr(CService(ip, GetDefaultPort()));
            addr.nTime = GetTime() - GetRand(nOneWeek) - nOneWeek;
            vFixedSeeds.push_back(addr);
        }
    }

    virtual const CBlock& GenesisBlock() const
    {
        return genesis;
    }
    virtual NET_TYPE NetworkID() const
    {
        return MAIN_NET;
    }
    virtual bool InitialConfig()
    {
        return CBaseParams::InitialConfig();
    }
    virtual int GetBlockMaxNonce() const
    {
        return 1000;
    }
    virtual const vector<CAddress>& FixedSeeds() const {
        return vFixedSeeds;
    }
    virtual bool IsInFixedSeeds(CAddress &addr) {
        vector<CAddress>::iterator iterAddr = find(vFixedSeeds.begin(), vFixedSeeds.end(), addr);
        return iterAddr != vFixedSeeds.end();
    }

protected:
    CBlock genesis;
    vector<CAddress> vFixedSeeds;
};

class CTestNetParams: public CMainParams {
public:
    CTestNetParams() {
        // The message start string is designed to be unlikely to occur in normal data.
        // The characters are rarely used upper ASCII, not valid as UTF-8, and produce
        // a large 4-byte int at any alignment.
        memcpy(pchMessageStart,IniCfg().GetMagicNumber(TEST_NET),sizeof(pchMessageStart));
        nSubsidyHalvingInterval = IniCfg().GetHalvingInterval(TEST_NET);
        vAlertPubKey =  ParseHex(IniCfg().GetCheckPointPkey(TEST_NET));
        nDefaultPort = IniCfg().GetnDefaultPort(TEST_NET) ;
        nRPCPort = IniCfg().GetnRPCPort(TEST_NET);
        nUIPort = IniCfg().GetnUIPort(TEST_NET);
        strDataDir = "testnet";
        CheckPointPKey = IniCfg().GetCheckPointPkey(TEST_NET);
        // Modify the testnet genesis block so the timestamp is valid for a later start.
        genesis.SetTime(IniCfg().GetStartTimeInit(TEST_NET));
        genesis.SetNonce(99);
        genesis.vptx.clear();
        assert(CreateGenesisRewardTx(genesis.vptx, TEST_NET));
        assert(CreateGenesisDelegateTx(genesis.vptx, TEST_NET));
        genesis.SetHashMerkleRoot(genesis.BuildMerkleTree());
        hashGenesisBlock = genesis.GetHash();
        for(auto & item : vFixedSeeds)
            item.SetPort(GetDefaultPort());

        assert(hashGenesisBlock == IniCfg().GetIntHash(TEST_NET));
        vSeeds.push_back(CDNSSeedData("seed1.waykitest.net", "n1.waykitest.net"));
        vSeeds.push_back(CDNSSeedData("seed2.waykitest.net", "n2.waykitest.net"));

        base58Prefixes[PUBKEY_ADDRESS]  = IniCfg().GetAddressPrefix(TEST_NET,PUBKEY_ADDRESS);
        base58Prefixes[SCRIPT_ADDRESS]  = IniCfg().GetAddressPrefix(TEST_NET,SCRIPT_ADDRESS);
        base58Prefixes[SECRET_KEY]      = IniCfg().GetAddressPrefix(TEST_NET,SECRET_KEY);
        base58Prefixes[EXT_PUBLIC_KEY]  = IniCfg().GetAddressPrefix(TEST_NET,EXT_PUBLIC_KEY);
        base58Prefixes[EXT_SECRET_KEY]  = IniCfg().GetAddressPrefix(TEST_NET,EXT_SECRET_KEY);
    }

    virtual NET_TYPE NetworkID() const
    {
        return TEST_NET;
    }

    virtual bool InitialConfig()
    {
        CMainParams::InitialConfig();
        fServer = true;
        return true;
    }

    virtual int GetBlockMaxNonce() const
    {
        return 1000;
    }
};

//
// Regression test
//
class CRegTestParams: public CTestNetParams {
public:
    CRegTestParams() {
        memcpy(pchMessageStart, IniCfg().GetMagicNumber(REGTEST_NET),
            sizeof(pchMessageStart));
        nSubsidyHalvingInterval =  IniCfg().GetHalvingInterval(REGTEST_NET);
        bnProofOfStakeLimit = ~arith_uint256(0) >> 6;     //target:00000011 11111111 11111111
        genesis.SetTime(IniCfg().GetStartTimeInit(REGTEST_NET));
        genesis.SetNonce(68);
        genesis.vptx.clear();
        assert(CreateGenesisRewardTx(genesis.vptx, REGTEST_NET));
        assert(CreateGenesisDelegateTx(genesis.vptx, REGTEST_NET));
        genesis.SetHashMerkleRoot(genesis.BuildMerkleTree());
        hashGenesisBlock = genesis.GetHash();
        nDefaultPort = IniCfg().GetnDefaultPort(REGTEST_NET) ;
        nBlockInterval = 10;
        strDataDir = "regtest";
        assert(hashGenesisBlock == IniCfg().GetIntHash(REGTEST_NET));

        vFixedSeeds.clear();
        vSeeds.clear();  // Regtest mode doesn't have any DNS seeds.
    }

    virtual bool RequireRPCPassword() const {
        return false;
    }
    virtual NET_TYPE NetworkID() const {
        return REGTEST_NET;
    }
    virtual bool InitialConfig() {
        CTestNetParams::InitialConfig();

        nSubsidyHalvingInterval = GetArg("-subsidyhalvinginterval", IniCfg().GetHalvingInterval(REGTEST_NET));
        nBlockInterval = GetArg("-blockinterval", 10);
        fServer = true;
        return true;
    }
};

const vector<string> &CBaseParams::GetMultiArgs(const string& strArg) {
    return m_mapMultiArgs[strArg];
}
int CBaseParams::GetArgsSize() {
    return m_mapArgs.size();
}
int CBaseParams::GetMultiArgsSize() {
    return m_mapMultiArgs.size();
}

string CBaseParams::GetArg(const string& strArg, const string& strDefault) {
    if (m_mapArgs.count(strArg))
        return m_mapArgs[strArg];
    return strDefault;
}

int64_t CBaseParams::GetArg(const string& strArg, int64_t nDefault) {
    if (m_mapArgs.count(strArg))
        return atoi64(m_mapArgs[strArg]);
    return nDefault;
}

bool CBaseParams::GetBoolArg(const string& strArg, bool fDefault) {
    if (m_mapArgs.count(strArg)) {
        if (m_mapArgs[strArg].empty())
            return true;
        return (atoi(m_mapArgs[strArg]) != 0);
    }
    return fDefault;
}

bool CBaseParams::SoftSetArg(const string& strArg, const string& strValue) {
    if (m_mapArgs.count(strArg))
        return false;
    m_mapArgs[strArg] = strValue;
    return true;
}

bool CBaseParams::SoftSetArgCover(const string& strArg, const string& strValue) {
    m_mapArgs[strArg] = strValue;
    return true;
}

void CBaseParams::EraseArg(const string& strArgKey) {
    m_mapArgs.erase(strArgKey);
}

bool CBaseParams::SoftSetBoolArg(const string& strArg, bool fValue) {
    if (fValue)
        return SoftSetArg(strArg, string("1"));
    else
        return SoftSetArg(strArg, string("0"));
}

bool CBaseParams::IsArgCount(const string& strArg) {
    if (m_mapArgs.count(strArg)) {
        return true;
    }
    return false;
}

CBaseParams &SysCfg() {
    static shared_ptr<CBaseParams> pParams;

    if (pParams.get() == NULL) {
        bool fRegTest = CBaseParams::GetBoolArg("-regtest", false);
        bool fTestNet = CBaseParams::GetBoolArg("-testnet", false);
        if (fTestNet && fRegTest) {
            fprintf(stderr, "Error: Invalid combination of -regtest and -testnet.\n");
        }

        if (fRegTest) {
            pParams = std::make_shared<CRegTestParams>();
        } else if (fTestNet) {
            pParams = std::make_shared<CTestNetParams>();
        } else {
            pParams = std::make_shared<CMainParams>();
        }

    }
    assert(pParams != NULL);
    return *pParams.get();
}

//write for mainnet code
const CBaseParams &SysParamsMain() {
    static std::shared_ptr<CBaseParams> pParams;
    pParams = std::make_shared<CMainParams>();
    assert(pParams != NULL);
    return *pParams.get();
}

//write for testNet code
const CBaseParams &SysParamsTest() {
    static std::shared_ptr<CBaseParams> pParams;
    pParams = std::make_shared<CTestNetParams>();
    assert(pParams != NULL);
    return *pParams.get();
}

//write for RegTestNet code
const CBaseParams &SysParamsReg() {
    static std::shared_ptr<CBaseParams> pParams;
    pParams = std::make_shared<CRegTestParams>();
    assert(pParams != NULL);
    return *pParams.get();
}

static void InterpretNegativeSetting(string name, map<string, string>& mapSettingsRet) {
    // interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
    if (name.find("-no") == 0) {
        string positive("-");
        positive.append(name.begin() + 3, name.end());
        if (mapSettingsRet.count(positive) == 0) {
            bool value = !SysCfg().GetBoolArg(name, false);
            mapSettingsRet[positive] = (value ? "1" : "0");
        }
    }
}

void CBaseParams::ParseParameters(int argc, const char* const argv[]) {
    m_mapArgs.clear();
    m_mapMultiArgs.clear();
    for (int i = 1; i < argc; i++) {
        string str(argv[i]);
        string strValue;
        size_t is_index = str.find('=');
        if (is_index != string::npos) {
            strValue = str.substr(is_index + 1);
            str = str.substr(0, is_index);
        }
#ifdef WIN32
        boost::to_lower(str);
        if (boost::algorithm::starts_with(str, "/"))
            str = "-" + str.substr(1);
#endif
        if (str[0] != '-')
            break;

        m_mapArgs[str] = strValue;
        m_mapMultiArgs[str].push_back(strValue);
    }

    for (auto& entry : m_mapArgs) {
        string name = entry.first;

        //  interpret --foo as -foo (as long as both are not set)
        if (name.find("--") == 0) {
            string singleDash(name.begin() + 1, name.end());
            if (m_mapArgs.count(singleDash) == 0)
                m_mapArgs[singleDash] = entry.second;
            name = singleDash;
        }

        // interpret -nofoo as -foo=0 (and -nofoo=0 as -foo=1) as long as -foo not set
        InterpretNegativeSetting(name, m_mapArgs);
    }
}

bool CBaseParams::CreateGenesisRewardTx(vector<std::shared_ptr<CBaseTx> > &vRewardTx, NET_TYPE type) {
    vector<string> vInitPubKey = IniCfg().GetIntPubKey(type);
    for (size_t i = 0; i < vInitPubKey.size(); ++i) {
        int64_t money(0);
        if (i > 0) {
            money = IniCfg().GetCoinInitValue() * COIN;
        }
        shared_ptr<CRewardTx> pRewardTx = std::make_shared<CRewardTx>(ParseHex(vInitPubKey[i].c_str()),
                money, 0);
        pRewardTx->nVersion = nTxVersion1;
        if (pRewardTx.get())
            vRewardTx.push_back(pRewardTx);
        else
            return false;
    }
    return true;

};

bool CBaseParams::CreateGenesisDelegateTx(vector<std::shared_ptr<CBaseTx> > &vDelegateTx, NET_TYPE type) {
    vector<string> vDelegatePubKey = IniCfg().GetDelegatePubKey(type);
    vector<string> vInitPubKey = IniCfg().GetIntPubKey(type);
    vector<COperVoteFund> vOperVoteFund;
    for (size_t i = 0; i < vDelegatePubKey.size(); ++i) {
        uint64_t votes = IniCfg().GetCoinInitValue() * COIN  / 100;
        CID voteId(CPubKey(ParseHex(vDelegatePubKey[i].c_str())));
        CVoteFund fund(voteId, votes);
        COperVoteFund operVoteFund(ADD_FUND, fund);
        vOperVoteFund.push_back(operVoteFund);
    }
    CRegID accountId(0, 1);
    shared_ptr<CDelegateTx> pDelegateTx = std::make_shared<CDelegateTx>(accountId.GetVec6(),
            vOperVoteFund, 10000, 0);
    pDelegateTx->signature = ParseHex(IniCfg().GetDelegateSignature(type));
    pDelegateTx->nVersion = nTxVersion1;
    vDelegateTx.push_back(pDelegateTx);
    return true;
}

bool CBaseParams::InitializeParams(int argc, const char* const argv[]) {
    ParseParameters(argc, argv);
    if (!boost::filesystem::is_directory(GetDataDir(false))) {
        fprintf(stderr, "Error: Specified data directory \"%s\" does not exist.\n", CBaseParams::m_mapArgs["-datadir"].c_str());
        return false;
    }
    try {
        ReadConfigFile(CBaseParams::m_mapArgs, CBaseParams::m_mapMultiArgs);
    } catch (exception &e) {
        fprintf(stderr, "Error reading configuration file: %s\n", e.what());
        return false;
    }
    return true;
}

int64_t CBaseParams::GetTxFee() const{
    return paytxfee;
}
int64_t CBaseParams::SetDefaultTxFee(int64_t fee) const{
    paytxfee = fee;
    return fee;
}

CBaseParams::CBaseParams() {
    fImporting = false;
    fReindex = false;
    fBenchmark = false;
    fTxIndex = false;
    nLogMaxSize = 100 * 1024 * 1024;//100M
    nTxCacheHeight = 500;
    nTimeBestReceived = 0;
    nScriptCheckThreads = 0;
    nViewCacheSize = 2000000;
    nBlockInterval = 10;
    nSubsidyHalvingInterval = 0;
    paytxfee = 10000;
    nDefaultPort = 0;
    fPrintLogToConsole= 0;
    fPrintLogToFile = 0;
    fLogTimestamps = 0;
    fLogPrintFileLine = 0;
    fDebug = 0;
    fDebugAll= 0 ;
    fServer = 0 ;
    fServer = 0;
    nRPCPort = 0;
    bOutPut = false;
    nUIPort = 0;
}
