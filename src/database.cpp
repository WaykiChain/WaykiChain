#include "database.h"
#include <algorithm>
#include "chainparams.h"
#include "core.h"
#include "main.h"
#include "serialize.h"
#include "util.h"
#include "vm/vmrunenv.h"

bool CAccountView::GetAccount(const CKeyID &keyId, CAccount &account) { return false; }
bool CAccountView::SetAccount(const CKeyID &keyId, const CAccount &account) { return false; }
bool CAccountView::SetAccount(const vector<unsigned char> &accountId, const CAccount &account) { return false; }
bool CAccountView::HaveAccount(const CKeyID &keyId) { return false; }
uint256 CAccountView::GetBestBlock() { return uint256(); }
bool CAccountView::SetBestBlock(const uint256 &hashBlock) { return false; }
bool CAccountView::BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<vector<unsigned char>, CKeyID> &mapKeyIds, const uint256 &hashBlock) { return false; }
bool CAccountView::BatchWrite(const vector<CAccount> &vAccounts) { return false; }
bool CAccountView::EraseAccount(const CKeyID &keyId) { return false; }
bool CAccountView::SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId) { return false; }
bool CAccountView::GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId) { return false; }
bool CAccountView::GetAccount(const vector<unsigned char> &accountId, CAccount &account) { return false; }
bool CAccountView::EraseKeyId(const vector<unsigned char> &accountId) { return false; }
bool CAccountView::SaveAccountInfo(const vector<unsigned char> &accountId, const CKeyID &keyId, const CAccount &account) { return false; }

Object CAccountView::ToJsonObj(char Prefix) { return Object(); }

std::tuple<uint64_t, uint64_t> CAccountViewBacked::TraverseAccount() { return pBase->TraverseAccount(); }

CAccountViewBacked::CAccountViewBacked(CAccountView &accountView) : pBase(&accountView) {}

bool CAccountViewBacked::GetAccount(const CKeyID &keyId, CAccount &account) {
    return pBase->GetAccount(keyId, account);
}
bool CAccountViewBacked::SetAccount(const CKeyID &keyId, const CAccount &account) {
    return pBase->SetAccount(keyId, account);
}
bool CAccountViewBacked::SetAccount(const vector<unsigned char> &accountId, const CAccount &account) {
    return pBase->SetAccount(accountId, account);
}
bool CAccountViewBacked::HaveAccount(const CKeyID &keyId) {
    return pBase->HaveAccount(keyId);
}
uint256 CAccountViewBacked::GetBestBlock() {
    return pBase->GetBestBlock();
}
bool CAccountViewBacked::SetBestBlock(const uint256 &hashBlock) {
    return pBase->SetBestBlock(hashBlock);
}
bool CAccountViewBacked::BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<std::vector<unsigned char>, CKeyID> &mapKeyIds, const uint256 &hashBlock) {
    return pBase->BatchWrite(mapAccounts, mapKeyIds, hashBlock);
}
bool CAccountViewBacked::BatchWrite(const vector<CAccount> &vAccounts) {
    return pBase->BatchWrite(vAccounts);
}
bool CAccountViewBacked::EraseAccount(const CKeyID &keyId) {
    return pBase->EraseAccount(keyId);
}
bool CAccountViewBacked::SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId) {
    return pBase->SetKeyId(accountId, keyId);
}
bool CAccountViewBacked::GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId) {
    return pBase->GetKeyId(accountId, keyId);
}
bool CAccountViewBacked::EraseKeyId(const vector<unsigned char> &accountId) {
    return pBase->EraseKeyId(accountId);
}
bool CAccountViewBacked::GetAccount(const vector<unsigned char> &accountId, CAccount &account) {
    return pBase->GetAccount(accountId, account);
}
bool CAccountViewBacked::SaveAccountInfo(const vector<unsigned char> &accountId, const CKeyID &keyId,
                                         const CAccount &account) {
    return pBase->SaveAccountInfo(accountId, keyId, account);
}

CAccountViewCache::CAccountViewCache(CAccountView &accountView, bool fDummy) :
    CAccountViewBacked(accountView), hashBlock(uint256()) {}

bool CAccountViewCache::GetAccount(const CKeyID &keyId, CAccount &account) {
    if (cacheAccounts.count(keyId)) {
        if (cacheAccounts[keyId].keyID != uint160()) {
            account = cacheAccounts[keyId];
            return true;
        } else
            return false;
    }
    if (pBase->GetAccount(keyId, account)) {
        cacheAccounts.insert(make_pair(keyId, account));
        //cacheAccounts[keyId] = account;
        return true;
    }
    return false;
}
bool CAccountViewCache::SetAccount(const CKeyID &keyId, const CAccount &account) {
    cacheAccounts[keyId] = account;
    return true;
}
bool CAccountViewCache::SetAccount(const vector<unsigned char> &accountId, const CAccount &account) {
    if (accountId.empty()) {
        return false;
    }
    if (cacheKeyIds.count(accountId)) {
        cacheAccounts[cacheKeyIds[accountId]] = account;
        return true;
    }
    return false;
}
bool CAccountViewCache::HaveAccount(const CKeyID &keyId) {
    if (cacheAccounts.count(keyId))
        return true;
    else
        return pBase->HaveAccount(keyId);
}
uint256 CAccountViewCache::GetBestBlock() {
    if (hashBlock == uint256())
        return pBase->GetBestBlock();
    return hashBlock;
}
bool CAccountViewCache::SetBestBlock(const uint256 &hashBlockIn) {
    hashBlock = hashBlockIn;
    return true;
}
bool CAccountViewCache::BatchWrite(const map<CKeyID, CAccount> &mapAccounts, const map<vector<unsigned char>, CKeyID> &mapKeyIds, const uint256 &hashBlockIn) {
    for (map<CKeyID, CAccount>::const_iterator it = mapAccounts.begin(); it != mapAccounts.end(); ++it) {
        if (uint160() == it->second.keyID) {
            pBase->EraseAccount(it->first);
            cacheAccounts.erase(it->first);
        } else {
            cacheAccounts[it->first] = it->second;
        }
    }

    for (map<vector<unsigned char>, CKeyID>::const_iterator itKeyId = mapKeyIds.begin(); itKeyId != mapKeyIds.end(); ++itKeyId)
        cacheKeyIds[itKeyId->first] = itKeyId->second;
    hashBlock = hashBlockIn;
    return true;
}
bool CAccountViewCache::BatchWrite(const vector<CAccount> &vAccounts) {
    for (vector<CAccount>::const_iterator it = vAccounts.begin(); it != vAccounts.end(); ++it) {
        if (it->IsEmptyValue() && !it->IsRegistered()) {
            cacheAccounts[it->keyID]       = *it;
            cacheAccounts[it->keyID].keyID = uint160();
        } else {
            cacheAccounts[it->keyID] = *it;
        }
    }
    return true;
}
bool CAccountViewCache::EraseAccount(const CKeyID &keyId) {
    if (cacheAccounts.count(keyId))
        cacheAccounts[keyId].keyID = uint160();
    else {
        CAccount account;
        if (pBase->GetAccount(keyId, account)) {
            account.keyID        = uint160();
            cacheAccounts[keyId] = account;
        }
    }
    return true;
}

bool CAccountViewCache::SetKeyId(const vector<unsigned char> &accountId, const CKeyID &keyId) {
    if (accountId.empty())
        return false;
    cacheKeyIds[accountId] = keyId;
    return true;
}

bool CAccountViewCache::GetKeyId(const vector<unsigned char> &accountId, CKeyID &keyId)
{
    if (accountId.empty())
        return false;

    if (cacheKeyIds.count(accountId)) {
        keyId = cacheKeyIds[accountId];
        if (keyId != uint160()) {
            return true;
        } else {
            return false;
        }
    }

    if (pBase->GetKeyId(accountId, keyId)) {
        cacheKeyIds.insert(make_pair(accountId, keyId));
        //cacheKeyIds[accountId] = keyId;
        return true;
    }
    return false;
}

bool CAccountViewCache::EraseKeyId(const vector<unsigned char> &accountId) {
    if (accountId.empty())
        return false;
    if (cacheKeyIds.count(accountId))
        cacheKeyIds[accountId] = uint160();
    else {
        CKeyID keyId;
        if (pBase->GetKeyId(accountId, keyId)) {
            cacheKeyIds[accountId] = uint160();
        }
    }
    return true;
}
bool CAccountViewCache::GetAccount(const vector<unsigned char> &accountId, CAccount &account) {
    if (accountId.empty()) {
        return false;
    }
    if (cacheKeyIds.count(accountId)) {
        CKeyID keyId(cacheKeyIds[accountId]);
        if (keyId != uint160()) {
            if (cacheAccounts.count(keyId)) {
                account = cacheAccounts[keyId];
                if (account.keyID != uint160()) {  // 判断此帐户是否被删除了
                    return true;
                } else {
                    return false;  //已删除返回false
                }
            } else {
                return pBase->GetAccount(keyId, account);  //缓存map中没有，从上级存取
            }
        } else {
            return false;  //accountId已删除说明账户信息也已删除
        }
    } else {
        CKeyID keyId;
        if (pBase->GetKeyId(accountId, keyId)) {
            cacheKeyIds[accountId] = keyId;
            if (cacheAccounts.count(keyId) > 0) {
                account = cacheAccounts[keyId];
                if (account.keyID != uint160()) {  // 判断此帐户是否被删除了
                    return true;
                } else {
                    return false;  //已删除返回false
                }
            }
            bool ret = pBase->GetAccount(keyId, account);
            if (ret) {
                cacheAccounts[keyId] = account;
                return true;
            }
        }
    }
    return false;
}

