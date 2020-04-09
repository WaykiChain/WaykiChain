// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DBCONF_H
#define PERSIST_DBCONF_H

#include <leveldb/slice.h>
#include <string>

#include "config/version.h"
#include "commons/serialize.h"

typedef leveldb::Slice Slice;

#define DEF_DB_NAME_ENUM(enumType, enumName, cacheSize) enumType,
#define DEF_DB_NAME_ARRAY(enumType, enumName, cacheSize) enumName,
#define DEF_CACHE_SIZE_ARRAY(enumType, enumName, cacheSize) cacheSize,

//         DBNameType            DBName             DBCacheSize           description
//         ----------           --------------    --------------     ----------------------------
#define DB_NAME_LIST(DEFINE)                                                                                    \
    DEFINE( SYSPARAM,           "params",         (50  << 10) )      /* 50KB:   system params */                \
    DEFINE( ACCOUNT,            "accounts",       (50  << 20) )      /* 50MB:   accounts & account assets */    \
    DEFINE( ASSET,              "assets",         (100 << 10) )      /* 100KB:  asset registry */               \
    DEFINE( BLOCK,              "blocks",         (500 << 10) )      /* 500KB:  block & tx indexes */           \
    DEFINE( CONTRACT,           "contracts",      (50  << 20) )      /* 50MB:   contract */                     \
    DEFINE( DELEGATE,           "delegates",      (100 << 10) )      /* 100KB:  delegates */                    \
    DEFINE( CDP,                "cdps",           (50  << 20) )      /* 50MB:   cdp */                          \
    DEFINE( CLOSEDCDP,          "closedcdps",     (1   << 20) )      /* 1MB:    closed cdp */                   \
    DEFINE( DEX,                "dexes",          (50  << 20) )      /* 50MB:   dex */                          \
    DEFINE( LOG,                "logs",           (100 << 10) )      /* 100KB:  log */                          \
    DEFINE( RECEIPT,            "receipts",       (100 << 10) )      /* 100KB:  tx receipt */                   \
    DEFINE( UTXO,               "utxo",           (50  << 20) )      /* 50MB:   utxo tx track db */             \
    DEFINE( SYSGOVERN,          "governs",        (100 << 10) )      /* 100KB:  governors */                    \
    DEFINE( PRICEFEED,          "pricefeed",      (50  << 10) )      /* 50KB:   price feeds*/                   \
    DEFINE( AXC,                "axc",            (50  << 10) )      /* 50KB:   cross-chain */                  \
    /*                                                                  */                                      \
    /* Add new Enum elements above, DB_NAME_COUNT Must be the last one */                                       \
    DEFINE( DB_NAME_COUNT,        "",               0)                  /* enum count, must be the last one */

enum DBNameType {
    DB_NAME_LIST(DEF_DB_NAME_ENUM)
};


#define DB_NAME_NONE DB_NAME_COUNT

static const int32_t DBCacheSize[DBNameType::DB_NAME_COUNT + 1] {
    DB_NAME_LIST(DEF_CACHE_SIZE_ARRAY)
};

static const std::string kDbNames[DBNameType::DB_NAME_COUNT + 1] {
    DB_NAME_LIST(DEF_DB_NAME_ARRAY)
};

inline const std::string& GetDbName(DBNameType dbNameType) {
    assert(dbNameType >= 0 && dbNameType < DBNameType::DB_NAME_COUNT);
    return kDbNames[dbNameType];
}

namespace dbk {


