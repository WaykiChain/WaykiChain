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

#include "commons/json/json_spirit_value.h"
#include "commons/uint256.h"
#include "commons/arith_uint256.h"
#include "commons/util/util.h"
#include "config/scoin.h"
#include "entities/id.h"

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
    MAIN_NET            = 0,
    TEST_NET            = 1,
    REGTEST_NET         = 2,
    NULL_NETWORK_TYPE   = 3
} NET_TYPE;

static const string NetTypeNames[] = { "MAIN_NET", "TEST_NET", "REGTEST_NET" };

typedef enum {
    PUBKEY_ADDRESS,     //!< PUBKEY_ADDRESS
    SCRIPT_ADDRESS,     //!< SCRIPT_ADDRESS
    SECRET_KEY,         //!< SECRET_KEY
    EXT_PUBLIC_KEY,     //!< EXT_PUBLIC_KEY
    EXT_SECRET_KEY,     //!< EXT_SECRET_KEY
    ACC_ADDRESS,        //!< ACC_ADDRESS
    MAX_BASE58_TYPES    //!< MAX_BASE58_TYPES
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
    mutable bool fDebug;
    mutable bool fServer;
    mutable bool fDisplayValueInSawi;
    mutable bool fImporting;
    mutable bool fReindex;
    mutable bool fBenchmark;
    mutable bool fTxIndex;
    mutable bool fLogFailures;
    mutable bool fGenReceipt;
    mutable int64_t nTimeBestReceived;
    mutable uint32_t nCacheSize;
    mutable int32_t nTxCacheHeight;
    mutable int32_t nMaxForkTime;  // to limit the maximum fork time in seconds.

public:
    virtual ~CBaseParams() {}

    virtual string ToString() const {
        string te = "";

        for (auto & tep1 : m_mapMultiArgs) {
            te += strprintf("key:%s\n",tep1.first);
            vector<string> tep = tep1.second;
            for (auto const & tep3:tep) {
                te += strprintf("value:%s\n",tep3.c_str());
            }
        }

        te += strprintf("fDebug:%s\n",                              fDebug);
        te += strprintf("fServer:%d\n",                             fServer);
        te += strprintf("fImporting:%d\n",                          fImporting);
        te += strprintf("fReindex:%d\n",                            fReindex);
        te += strprintf("fBenchmark:%d\n",                          fBenchmark);
        te += strprintf("fTxIndex:%d\n",                            fTxIndex);
        te += strprintf("fLogFailures:%d\n",                        fLogFailures);
        te += strprintf("nTimeBestReceived:%llu\n",                 nTimeBestReceived);
        te += strprintf("nBlockIntervalPreVer2Fork:%u\n",  nBlockIntervalPreVer2Fork);
        te += strprintf("nBlockIntervalPostVer2Fork:%u\n",     nBlockIntervalPostVer2Fork);
        te += strprintf("nCacheSize:%u\n",                          nCacheSize);
        te += strprintf("nTxCacheHeight:%u\n",                      nTxCacheHeight);
        te += strprintf("nMaxForkTime:%d\n",                        nMaxForkTime);

        return te;
    }

    virtual uint32_t GetBlockMaxNonce() const { return 1000; }
    int64_t GetTxFee() const;
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

    void InitConfig(bool fServerIn, bool fDebugIn, bool fDisplayValueInSawiIn, int64_t maxForkTimeIn) {
        fServer         = fServerIn;
        fDebug          = fDebugIn;
        nMaxForkTime    = maxForkTimeIn;
        fDisplayValueInSawi = fDisplayValueInSawiIn;
    }

    bool IsDebug() const { return fDebug; }
    bool IsServer() const { return fServer; }
    bool IsDisplayValueInSawi() const { return fDisplayValueInSawi; }
    bool IsImporting() const { return fImporting; }
    bool IsReindex() const { return fReindex; }
    bool IsBenchmark() const { return fBenchmark; }
    bool IsTxIndex() const { return fTxIndex; }
    bool IsLogFailures() const { return fLogFailures; };
    bool IsGenReceipt() const { return fGenReceipt; };
    int64_t GetBestRecvTime() const { return nTimeBestReceived; }
    uint32_t GetCacheSize() const { return nCacheSize; }
    int32_t GetTxCacheHeight() const { return nTxCacheHeight; }
    void SetImporting(bool flag) const { fImporting = flag; }
    void SetReIndex(bool flag) const { fReindex = flag; }
    void SetBenchMark(bool flag) const { fBenchmark = flag; }
    void SetTxIndex(bool flag) const { fTxIndex = flag; }
    void SetLogFailures(bool flag) const { fLogFailures = flag; }
    void SetGenReceipt(bool flag) const { fGenReceipt = flag; }
    void SetBestRecvTime(int64_t nTime) const { nTimeBestReceived = nTime; }
    int32_t GetMaxForkHeight(int32_t currBlockHeight) const;
    const MessageStartChars& MessageStart() const { return pchMessageStart; }
    const vector<uint8_t>& AlertKey() const { return vAlertPubKey; }
    int32_t GetDefaultPort() const { return nDefaultPort; }

    uint32_t GetVer2GenesisHeight() const { return nVer2GenesisHeight; }
    uint32_t GetVer2ForkHeight() const { return nVer2ForkHeight; }
    uint32_t GetVer3ForkHeight() const { return nVer3ForkHeight; }

    uint32_t GetBlockIntervalPreVer2Fork()  const { return nBlockIntervalPreVer2Fork; }
    uint32_t GetBlockIntervalPostVer2Fork() const { return nBlockIntervalPostVer2Fork; }
    uint32_t GetContinuousProducePreVer3Fork()  const { return nContinuousBlockProducePreVer3Fork; }
    uint32_t GetContinuousProducePostVer3Fork() const { return nContinuousBlockProducePostVer3Fork; }

    CRegID GetFcoinGenesisRegId()   const { return CRegID(nVer2GenesisHeight, 1); }
    CRegID GetDexMatchSvcRegId()    const { return CRegID(nVer2GenesisHeight, 3); }
    CRegID GetDex0OwnerRegId()      const { return CRegID(nVer2GenesisHeight, 3); }

    virtual uint64_t GetMaxFee() const { return 1000 * COIN; }
    virtual const CBlock& GenesisBlock() const = 0;
    const uint256& GetGenesisBlockHash() const { return genesisBlockHash; }
    bool CreateGenesisBlockRewardTx(vector<std::shared_ptr<CBaseTx> >& vptx, NET_TYPE type);
    bool CreateGenesisDelegateTx(vector<std::shared_ptr<CBaseTx> >& vptx, NET_TYPE type);
    bool CreateFundCoinMintTx(vector<std::shared_ptr<CBaseTx> >& vptx, NET_TYPE type);
    virtual bool RequireRPCPassword() const { return true; }
    const string& DataDir() const { return strDataDir; }
    virtual NET_TYPE NetworkID() const = 0;
    const vector<CDNSSeedData>& DNSSeeds() const { return vSeeds; }
    const vector<uint8_t>& Base58Prefix(Base58Type type) const { return base58Prefixes[type]; }
    virtual const vector<CAddress>& FixedSeeds() const = 0;
    virtual bool IsInFixedSeeds(CAddress& addr)        = 0;
    int RPCPort() const { return nRPCPort; }
    static bool LoadParamsFromConfigFile(int argc, const char* const argv[]);
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

    string strDataDir;
    uint256 genesisBlockHash;
    MessageStartChars pchMessageStart;
    // Raw pub key bytes for the broadcast alert signing key.
    vector<uint8_t> vAlertPubKey;
    int32_t nDefaultPort;
    int32_t nRPCPort;
    string alartPKey;
    uint32_t nVer2GenesisHeight;
    uint32_t nVer2ForkHeight;
    uint32_t nVer3ForkHeight;
    uint32_t nBlockIntervalPreVer2Fork;
    uint32_t nBlockIntervalPostVer2Fork;
    uint32_t nContinuousBlockProducePreVer3Fork;
    uint32_t nContinuousBlockProducePostVer3Fork;

    vector<CDNSSeedData> vSeeds;
    vector<uint8_t> base58Prefixes[MAX_BASE58_TYPES];
};

extern CBaseParams &SysCfg();

inline json_spirit::Value JsonValueFromAmount(uint64_t amount) {
    if (SysCfg().IsDisplayValueInSawi())
        return amount;
    else
        return (double)amount / (double)COIN;
}

inline double ValueFromAmount(uint64_t amount) { return (double)amount / (double)COIN; }

// Note: it's deliberate that this returns "false" for regression test mode.
inline bool TestNet() { return SysCfg().NetworkID() == TEST_NET; }

inline bool RegTest() { return SysCfg().NetworkID() == REGTEST_NET; }

// write for test code
extern const CBaseParams& SysParamsMain();

// write for test code
extern const CBaseParams& SysParamsTest();

// write for test code
extern const CBaseParams& SysParamsReg();

extern string initPubKey[3];

#endif