bool CAccountViewCache::SaveAccountInfo(const CRegID &regid, const CKeyID &keyId, const CAccount &account) {
    cacheKeyIds[regid.GetVec6()] = keyId;
    cacheAccounts[keyId]         = account;
    return true;
}

bool CAccountViewCache::GetAccount(const CUserID &userId, CAccount &account) {
    bool ret = false;
    if (userId.type() == typeid(CRegID)) {
        ret = GetAccount(boost::get<CRegID>(userId).GetVec6(), account);
        //		if(ret) assert(boost::get<CRegID>(userId) == account.regID);
    } else if (userId.type() == typeid(CKeyID)) {
        ret = GetAccount(boost::get<CKeyID>(userId), account);
        //		if(ret) assert(boost::get<CKeyID>(userId) == account.keyID);
    } else if (userId.type() == typeid(CPubKey)) {
        ret = GetAccount(boost::get<CPubKey>(userId).GetKeyID(), account);
        //		if(ret) assert((boost::get<CPubKey>(userId)).GetKeyID() == account.keyID);
    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("GetAccount input userid can't be CNullID type");
    }
    return ret;
}

bool CAccountViewCache::GetKeyId(const CUserID &userId, CKeyID &keyId) {
    if (userId.type() == typeid(CRegID)) {
        return GetKeyId(boost::get<CRegID>(userId).GetVec6(), keyId);
    } else if (userId.type() == typeid(CPubKey)) {
        keyId = boost::get<CPubKey>(userId).GetKeyID();
        return true;
    } else if (userId.type() == typeid(CKeyID)) {
        keyId = boost::get<CKeyID>(userId);
        return true;
    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("GetKeyId input userid can't be CNullID type");
    }
    return ERRORMSG("GetKeyId input userid is unknow type");
}

bool CAccountViewCache::SetKeyId(const CUserID &userId, const CKeyID &keyId) {
    if (userId.type() == typeid(CRegID)) {
        return SetKeyId(boost::get<CRegID>(userId).GetVec6(), keyId);
    } else {
        //		assert(0);
    }
    return false;
}

bool CAccountViewCache::GetUserId(const string &addr, CUserID &userId) {
    CRegID regId(addr);
    if (!regId.IsEmpty()) {
        userId = regId;
        return true;
    }

    CKeyID keyId(addr);
    if (!keyId.IsEmpty()) {
        userId = keyId;
        return true;
    }

    return false;
}

bool CAccountViewCache::GetRegId(const CKeyID &keyId, CRegID &regId) {
    CAccount acct;
    if (CAccountViewCache::GetAccount(CUserID(keyId), acct)) {
        regId = acct.regID;
        return true;
    } else {
        //throw runtime_error(tinyformat::format("GetRegId :account id %s not exist\n", keyId.ToAddress()));
        return false;
    }
}

bool CAccountViewCache::GetRegId(const CUserID &userId, CRegID &regId) const {
    CAccountViewCache tempView(*this);
    CAccount account;
    if (userId.type() == typeid(CRegID)) {
        regId = boost::get<CRegID>(userId);
        return true;
    }
    if (tempView.GetAccount(userId, account)) {
        regId = account.regID;
        return !regId.IsEmpty();
    }
    return false;
}

bool CAccountViewCache::SetAccount(const CUserID &userId, const CAccount &account) {
    if (userId.type() == typeid(CRegID)) {
        return SetAccount(boost::get<CRegID>(userId).GetVec6(), account);
    } else if (userId.type() == typeid(CKeyID)) {
        return SetAccount(boost::get<CKeyID>(userId), account);
    } else if (userId.type() == typeid(CPubKey)) {
        return SetAccount(boost::get<CPubKey>(userId).GetKeyID(), account);
    } else if (userId.type() == typeid(CNullID)) {
        return ERRORMSG("SetAccount input userid can't be CNullID type");
    }
    return ERRORMSG("SetAccount input userid is unknow type");
}

bool CAccountViewCache::EraseAccount(const CUserID &userId) {
    if (userId.type() == typeid(CKeyID)) {
        return EraseAccount(boost::get<CKeyID>(userId));
    } else if (userId.type() == typeid(CPubKey)) {
        return EraseAccount(boost::get<CPubKey>(userId).GetKeyID());
    } else {
        return ERRORMSG("EraseAccount account type error!");
        //		assert(0);
    }
    return false;
}
bool CAccountViewCache::HaveAccount(const CUserID &userId) {
    if (userId.type() == typeid(CKeyID)) {
        return HaveAccount(boost::get<CKeyID>(userId));
    } else {
        //		assert(0);
    }
    return false;
}
bool CAccountViewCache::EraseId(const CUserID &userId) {
    if (userId.type() == typeid(CRegID)) {
        return EraseKeyId(boost::get<CRegID>(userId).GetVec6());
    } else {
        //		assert(0);
    }
    return false;
}

bool CAccountViewCache::Flush() {
    bool fOk = pBase->BatchWrite(cacheAccounts, cacheKeyIds, hashBlock);
    if (fOk) {
        cacheAccounts.clear();
        cacheKeyIds.clear();
    }
    return fOk;
}

int64_t CAccountViewCache::GetRawBalance(const CUserID &userId) const {
    CAccountViewCache tempvew(*this);
    CAccount account;
    if (tempvew.GetAccount(userId, account)) {
        return account.GetRawBalance();
    }
    return 0;
}

unsigned int CAccountViewCache::GetCacheSize() {
    return ::GetSerializeSize(cacheAccounts, SER_DISK, CLIENT_VERSION) + ::GetSerializeSize(cacheKeyIds, SER_DISK, CLIENT_VERSION);
}

Object CAccountViewCache::ToJsonObj() const {
    Object obj;
    Array arrayObj;
    obj.push_back(Pair("hashBlock", hashBlock.ToString()));
    arrayObj.push_back(pBase->ToJsonObj('a'));
    arrayObj.push_back(pBase->ToJsonObj('k'));
    obj.push_back(Pair("cacheView", arrayObj));
    //	Array arrayObj;
    //	for (auto& item : cacheAccounts) {
    //		Object obj;
    //		obj.push_back(Pair("keyID", item.first.ToString()));
    //		obj.push_back(Pair("account", item.second.ToString()));
    //		arrayObj.push_back(obj);
    //	}
    //	obj.push_back(Pair("cacheAccounts", arrayObj));
    //
    //	for (auto& item : cacheKeyIds) {
    //		Object obj;
    //		obj.push_back(Pair("accountID", HexStr(item.first)));
    //		obj.push_back(Pair("keyID", item.second.ToString()));
    //		arrayObj.push_back(obj);
    //	}
    //
    //	obj.push_back(Pair("cacheKeyIds", arrayObj));
    return obj;
}

void CAccountViewCache::SetBaseData(CAccountView *pNewBase) {
    pBase = pNewBase;
}

bool CScriptDBView::GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) { return false; }
bool CScriptDBView::SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) { return false; }
bool CScriptDBView::BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb) { return false; }
bool CScriptDBView::EraseKey(const vector<unsigned char> &vKey) { return false; }
bool CScriptDBView::HasData(const vector<unsigned char> &vKey) { return false; }
bool CScriptDBView::GetScript(const int &nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue) { return false; }
bool CScriptDBView::GetContractData(const int nCurBlockHeight, const vector<unsigned char> &vScriptId, const int &nIndex,
                                    vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData) {
    return false;
}
bool CScriptDBView::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) { return false; }
bool CScriptDBView::WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CScriptDBOperLog> &vTxIndexOperDB) { return false; }
bool CScriptDBView::WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CScriptDBOperLog &operLog) { return false; }
bool CScriptDBView::ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput) { return false; }
bool CScriptDBView::GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &vTxHash) { return false; }
bool CScriptDBView::SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CScriptDBOperLog &operLog) { return false; }
bool CScriptDBView::GetAllScriptAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc) { return false; }

Object CScriptDBView::ToJsonObj(string Prefix) { return Object(); }

CScriptDBViewBacked::CScriptDBViewBacked(CScriptDBView &dataBaseView) { pBase = &dataBaseView; }
bool CScriptDBViewBacked::GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) { return pBase->GetData(vKey, vValue); }
bool CScriptDBViewBacked::SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) { return pBase->SetData(vKey, vValue); }
bool CScriptDBViewBacked::BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapContractDb) { return pBase->BatchWrite(mapContractDb); }
bool CScriptDBViewBacked::EraseKey(const vector<unsigned char> &vKey) { return pBase->EraseKey(vKey); }
bool CScriptDBViewBacked::HasData(const vector<unsigned char> &vKey) { return pBase->HasData(vKey); }
bool CScriptDBViewBacked::GetScript(const int &nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue) { return pBase->GetScript(nIndex, vScriptId, vValue); }
bool CScriptDBViewBacked::GetContractData(const int nCurBlockHeight, const vector<unsigned char> &vScriptId,
                                          const int &nIndex, vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData) {
    return pBase->GetContractData(nCurBlockHeight, vScriptId, nIndex, vScriptKey, vScriptData);
}
bool CScriptDBViewBacked::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) { return pBase->ReadTxIndex(txid, pos); }
bool CScriptDBViewBacked::WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CScriptDBOperLog> &vTxIndexOperDB) { return pBase->WriteTxIndex(list, vTxIndexOperDB); }
bool CScriptDBViewBacked::WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CScriptDBOperLog &operLog) { return pBase->WriteTxOutPut(txid, vOutput, operLog); }
bool CScriptDBViewBacked::ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput) { return pBase->ReadTxOutPut(txid, vOutput); }
bool CScriptDBViewBacked::GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &vTxHash) { return pBase->GetTxHashByAddress(keyId, nHeight, vTxHash); }
bool CScriptDBViewBacked::SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CScriptDBOperLog &operLog) { return pBase->SetTxHashByAddress(keyId, nHeight, nIndex, strTxHash, operLog); }
bool CScriptDBViewBacked::GetAllScriptAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc) { return pBase->GetAllScriptAcc(scriptId, mapAcc); }

