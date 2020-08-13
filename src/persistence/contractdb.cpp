// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "contractdb.h"
#include "entities/account.h"


#include "entities/account.h"
#include "entities/id.h"
#include "entities/key.h"
#include "commons/uint256.h"
#include "commons/util/util.h"
#include "vm/luavm/luavmrunenv.h"

#include <stdint.h>

using namespace std;

/************************ contract account ******************************/
bool CContractDBCache::GetContractAccount(const CRegID &contractRegId, const string &accountKey,
                                          CAppUserAccount &appAccOut) {
    auto key = std::make_pair(CRegIDKey(contractRegId), accountKey);
    return contractAccountCache.GetData(key, appAccOut);
}

bool CContractDBCache::SetContractAccount(const CRegID &contractRegId, const CAppUserAccount &appAccIn) {
    if (appAccIn.IsEmpty()) {
        return false;
    }
    auto key = std::make_pair(CRegIDKey(contractRegId), appAccIn.GetAccUserId());
    return contractAccountCache.SetData(key, appAccIn);
}

/************************ contract in cache ******************************/
bool CContractDBCache::GetContract(const CRegID &contractRegId, CUniversalContractStore &contractStore) {
    return contractCache.GetData(CRegIDKey(contractRegId), contractStore);
}

bool CContractDBCache::SaveContract(const CRegID &contractRegId, const CUniversalContractStore &contractStore) {
    return contractCache.SetData(CRegIDKey(contractRegId), contractStore);
}

bool CContractDBCache::HasContract(const CRegID &contractRegId) {
    return contractCache.HasData(CRegIDKey(contractRegId));
}

bool CContractDBCache::EraseContract(const CRegID &contractRegId) {
    return contractCache.EraseData(CRegIDKey(contractRegId));
}

/************************ contract managed APP data ******************************/
bool CContractDBCache::GetContractData(const CRegID &contractRegId, const string &contractKey, string &contractData) {
    auto key = std::make_pair(CRegIDKey(contractRegId), contractKey);
    return contractDataCache.GetData(key, contractData);
}

bool CContractDBCache::SetContractData(const CRegID &contractRegId, const string &contractKey,
                                       const string &contractData) {
    auto key = std::make_pair(CRegIDKey(contractRegId), contractKey);
    return contractDataCache.SetData(key, contractData);
}

bool CContractDBCache::HasContractData(const CRegID &contractRegId, const string &contractKey) {
    auto key = std::make_pair(CRegIDKey(contractRegId), contractKey);
    return contractDataCache.HasData(key);
}

bool CContractDBCache::EraseContractData(const CRegID &contractRegId, const string &contractKey) {
    auto key = std::make_pair(CRegIDKey(contractRegId), contractKey);
    return contractDataCache.EraseData(key);
}

bool CContractDBCache::GetContractTraces(const uint256 &txid, string &contractTraces) {
    return contractTracesCache.GetData(txid, contractTraces);
}

bool CContractDBCache::SetContractTraces(const uint256 &txid, const string &contractTraces) {
    return contractTracesCache.SetData(txid, contractTraces);
}

bool CContractDBCache::GetContractLogs(const uint256 &txid, string &contractLogs) {
    return contractLogsCache.GetData(txid, contractLogs);
}

bool CContractDBCache::SetContractLogs(const uint256 &txid, const string &contractLogs) {
    return contractLogsCache.SetData(txid, contractLogs);
}

bool CContractDBCache::Flush() {
    contractCache.Flush();
    contractDataCache.Flush();
    contractAccountCache.Flush();
    contractTracesCache.Flush();
    contractLogsCache.Flush();

    return true;
}

uint32_t CContractDBCache::GetCacheSize() const {
    return contractCache.GetCacheSize() +
        contractDataCache.GetCacheSize() +
        contractTracesCache.GetCacheSize() +
        contractLogsCache.GetCacheSize();
}

shared_ptr<CDBContractDataIterator> CContractDBCache::CreateContractDataIterator(const CRegID &contractRegid,
        const string &contractKeyPrefix) {

    if (contractKeyPrefix.size() > CDBContractKey::MAX_KEY_SIZE) {
        LogPrint(BCLog::ERROR, "CContractDBCache::CreateContractDatasGetter() contractKeyPrefix.size()=%u "
                 "exceeded the max size=%u", contractKeyPrefix.size(), CDBContractKey::MAX_KEY_SIZE);
        return nullptr;
    }
    return make_shared<CDBContractDataIterator>(contractDataCache, contractRegid, contractKeyPrefix);
}