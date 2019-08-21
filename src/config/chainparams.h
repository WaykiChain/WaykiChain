// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef COIN_CHAIN_PARAMS_H
#define COIN_CHAIN_PARAMS_H
#include <memory>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <vector>

#include "entities/id.h"
#include "commons/uint256.h"
#include "commons/arith_uint256.h"
#include "commons/util.h"
#include "config/scoin.h"

using namespace std;

#define MESSAGE_START_SIZE 4
typedef uint8_t MessageStartChars[MESSAGE_START_SIZE];

class CAddress;
class CBaseTx;
class CBlock;

struct CDNSSeedData {
    string name, host;
    CDNSSeedData(const string& strName, const string& strHost) : name(strName), host(strHost) {}
};

typedef enum {
    MAIN_NET,          //!< MAIN_NET
    TEST_NET,          //!< TESTNET
    REGTEST_NET,       //!< REGTEST_NET
    MAX_NETWORK_TYPES  //!< MAX_NETWORK_TYPES
} NET_TYPE;

typedef enum {
    PUBKEY_ADDRESS,   //!< PUBKEY_ADDRESS
    SCRIPT_ADDRESS, //!< SCRIPT_ADDRESS
    SECRET_KEY,       //!< SECRET_KEY
    EXT_PUBLIC_KEY,   //!< EXT_PUBLIC_KEY
    EXT_SECRET_KEY,   //!< EXT_SECRET_KEY
    ACC_ADDRESS,      //!< ACC_ADDRESS
    MAX_BASE58_TYPES  //!< MAX_BASE58_TYPES
} Base58Type;

/**
 * CChainParams defines various tweakable parameters of a given instance of the
 * Coin system. There are three: the main network on which people trade goods
 * and services, the public test network which gets reset from time to time and
 * a regression test mode which is intended for private networks only. It has
 * minimal difficulty to ensure that blocks can be found instantly.
 */
class CBaseParams {
protected:
    mutable bool fDebugAll;
    mutable bool fDebug;
    mutable bool fPrintLogToConsole;
    mutable bool fPrintLogToFile;
    mutable bool fLogTimestamps;
    mutable bool fLogPrintFileLine;
    mutable bool fServer;
    mutable bool fImporting;
    mutable bool fReindex;
    mutable bool fBenchmark;
    mutable bool fTxIndex;
    mutable bool fLogFailures;
    mutable int64_t nTimeBestReceived;
    mutable uint64_t payTxFee;
    mutable uint32_t nViewCacheSize;
    mutable int32_t nTxCacheHeight;
    mutable uint32_t nLogMaxSize;  // to limit the maximum log file size in bytes
    mutable bool bContractLog;     // whether to save contract script operation account log

public:
    virtual ~CBaseParams() {}

    virtual bool InitialConfig() {
        fServer = GetBoolArg("-server", false);

        m_mapMultiArgs["-debug"].push_back("ERROR");  // Enable ERROR logger by default
        fDebug = !m_mapMultiArgs["-debug"].empty();
        if (fDebug) {
            fDebugAll          = GetBoolArg("-logprintall", false);
            fPrintLogToConsole = GetBoolArg("-logprinttoconsole", false);
            fLogTimestamps     = GetBoolArg("-logtimestamps", true);
            fPrintLogToFile    = GetBoolArg("-logprinttofile", false);
            fLogPrintFileLine  = GetBoolArg("-logprintfileline", false);
        }
        int64_t nTransactionFee ;
        if (ParseMoney(GetArg("-paytxfee", ""), nTransactionFee) && nTransactionFee > 0) {
            payTxFee = nTransactionFee;
        }

        nLogMaxSize    = GetArg("-logmaxsize", 100) * 1024 * 1024;
        bContractLog   = GetBoolArg("-contractlog", false);  // contract account change log

        return true;
    }