CScriptDBViewCache::CScriptDBViewCache(CScriptDBView &base, bool fDummy) : CScriptDBViewBacked(base) {
    mapContractDb.clear();
}

bool CScriptDBViewCache::GetData(const vector<unsigned char> &vKey, vector<unsigned char> &vValue) {
    if (mapContractDb.count(vKey) > 0) {
        if (!mapContractDb[vKey].empty()) {
            vValue = mapContractDb[vKey];
            return true;
        } else {
            return false;
        }
    }

    if (!pBase->GetData(vKey, vValue)) {
        return false;
    }
    mapContractDb[vKey] = vValue;  //cache it here for speed in-mem access
    return true;
}

bool CScriptDBViewCache::SetData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) {
    mapContractDb[vKey] = vValue;
    return true;
}

bool CScriptDBViewCache::UndoScriptData(const vector<unsigned char> &vKey, const vector<unsigned char> &vValue) {
    vector<unsigned char> vPrefix(vKey.begin(), vKey.begin() + 4);
    vector<unsigned char> vScriptDataPrefix = {'d', 'a', 't', 'a'};
    if (vPrefix == vScriptDataPrefix) {
        assert(vKey.size() > 10);
        if (vKey.size() < 10) {
            return ERRORMSG("UndoScriptData() : vKey=%s error!\n", HexStr(vKey));
        }
        vector<unsigned char> vScriptCountKey = {'s', 'd', 'n', 'u', 'm'};
        vector<unsigned char> vScriptId(vKey.begin() + 4, vKey.begin() + 10);
        vector<unsigned char> vOldValue;
        if (mapContractDb.count(vKey)) {
            vOldValue = mapContractDb[vKey];
        } else {
            GetData(vKey, vOldValue);
        }
        vScriptCountKey.insert(vScriptCountKey.end(), vScriptId.begin(), vScriptId.end());
        CDataStream ds(SER_DISK, CLIENT_VERSION);

        int nCount(0);
        if (vValue.empty()) {  //key所对应的值由非空设置为空，计数减1
            if (!vOldValue.empty()) {
                if (!GetContractItemCount(vScriptId, nCount))
                    return false;
                --nCount;
                if (!SetContractItemCount(vScriptId, nCount))
                    return false;
            }
        } else {  //key所对应的值由空设置为非空，计数加1
            if (vOldValue.empty()) {
                GetContractItemCount(vScriptId, nCount);
                ++nCount;
                if (!SetContractItemCount(vScriptId, nCount))
                    return false;
            }
        }
    }
    mapContractDb[vKey] = vValue;
    return true;
}

bool CScriptDBViewCache::BatchWrite(const map<vector<unsigned char>, vector<unsigned char> > &mapData) {
    for (auto &items : mapData) {
        mapContractDb[items.first] = items.second;
    }
    return true;
}

bool CScriptDBViewCache::EraseKey(const vector<unsigned char> &vKey) {
    if (mapContractDb.count(vKey) > 0) {
        mapContractDb[vKey].clear();
    } else {
        vector<unsigned char> vValue;
        if (pBase->GetData(vKey, vValue)) {
            vValue.clear();
            mapContractDb[vKey] = vValue;
        } else {
            return false;
        }
    }
    return true;
}

bool CScriptDBViewCache::HasData(const vector<unsigned char> &vKey) {
    if (mapContractDb.count(vKey) > 0) {
        if (!mapContractDb[vKey].empty())
            return true;
        else
            return false;
    }
    return pBase->HasData(vKey);
}

