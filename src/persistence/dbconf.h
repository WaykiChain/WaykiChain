// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DBCONF_H
#define PERSIST_DBCONF_H

#include <leveldb/slice.h>
#include <string>

#include "version.h"
#include "commons/serialize.h"

typedef leveldb::Slice Slice;

#define DEF_DB_NAME_ENUM(enumType, enumName) enumType,
#define DEF_DB_NAME_ARRAY(enumType, enumName) enumName,

//         DBNameType            DBName           description
//         ----------           --------------     ----------------------------
#define DB_NAME_LIST(DEFINE) \
    DEFINE( ACCOUNT,             "account")      /* account */ \
    DEFINE( BLOCK,               "block")        /* account */ \
    DEFINE( CONTRACT,            "contract")     /* contract */ \
    DEFINE( DELEGATE,            "delegate")     /* delegate */ \
    DEFINE( CDP,                 "dcp")          /* dcp */ \
    DEFINE( DEX,                 "dex")          /* dex */ \
    /*                                                                */  \
    /* Add new Enum elements above, DB_NAME_COUNT Must be the last one */ \
    DEFINE( DB_NAME_COUNT,        "")       /* enum count, must be the last one */

enum DBNameType {
    DB_NAME_LIST(DEF_DB_NAME_ENUM)
};

#define DB_NAME_NONE DB_NAME_COUNT

static const std::string kDbNames[DBNameType::DB_NAME_COUNT + 1] {
    DB_NAME_LIST(DEF_DB_NAME_ARRAY)
};

inline const std::string& GetDbName(DBNameType dbNameType) {
    assert(dbNameType >= 0 && dbNameType < DBNameType::DB_NAME_COUNT);
    return kDbNames[dbNameType];
}

/**
 * database key
 *
 *    取脚本 时 第一个vector 是scriptKey = "def" + "scriptid";
      取应用账户时第一个vector是scriptKey = "acct" + "scriptid"+"_" + "accUserId";
      取脚本总条数时第一个vector是scriptKey ="snum",
      取脚本数据总条数时第一个vector是scriptKey ="sdnum";
      取脚本数据时第一个vector是scriptKey ="data" + "vScriptId" + "_" + "vScriptKey"
      取交易关联账户时第一个vector是scriptKey ="tx" + "txHash"
 */
namespace dbk {


