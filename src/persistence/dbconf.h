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

    #define EACH_ENUM_DEFINE_TYPE(enumType, enumName) enumType,
    #define EACH_ENUM_DEFINE_NAMES(enumType, enumName) enumName,

    //                 type                name(prefix)       description
    //               ----------           ------------   -----------------------------
    #define DBK_PREFIX_LIST(EACH_ENUM_DEFINE) \
        EACH_ENUM_DEFINE( EMPTY,              "")         /* empty prefix ) */ \
        EACH_ENUM_DEFINE( REINDEX,            "ridx")     /* [prefix] --> $Reindex = 1 | 0 */ \
        EACH_ENUM_DEFINE( FLAG,               "flag")     /* [prefix] --> $Flag = 1 | 0 */ \
        EACH_ENUM_DEFINE( VOTE,               "vote")     /* "vote{(uint64t)MAX - $votedBcoins}_{$RegId} --> 1 */ \
        EACH_ENUM_DEFINE( LIST_KEYID_TX,      "lktx")     /* lktx{$KeyId}{$Height}{$Index} --> $txid */ \
        EACH_ENUM_DEFINE( TXID_KEYIDS,        "tids")     /* tids{$txid} --> {$KeyId1, $KeyId2, ...}*/ \
        EACH_ENUM_DEFINE( TXID_DISKINDEX,     "tidx")     /* tidx{$txid} --> $DiskTxPos */ \
        EACH_ENUM_DEFINE( REGID_KEYID,        "rkey")     /* rkey{$RegID} --> $KeyId */ \
        EACH_ENUM_DEFINE( NICKID_KEYID,       "nkey")     /* nkey{$NickID} --> $KeyId */ \
        EACH_ENUM_DEFINE( KEYID_ACCOUNT,      "idac")     /* idac{$KeyID} --> $CAccount */ \
        EACH_ENUM_DEFINE( BEST_BLOCKHASH,     "bbkh")     /* [prefix] --> $BestBlockHash */ \
        EACH_ENUM_DEFINE( BLOCK_INDEX,        "bidx")     /* pbfl --> $nFile */ \
        EACH_ENUM_DEFINE( BLOCKFILE_NUM_INFO, "bfni")     /* BlockFileNum --> $BlockFileInfo */ \
        EACH_ENUM_DEFINE( LAST_BLOCKFILE,     "ltbf")     /* [prefix] --> $LastBlockFile */ \
        EACH_ENUM_DEFINE( CONTRACT_COUNT,     "cnum")     /* cnum{$ContractRegId} --> $total_num_of_cntracts */ \
        EACH_ENUM_DEFINE( CONTRACT_DEF,       "cdef")     /* cdef{$ContractRegId} --> $ContractContent */ \
        EACH_ENUM_DEFINE( CONTRACT_DATA,      "cdat")     /* cdat{$RegId}_{$DataKey} --> $Data */ \
        EACH_ENUM_DEFINE( CONTRACT_TX_OUT,    "cout")     /* cout{txid} --> $VmOperateOutput */ \
        EACH_ENUM_DEFINE( CONTRACT_ITEM_NUM,  "citn")     /* citn{ContractRegId} --> $total_num_of_contract_i */ \
        EACH_ENUM_DEFINE( STAKE_FCOIN,        "fcoin")    /* fcoin{(uint64t)MAX - stakedFcoins}_{RegId} --> 1 */ \
        EACH_ENUM_DEFINE( CDP,                "cdp")      /* cdp{$RegID} --> blockHeight,mintedScoins */ \
        EACH_ENUM_DEFINE( CDP_IR_PARAM_A,     "ira")      /* [prefix] --> param_a */ \
        EACH_ENUM_DEFINE( CDP_IR_PARAM_B,     "irb")      /* [prefix] --> param_b */ \
        \
        /* Add new Enum elements above, PREFIX_COUNT Must be the last one */ \
        EACH_ENUM_DEFINE( PREFIX_COUNT,        "")       /* enum count, must be the last one */


    enum PrefixType {
        DBK_PREFIX_LIST(EACH_ENUM_DEFINE_TYPE)
    };

    static const std::string gPrefixNames[PREFIX_COUNT + 1] = {
        DBK_PREFIX_LIST(EACH_ENUM_DEFINE_NAMES)
    };

    inline const std::string& GetKeyPrefix(PrefixType prefixType) {
        assert(prefixType >= 0 && prefixType <= PREFIX_COUNT);
        return gPrefixNames[prefixType];
    };

    template<typename PrefixElement>
    std::string GenDbKey(PrefixType keyPrefixType, PrefixElement element) {
        assert(keyPrefixType >= 0 && keyPrefixType <= PREFIX_COUNT);
        const string &prefix = GetKeyPrefix(keyPrefixType);
        CDataStream ssKeyTemp(SER_DISK, CLIENT_VERSION);
        ssKeyTemp.write(prefix.c_str(), prefix.size());
        ssKeyTemp << element;
        return std::string(ssKeyTemp.begin(), ssKeyTemp.end());
    }



}

static const string DB_NAME_CONTRACT = "contract";

typedef leveldb::Slice Slice;

class SliceIterator {
public:
    SliceIterator(Slice &sliceIn): slice(sliceIn) {}
    inline const char* begin() const { return slice.data(); };
    inline const char* end() const { return slice.data() + slice.size(); };
private:
    Slice &slice;
};

#endif //PERSIST_DBCONF_H