bool CScriptDBViewCache::GetScript(const int nIndex, vector<unsigned char> &vScriptId, vector<unsigned char> &vValue) {
    if (0 == nIndex) {
        vector<unsigned char> scriptKey = {'d', 'e', 'f'};
        vector<unsigned char> vDataKey;
        vector<unsigned char> vDataValue;
        vDataKey.clear();
        vDataValue.clear();
        for (auto &item : mapContractDb) {  //遍历本级缓存数据，找出合法的最小的key值
            vector<unsigned char> vTemp(item.first.begin(), item.first.begin() + 3);
            if (scriptKey == vTemp) {
                if (item.second.empty()) {
                    continue;
                }
                vDataKey   = item.first;
                vDataValue = item.second;
                break;
            }
        }
        if (!pBase->GetScript(nIndex, vScriptId, vValue)) {  //上级没有获取符合条件的key值
            if (vDataKey.empty())
                return false;
            else {  //返回本级缓存的查询结果
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        } else {                     //上级获取到符合条件的key值
            if (vDataKey.empty()) {  //缓存中没有符合条件的key，直接返回上级的查询结果
                return true;
            }
            vector<unsigned char> dataKeyTemp = {'d', 'e', 'f'};
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptId.begin(), vScriptId.end());  //上级得到的key值
            if (dataKeyTemp < vDataKey) {                                               //若上级查询的key小于本级缓存的key,且此key在缓存中没有，则直接返回数据库中查询的结果
                if (mapContractDb.count(dataKeyTemp) == 0)
                    return true;
                else {
                    mapContractDb[dataKeyTemp].clear();           //在缓存中dataKeyTemp已经被删除过了，重新将此key对应的value清除
                    return GetScript(nIndex, vScriptId, vValue);  //重新从数据库中获取下一条数据
                }
            } else {  //若上级查询的key大于等于本级缓存的key,返回本级的数据
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        }
    } else if (1 == nIndex) {
        vector<unsigned char> vKey = {'d', 'e', 'f'};
        vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
        vector<unsigned char> vPreKey(vKey);
        map<vector<unsigned char>, vector<unsigned char> >::iterator iterFindKey = mapContractDb.upper_bound(vPreKey);
        vector<unsigned char> vDataKey;
        vector<unsigned char> vDataValue;
        vDataKey.clear();
        vDataValue.clear();
        vector<unsigned char> vKeyTemp = {'d', 'e', 'f'};
        while (iterFindKey != mapContractDb.end()) {
            vector<unsigned char> vTemp(iterFindKey->first.begin(), iterFindKey->first.begin() + 3);
            if (vKeyTemp == vTemp) {
                if (iterFindKey->second.empty()) {
                    ++iterFindKey;
                    continue;
                } else {
                    vDataKey   = iterFindKey->first;
                    vDataValue = iterFindKey->second;
                    break;
                }
            } else {
                ++iterFindKey;
            }
        }
        if (!pBase->GetScript(nIndex, vScriptId, vValue)) {  //从BASE获取指定键值之后的下一个值
            if (vDataKey.empty())
                return false;
            else {
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        } else {
            if (vDataKey.empty())  //缓存中没有符合条件的key，直接返回上级的查询结果
                return true;
            vector<unsigned char> dataKeyTemp = {'d', 'e', 'f'};
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptId.begin(), vScriptId.end());  //上级得到的key值
            if (dataKeyTemp < vDataKey) {
                if (mapContractDb.count(dataKeyTemp) == 0)
                    return true;
                else {
                    mapContractDb[dataKeyTemp].clear();           //在缓存中dataKeyTemp已经被删除过了，重新将此key对应的value清除
                    return GetScript(nIndex, vScriptId, vValue);  //重新从数据库中获取下一条数据
                }
            } else {  //若上级查询的key大于等于本级缓存的key,返回本级的数据
                vScriptId.clear();
                vValue.clear();
                vScriptId.assign(vDataKey.begin() + 3, vDataKey.end());
                vValue = vDataValue;
                return true;
            }
        }
    } else {
        //		assert(0);
    }
    return true;
}
bool CScriptDBViewCache::SetScript(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vValue) {
    vector<unsigned char> scriptKey = {'d', 'e', 'f'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());

    if (!HaveScript(vScriptId)) {
        int nCount(0);
        GetScriptCount(nCount);
        ++nCount;
        if (!SetScriptCount(nCount))
            return false;
    }
    return SetData(scriptKey, vValue);
}
bool CScriptDBViewCache::Flush() {
    bool ok = pBase->BatchWrite(mapContractDb);
    if (ok) {
        mapContractDb.clear();
    }
    return ok;
}
unsigned int CScriptDBViewCache::GetCacheSize() {
    return ::GetSerializeSize(mapContractDb, SER_DISK, CLIENT_VERSION);
}

bool CScriptDBViewCache::WriteTxOutPut(const uint256 &txid, const vector<CVmOperate> &vOutput, CScriptDBOperLog &operLog) {
    vector<unsigned char> vKey = {'o', 'u', 't', 'p', 'u', 't'};
    CDataStream ds1(SER_DISK, CLIENT_VERSION);
    ds1 << txid;
    vKey.insert(vKey.end(), ds1.begin(), ds1.end());

    vector<unsigned char> vValue;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << vOutput;
    vValue.assign(ds.begin(), ds.end());

    vector<unsigned char> oldValue;
    oldValue.clear();
    GetData(vKey, oldValue);
    operLog = CScriptDBOperLog(vKey, oldValue);
    return SetData(vKey, vValue);
}

bool CScriptDBViewCache::SetTxHashByAddress(const CKeyID &keyId, int nHeight, int nIndex, const string &strTxHash, CScriptDBOperLog &operLog) {
    vector<unsigned char> vKey = {'A', 'D', 'D', 'R'};
    vector<unsigned char> oldValue;
    oldValue.clear();
    GetData(vKey, oldValue);
    operLog = CScriptDBOperLog(vKey, oldValue);

    CDataStream ds1(SER_DISK, CLIENT_VERSION);
    ds1 << keyId;
    ds1 << nHeight;
    ds1 << nIndex;
    vKey.insert(vKey.end(), ds1.begin(), ds1.end());
    vector<unsigned char> vValue(strTxHash.begin(), strTxHash.end());
    return SetData(vKey, vValue);
}

bool CScriptDBViewCache::GetTxHashByAddress(const CKeyID &keyId, int nHeight, map<vector<unsigned char>, vector<unsigned char> > &vTxHash) {
    pBase->GetTxHashByAddress(keyId, nHeight, vTxHash);

    vector<unsigned char> vPreKey = {'A', 'D', 'D', 'R'};
    CDataStream ds1(SER_DISK, CLIENT_VERSION);
    ds1 << keyId;
    ds1 << nHeight;
    vPreKey.insert(vPreKey.end(), ds1.begin(), ds1.end());

    map<vector<unsigned char>, vector<unsigned char> >::iterator iterFindKey = mapContractDb.upper_bound(vPreKey);
    while (iterFindKey != mapContractDb.end()) {
        if (0 == memcmp((char *)&iterFindKey->first[0], (char *)&vPreKey[0], 24)) {
            if (iterFindKey->second.empty())
                vTxHash.erase(iterFindKey->first);
            else {
                vTxHash.insert(make_pair(iterFindKey->first, iterFindKey->second));
            }
            ++iterFindKey;
        } else {
            break;
        }
    }
    return true;
}
bool CScriptDBViewCache::GetAllScriptAcc(const CRegID &scriptId, map<vector<unsigned char>, vector<unsigned char> > &mapAcc) { return pBase->GetAllScriptAcc(scriptId, mapAcc); }

bool CScriptDBViewCache::ReadTxOutPut(const uint256 &txid, vector<CVmOperate> &vOutput) {
    vector<unsigned char> vKey = {'o', 'u', 't', 'p', 'u', 't'};
    CDataStream ds1(SER_DISK, CLIENT_VERSION);
    ds1 << txid;
    vKey.insert(vKey.end(), ds1.begin(), ds1.end());
    vector<unsigned char> vValue;
    if (!GetData(vKey, vValue))
        return false;
    CDataStream ds(vValue, SER_DISK, CLIENT_VERSION);
    ds >> vOutput;
    return true;
}

bool CScriptDBViewCache::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << txid;
    vector<unsigned char> vTxHash = {'T'};
    vTxHash.insert(vTxHash.end(), ds.begin(), ds.end());
    vector<unsigned char> vTxPos;

    if (mapContractDb.count(vTxHash)) {
        if (mapContractDb[vTxHash].empty()) {
            return false;
        }
        vTxPos = mapContractDb[vTxHash];
        CDataStream dsPos(vTxPos, SER_DISK, CLIENT_VERSION);
        dsPos >> pos;
    } else {
        if (!GetData(vTxHash, vTxPos))
            return false;
        CDataStream dsPos(vTxPos, SER_DISK, CLIENT_VERSION);
        dsPos >> pos;
    }
    return true;
}
bool CScriptDBViewCache::WriteTxIndex(const vector<pair<uint256, CDiskTxPos> > &list, vector<CScriptDBOperLog> &vTxIndexOperDB) {
    for (vector<pair<uint256, CDiskTxPos> >::const_iterator it = list.begin(); it != list.end(); it++) {
        LogPrint("txindex", "txhash:%s dispos: nFile=%d, nPos=%d nTxOffset=%d\n", it->first.GetHex(), it->second.nFile, it->second.nPos, it->second.nTxOffset);
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << it->first;
        vector<unsigned char> vTxHash = {'T'};
        vTxHash.insert(vTxHash.end(), ds.begin(), ds.end());
        vector<unsigned char> vTxPos;
        CDataStream dsPos(SER_DISK, CLIENT_VERSION);
        dsPos << it->second;
        vTxPos.insert(vTxPos.end(), dsPos.begin(), dsPos.end());
        CScriptDBOperLog txIndexOper;
        txIndexOper.vKey = vTxHash;
        GetData(vTxHash, txIndexOper.vValue);
        vTxIndexOperDB.push_back(txIndexOper);
        if (!SetData(vTxHash, vTxPos))
            return false;
    }
    return true;
}

bool CScriptDBViewCache::GetScript(const vector<unsigned char> &vScriptId, vector<unsigned char> &vValue) {
    vector<unsigned char> scriptKey = {'d', 'e', 'f'};

    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());
    return GetData(scriptKey, vValue);
}
bool CScriptDBViewCache::GetScript(const CRegID &scriptId, vector<unsigned char> &vValue) {
    return GetScript(scriptId.GetVec6(), vValue);
}