    //                 type        name(prefix)  db name             description
    //               ----------    ------------ -------------  -----------------------------------
    #define DBK_PREFIX_LIST(DEFINE) \
        DEFINE( EMPTY,                "",      DB_NAME_NONE )  /* empty prefix  */ \
        /*                                                                      */ \
        /**** single-value sys_conf db (global parameters)                      */ \
        DEFINE( SYS_PARAM,            "sysp",   SYSPARAM )       /* conf{$ParamName} --> $ParamValue */ \
        DEFINE( MINER_FEE,            "minf",   SYSPARAM )         \
        DEFINE( CDP_PARAM,            "cdpp",   SYSPARAM )         \
        DEFINE( CDP_INTEREST_PARAMS,  "cips",   SYSPARAM )       /* [prefix]*/  \
        DEFINE( TOTAL_BPS_SIZE,       "bpsi",   SYSPARAM )           \
        DEFINE( NEW_TOTAL_BPS_SIZE,   "nbps",   SYSPARAM )           \
        DEFINE( SYS_GOVERN,           "govn",   SYSGOVERN )       /* govn --> $list of governors */ \
        DEFINE( GOVN_PROP,            "pgvn",   SYSGOVERN )       /* pgvn{propid} --> proposal */ \
        DEFINE( GOVN_APPROVAL_LIST,   "galt",   SYSGOVERN )       /* sgvn{propid} --> vector(regid) */ \
        DEFINE( AXCOUT_PROPOSAL_TXUID, "aptu",  SYSGOVERN )     \
        /*** Asset Registry DB */ \
        DEFINE( ASSET,                "asst",   ASSET )          /* asst{$AssetName} --> $Asset */ \
        DEFINE( AXC_COIN_PEERTOSELF,   "acps",  ASSET )        /* [prefix]swapin-txid -->  amount */      \
        DEFINE( AXC_COIN_SELFTOPEER,   "acsp",  ASSET )                             \
        /**** block db                                                                          */ \
        DEFINE( BLOCK_INDEX,          "bidx",   BLOCK )         /* pbfl --> $nFile */ \
        DEFINE( BLOCKFILE_NUM_INFO,   "bfni",   BLOCK )         /* BlockFileNum --> $BlockFileInfo */ \
        DEFINE( LAST_BLOCKFILE,       "ltbf",   BLOCK )         /* [prefix] --> $LastBlockFile */ \
        DEFINE( REINDEX,              "ridx",   BLOCK )         /* [prefix] --> $Reindex = 1 | 0 */ \
        DEFINE( FINALITY_BLOCK,       "finb",   BLOCK )         /* [prefix] --> &globalfinblock height and hash */ \
        DEFINE( FLAG,                 "flag",   BLOCK )         /* [prefix] --> $Flag = 1 | 0 */ \
        DEFINE( BEST_BLOCKHASH,       "bbkh",   BLOCK )         /* [prefix] --> $BestBlockHash */ \
        DEFINE( TXID_DISKINDEX,       "tidx",   BLOCK )         /* tidx{$txid} --> $DiskTxPos */ \
        /**** account db                                                                      */ \
        DEFINE( REGID_KEYID,          "rkey",   ACCOUNT )       /* rkey{$RegID} --> $KeyId */ \
        DEFINE( KEYID_ACCOUNT,        "idac",   ACCOUNT )       /* idac{$KeyID} --> $CAccount */ \
        /**** contract db                                                                      */ \
        DEFINE( CONTRACT_DEF,         "cdef",   CONTRACT )      /* cdef{$ContractRegId} --> $ContractContent */ \
        DEFINE( CONTRACT_DATA,        "cdat",   CONTRACT )      /* cdat{$RegId}{$DataKey} --> $Data */ \
        DEFINE( CONTRACT_ACCOUNT,     "cacc",   CONTRACT )      /* cacc{$ContractRegId}{$AccUserId} --> appUserAccount */ \
        DEFINE( CONTRACT_TRACES,      "ctrs",   CONTRACT )      /* [prefix]{$txid} --> contract_traces */ \
        /**** delegate db                                                                      */ \
        DEFINE( VOTE,                 "vote",   DELEGATE )      /* "vote{(uint64t)MAX - $votedBcoins}{$RegId} --> 1 */ \
        DEFINE( LAST_VOTE_HEIGHT,     "lvht",   DELEGATE )      /* "[prefix] --> last_vote_height */ \
        DEFINE( PENDING_DELEGATES,    "pdds",   DELEGATE )      /* "[prefix] --> pending_delegates */ \
        DEFINE( ACTIVE_DELEGATES,     "atds",   DELEGATE )      /* "[prefix] --> active_delegates */ \
        DEFINE( REGID_VOTE,           "ridv",   DELEGATE )      /* "ridv --> $votes" */ \
        /**** cdp db                                                                     */ \
        DEFINE( CDP,                  "cid",    CDP )           /* cid{$cdpid} --> CUserCDP */ \
        DEFINE( CDP_BCOIN_STATUS,     "cbcs",   CDP )           /* [prefix]{$bcoin_symbol} --> $bcoinStatus */ \
        DEFINE( USER_CDP,             "ucdp",   CDP )           /* [prefix]{$RegID}{$AssetSymbol}{$ScoinSymbol} --> {set<cdpid>} */ \
        DEFINE( CDP_RATIO_INDEX,      "crid",   CDP )           /* [prefix]{$cdpCoinPair}{$Ratio}{$height}{$cdpid} --> $userCDP */ \
        DEFINE( CDP_HEIGHT_INDEX,     "chid",   CDP )           /* [prefix]{$cdpCoinPair}{$height}{$cdpid} -> $userCDP */ \
        DEFINE( CDP_GLOBAL_DATA,      "cgdt",   CDP )           /* [prefix]{$cdpCoinPair} -> $cdpGlobalData */ \
        /**** cdp closed by redeem/forced or manned liquidate ***/  \
        DEFINE( CLOSED_CDP_TX,        "ctx",    CLOSEDCDP )     /* ccdp{cdpid} -> 1 */ \
        DEFINE( CLOSED_TX_CDP,        "txc",    CLOSEDCDP )     /* ccdp{cdpid} -> 1 */ \
        /**** dex db                                                                    */ \
        DEFINE( DEX_ACTIVE_ORDER,     "dato",       DEX )        /* [prefix]{txid} --> active order */ \
        DEFINE( DEX_BLOCK_ORDERS,     "dbos",       DEX )        /* [prefix]{height, generate_type, txid} --> active order */ \
        DEFINE( DEX_OPERATOR_LAST_ID, "doli",       DEX )        /* [prefix] --> dex_operator_new_id */ \
        DEFINE( DEX_OPERATOR_DETAIL,  "dode",       DEX )        /* [prefix]{dex_operator_id} --> dex_operator_detail */ \
        DEFINE( DEX_OPERATOR_OWNER_MAP, "doom",     DEX )        /* [prefix]{owner_name} --> dex_operator_id */ \
        DEFINE( DEX_OPERATOR_TRADE_PAIR, "dotp",    DEX )              \
        DEFINE( DEX_QUOTE_COIN_SYMBOL, "dqcs",      DEX)                    \
        /**** log db                                                                    */ \
        DEFINE( TX_EXECUTE_FAIL,      "txef",       LOG )        /* [prefix]{height}{txid} --> {error code, error message} */ \
        /**** tx receipt db                                                                 */ \
        DEFINE( TX_RECEIPT,           "txrc",       RECEIPT )    /* [prefix]{txid} --> {receipts} */ \
        DEFINE( BLOCK_RECEIPT,        "bkrc",       RECEIPT )    /* [prefix]{txid} --> {receipts} */ \
        /**** tx coinutxo db                                                                 */ \
        DEFINE( TX_UTXO,              "utxo",       UTXO )       /* [prefix]{txid-voutindex} --> 1 */ \
        /**** tx coinutxo db                                                                 */ \
        DEFINE( UTXO_PWSDPRF,         "pwdp",       UTXO )       /* [prefix]{txid-voutindex} --> {passwordProof} */ \
        /**** price feed db                                                                 */ \
        DEFINE( MEDIAN_PRICES,        "mdps",       PRICEFEED)   /* [prefix] --> median prices */ \
        DEFINE( PRICE_FEED_COIN_PAIRS, "pfcp",      PRICEFEED)   /* [prefix] --> price feed coin pairs */      \
        DEFINE( PRICE_FEEDERS,         "pfdr",      PRICEFEED)   /* [prefix] --> price feeder */      \
        /**** AXC                                                                             */ \
        DEFINE( AXC_SWAP_IN,           "axci",      AXC)        /* [prefix]swapin-txid -->  amount */      \
        /*                                                                             */ \
        /* Add new Enum elements above, PREFIX_COUNT Must be the last one              */ \
        DEFINE( PREFIX_COUNT,          "",       DB_NAME_NONE)    /* enum count, must be the last one */


