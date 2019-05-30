// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DBCONF_H
#define PERSIST_DBCONF_H

#include <string>

using namespace std;

/**
 * database key 
 */
namespace dbk {

    #define EACH_ENUM_DEFINE_TYPE(enumType, enumName, description) enumType
    #define EACH_ENUM_DEFINE_NAME(enumType, enumName, description) enumName

        //                 type                name(prefix)       description
        //               ----------           ------------   -----------------------------
    #define DBK_PREFIX_LIST(EACH_ENUM_DEFINE) \
        EACH_ENUM_DEFINE( REINDEX,             "ridx",     "rind --> 1 | 0" ), \
        EACH_ENUM_DEFINE( FLAG,                "flag",     "flag --> 1 | 0" ), \
        EACH_ENUM_DEFINE( VOTE,                "vote",     "vote{(uint64t)MAX - $votedBcoins}_{$RegId} --> 1" ), \
        EACH_ENUM_DEFINE( KEYID_2_TXID,        "idtx",     "idtx{$KeyId}{$Height}{$Index} --> $txid" ), \
        EACH_ENUM_DEFINE( TXID_2_KEYIDS,       "tids",     "tids{$txid} --> {$KeyId1, $KeyId2, ...}" ), \
        EACH_ENUM_DEFINE( TXID_2_DISK_INDEX,   "tidx",     "tidx{$txid} --> $DiskTxPos" ), \
        EACH_ENUM_DEFINE( REGID_2_KEYID,       "rkey",     "rkey{$RegID} --> $KeyId" ), \
        EACH_ENUM_DEFINE( NICKID_2_KEYID,      "nkey",     "nkey{$NickID} --> $KeyId" ), \
        EACH_ENUM_DEFINE( KEYID_2_ACCOUNT,     "idac",     "idac{$KeyID} --> $CAccount" ), \
        EACH_ENUM_DEFINE( PERSIST_BLOCK_HASH,  "pbkh",     "pbkh --> $BlockHash" ), \
        EACH_ENUM_DEFINE( PERSIST_BLOCK_FILE,  "pbfl",     "pbfl --> $nFile" ), \
        EACH_ENUM_DEFINE( CONTRACT_COUNT,      "cnum",     "cnum{$ContractRegId} --> $total_num_of_cntracts" ), \
        EACH_ENUM_DEFINE( CONTRACT_DEF,        "cdef",     "cdef{$ContractRegId} --> $ContractContent" ), \
        EACH_ENUM_DEFINE( CONTRACT_DATA,       "cdat",     "cdat{$RegId}_{$DataKey} --> $Data" ), \
        EACH_ENUM_DEFINE( CONTRACT_TX_OUT,     "cout",     "cout{txid} --> $VmOperateOutput" ), \
        EACH_ENUM_DEFINE( CONTRACT_ITEM_NUM,   "citn",     "citn{ContractRegId} --> $total_num_of_contract_i" ), \
        EACH_ENUM_DEFINE( STAKE_FCOIN,         "fcoin",    "fcoin{(uint64t)MAX - stakedFcoins}_{RegId} --> 1" ),


    enum PrefixType {
        DBK_PREFIX_LIST(EACH_ENUM_DEFINE_TYPE)
    };

    static const int PREFIX_COUNT = PrefixType::STAKE_FCOIN + 1;

    static const std::string gPrefixNames[PREFIX_COUNT] = {
        DBK_PREFIX_LIST(EACH_ENUM_DEFINE_NAME)
    };

    template<typename PrefixElement>
    std::string GenDbKey(PrefixType keyPrefixType, PrefixElement element) {
        assert(keyPrefixType >= 0 && keyPrefixType <= PREFIX_COUNT);
        const string &prefix = gPrefixNames[ keyPrefixType ];
        CDataStream ssKeyTemp(SER_DISK, CLIENT_VERSION);
        ssKeyTemp.write(prefix.c_str(), prefix.size());
        ssKeyTemp << element;
        return std::string(ssKeyTemp.begin(), ssKeyTemp.end());
    }

}

#endif //PERSIST_DBCONF_H