    //                 type        name(prefix)  db name             description
    //               ----------    ------------ -------------  -----------------------------------
    #define DBK_PREFIX_LIST(DEFINE) \
        DEFINE( EMPTY,                "",      DB_NAME_NONE )  /* empty prefix  */ \
        /**** block db                                                                         */ \
        DEFINE( BLOCK_INDEX,          "bidx",  BLOCK )         /* pbfl --> $nFile */ \
        DEFINE( BLOCKFILE_NUM_INFO,   "bfni",  BLOCK )         /* BlockFileNum --> $BlockFileInfo */ \
        DEFINE( LAST_BLOCKFILE,       "ltbf",  BLOCK )         /* [prefix] --> $LastBlockFile */ \
        DEFINE( REINDEX,              "ridx",  BLOCK )         /* [prefix] --> $Reindex = 1 | 0 */ \
        DEFINE( FLAG,                 "flag",  BLOCK )         /* [prefix] --> $Flag = 1 | 0 */ \
        /**** account db                                                                     */ \
        DEFINE( REGID_KEYID,          "rkey",  ACCOUNT )       /* rkey{$RegID} --> $KeyId */ \
        DEFINE( NICKID_KEYID,         "nkey",  ACCOUNT )       /* nkey{$NickID} --> $KeyId */ \
        DEFINE( KEYID_ACCOUNT,        "idac",  ACCOUNT )       /* idac{$KeyID} --> $CAccount */ \
        DEFINE( BEST_BLOCKHASH,       "bbkh",  ACCOUNT )       /* [prefix] --> $BestBlockHash */ \
        /**** contract db                                                                     */ \
        DEFINE( LIST_KEYID_TX,        "lktx",  CONTRACT )      /* lktx{$KeyId}{$Height}{$Index} --> $txid */ \
        DEFINE( TXID_DISKINDEX,       "tidx",  CONTRACT )      /* tidx{$txid} --> $DiskTxPos */ \
        DEFINE( CONTRACT_DEF,         "cdef",  CONTRACT )      /* cdef{$ContractRegId} --> $ContractContent */ \
        DEFINE( CONTRACT_DATA,        "cdat",  CONTRACT )      /* cdat{$RegId}{$DataKey} --> $Data */ \
        DEFINE( CONTRACT_TX_OUT,      "cout",  CONTRACT )      /* cout{$txid} --> $VmOperateOutput */ \
        DEFINE( CONTRACT_ITEM_NUM,    "citn",  CONTRACT )      /* citn{$ContractRegId} --> $total_num_of_contract_i */ \
        DEFINE( CONTRACT_RELATED_KID, "crid",  CONTRACT )      /* cacs{$ContractTxId} --> $set<CKeyID> */ \
        DEFINE( CONTRACT_ACCOUNT,     "cacc",  CONTRACT )      /* cacc{$ContractRegId}{$AccUserId} --> appUserAccount */ \
        /**** delegate db                                                                     */ \
        DEFINE( VOTE,                 "vote",  DELEGATE )      /* "vote{(uint64t)MAX - $votedBcoins}{$RegId} --> 1 */ \
        DEFINE( REGID_VOTE,           "ridv",  DELEGATE )      /* "ridv --> $votes" */ \
        /**** cdp db                                                                     */ \
        DEFINE( STAKE_FCOIN,          "fcoin", CDP )           /* fcoin{(uint64t)MAX - stakedFcoins}_{RegId} --> 1 */ \
        DEFINE( CDP,                  "cdp",   CDP )           /* cdp{$RegID}{$TxCord} --> { lastBlockHeight, mintedScoins, totalStakedBcoins, totalOwedScoins } */ \
        DEFINE( CDP_IR_PARAM_A,       "ira",   CDP )           /* [prefix] --> param_a */ \
        DEFINE( CDP_IR_PARAM_B,       "irb",   CDP )           /* [prefix] --> param_b */ \
        DEFINE( CDP_COLLATERAL_RATIO, "ccr",   CDP )           /* [prefix] --> collateralRatio */ \
        DEFINE( CDP_OPEN_LIQUIDATE_RATIO, "colr", CDP )        /* [prefix] --> openLiquidateRatio */ \
        DEFINE( CDP_FORCE_LIQUIDATE_RATIO, "cflr", CDP )       /* [prefix] --> forceLiquidateRatio */ \
        /**** dex db                                                                    */ \
        DEFINE( DEX_BUY_ORDER,        "dexb",  DEX )           /* [prefix]{micc|wusd|wicc} --> buy order */ \
        DEFINE( DEX_SELL_ORDER,       "dexs",  DEX )           /* [prefix]{micc|wusd|wicc} --> sell order */ \
        /*                                                                             */ \
        /* Add new Enum elements above, PREFIX_COUNT Must be the last one              */ \
        DEFINE( PREFIX_COUNT,         "",      DB_NAME_NONE)   /* enum count, must be the last one */


    #define DEF_DB_PREFIX_ENUM(enumType, enumName, dbName) enumType,
    #define DEF_DB_PREFIX_NAME_ARRAY(enumType, enumName, dbName) enumName,
    #define DEF_DB_PREFIX_NAME_MAP(enumType, enumName, dbName) { enumName, enumType },
    #define DEF_DB_PREFIX_DBNAME(enumType, enumName, dbName) DBNameType::dbName,

    enum PrefixType {
        DBK_PREFIX_LIST(DEF_DB_PREFIX_ENUM)
    };

    static const std::string kPrefixNames[PREFIX_COUNT + 1] = {
        DBK_PREFIX_LIST(DEF_DB_PREFIX_NAME_ARRAY)
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
        if (!prefix.empty() && !slice.starts_with(Slice(prefix))) {
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