    #define DEF_DB_PREFIX_ENUM(enumType, enumName, dbName) enumType,
    #define DEF_DB_PREFIX_NAME_ARRAY(enumType, enumName, dbName) enumName,
    #define DEF_DB_PREFIX_NAME_MAP(enumType, enumName, dbName) { enumName, enumType },
    #define DEF_DB_PREFIX_DBNAME(enumType, enumName, dbName) DBNameType::dbName,
    #define DEF_DB_PREFIX_MEMO(enumType, enumName, dbName) { #enumType },

    enum PrefixType {
        DBK_PREFIX_LIST(DEF_DB_PREFIX_ENUM)
    };

    static const std::string kPrefixNames[PREFIX_COUNT + 1] = {
        DBK_PREFIX_LIST(DEF_DB_PREFIX_NAME_ARRAY)
    };

    static const std::string kPrefixMemos[PREFIX_COUNT + 1] = {
        DBK_PREFIX_LIST(DEF_DB_PREFIX_MEMO)
    };

    static const std::map<std::string, PrefixType> gPrefixNameMap = {
        DBK_PREFIX_LIST(DEF_DB_PREFIX_NAME_MAP)
    };

    static const DBNameType kDbPrefix2DbName[PREFIX_COUNT + 1] = {
        DBK_PREFIX_LIST(DEF_DB_PREFIX_DBNAME)
    };