bool CScriptDBViewCache::GetContractData(const int nCurBlockHeight, const vector<unsigned char> &vScriptId,
                                         const vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData) {
    //	assert(vScriptKey.size() == 8);
    vector<unsigned char> vKey = {'d', 'a', 't', 'a'};

    vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
    vKey.push_back('_');
    vKey.insert(vKey.end(), vScriptKey.begin(), vScriptKey.end());
    vector<unsigned char> vValue;
    if (!GetData(vKey, vValue))
        return false;
    if (vValue.empty())
        return false;
    vScriptData = vValue;
    //	CDataStream ds(vValue, SER_DISK, CLIENT_VERSION);
    //	ds >> vScriptData;
    return true;
}
bool CScriptDBViewCache::GetContractData(const int nCurBlockHeight, const vector<unsigned char> &vScriptId,
                                         const int &nIndex, vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData) {
    if (0 == nIndex) {
        vector<unsigned char> vKey = {'d', 'a', 't', 'a'};
        vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
        vKey.push_back('_');
        vector<unsigned char> vDataKey;
        vector<unsigned char> vDataValue;
        vDataKey.clear();
        vDataValue.clear();
        map<vector<unsigned char>, vector<unsigned char> >::iterator iterFindKey = mapContractDb.upper_bound(vKey);
        while (iterFindKey != mapContractDb.end()) {
            vector<unsigned char> vKeyTemp(vKey.begin(), vKey.begin() + vScriptId.size() + 5);
            vector<unsigned char> vTemp(iterFindKey->first.begin(), iterFindKey->first.begin() + vScriptId.size() + 5);
            if (vKeyTemp == vTemp) {
                if (iterFindKey->second.empty()) {
                    ++iterFindKey;
                    continue;
                } else {
                    vDataKey   = iterFindKey->first;
                    vDataValue = iterFindKey->second;
                    break;
                }
            } else {
                break;
            }
        }
        bool bUpLevelRet(false);
        int nIndexTemp = nIndex;
        while ((bUpLevelRet = pBase->GetContractData(nCurBlockHeight, vScriptId, nIndexTemp, vScriptKey, vScriptData))) {
            //			LogPrint("INFO", "nCurBlockHeight:%d this addr:0x%x, nIndex:%d, count:%lld\n ScriptKey:%s\n nHeight:%d\n ScriptData:%s\n vDataKey:%s\n vDataValue:%s\n",
            //					nCurBlockHeight, this, nIndexTemp, ++llCount, HexStr(vScriptKey), nHeight, HexStr(vScriptData), HexStr(vDataKey), HexStr(vDataValue));
            nIndexTemp = 1;
            vector<unsigned char> dataKeyTemp(vKey.begin(), vKey.end());
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptKey.begin(), vScriptKey.end());
            //			LogPrint("INFO", "dataKeyTemp:%s\n vDataKey:%s\n", HexStr(dataKeyTemp), HexStr(vDataKey));
            //			if(mapContractDb.count(dataKeyTemp) > 0) {//本级缓存包含上级查询结果的key
            //				if(dataKeyTemp != vDataKey) {  //本级和上级查找key不同，说明上级获取的数据在本级已被删除
            //					continue;
            //				} else {
            //					if(vDataValue.empty()) { //本级和上级数据key相同,且本级数据已经删除，重新从上级获取下一条数据
            //						LogPrint("INFO", "dataKeyTemp equal vDataKey and vDataValue empty redo getcontractdata()\n");
            //						continue;
            //					}
            //					vScriptKey.clear();
            //					vScriptData.clear();
            //					vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
            //					CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
            //					ds >> nHeight;
            //					ds >> vScriptData;
            //					return true;
            //				}
            //			} else {//本级缓存不包含上级查询结果key
            //				if (dataKeyTemp < vDataKey) { //上级获取key值小
            //					return true;      //返回上级的查询结果
            //				} else {              //本级查询结果Key值小，返回本级查询结果
            //					vScriptKey.clear();
            //					vScriptData.clear();
            //					vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
            //					CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
            //					ds >> nHeight;
            //					ds >> vScriptData;
            //					return true;
            //				}
            //			}
            if (vDataKey.empty()) {  //缓存中没有符合条件的key，直接返回上级的查询结果
                if (mapContractDb.count(dataKeyTemp) <= 0) {
                    //					CDataStream ds(vScriptData, SER_DISK, CLIENT_VERSION);
                    //					ds >> vScriptData;
                    return true;
                } else {
                    //					LogPrint("INFO", "local level contains dataKeyTemp,but the value is empty,need redo getcontractdata()\n");
                    continue;  //重新从数据库中获取下一条数据
                }
            } else {
                if (dataKeyTemp < vDataKey) {
                    if (mapContractDb.count(dataKeyTemp) <= 0) {
                        return true;
                    } else {
                        //						LogPrint("INFO", "dataKeyTemp less than vDataKey and vDataValue empty redo getcontractdata()\n");
                        continue;  //重新从数据库中获取下一条数据
                    }
                } else {
                    if (vDataValue.empty()) {  //本级和上级数据key相同,且本级数据已经删除，重新从上级获取下一条数据
                                               //						LogPrint("INFO", "dataKeyTemp equal vDataKey and vDataValue empty redo getcontractdata()\n");
                        continue;
                    }
                    vScriptKey.clear();
                    vScriptData.clear();
                    vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                    vScriptData = vDataValue;
                    //					CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
                    //					ds >> vScriptData;
                    return true;
                }
            }
        }
        if (!bUpLevelRet) {
            if (vDataKey.empty())
                return false;
            else {
                vScriptKey.clear();
                vScriptData.clear();
                vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                if (vDataValue.empty()) {
                    return false;
                }
                vScriptData = vDataValue;
                //				CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
                //				ds >> vScriptData;
                return true;
            }
        }
    } else if (1 == nIndex) {
        vector<unsigned char> vKey = {'d', 'a', 't', 'a'};
        vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
        vKey.push_back('_');
        vector<unsigned char> vPreKey(vKey);
        vPreKey.insert(vPreKey.end(), vScriptKey.begin(), vScriptKey.end());
        map<vector<unsigned char>, vector<unsigned char> >::iterator iterFindKey = mapContractDb.upper_bound(vPreKey);
        vector<unsigned char> vDataKey;
        vector<unsigned char> vDataValue;
        vDataValue.clear();
        vDataKey.clear();
        while (iterFindKey != mapContractDb.end()) {
            vector<unsigned char> vKeyTemp(vKey.begin(), vKey.begin() + vScriptId.size() + 5);
            vector<unsigned char> vTemp(iterFindKey->first.begin(), iterFindKey->first.begin() + vScriptId.size() + 5);
            if (vKeyTemp == vTemp) {
                if (iterFindKey->second.empty()) {
                    ++iterFindKey;
                    continue;
                } else {
                    vDataKey   = iterFindKey->first;
                    vDataValue = iterFindKey->second;
                    break;
                }
            } else {
                break;
                //++iterFindKey;
            }
        }
        bool bUpLevelRet(false);
        while ((bUpLevelRet = pBase->GetContractData(nCurBlockHeight, vScriptId, nIndex, vScriptKey, vScriptData))) {
            //			LogPrint("INFO", "nCurBlockHeight:%d this addr:0x%x, nIndex:%d, count:%lld\n ScriptKey:%s\n nHeight:%d\n ScriptData:%s\n vDataKey:%s\n vDataValue:%s\n",
            //					nCurBlockHeight, this, nIndex, ++llCount, HexStr(vScriptKey), nHeight, HexStr(vScriptData), HexStr(vDataKey), HexStr(vDataValue));
            vector<unsigned char> dataKeyTemp(vKey.begin(), vKey.end());
            dataKeyTemp.insert(dataKeyTemp.end(), vScriptKey.begin(), vScriptKey.end());
            //			LogPrint("INFO", "dataKeyTemp:%s\n vDataKey:%s\n", HexStr(dataKeyTemp), HexStr(vDataKey));
            //			if(mapContractDb.count(dataKeyTemp) > 0) {//本级缓存包含上级查询结果的key
            //				if(dataKeyTemp != vDataKey) {  //本级和上级查找key不同，说明上级获取的数据在本级已被删除
            //					continue;
            //				} else {
            //					if(vDataValue.empty()) { //本级和上级数据key相同,且本级数据已经删除，重新从上级获取下一条数据
            //						LogPrint("INFO", "dataKeyTemp equal vDataKey and vDataValue empty redo getcontractdata()\n");
            //						continue;
            //					}
            //					vScriptKey.clear();
            //					vScriptData.clear();
            //					vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
            //					CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
            //					ds >> nHeight;
            //					ds >> vScriptData;
            //					return true;
            //				}
            //			} else {//本级缓存不包含上级查询结果key
            //				if (dataKeyTemp < vDataKey) { //上级获取key值小
            //					return true;      //返回上级的查询结果
            //				} else {              //本级查询结果Key值小，返回本级查询结果
            //					vScriptKey.clear();
            //					vScriptData.clear();
            //					vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
            //					CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
            //					ds >> nHeight;
            //					ds >> vScriptData;
            //					return true;
            //				}
            //			}
            if (vDataKey.empty()) {  //缓存中没有符合条件的key，直接返回上级的查询结果
                if (mapContractDb.count(dataKeyTemp) <= 0) {
                    //					CDataStream ds(vScriptData, SER_DISK, CLIENT_VERSION);
                    //					ds >> vScriptData;
                    return true;
                } else {
                    //					LogPrint("INFO", "local level contains dataKeyTemp,but the value is empty,need redo getcontractdata()\n");
                    continue;  //重新从数据库中获取下一条数据
                }
            } else {
                if (dataKeyTemp < vDataKey) {
                    if (mapContractDb.count(dataKeyTemp) == 0)
                        return true;
                    else {
                        //						LogPrint("INFO", "dataKeyTemp less than vDataKey and vDataValue empty redo getcontractdata()\n");
                        continue;  //在缓存中dataKeyTemp已经被删除过了，重新从数据库中获取下一条数据
                    }
                } else {
                    if (vDataValue.empty()) {  //本级和上级数据key相同,且本级数据已经删除，重新从上级获取下一条数据
                                               //						LogPrint("INFO", "dataKeyTemp equal vDataKey and vDataValue empty redo getcontractdata()\n");
                        continue;
                    }
                    vScriptKey.clear();
                    vScriptData.clear();
                    vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                    vScriptData = vDataValue;
                    //					CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
                    //					ds >> vScriptData;
                    return true;
                }
            }
        }
        if (!bUpLevelRet) {
            if (vDataKey.empty())
                return false;
            else {
                vScriptKey.clear();
                vScriptData.clear();
                vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
                if (vDataValue.empty()) {
                    return false;
                }
                vScriptData = vDataValue;
                //				CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
                //				ds >> vScriptData;
                return true;
            }
        }
    } else {
        //		assert(0);
        return ERRORMSG("getcontractdata error");
    }
    //	vector<unsigned char> vKey = { 'd', 'a', 't', 'a' };
    //	vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
    //	vKey.push_back('_');
    //	vector<unsigned char> vDataKey;
    //	vector<unsigned char> vDataValue;
    //	if(0 == nIndex) {
    //		vDataKey.clear();
    //		vDataValue.clear();
    //		for (auto &item : mapContractDb) {   //遍历本级缓存数据，找出合法的最小的key值
    //			vector<unsigned char> vTemp(item.first.begin(),item.first.begin()+vScriptId.size()+5);
    //			if(vKey == vTemp) {
    //				if(item.second.empty()) {
    //					continue;
    //				}
    //				vDataKey = item.first;
    //				vDataValue = item.second;
    //				CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
    //				ds >> nHeight;
    //				if(nHeight <= nCurBlockHeight) { //若找到的key对应的数据保存时间已经超时，则需要删除该数据项，继续找下一个符合条件的key
    //					CScriptDBOperLog operLog(vDataKey, vDataValue);
    //					vDataKey.clear();
    //					vDataValue.clear();
    //					setOperLog.insert(operLog);
    //					continue;
    //				}
    //				break;
    //			}
    //		}
    //	}
    //	else if (1 == nIndex) {
    //		vector<unsigned char> vPreKey(vKey);
    //		vPreKey.insert(vPreKey.end(), vScriptKey.begin(), vScriptKey.end());
    //		map<vector<unsigned char>, vector<unsigned char> >::iterator iterFindKey = mapContractDb.upper_bound(vPreKey);
    //		vDataValue.clear();
    //		vDataKey.clear();
    //		while (iterFindKey != mapContractDb.end()) {
    //			vector<unsigned char> vKeyTemp(vKey.begin(), vKey.begin() + vScriptId.size() + 5);
    //			vector<unsigned char> vTemp(iterFindKey->first.begin(), iterFindKey->first.begin() + vScriptId.size() + 5);
    //			if (vKeyTemp == vTemp) {
    //				if (iterFindKey->second.empty()) {
    //					++iterFindKey;
    //					continue;
    //				} else {
    //					vDataKey = iterFindKey->first;
    //					vDataValue = iterFindKey->second;
    //					CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
    //					ds >> nHeight;
    //					if (nHeight <= nCurBlockHeight) { //若找到的key对应的数据保存时间已经超时，则需要删除该数据项，继续找下一个符合条件的key
    //						CScriptDBOperLog operLog(vDataKey, iterFindKey->second);
    //						vDataKey.clear();
    //						vDataValue.clear();
    //						setOperLog.insert(operLog);
    //						++iterFindKey;
    //						continue;
    //					}
    //					break;
    //				}
    //			} else {
    //				++iterFindKey;
    //			}
    //		}
    //	}
    //	else {
    //		assert(0);
    //	}
    //	bool bUpLevelRet(false);
    //	unsigned long llCount(0);
    //	int nIndexTemp = nIndex;
    //	while((bUpLevelRet = pBase->getContractData(nCurBlockHeight, vScriptId, nIndexTemp, vScriptKey, vScriptData, nHeight, setOperLog))) {
    //		LogPrint("INFO", "nCurBlockHeight:%d this addr:%x, nIndex:%d, count:%lld\n ScriptKey:%s\n nHeight:%d\n ScriptData:%s\n vDataKey:%s\n vDataValue:%s\n",
    //				nCurBlockHeight, this, nIndexTemp, ++llCount, HexStr(vScriptKey), nHeight, HexStr(vScriptData), HexStr(vDataKey), HexStr(vDataValue));
    //		nIndexTemp = 1;
    //		for(auto &itemKey : mapContractDb) {
    //			vector<unsigned char> vKeyTemp(itemKey.first.begin(), itemKey.first.begin() + vKey.size());
    //			if(vKeyTemp == vKey) {
    //				LogPrint("INFO", "vKey:%s\n vValue:%s\n", HexStr(itemKey.first), HexStr(itemKey.second));
    //			}
    //		}
    //		set<CScriptDBOperLog>::iterator iterOperLog = setOperLog.begin();
    //		for (; iterOperLog != setOperLog.end();) { //防止由于没有flush cache，对数据库中超时的脚本数据项，在cache中多次删除，引起删除失败
    //			if (mapContractDb.count(iterOperLog->vKey) > 0 && mapContractDb[iterOperLog->vKey].empty()) {
    //				LogPrint("INFO", "DeleteData key:%s\n", HexStr(iterOperLog->vKey));
    //				setOperLog.erase(iterOperLog++);
    //			}else {
    //				++iterOperLog;
    //			}
    //		}
    //		vector<unsigned char> dataKeyTemp(vKey.begin(), vKey.end());
    //		dataKeyTemp.insert(dataKeyTemp.end(), vScriptKey.begin(), vScriptKey.end());
    //		LogPrint("INFO", "dataKeyTemp:%s\n vDataKey:%s\n", HexStr(dataKeyTemp), HexStr(vDataKey));
    //		if(mapContractDb.count(dataKeyTemp) > 0) {//本级缓存包含上级查询结果的key
    //			if(dataKeyTemp != vDataKey) {  //本级和上级查找key不同，说明上级获取的数据在本级已被删除
    //				LogPrint("INFO", "dataKeyTemp equal vDataKey and vDataValue empty redo getcontractdata()\n");
    //				continue;
    //			} else {
    //				if(vDataValue.empty()) { //本级和上级数据key相同,且本级数据已经删除，重新从上级获取下一条数据
    //					LogPrint("INFO", "dataKeyTemp equal vDataKey and vDataValue empty redo getcontractdata()\n");
    //					continue;
    //				}
    //				vScriptKey.clear();
    //				vScriptData.clear();
    //				vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
    //				CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
    //				ds >> nHeight;
    //				ds >> vScriptData;
    //				return true;
    //			}
    //		} else {//本级缓存不包含上级查询结果key
    //			if (dataKeyTemp < vDataKey) { //上级获取key值小
    //				return true;      //返回上级的查询结果
    //			} else {              //本级查询结果Key值小，返回本级查询结果
    //				vScriptKey.clear();
    //				vScriptData.clear();
    //				vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
    //				CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
    //				ds >> nHeight;
    //				ds >> vScriptData;
    //				return true;
    //			}
    //		}
    //		if(!bUpLevelRet) {
    //			set<CScriptDBOperLog>::iterator iterOperLog = setOperLog.begin();
    //			for (; iterOperLog != setOperLog.end();) { //防止由于没有flush cache，对数据库中超时的脚本数据项，在cache中多次删除，引起删除失败
    //				if (mapContractDb.count(iterOperLog->vKey) > 0 && mapContractDb[iterOperLog->vKey].empty()) {
    //					LogPrint("INFO", "DeleteData key:%s\n", HexStr(iterOperLog->vKey));
    //					setOperLog.erase(iterOperLog++);
    //				}else{
    //					++iterOperLog;
    //				}
    //			}
    //			if (vDataKey.empty())
    //				return false;
    //			else {
    //				vScriptKey.clear();
    //				vScriptData.clear();
    //				vScriptKey.insert(vScriptKey.end(), vDataKey.begin() + 11, vDataKey.end());
    //				if (vDataValue.empty()) {
    //					return false;
    //				}
    //				CDataStream ds(vDataValue, SER_DISK, CLIENT_VERSION);
    //				ds >> nHeight;
    //				ds >> vScriptData;
    //				return true;
    //			}
    //		}
    //	}
    return true;
}
bool CScriptDBViewCache::SetContractData(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vScriptKey,
                                         const vector<unsigned char> &vScriptData, CScriptDBOperLog &operLog)
{
    vector<unsigned char> vKey = {'d', 'a', 't', 'a'};
    vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
    vKey.push_back('_');
    vKey.insert(vKey.end(), vScriptKey.begin(), vScriptKey.end());
    vector<unsigned char> vNewValue(vScriptData.begin(), vScriptData.end());
    if (!HasData(vKey)) {
        int nCount(0);
        GetContractItemCount(vScriptId, nCount);
        ++nCount;
        if (!SetContractItemCount(vScriptId, nCount))
            return false;
    }

    vector<unsigned char> oldValue;
    oldValue.clear();
    GetData(vKey, oldValue);
    operLog = CScriptDBOperLog(vKey, oldValue);
    bool ret = SetData(vKey, vNewValue);
    return ret;
}

