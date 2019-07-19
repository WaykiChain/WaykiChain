// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contractdb.h"

#include "entities/account.h"
#include "entities/id.h"
#include "entities/key.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include "vm/luavm/vmrunenv.h"

#include <stdint.h>

using namespace std;

/************************ contract account ******************************/
bool CContractDBCache::GetContractAccount(const CRegID &contractRegId, const string &accountKey,
                                          CAppUserAccount &appAccOut) {
    auto key = std::make_pair(contractRegId.ToRawString(), accountKey);
    return contractAccountCache.GetData(key, appAccOut);
}

bool CContractDBCache::SetContractAccount(const CRegID &contractRegId, const CAppUserAccount &appAccIn,
                                          CDBOpLogMap &dbOpLogMap) {
    if (appAccIn.IsEmpty()) {
        return false;
    }
    auto key = std::make_pair(contractRegId.ToRawString(), appAccIn.GetAccUserId());
    return contractAccountCache.SetData(key, appAccIn, dbOpLogMap);
}

bool CContractDBCache::UndoContractAccount(CDBOpLogMap &dbOpLogMap) {
    return contractAccountCache.UndoData(dbOpLogMap);
}

/************************ contract script ******************************/
bool CContractDBCache::GetContractScript(const CRegID &contractRegId, string &contractScript) {
    return scriptCache.GetData(contractRegId.ToRawString(), contractScript);
}

bool CContractDBCache::SetContractScript(const CRegID &contractRegId, const string &contractScript) {
    return scriptCache.SetData(contractRegId.ToRawString(), contractScript);
}

bool CContractDBCache::HaveContractScript(const CRegID &contractRegId) {
    return scriptCache.HaveData(contractRegId.ToRawString());
}

bool CContractDBCache::EraseContractScript(const CRegID &contractRegId) {
    return scriptCache.EraseData(contractRegId.ToRawString());
}

/************************ contract data ******************************/
bool CContractDBCache::GetContractData(const CRegID &contractRegId, const string &contractKey, string &contractData) {
    auto key = std::make_pair(contractRegId.ToRawString(), contractKey);
    return contractDataCache.GetData(key, contractData);
}

bool CContractDBCache::SetContractData(const CRegID &contractRegId, const string &contractKey,
                                       const string &contractData, CDBOpLogMap &dbOpLogMap) {
    auto key = std::make_pair(contractRegId.ToRawString(), contractKey);
    return contractDataCache.SetData(key, contractData, dbOpLogMap);
}

bool CContractDBCache::HaveContractData(const CRegID &contractRegId, const string &contractKey) {
    auto key = std::make_pair(contractRegId.ToRawString(), contractKey);
    return contractDataCache.HaveData(key);
}

bool CContractDBCache::EraseContractData(const CRegID &contractRegId, const string &contractKey,
                                         CDBOpLogMap &dbOpLogMap) {
    auto key = std::make_pair(contractRegId.ToRawString(), contractKey);
    return contractDataCache.EraseData(key, dbOpLogMap);
}

bool CContractDBCache::UndoContractData(CDBOpLogMap &dbOpLogMap) { return contractDataCache.UndoData(dbOpLogMap); }

bool CContractDBCache::GetContractScripts(map<string, string> &contractScript) {
    return scriptCache.GetAllElements(contractScript);
}

bool CContractDBCache::GetContractData(const CRegID &contractRegId, vector<std::pair<string, string>> &contractData) {
    map<std::pair<string, string>, string> elements;
    if (!contractDataCache.GetAllElements(contractRegId.ToRawString(), elements)) {
        return false;
    }

    for (const auto item : elements) {
        contractData.emplace_back(std::get<1>(item.first), item.second);
    }

    return true;
}

bool CContractDBCache::Flush() {
    scriptCache.Flush();
    txOutputCache.Flush();
    acctTxListCache.Flush();
    txDiskPosCache.Flush();
    contractRelatedKidCache.Flush();
    contractDataCache.Flush();
    contractAccountCache.Flush();

    return true;
}