    virtual string ToString() const {
        string te = "";

        for (auto & tep1 : m_mapMultiArgs) {
            te += strprintf("key:%s\n",tep1.first);
            vector<string> tep = tep1.second;
            for (auto const & tep3:tep) {
                te += strprintf("value:%s\n",tep3.c_str());
            }
        }
        te += strprintf("fDebugAll:%s\n",                           fDebugAll);
        te += strprintf("fDebug:%s\n",                              fDebug);
        te += strprintf("fPrintLogToConsole:%d\n",                  fPrintLogToConsole);
        te += strprintf("fPrintLogToFile:%d\n",                     fPrintLogToFile);
        te += strprintf("fLogTimestamps:%d\n",                      fLogTimestamps);
        te += strprintf("fLogPrintFileLine:%d\n",                   fLogPrintFileLine);
        te += strprintf("fServer:%d\n",                             fServer);
        te += strprintf("fImporting:%d\n",                          fImporting);
        te += strprintf("fReindex:%d\n",                            fReindex);
        te += strprintf("fBenchmark:%d\n",                          fBenchmark);
        te += strprintf("fTxIndex:%d\n",                            fTxIndex);
        te += strprintf("fLogFailures:%d\n",                        fLogFailures);
        te += strprintf("nTimeBestReceived:%llu\n",                 nTimeBestReceived);
        te += strprintf("paytxfee:%llu\n",                          payTxFee);
        te += strprintf("nBlockIntervalPreStableCoinRelease:%u\n",  nBlockIntervalPreStableCoinRelease);
        te += strprintf("nBlockIntervalStableCoinRelease:%u\n",     nBlockIntervalStableCoinRelease);
        te += strprintf("nViewCacheSize:%u\n",                      nViewCacheSize);
        te += strprintf("nTxCacheHeight:%u\n",                      nTxCacheHeight);
        te += strprintf("nLogMaxSize:%u\n",                         nLogMaxSize);

        return te;
    }