    inline const std::string& GetKeyPrefix(PrefixType prefixType) {
        assert(prefixType >= 0 && prefixType <= PREFIX_COUNT);
        return kPrefixNames[prefixType];
    };

    inline const std::string& GetKeyPrefixMemo(PrefixType prefixType) {
        assert(prefixType >= 0 && prefixType <= PREFIX_COUNT);
        return kPrefixMemos[prefixType];
    };

    inline DBNameType GetDbNameEnumByPrefix(PrefixType prefixType) {
        assert(prefixType > 0 && prefixType <= PREFIX_COUNT);
        return kDbPrefix2DbName[prefixType];
    };

    inline PrefixType ParseKeyPrefixType(const std::string &keyPrefix) {
        auto it = gPrefixNameMap.find(keyPrefix);
        if (it != gPrefixNameMap.end())
            return it->second;
        return EMPTY;
    };

    template<typename KeyElement>
    std::string GenDbKey(PrefixType keyPrefixType, const KeyElement &keyElement) {
        CDataStream ssKeyTemp(SER_DISK, CLIENT_VERSION);
        assert(keyPrefixType != EMPTY);
        const string &prefix = GetKeyPrefix(keyPrefixType);
        ssKeyTemp.write(prefix.c_str(), prefix.size()); // write buffer only, exclude size prefix
        ssKeyTemp << keyElement;
        return std::string(ssKeyTemp.begin(), ssKeyTemp.end());
    }

    template<typename KeyElement>
    bool ParseDbKey(const Slice& slice, PrefixType keyPrefixType, KeyElement &keyElement) {
        assert(slice.size() > 0);
        const string &prefix = GetKeyPrefix(keyPrefixType);
        if (prefix.empty() || !slice.starts_with(Slice(prefix))) {
            return false;
        }

        CDataStream ssKeyTemp(slice.data(), slice.data() + slice.size(), SER_DISK, CLIENT_VERSION);
        ssKeyTemp.ignore(prefix.size());
        ssKeyTemp >> keyElement;

        return true;
    }

    template<typename KeyElement>
    bool ParseDbKey(const std::string& key, PrefixType keyPrefixType, KeyElement &keyElement) {
        return ParseDbKey(Slice(key), keyPrefixType, keyElement);
    }

    // CDBTailKey
    // support partial match.
    // must be last element of pair or tuple key,
    template<uint32_t __MAX_KEY_SIZE>
    class CDBTailKey {
    public:
        enum { MAX_KEY_SIZE = __MAX_KEY_SIZE };
    private:
        string key;
    public:
        CDBTailKey() {}
        CDBTailKey(const string &keyIn): key(keyIn) { assert(keyIn.size() <= MAX_KEY_SIZE); }

        const string& GetKey() const { return key; }

        inline bool StartWith(const string& prefix) const {
            return key.compare(0, prefix.size(), prefix) == 0;
        }

        inline bool StartWith(const CDBTailKey& prefix) const {
            return StartWith(prefix.key);
        }

        inline uint32_t GetSerializeSize(int32_t nType, int32_t nVersion) const {
            return key.size();
        }

        void Serialize(CDataStream &s, int nType, int nVersion) const {
            s.write(key.data(), key.size());
        }

        void Unserialize(CDataStream &s, int nType, int nVersion) {
            if (s.size() > MAX_KEY_SIZE) {
                throw ios_base::failure("CDBTailKey::Unserialize size excceded max size");
            }
            // read key from s.begin() to s.end(), s.begin() is current read pos
            key.resize(s.size());
            s.read(key.data(), s.size());
        }

        bool operator==(const CDBTailKey &other) {
            return key == other.key;
        }

        bool operator<(const CDBTailKey &other) const {
            return this->key < other.key;
        }

        bool IsEmpty() const { return key.empty(); }

        void SetEmpty() { key.clear(); }

        string ToString() const {
            return key;
        }

    };
}

class SliceIterator {
public:
    SliceIterator(Slice &sliceIn): slice(sliceIn) {}
    inline const char* begin() const { return slice.data(); };
    inline const char* end() const { return slice.data() + slice.size(); };
private:
    Slice &slice;
};

#endif  // PERSIST_DBCONF_H