bool CScriptDBViewCache::HaveScript(const vector<unsigned char> &vScriptId) {
    vector<unsigned char> scriptKey = {'d', 'e', 'f'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());
    return HasData(scriptKey);
}

bool CScriptDBViewCache::GetScriptCount(int &nCount) {
    vector<unsigned char> scriptKey = {'s', 'n', 'u', 'm'};
    vector<unsigned char> vValue;
    if (!GetData(scriptKey, vValue))
        return false;
    CDataStream ds(vValue, SER_DISK, CLIENT_VERSION);
    ds >> nCount;
    return true;
}

bool CScriptDBViewCache::SetScriptCount(const int nCount) {
    vector<unsigned char> scriptKey = {'s', 'n', 'u', 'm'};
    vector<unsigned char> vValue;
    vValue.clear();
    if (nCount > 0) {
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << nCount;
        vValue.insert(vValue.end(), ds.begin(), ds.end());
    } else if (nCount < 0) {
        //		assert(0);
        return false;
    }
    if (!SetData(scriptKey, vValue))
        return false;
    return true;
}
bool CScriptDBViewCache::EraseScript(const vector<unsigned char> &vScriptId) {
    vector<unsigned char> scriptKey = {'d', 'e', 'f'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());
    if (HaveScript(vScriptId)) {
        int nCount(0);
        if (!GetScriptCount(nCount))
            return false;
        if (!SetScriptCount(--nCount))
            return false;
    }
    return EraseKey(scriptKey);
}
bool CScriptDBViewCache::GetContractItemCount(const vector<unsigned char> &vScriptId, int &nCount) {
    vector<unsigned char> scriptKey = {'s', 'd', 'n', 'u', 'm'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());
    vector<unsigned char> vValue;
    if (!GetData(scriptKey, vValue)) {
        nCount = 0;
        return true;
    }

    CDataStream ds(vValue, SER_DISK, CLIENT_VERSION);
    ds >> nCount;
    return true;
}

bool CScriptDBViewCache::SetContractItemCount(const vector<unsigned char> &vScriptId, int nCount) {
    if (nCount < 0)
        return false;

    vector<unsigned char> scriptKey = {'s', 'd', 'n', 'u', 'm'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());

    vector<unsigned char> vValue;
    vValue.clear();

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << nCount;
    vValue.insert(vValue.end(), ds.begin(), ds.end());

    if (!SetData(scriptKey, vValue))
        return false;

    return true;
}

bool CScriptDBViewCache::EraseAppData(const vector<unsigned char> &vScriptId,
    const vector<unsigned char> &vScriptKey, CScriptDBOperLog &operLog)
{
    vector<unsigned char> vKey = {'d', 'a', 't', 'a'};
    vKey.insert(vKey.end(), vScriptId.begin(), vScriptId.end());
    vKey.push_back('_');
    vKey.insert(vKey.end(), vScriptKey.begin(), vScriptKey.end());

    if (HasData(vKey)) {
        int nCount(0);
        if (!GetContractItemCount(vScriptId, nCount))
            return false;

        if (!SetContractItemCount(vScriptId, --nCount))
            return false;

        vector<unsigned char> vValue;
        if (!GetData(vKey, vValue))
            return false;

        operLog = CScriptDBOperLog(vKey, vValue);

        if (!EraseKey(vKey))
            return false;
    }

    return true;
}

