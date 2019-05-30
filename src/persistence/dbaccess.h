// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DBACCESS_H
#define PERSIST_DBACCESS_H

#include "leveldbwrapper.h"

#include <vector>

enum DBK_PrefixType {
    DBK_RedIndex = 0,
    DBK_Flag,
    DBK_Vote,
    DBK_KeyId2TxId,
    DBK_TxId2KeyIds,
    DBK_TxId2DiskIndex,
    DBK_RegId2KeyId,
    DBK_NickId2KeyId,
    DBK_KeyId2Account,
    DBK_PersistBlockHash,
    DBK_PersistBlockFile,
    DBK_ContractCount,
    DBK_ContractDef,
    DBK_ContractData,
    DBK_ContractTxOut,
    DBK_ContractItemNum,
    DBK_StakeFcoin,

};

static const string DBK_PrefixList[] = {
    "ridx",  // rind --> 1 | 0
    "flag",  // flag --> 1 | 0
    "vote",  // vote{(uint64t)MAX - $votedBcoins}_{$RegId} --> 1
    "idtx",  // idtx{$KeyId}{$Height}{$Index} --> $txid
    "tids",  // tids{$txid} --> {$KeyId1, $KeyId2, ...}
    "tidx",  // tidx{$txid} --> $DiskTxPos
    "rkey",  // rkey{$RegID} --> $KeyId
    "nkey",  // nkey{$NickID} --> $KeyId
    "idac",  // idac{$KeyID} --> $CAccount
    "pbkh",  // pbkh --> $BlockHash
    "pbfl",  // pbfl --> $nFile
    "cnum",  // cnum{$ContractRegId} --> $total_num_of_cntracts
    "cdef",  // cdef{$ContractRegId} --> $ContractContent
    "cdat",  // cdat{$RegId}_{$DataKey} --> $Data
    "cout",  // cout{txid} --> $VmOperateOutput
    "citn",  // citn{ContractRegId} --> $total_num_of_contract_items
    "fcoin"  // fcoin{(uint64t)MAX - stakedFcoins}_{RegId} --> 1
};

class CDbAccess {
public:
    CDbAccess(CLevelDBWrapper &ldbWrapperIn): ldbWrapper(ldbWrapperIn) {};

    bool GetData(DBK_PrefixType keyPrefixType);

private:
    CLevelDBWrapper &ldbWrapper;
};

#endif //