uint32_t CContractDBCache::GetCacheSize() const {
    return scriptCache.GetCacheSize() +
        txOutputCache.GetCacheSize() +
        acctTxListCache.GetCacheSize() +
        txDiskPosCache.GetCacheSize() +
        contractRelatedKidCache.GetCacheSize() +
        contractDataCache.GetCacheSize() +
        contractAccountCache.GetCacheSize();
}

bool CContractDBCache::WriteTxOutput(const uint256 &txid, const vector<CVmOperate> &vOutput, CDBOpLogMap &dbOpLogMap) {
    return txOutputCache.SetData(txid, vOutput, dbOpLogMap);
}

bool CContractDBCache::SetTxHashByAddress(const CKeyID &keyId, uint32_t height, uint32_t index, const uint256 &txid,
                                          CDBOpLogMap &dbOpLogMap) {
    auto key = make_tuple(keyId, height, index);
    return acctTxListCache.SetData(key, txid, dbOpLogMap);
}

bool CContractDBCache::UndoTxHashByAddress(CDBOpLogMap &dbOpLogMap) { return acctTxListCache.UndoData(dbOpLogMap); }

bool CContractDBCache::GetTxHashByAddress(const CKeyID &keyId, uint32_t height, map<string, string> &mapTxHash) {
    return false;
    /* TODO: implements get list in cache
        pBase->GetTxHashByAddress(keyId, nHeight, mapTxHash);

        string vPreKey = {'A', 'D', 'D', 'R'};
        CDataStream ds1(SER_DISK, CLIENT_VERSION);
        ds1 << keyId;
        ds1 << nHeight;
        vPreKey.insert(vPreKey.end(), ds1.begin(), ds1.end());

        map<string, string >::iterator iterFindKey =
            mapContractDb.upper_bound(vPreKey);
        while (iterFindKey != mapContractDb.end()) {
            if (0 == memcmp((char *)&iterFindKey->first[0], (char *)&vPreKey[0], 28)) {
                if (iterFindKey->second.empty())
                    mapTxHash.erase(iterFindKey->first);
                else {
                    mapTxHash.insert(make_pair(iterFindKey->first, iterFindKey->second));
                }
            } else {
                break;
            }
        }
        return true;
    */
}

bool CContractDBCache::GetContractAccounts(const CRegID &scriptId, map<string, string> &mapAcc) {
    return false;
    /* TODO: GetContractAccounts
    return pBase->GetContractAccounts(scriptId, mapAcc);
    */
}

bool CContractDBCache::GetTxOutput(const uint256 &txid, vector<CVmOperate> &vOutput) {
    vector<CVmOperate> value;
    if (!txOutputCache.GetData(txid, value))
        return false;
    return true;
}

bool CContractDBCache::UndoTxOutput(CDBOpLogMap &dbOpLogMap) { return txOutputCache.UndoData(dbOpLogMap); }

bool CContractDBCache::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) { return txDiskPosCache.GetData(txid, pos); }

bool CContractDBCache::WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list, CDBOpLogMap &dbOpLogMap) {
    for (auto it : list) {
        LogPrint("txindex", "txid:%s dispos: nFile=%d, nPos=%d nTxOffset=%d\n", it.first.GetHex(), it.second.nFile,
                 it.second.nPos, it.second.nTxOffset);

        if (!txDiskPosCache.SetData(it.first, it.second, dbOpLogMap)) {
            return false;
        }
    }
    return true;
}

bool CContractDBCache::SetTxRelAccout(const uint256 &txid, const set<CKeyID> &relAccount) {
    return contractRelatedKidCache.SetData(txid, relAccount);
}
bool CContractDBCache::GetTxRelAccount(const uint256 &txid, set<CKeyID> &relAccount) {
    return contractRelatedKidCache.GetData(txid, relAccount);
}

bool CContractDBCache::EraseTxRelAccout(const uint256 &txid) { return contractRelatedKidCache.EraseData(txid); }