bool CScriptDBViewCache::EraseAppData(const vector<unsigned char> &vKey)
{
    if (vKey.size() < 12) {
        return ERRORMSG("EraseAppData delete script data key value error!");
        //		assert(0);
    }
    vector<unsigned char> vScriptId(vKey.begin() + 4, vKey.begin() + 10);
    vector<unsigned char> vScriptKey(vKey.begin() + 11, vKey.end());
    CScriptDBOperLog operLog;
    return EraseAppData(vScriptId, vScriptKey, operLog);
}

bool CScriptDBViewCache::HasScriptData(const vector<unsigned char> &vScriptId, const vector<unsigned char> &vScriptKey) {
    vector<unsigned char> scriptKey = {'d', 'a', 't', 'a'};
    scriptKey.insert(scriptKey.end(), vScriptId.begin(), vScriptId.end());
    scriptKey.push_back('_');
    scriptKey.insert(scriptKey.end(), vScriptKey.begin(), vScriptKey.end());
    return HasData(scriptKey);
}

bool CScriptDBViewCache::GetScript(const int nIndex, CRegID &scriptId, vector<unsigned char> &vValue) {
    vector<unsigned char> tem;
    if (nIndex != 0) {
        tem = scriptId.GetVec6();
    }
    if (GetScript(nIndex, tem, vValue)) {
        scriptId.SetRegID(tem);
        return true;
    }

    return false;
}
bool CScriptDBViewCache::SetScript(const CRegID &scriptId, const vector<unsigned char> &vValue) {
    return SetScript(scriptId.GetVec6(), vValue);
}
bool CScriptDBViewCache::HaveScript(const CRegID &scriptId) {
    return HaveScript(scriptId.GetVec6());
}
bool CScriptDBViewCache::EraseScript(const CRegID &scriptId) {
    return EraseScript(scriptId.GetVec6());
}
bool CScriptDBViewCache::GetContractItemCount(const CRegID &scriptId, int &nCount) {
    return GetContractItemCount(scriptId.GetVec6(), nCount);
}
bool CScriptDBViewCache::EraseAppData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey, CScriptDBOperLog &operLog) {
    return EraseAppData(scriptId.GetVec6(), vScriptKey, operLog);
}
bool CScriptDBViewCache::HasScriptData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey) {
    return HasScriptData(scriptId.GetVec6(), vScriptKey);
}
bool CScriptDBViewCache::GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const vector<unsigned char> &vScriptKey,
                                         vector<unsigned char> &vScriptData) {
    return GetContractData(nCurBlockHeight, scriptId.GetVec6(), vScriptKey, vScriptData);
}
bool CScriptDBViewCache::GetContractData(const int nCurBlockHeight, const CRegID &scriptId, const int &nIndex, vector<unsigned char> &vScriptKey, vector<unsigned char> &vScriptData) {
    return GetContractData(nCurBlockHeight, scriptId.GetVec6(), nIndex, vScriptKey, vScriptData);
}
bool CScriptDBViewCache::SetContractData(const CRegID &scriptId, const vector<unsigned char> &vScriptKey,
                                         const vector<unsigned char> &vScriptData, CScriptDBOperLog &operLog) {
    return SetContractData(scriptId.GetVec6(), vScriptKey, vScriptData, operLog);
}
bool CScriptDBViewCache::SetTxRelAccout(const uint256 &txHash, const set<CKeyID> &relAccount) {
    vector<unsigned char> vKey = {'t', 'x'};
    vector<unsigned char> vValue;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << txHash;
    vKey.insert(vKey.end(), ds.begin(), ds.end());
    ds.clear();
    ds << relAccount;
    vValue.assign(ds.begin(), ds.end());
    return SetData(vKey, vValue);
}
bool CScriptDBViewCache::GetTxRelAccount(const uint256 &txHash, set<CKeyID> &relAccount) {
    vector<unsigned char> vKey = {'t', 'x'};
    vector<unsigned char> vValue;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << txHash;
    vKey.insert(vKey.end(), ds.begin(), ds.end());
    if (!GetData(vKey, vValue))
        return false;
    ds.clear();
    vector<char> temp;
    temp.assign(vValue.begin(), vValue.end());
    ds.insert(ds.end(), temp.begin(), temp.end());
    ds >> relAccount;
    return true;
}
bool CScriptDBViewCache::EraseTxRelAccout(const uint256 &txHash) {
    vector<unsigned char> vKey = {'t', 'x'};
    vector<unsigned char> vValue;
    vValue.clear();
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << txHash;
    vKey.insert(vKey.end(), ds.begin(), ds.end());
    SetData(vKey, vValue);
    return true;
}
Object CScriptDBViewCache::ToJsonObj() const {
    Object obj;
    Array arrayObj;
    //	for (auto& item : mapContractDb) {
    //		Object obj;
    //		obj.push_back(Pair("key", HexStr(item.first)));
    //		obj.push_back(Pair("value", HexStr(item.second)));
    //		arrayObj.push_back(obj);
    //	}
    //	obj.push_back(Pair("mapContractDb", arrayObj));
    arrayObj.push_back(pBase->ToJsonObj("def"));
    arrayObj.push_back(pBase->ToJsonObj("data"));
    arrayObj.push_back(pBase->ToJsonObj("author"));
    obj.push_back(Pair("mapContractDb", arrayObj));
    return obj;
}
void CScriptDBViewCache::SetBaseData(CScriptDBView *pNewBase) {
    pBase = pNewBase;
}
string CScriptDBViewCache::ToString() {
    string str("");
    vector<unsigned char> vPrefix = {'d', 'a', 't', 'a'};
    for (auto &item : mapContractDb) {
        vector<unsigned char> vTemp(item.first.begin(), item.first.begin() + 4);
        if (vTemp == vPrefix) {
            str = strprintf("vKey=%s\n vData=%s\n", HexStr(item.first), HexStr(item.second));
        }
    }
    return str;
}

bool CScriptDBViewCache::SetDelegateData(const CAccount &delegateAcct, CScriptDBOperLog &operLog)
{
    CRegID regId(0, 0);
    vector<unsigned char> vVoteKey = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    uint64_t nMaxNumber            = 0xFFFFFFFFFFFFFFFF;
    string strVotes                = strprintf("%016x", nMaxNumber - delegateAcct.llVotes);
    vVoteKey.insert(vVoteKey.end(), strVotes.begin(), strVotes.end());
    vVoteKey.push_back('_');
    vVoteKey.insert(vVoteKey.end(), delegateAcct.regID.GetVec6().begin(), delegateAcct.regID.GetVec6().end());
    vector<unsigned char> vVoteValue;
    vVoteValue.push_back(1);
    if (!SetContractData(regId, vVoteKey, vVoteValue, operLog))
        return false;

    return true;
}

bool CScriptDBViewCache::SetDelegateData(const vector<unsigned char> &vKey) {
    if (vKey.empty()) {
        return true;
    }
    vector<unsigned char> vValue;
    vValue.push_back(1);
    if (!SetData(vKey, vValue)) {
        return false;
    }
    return true;
}

bool CScriptDBViewCache::EraseDelegateData(const CAccount &delegateAcct, CScriptDBOperLog &operLog) {
    CRegID regId(0, 0);
    vector<unsigned char> vVoteOldKey = {'d', 'e', 'l', 'e', 'g', 'a', 't', 'e', '_'};
    uint64_t nMaxNumber               = 0xFFFFFFFFFFFFFFFF;
    string strOldVoltes               = strprintf("%016x", nMaxNumber - delegateAcct.llVotes);
    vVoteOldKey.insert(vVoteOldKey.end(), strOldVoltes.begin(), strOldVoltes.end());
    vVoteOldKey.push_back('_');
    vVoteOldKey.insert(vVoteOldKey.end(), delegateAcct.regID.GetVec6().begin(), delegateAcct.regID.GetVec6().end());
    if (!EraseAppData(regId, vVoteOldKey, operLog))
        return false;

    return true;
}

bool CScriptDBViewCache::EraseDelegateData(const vector<unsigned char> &vKey) {
    if (!EraseKey(vKey))
        return false;

    return true;
}

uint256 CTransactionDBView::HasTx(const uint256 &txHash) { return uint256(); }
bool CTransactionDBView::IsContainBlock(const CBlock &block) { return false; }
bool CTransactionDBView::AddBlockToCache(const CBlock &block) { return false; }
bool CTransactionDBView::DeleteBlockFromCache(const CBlock &block) { return false; }
bool CTransactionDBView::LoadTransaction(map<uint256, vector<uint256> > &mapTxHashByBlockHash) { return false; }
bool CTransactionDBView::BatchWrite(const map<uint256, vector<uint256> > &mapTxHashByBlockHash) { return false; }

CTransactionDBViewBacked::CTransactionDBViewBacked(CTransactionDBView &transactionView) {
    pBase = &transactionView;
}

uint256 CTransactionDBViewBacked::HasTx(const uint256 &txHash) {
    return pBase->HasTx(txHash);
}

bool CTransactionDBViewBacked::IsContainBlock(const CBlock &block) {
    return pBase->IsContainBlock(block);
}