    virtual uint32_t GetBlockMaxNonce() const { return 1000; }
    int64_t GetTxFee() const;
    int64_t SetDefaultTxFee(int64_t fee) const;
    virtual string GetDefaultTestDataPath() const {
        char findchar;
        #ifdef WIN32
        findchar = '\\';
        #else
        findchar = '/';
        #endif

        string strCurDir = boost::filesystem::initial_path<boost::filesystem::path>().string();
        int index = strCurDir.find_last_of(findchar);
        int count = 3;
        while (count--) {
            index = strCurDir.find_last_of(findchar);
            strCurDir = strCurDir.substr(0, index);

        }

        #ifdef WIN32
        strCurDir += "\\Coin_test\\data\\";
        return strCurDir;
        #else
        strCurDir +="/Coin_test/data/";
        return strCurDir;
        #endif
    }
public:
    int GetConnectTimeOut() const {
        int nConnectTimeout = 5000;
        if (m_mapArgs.count("-timeout")) {
            int nNewTimeout = GetArg("-timeout", 5000);
            if (nNewTimeout > 0 && nNewTimeout < 600000) nConnectTimeout = nNewTimeout;
        }
        return nConnectTimeout;
    }
    bool IsDebug() const { return fDebug; }
    bool IsDebugAll() const { return fDebugAll; }
    bool IsPrintLogToConsole() const { return fPrintLogToConsole; }
    bool IsPrintLogToFile() const { return fPrintLogToFile; }
    bool IsLogTimestamps() const { return fPrintLogToFile; }
    bool IsLogPrintLine() const { return fLogPrintFileLine; }
    bool IsServer() const { return fServer; }
    bool IsImporting() const { return fImporting; }
    bool IsReindex() const { return fReindex; }
    bool IsBenchmark() const { return fBenchmark; }
    bool IsTxIndex() const { return fTxIndex; }
    bool IsLogFailures() const { return fLogFailures; };
    int64_t GetBestRecvTime() const { return nTimeBestReceived; }
    uint32_t GetViewCacheSize() const { return nViewCacheSize; }
    int32_t GetTxCacheHeight() const { return nTxCacheHeight; }
    uint32_t GetLogMaxSize() const { return nLogMaxSize; }
    void SetImporting(bool flag) const { fImporting = flag; }
    void SetReIndex(bool flag) const { fReindex = flag; }
    void SetBenchMark(bool flag) const { fBenchmark = flag; }
    void SetTxIndex(bool flag) const { fTxIndex = flag; }
    void SetLogFailures(bool flag) const { fLogFailures = flag; }
    void SetBestRecvTime(int64_t nTime) const { nTimeBestReceived = nTime; }
    void SetViewCacheSize(uint32_t nSize) const { nViewCacheSize = nSize; }
    void SetTxCacheHeight(int32_t height) const { nTxCacheHeight = height; }
    bool IsContractLogOn() const { return bContractLog; }
    const MessageStartChars& MessageStart() const { return pchMessageStart; }
    const vector<uint8_t>& AlertKey() const { return vAlertPubKey; }
    int32_t GetDefaultPort() const { return nDefaultPort; }
    uint32_t GetBlockIntervalPreStableCoinRelease() const { return nBlockIntervalPreStableCoinRelease; }
    uint32_t GetBlockIntervalStableCoinRelease() const { return nBlockIntervalStableCoinRelease; }
    uint32_t GetFeatureForkHeight() const { return nFeatureForkHeight; }
    uint32_t GetStableCoinGenesisHeight() const { return nStableCoinGenesisHeight; }
    CRegID GetFcoinGenesisRegId() const { return CRegID(nStableCoinGenesisHeight, kFcoinGenesisIssueTxIndex); }
    CRegID GetDexMatchSvcRegId() const    { return CRegID(nStableCoinGenesisHeight, kDexMatchSvcRegisterTxIndex); }
    virtual uint64_t GetMaxFee() const { return 1000 * COIN; }
    virtual const CBlock& GenesisBlock() const = 0;
    const uint256& GetGenesisBlockHash() const { return genesisBlockHash; }
    bool CreateGenesisBlockRewardTx(vector<std::shared_ptr<CBaseTx> >& vptx, NET_TYPE type);
    bool CreateGenesisDelegateTx(vector<std::shared_ptr<CBaseTx> >& vptx, NET_TYPE type);
    bool CreateFundCoinRewardTx(vector<std::shared_ptr<CBaseTx> >& vptx, NET_TYPE type);
    virtual bool RequireRPCPassword() const { return true; }
    const string& DataDir() const { return strDataDir; }
    virtual NET_TYPE NetworkID() const = 0;
    const vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const vector<uint8_t>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    virtual const vector<CAddress>& FixedSeeds() const = 0;
    virtual bool IsInFixedSeeds(CAddress& addr)        = 0;
    int RPCPort() const { return nRPCPort; }
    static bool InitializeParams(int argc, const char* const argv[]);
    static int64_t GetArg(const string& strArg, int64_t nDefault);
    static string GetArg(const string& strArg, const string& strDefault);
    static bool GetBoolArg(const string& strArg, bool fDefault);
    static bool SoftSetArg(const string& strArg, const string& strValue);
    static bool SoftSetBoolArg(const string& strArg, bool fValue);
    static bool IsArgCount(const string& strArg);
    static bool SoftSetArgCover(const string& strArg, const string& strValue);
    static void EraseArg(const string& strArgKey);
    static void ParseParameters(int argc, const char* const argv[]);
    static const vector<string>& GetMultiArgs(const string& strArg);
    static int GetArgsSize();
    static int GetMultiArgsSize();
    static map<string, string> GetMapArgs() { return m_mapArgs; }
    static map<string, vector<string> > GetMapMultiArgs() { return m_mapMultiArgs; }
    static void SetMapArgs(const map<string, string>& mapArgs) { m_mapArgs = mapArgs; }
    static void SetMultiMapArgs(const map<string, vector<string> >& mapMultiArgs) { m_mapMultiArgs = mapMultiArgs; }

protected:
    static map<string, string> m_mapArgs;
    static map<string, vector<string> > m_mapMultiArgs;

protected:
    CBaseParams();

    uint256 genesisBlockHash;
    MessageStartChars pchMessageStart;
    // Raw pub key bytes for the broadcast alert signing key.
    vector<uint8_t> vAlertPubKey;
    int32_t nDefaultPort;
    int32_t nRPCPort;
    string alartPKey;
    uint32_t nStableCoinGenesisHeight;
    uint32_t nFeatureForkHeight;
    uint32_t nBlockIntervalPreStableCoinRelease;
    uint32_t nBlockIntervalStableCoinRelease;
    string strDataDir;
    vector<CDNSSeedData> vSeeds;
    vector<uint8_t> base58Prefixes[MAX_BASE58_TYPES];
};

extern CBaseParams &SysCfg();

// Note: it's deliberate that this returns "false" for regression test mode.
inline bool TestNet() { return SysCfg().NetworkID() == TEST_NET; }

inline bool RegTest() { return SysCfg().NetworkID() == REGTEST_NET; }

// write for test code
extern const CBaseParams& SysParamsMain();

// write for test code
extern const CBaseParams& SysParamsTest();

// write for test code
extern const CBaseParams& SysParamsReg();

extern string initPubKey[];

#endif
