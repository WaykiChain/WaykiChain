// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_DBCONF_H
#define PERSIST_DBCONF_H

#include <string>

using namespace std;

namespace dbconf {

static const string DBK_RedIndex            {"ridx"}; // rind --> 1 | 0
static const string DBK_Flag                {"flag"}; // flag --> 1 | 0

static const string DBK_Vote                {"vote"}; // vote{(uint64t)MAX - $votedBcoins}_{$RegId} --> 1

static const string DBK_KeyId2TxId          {"idtx"}; // idtx{$KeyId}{$Height}{$Index} --> $txid
static const string DBK_TxId2KeyIds         {"tids"}; // tids{$txid} --> {$KeyId1, $KeyId2, ...}
static const string DBK_TxId2DiskIndex      {"tidx"}; // tidx{$txid} --> $DiskTxPos

static const string DBK_RegId2KeyId         {"rkey"}; // rkey{$RegID} --> $KeyId
static const string DBK_NickId2KeyId        {"nkey"}; // nkey{$NickID} --> $KeyId
static const string DBK_KeyId2Account       {"idac"}; // idac{$KeyID} --> $CAccount

static const string DBK_PersistBlockHash    {"pbkh"}; // pbkh --> $BlockHash
static const string DBK_PersistBlockFile    {"pbfl"}; // pbfl --> $nFile

static const string DBK_ContractCount       {"cnum"}; // cnum{$ContractRegId} --> $total_num_of_cntracts
static const string DBK_ContractDef         {"cdef"}; // cdef{$ContractRegId} --> $ContractContent
static const string DBK_ContractData        {"cdat"}; // cdat{$RegId}_{$DataKey} --> $Data
static const string DBK_ContractTxOut       {"cout"}; // cout{txid} --> $VmOperateOutput
static const string DBK_ContractItemNum     {"citn"}; // citn{ContractRegId} --> $total_num_of_contract_items

static const string DBK_StakeFcoin          {"fcoin"}; // fcoin{(uint64t)MAX - stakedFcoins}_{RegId} --> 1

}

#endif //PERSIST_DBCONF_H