bool CTransactionDBViewBacked::AddBlockToCache(const CBlock &block) {
    return pBase->AddBlockToCache(block);
}

bool CTransactionDBViewBacked::DeleteBlockFromCache(const CBlock &block) {
    return pBase->DeleteBlockFromCache(block);
}

bool CTransactionDBViewBacked::LoadTransaction(map<uint256, vector<uint256> > &mapTxHashByBlockHash) {
    return pBase->LoadTransaction(mapTxHashByBlockHash);
}

bool CTransactionDBViewBacked::BatchWrite(const map<uint256, vector<uint256> > &mapTxHashByBlockHashIn) {
    return pBase->BatchWrite(mapTxHashByBlockHashIn);
}

CTransactionDBCache::CTransactionDBCache(CTransactionDBView &txCacheDB, bool fDummy) : CTransactionDBViewBacked(txCacheDB) {}

bool CTransactionDBCache::IsContainBlock(const CBlock &block) {
    //(mapTxHashByBlockHash.count(block.GetHash()) > 0 && mapTxHashByBlockHash[block.GetHash()].size() > 0)
    return (IsInMap(mapTxHashByBlockHash, block.GetHash()) || pBase->IsContainBlock(block));
}

bool CTransactionDBCache::AddBlockToCache(const CBlock &block) {
    vector<uint256> vTxHash;
    vTxHash.clear();
    for (auto &ptx : block.vptx) {
        vTxHash.push_back(ptx->GetHash());
    }
    mapTxHashByBlockHash[block.GetHash()] = vTxHash;
    //	LogPrint("txcache", "CTransactionDBCache:AddBlockToCache() the block height=%d hash=%s is in TxCache\n", block.nHeight, block.GetHash().GetHex());
    //	LogPrint("txcache", "mapTxHashByBlockHash size:%d\n", mapTxHashByBlockHash.size());
    //	map<int, uint256> mapTxCacheBlockHash;
    //	mapTxCacheBlockHash.clear();
    //	for (auto &item : mapTxHashByBlockHash) {
    //		mapTxCacheBlockHash.insert(make_pair(mapBlockIndex[item.first]->nHeight, item.first));
    //	}
    //	for(auto &item1 : mapTxCacheBlockHash) {
    //		LogPrint("txcache", "block height:%d, hash:%s\n", item1.first, item1.second.GetHex());
    //		for (auto &txHash : mapTxHashByBlockHash[item1.second])
    //			LogPrint("txcache", "txhash:%s\n", txHash.GetHex());
    //	}
    return true;
}

bool CTransactionDBCache::DeleteBlockFromCache(const CBlock &block) {
    //	LogPrint("txcache", "CTransactionDBCache::DeleteBlockFromCache() height=%d blockhash=%s \n", block.nHeight, block.GetHash().GetHex());
    if (IsContainBlock(block)) {
        vector<uint256> vTxHash;
        vTxHash.clear();
        mapTxHashByBlockHash[block.GetHash()] = vTxHash;
        return true;
    } else {
        LogPrint("ERROR", "the block hash:%s isn't in TxCache\n", block.GetHash().GetHex());
        return false;
    }
    return true;
}

uint256 CTransactionDBCache::HasTx(const uint256 &txHash) {
    for (auto &item : mapTxHashByBlockHash) {
        vector<uint256>::iterator it = find(item.second.begin(), item.second.end(), txHash);
        if (it != item.second.end()) {
            return item.first;
        }
    }
    uint256 blockHash = pBase->HasTx(txHash);
    if (IsInMap(mapTxHashByBlockHash, blockHash)) {
        return blockHash;
    }
    return uint256();
}

map<uint256, vector<uint256> > CTransactionDBCache::GetTxHashCache(void) {
    return mapTxHashByBlockHash;
}

bool CTransactionDBCache::BatchWrite(const map<uint256, vector<uint256> > &mapTxHashByBlockHashIn) {
    for (auto &item : mapTxHashByBlockHashIn) {
        mapTxHashByBlockHash[item.first] = item.second;
    }
    return true;
}

bool CTransactionDBCache::Flush() {
    bool bRet = pBase->BatchWrite(mapTxHashByBlockHash);
    if (bRet) {
        map<uint256, vector<uint256> >::iterator iter = mapTxHashByBlockHash.begin();
        for (; iter != mapTxHashByBlockHash.end();) {
            if (iter->second.empty()) {
                mapTxHashByBlockHash.erase(iter++);
            } else {
                iter++;
            }
        }
    }
    return bRet;
}

void CTransactionDBCache::AddTxHashCache(const uint256 &blockHash, const vector<uint256> &vTxHash) {
    mapTxHashByBlockHash[blockHash] = vTxHash;
}

bool CTransactionDBCache::LoadTransaction() {
    return pBase->LoadTransaction(mapTxHashByBlockHash);
}

void CTransactionDBCache::Clear() {
    mapTxHashByBlockHash.clear();
}

int CTransactionDBCache::GetSize() {
    int iCount(0);
    for (auto &i : mapTxHashByBlockHash) {
        if (!i.second.empty())
            ++iCount;
    }
    return iCount;
}

bool CTransactionDBCache::IsInMap(const map<uint256, vector<uint256> > &mMap, const uint256 &hash) const {
    if (hash == uint256())
        return false;
    auto te = mMap.find(hash);
    if (te != mMap.end()) {
        return !te->second.empty();
    }

    return false;
}

Object CTransactionDBCache::ToJsonObj() const {
    Array deletedobjArray;
    Array inCacheObjArray;
    for (auto &item : mapTxHashByBlockHash) {
        Object obj;
        obj.push_back(Pair("blockhash", item.first.ToString()));

        Array objTxInBlock;
        for (const auto &itemTx : item.second) {
            Object objTxHash;
            objTxHash.push_back(Pair("txhash", itemTx.ToString()));
            objTxInBlock.push_back(objTxHash);
        }
        obj.push_back(Pair("txHashes", objTxInBlock));
        if (item.second.size() > 0) {
            inCacheObjArray.push_back(obj);
        } else {
            deletedobjArray.push_back(obj);
        }
    }
    Object temobj;
    temobj.push_back(Pair("incachblock", inCacheObjArray));
    //	temobj.push_back(Pair("removecachblock", deletedobjArray));
    Object retobj;
    retobj.push_back(Pair("mapTxHashByBlockHash", temobj));
    return retobj;
}
void CTransactionDBCache::SetBaseData(CTransactionDBView *pNewBase) {
    pBase = pNewBase;
}

const map<uint256, vector<uint256> > &CTransactionDBCache::GetCacheMap() {
    return mapTxHashByBlockHash;
}

void CTransactionDBCache::SetCacheMap(const map<uint256, vector<uint256> > &mapCache) {
    mapTxHashByBlockHash = mapCache;
}

bool CScriptDBViewCache::GetScriptAcc(const CRegID &scriptId, const vector<unsigned char> &vAccKey,
                                      CAppUserAccount &appAccOut) {
    vector<unsigned char> scriptKey = {'a', 'c', 'c', 't'};
    vector<unsigned char> vRegId    = scriptId.GetVec6();
    scriptKey.insert(scriptKey.end(), vRegId.begin(), vRegId.end());
    scriptKey.push_back('_');
    scriptKey.insert(scriptKey.end(), vAccKey.begin(), vAccKey.end());
    vector<unsigned char> vValue;

    //LogPrint("vm","%s",HexStr(scriptKey));
    if (!GetData(scriptKey, vValue))
        return false;
    CDataStream ds(vValue, SER_DISK, CLIENT_VERSION);
    ds >> appAccOut;
    return true;
}

bool CScriptDBViewCache::SetScriptAcc(const CRegID &scriptId, const CAppUserAccount &appAccOut,
                                      CScriptDBOperLog &operlog) {
    vector<unsigned char> scriptKey = {'a', 'c', 'c', 't'};
    vector<unsigned char> vRegId    = scriptId.GetVec6();
    vector<unsigned char> vAccKey   = appAccOut.GetAccUserId();
    scriptKey.insert(scriptKey.end(), vRegId.begin(), vRegId.end());
    scriptKey.push_back('_');
    scriptKey.insert(scriptKey.end(), vAccKey.begin(), vAccKey.end());
    vector<unsigned char> vValue;
    operlog.vKey = scriptKey;
    if (GetData(scriptKey, vValue)) {
        operlog.vValue = vValue;
    }
    CDataStream ds(SER_DISK, CLIENT_VERSION);

    ds << appAccOut;
    //LogPrint("vm","%s",HexStr(scriptKey));
    vValue.clear();
    vValue.insert(vValue.end(), ds.begin(), ds.end());
    return SetData(scriptKey, vValue);
}

bool CScriptDBViewCache::EraseScriptAcc(const CRegID &scriptId, const vector<unsigned char> &vKey) {
    vector<unsigned char> scriptKey = {'a', 'c', 'c', 't'};
    vector<unsigned char> vRegId    = scriptId.GetVec6();
    scriptKey.insert(scriptKey.end(), vRegId.begin(), vRegId.end());
    scriptKey.push_back('_');
    scriptKey.insert(scriptKey.end(), vKey.begin(), vKey.end());
    vector<unsigned char> vValue;

    //LogPrint("vm","%s",HexStr(scriptKey));
    if (!GetData(scriptKey, vValue)) {
        return false;
    }

    return EraseKey(scriptKey);
}
