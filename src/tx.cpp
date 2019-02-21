#include "serialize.h"
#include <boost/foreach.hpp>
#include "hash.h"
#include "util.h"
#include "database.h"
#include "main.h"
#include <algorithm>
#include "txdb.h"
#include "vm/vmrunevn.h"
#include "core.h"
#include "miner.h"
#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
using namespace json_spirit;

static bool GetKeyId(const CAccountViewCache &view, const vector<unsigned char> &ret,
        CKeyID &KeyId) {
    if (ret.size() == 6) {
        CRegID regId(ret);
        KeyId = regId.getKeyID(view);
    } else if (ret.size() == 34) {
        string addr(ret.begin(), ret.end());
        KeyId = CKeyID(addr);
    }else{
        return false;
    }
    if (KeyId.IsEmpty())
        return false;

    return true;
}

bool CID::Set(const CRegID &id) {
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << id;
    vchData.clear();
    vchData.insert(vchData.end(), ds.begin(), ds.end());
    return true;
}

bool CID::Set(const CKeyID &id) {
    vchData.resize(20);
    memcpy(&vchData[0], &id, 20);
    return true;
}

bool CID::Set(const CPubKey &id) {
    vchData.resize(id.size());
    memcpy(&vchData[0], &id, id.size());
    return true;
}

bool CID::Set(const CNullID &id) {
    return true;
}

bool CID::Set(const CUserID &userid) {
    return boost::apply_visitor(CIDVisitor(this), userid);
}

CUserID CID::GetUserId()
{
    unsigned long len = vchData.size();
    if (1 < len && len <= 10) {
        CRegID regId;
        regId.SetRegIDByCompact(vchData);
        return CUserID(regId);
    } else if (len == 33) {
        CPubKey pubKey(vchData);
        return CUserID(pubKey);
    } else if (len == 20) {
        uint160 data = uint160(vchData);
        CKeyID keyId(data);
        return CUserID(keyId);
    } else if(vchData.empty()) {
        return CNullID();
    } else {
        LogPrint("ERROR", "vchData:%s, len:%d\n", HexStr(vchData).c_str(), len);
        throw ios_base::failure("GetUserId error from CID");
    }
    return CNullID();
}

bool CRegID::clean()
{
    nHeight = 0 ;
    nIndex = 0 ;
    vRegID.clear();
    return true;
}

CRegID::CRegID(const vector<unsigned char>& vIn) {
    assert(vIn.size() == 6);
    vRegID = vIn;
    nHeight = 0;
    nIndex = 0;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> nHeight;
    ds >> nIndex;
}

bool CRegID::IsSimpleRegIdStr(const string & str)
{
    int len = str.length();
    if (len >= 3) {
        int pos = str.find('-');

        if (pos > len - 1) {
            return false;
        }
        string firtstr = str.substr(0, pos);

        if (firtstr.length() > 10 || firtstr.length() == 0) //int max is 4294967295 can not over 10
            return false;

        for (auto te : firtstr) {
            if (!isdigit(te))
                return false;
        }
        string endstr = str.substr(pos + 1);
        if (endstr.length() > 10 || endstr.length() == 0) //int max is 4294967295 can not over 10
            return false;
        for (auto te : endstr) {
            if (!isdigit(te))
                return false;
        }
        return true;
    }
    return false;
}

bool CRegID::GetKeyID(const string & str,CKeyID &keyId)
{
    CRegID regId(str);
    if (regId.IsEmpty())
        return false;
    keyId = regId.getKeyID(*pAccountViewTip);
    return !keyId.IsEmpty();
}

bool CRegID::IsRegIdStr(const string & str)
{
    if(IsSimpleRegIdStr(str)){
        return true;
    }
    else if(str.length()==12){
        return true;
    }
    return false;
}

void CRegID::SetRegID(string strRegID)
{
    nHeight = 0;
    nIndex = 0;
    vRegID.clear();

    if (IsSimpleRegIdStr(strRegID)) {
        int pos = strRegID.find('-');
        nHeight = atoi(strRegID.substr(0, pos).c_str());
        nIndex = atoi(strRegID.substr(pos+1).c_str());
        vRegID.insert(vRegID.end(), BEGIN(nHeight), END(nHeight));
        vRegID.insert(vRegID.end(), BEGIN(nIndex), END(nIndex));
//      memcpy(&vRegID.at(0),&nHeight,sizeof(nHeight));
//      memcpy(&vRegID[sizeof(nHeight)],&nIndex,sizeof(nIndex));
    } else if(strRegID.length() == 12) {
        vRegID = ::ParseHex(strRegID);
        memcpy(&nHeight,&vRegID[0],sizeof(nHeight));
        memcpy(&nIndex,&vRegID[sizeof(nHeight)],sizeof(nIndex));
    }
}

void CRegID::SetRegID(const vector<unsigned char>& vIn)
{
    assert(vIn.size() == 6);
    vRegID = vIn;
    CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
    ds >> nHeight;
    ds >> nIndex;
}

CRegID::CRegID(string strRegID)
{
    SetRegID(strRegID);
}

CRegID::CRegID(uint32_t nHeightIn, uint16_t nIndexIn)
{
    nHeight = nHeightIn;
    nIndex = nIndexIn;
    vRegID.clear();
    vRegID.insert(vRegID.end(), BEGIN(nHeightIn), END(nHeightIn));
    vRegID.insert(vRegID.end(), BEGIN(nIndexIn), END(nIndexIn));
}

string CRegID::ToString() const
{
    if(!IsEmpty())
      return  strprintf("%d-%d", nHeight, nIndex);
    return string(" ");
}

CKeyID CRegID::getKeyID(const CAccountViewCache &view)const
{
    CKeyID ret;
    CAccountViewCache(view).GetKeyId(*this,ret);
    return ret;
}

void CRegID::SetRegIDByCompact(const vector<unsigned char> &vIn)
{
    if (vIn.size() > 0) {
        CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
        ds >> *this;
    } else {
        clean();
    }
}

bool CBaseTransaction::IsValidHeight(int nCurrHeight, int nTxCacheHeight) const
{
    if(REWARD_TX == nTxType)
        return true;
    if (nValidHeight > nCurrHeight + nTxCacheHeight / 2)
        return false;
    if (nValidHeight < nCurrHeight - nTxCacheHeight / 2)
        return false;
    return true;
}

bool CBaseTransaction::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = txundo.vAccountLog.rbegin();
    for (; rIterAccountLog != txundo.vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!view.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, undo ExecuteTx read accountId= %s account info error"),
                    UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, undo UndoOperateAccount failed"), UPDATE_ACCOUNT_FAIL,
                    "undo-operate-account-failed");
        }
        if (COMMON_TX == nTxType
                && (account.IsEmptyValue()
                        && (!account.PublicKey.IsFullyValid() || account.PublicKey.GetKeyID() != account.keyID))) {
            view.EraseAccount(userId);
        } else {
            if (!view.SetAccount(userId, account)) {
                return state.DoS(100,
                        ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, undo ExecuteTx write accountId= %s account info error"),
                        UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }
        }
    }
    if (DELEGATE_TX == nTxType) {
        vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
        if (SysCfg().GetAddressToTxFlag() && txundo.vScriptOperLog.size() > 0) {
            if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
                return state.DoS(100, ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, undo scriptdb data error"), UPDATE_ACCOUNT_FAIL,
                                 "bad-save-scriptdb");
            ++rIterScriptDBLog;
        }

        for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
            // recover the old value and erase the new value
            if (!scriptDB.SetDelegateData(rIterScriptDBLog->vKey))
                return state.DoS(100, ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, set delegate data error"), UPDATE_ACCOUNT_FAIL,
                                 "bad-save-scriptdb");

            ++rIterScriptDBLog;
            if (!scriptDB.EraseDelegateData(rIterScriptDBLog->vKey))
                return state.DoS(100, ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, erase delegate data error"), UPDATE_ACCOUNT_FAIL,
                                 "bad-save-scriptdb");
        }
    } else {
        vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
        for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
            if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
                return state.DoS(100, ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, undo scriptdb data error"), UPDATE_ACCOUNT_FAIL,
                                 "bad-save-scriptdb");
        }
    }

    if(CONTRACT_TX == nTxType) {
        if (!scriptDB.EraseTxRelAccout(GetHash()))
            return state.DoS(100, ERRORMSG("UndoExecuteTx() : CBaseTransaction UndoExecuteTx, erase tx rel account error"), UPDATE_ACCOUNT_FAIL,
                            "bad-save-scriptdb");
    }
    return true;
}

uint64_t CBaseTransaction::GetFuel(int nfuelRate) {
    uint64_t llFuel = ceil(nRunStep/100.0f) * nfuelRate;
    if (REG_CONT_TX == nTxType) {
        if (llFuel < 1 * COIN) {
            llFuel = 1 * COIN;
        }
    }
    return llFuel;
}

string CBaseTransaction::txTypeArray[7] = { "NULL_TXTYPE", "REWARD_TX", "REG_ACCT_TX", "COMMON_TX", "CONTRACT_TX", "REG_CONT_TX", "DELEGATE_TX"};

string COperVoteFund::voteOperTypeArray[3] = {"NULL_OPER", "ADD_FUND", "MINUS_FUND"};

int CBaseTransaction::GetFuelRate(CScriptDBViewCache &scriptDB) {
    if(0 == nFuelRate) {
        CDiskTxPos postx;
        if (scriptDB.ReadTxIndex(GetHash(), postx)) {
            CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
            CBlockHeader header;
            try {
                file >> header;
            } catch (std::exception &e) {
                return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
            }
            nFuelRate = header.GetFuelRate();
        }
        else {
            nFuelRate = GetElementForBurn(chainActive.Tip());
        }
    }
    return nFuelRate;
}

bool CRegisterAccountTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CAccount account;
    CRegID regId(nHeight, nIndex);
    CKeyID keyId = boost::get<CPubKey>(userId).GetKeyID();
    if (!view.GetAccount(userId, account))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, read source keyId %s account info error", keyId.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    CAccountLog acctLog(account);
    if(account.PublicKey.IsFullyValid() && account.PublicKey.GetKeyID() == keyId) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, read source keyId %s duplicate register", keyId.ToString()),
                    UPDATE_ACCOUNT_FAIL, "duplicate-register-account");
    }
    account.PublicKey = boost::get<CPubKey>(userId);
    if (llFees > 0) {
        if(!account.OperateAccount(MINUS_FREE, llFees, nHeight))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, not sufficient funds in account, keyid=%s", keyId.ToString()),
                    UPDATE_ACCOUNT_FAIL, "not-sufficiect-funds");
    }

    account.regID = regId;
    if (typeid(CPubKey) == minerId.type()) {
        account.MinerPKey = boost::get<CPubKey>(minerId);
        if (account.MinerPKey.IsValid() && !account.MinerPKey.IsFullyValid()) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, MinerPKey:%s Is Invalid", account.MinerPKey.ToString()),
                    UPDATE_ACCOUNT_FAIL, "MinerPKey Is Invalid");
        }
    }

    if (!view.SaveAccountInfo(regId, keyId, account)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, write source addr %s account info error", regId.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    txundo.vAccountLog.push_back(acctLog);
    txundo.txHash = GetHash();
    if(SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(userId, sendKeyId)) {
            return ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, get keyid by userId error!");
        }
        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    return true;
}
bool CRegisterAccountTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
        CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    //drop account
    CRegID accountId(nHeight, nIndex);
    CAccount oldAccount;
    if (!view.GetAccount(accountId, oldAccount))
        return state.DoS(100,
                ERRORMSG("ExecuteTx() : CRegisterAccountTx UndoExecuteTx, read secure account=%s info error", accountId.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    CKeyID keyId;
    view.GetKeyId(accountId, keyId);

    if (llFees > 0) {
        CAccountLog accountLog;
        if (!txundo.GetAccountOperLog(keyId, accountLog))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx UndoExecuteTx, read keyId=%s tx undo info error", keyId.GetHex()),
                    UPDATE_ACCOUNT_FAIL, "bad-read-txundoinfo");
        oldAccount.UndoOperateAccount(accountLog);
    }

    if (!oldAccount.IsEmptyValue()) {
        CPubKey empPubKey;
        oldAccount.PublicKey = empPubKey;
        oldAccount.MinerPKey = empPubKey;
        CUserID userId(keyId);
        view.SetAccount(userId, oldAccount);
    } else {
        view.EraseAccount(userId);
    }
    view.EraseId(accountId);
    return true;
}
bool CRegisterAccountTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    if (!boost::get<CPubKey>(userId).IsFullyValid()) {
        return false;
    }
    vAddr.insert(boost::get<CPubKey>(userId).GetKeyID());
    return true;
}
string CRegisterAccountTx::ToString(CAccountViewCache &view) const {
    string str;
    str += strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n",
    txTypeArray[nTxType],GetHash().ToString().c_str(), nVersion, boost::get<CPubKey>(userId).ToString(), llFees, boost::get<CPubKey>(userId).GetKeyID().ToAddress(), nValidHeight);
    return str;
}
Object CRegisterAccountTx::ToJSON(const CAccountViewCache &AccountView) const{
    Object result;

    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", txTypeArray[nTxType]));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("addr", boost::get<CPubKey>(userId).GetKeyID().ToAddress()));
    CID id(userId);
    CID minerIdTemp(minerId);
    result.push_back(Pair("pubkey", HexStr(id.GetID())));
    result.push_back(Pair("miner_pubkey", HexStr(minerIdTemp.GetID())));
    result.push_back(Pair("fees", llFees));
    result.push_back(Pair("height", nValidHeight));
   return result;
}
bool CRegisterAccountTx::CheckTransaction(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {

    if (userId.type() != typeid(CPubKey)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx userId must be CPubKey"),
                REJECT_INVALID, "userid-type-error");
    }

    if ((minerId.type() != typeid(CPubKey)) && (minerId.type() != typeid(CNullID))) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx minerId must be CPubKey or CNullID"),
                REJECT_INVALID, "minerid-type-error");
    }

    //check pubKey valid
    if (!boost::get<CPubKey>(userId).IsFullyValid()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterAccountTx CheckTransaction, register tx public key is invalid"),
                REJECT_INVALID, "bad-regtx-publickey");
    }

    //check signature script
    uint256 sighash = SignatureHash();
    if(!CheckSignScript(sighash, signature, boost::get<CPubKey>(userId))) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterAccountTx CheckTransaction, register tx signature error "),
                REJECT_INVALID, "bad-regtx-signature");
    }

    if (!MoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterAccountTx CheckTransaction, register tx fee out of range"),
                REJECT_INVALID, "bad-regtx-fee-toolarge");
    return true;
}

uint256 CRegisterAccountTx::GetHash() const {
    return SignatureHash();
}
uint256 CRegisterAccountTx::SignatureHash() const {
    CHashWriter ss(SER_GETHASH, 0);
    CID userPubkey(userId);
    CID minerPubkey(minerId);
    ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << userPubkey << minerPubkey << VARINT(llFees);
    return ss.GetHash();
}

bool CTransaction::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CAccount srcAcct;
    CAccount desAcct;
    CAccountLog desAcctLog;
    uint64_t minusValue = llFees+llValues;
    if (!view.GetAccount(srcRegId, srcAcct))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CTransaction ExecuteTx, read source addr %s account info error", boost::get<CRegID>(srcRegId).ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    CAccountLog srcAcctLog(srcAcct);
    if (!srcAcct.OperateAccount(MINUS_FREE, minusValue, nHeight))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CTransaction ExecuteTx, accounts insufficient funds"),
                UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");
    CUserID userId = srcAcct.keyID;
    if(!view.SetAccount(userId, srcAcct)){
        return state.DoS(100, ERRORMSG("UpdataAccounts() :CTransaction ExecuteTx, save account%s info error", boost::get<CRegID>(srcRegId).ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    uint64_t addValue = llValues;
    if (!view.GetAccount(desUserId, desAcct)) {
        if((COMMON_TX == nTxType) && (desUserId.type() == typeid(CKeyID))) {  // target account address not exist
            desAcct.keyID = boost::get<CKeyID>(desUserId);
            desAcctLog.keyID = desAcct.keyID;
        }
        else {
            return state.DoS(100, ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, get account info failed by regid:%s", boost::get<CRegID>(desUserId).ToString()),
                    UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    } else {
        desAcctLog.SetValue(desAcct);
    }
    if (!desAcct.OperateAccount(ADD_FREE, addValue, nHeight)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CTransaction ExecuteTx, operate accounts error"),
                UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");
    }
    if (!view.SetAccount(desUserId, desAcct)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CTransaction ExecuteTx, save account error, kyeId=%s", desAcct.keyID.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }
    txundo.vAccountLog.push_back(srcAcctLog);
    txundo.vAccountLog.push_back(desAcctLog);

    if (CONTRACT_TX == nTxType) {
        vector<unsigned char> vScript;
        if(!scriptDB.GetScript(boost::get<CRegID>(desUserId), vScript)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, read account faild, RegId=%s",
                    boost::get<CRegID>(desUserId).ToString()), READ_ACCOUNT_FAIL, "bad-read-account");
        }
        CVmRunEvn vmRunEvn;
        std::shared_ptr<CBaseTransaction> pTx = GetNewInstance();
        uint64_t el = GetFuelRate(scriptDB);
        int64_t llTime = GetTimeMillis();
        tuple<bool, uint64_t, string> ret = vmRunEvn.run(pTx, view, scriptDB, nHeight, el, nRunStep);
        if (!std::get<0>(ret))
            return state.DoS(100, ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, txhash=%s run script error:%s", GetHash().GetHex(), std::get<2>(ret)),
                    UPDATE_ACCOUNT_FAIL, "run-script-error:" + std::get<2>(ret));
        LogPrint("CONTRACT_TX", "execute contract elapse:%lld, txhash=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());
        set<CKeyID> vAddress;
        vector<std::shared_ptr<CAccount> > &vAccount = vmRunEvn.GetNewAccont();
        for (auto & itemAccount : vAccount) {  //更新对应的合约交易的账户信息
            vAddress.insert(itemAccount->keyID);
            userId = itemAccount->keyID;
            CAccount oldAcct;
            if(!view.GetAccount(userId, oldAcct)) {
                if(!itemAccount->keyID.IsNull()) {  //合约往未发生过转账记录地址转币
                    oldAcct.keyID = itemAccount->keyID;
                }else {
                return state.DoS(100, ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, read account info error"),
                        UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
                }
            }
            CAccountLog oldAcctLog(oldAcct);
            if (!view.SetAccount(userId, *itemAccount))
                return state.DoS(100, ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, write account info error"),
                        UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            txundo.vAccountLog.push_back(oldAcctLog);
        }
        txundo.vScriptOperLog.insert(txundo.vScriptOperLog.end(), vmRunEvn.GetDbLog()->begin(), vmRunEvn.GetDbLog()->end());
        vector<std::shared_ptr<CAppUserAccout> > &vAppUserAccount = vmRunEvn.GetRawAppUserAccount();
        for (auto & itemUserAccount : vAppUserAccount) {
            CKeyID itemKeyID;
            bool bValid = GetKeyId(view, itemUserAccount.get()->getaccUserId(), itemKeyID);
            if(bValid) {
                vAddress.insert(itemKeyID);
            }
        }

        if(!scriptDB.SetTxRelAccout(GetHash(), vAddress)) {
            return ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, save tx relate account info to script db error");
        }
    }
    txundo.txHash = GetHash();

    if(SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        CKeyID revKeyId;
        if(!view.GetKeyId(srcRegId, sendKeyId)) {
            return ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, get keyid by srcRegId error!");
        }
        if(!view.GetKeyId(desUserId, revKeyId)) {
            return ERRORMSG("ExecuteTx() : ContractTransaction ExecuteTx, get keyid by desUserId error!");
        }
        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
        if(!scriptDB.SetTxHashByAddress(revKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}
bool CTransaction::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(srcRegId, keyId))
        return false;

    vAddr.insert(keyId);
    CKeyID desKeyId;
    if (!view.GetKeyId(desUserId, desKeyId))
        return false;
    vAddr.insert(desKeyId);

    if (CONTRACT_TX == nTxType) {
        CVmRunEvn vmRunEvn;
        std::shared_ptr<CBaseTransaction> pTx = GetNewInstance();
        uint64_t el = GetFuelRate(scriptDB);
        CScriptDBViewCache scriptDBView(scriptDB, true);
        if (uint256() == pTxCacheTip->IsContainTx(GetHash())) {
            CAccountViewCache accountView(view, true);
            tuple<bool, uint64_t, string> ret = vmRunEvn.run(pTx, accountView, scriptDBView, chainActive.Height() + 1, el,
                    nRunStep);
            if (!std::get<0>(ret)) {
                return ERRORMSG("GetAddress()  : %s", std::get<2>(ret));
            }
            vector<shared_ptr<CAccount> > vpAccount = vmRunEvn.GetNewAccont();

            for (auto & item : vpAccount) {
                vAddr.insert(item->keyID);
            }

            vector<std::shared_ptr<CAppUserAccout> > &vAppUserAccount = vmRunEvn.GetRawAppUserAccount();
            for (auto & itemUserAccount : vAppUserAccount) {
                CKeyID itemKeyID;
                bool bValid = GetKeyId(view, itemUserAccount.get()->getaccUserId(), itemKeyID);
                if(bValid) {
                    vAddr.insert(itemKeyID);
                }
            }
        } else {
            set<CKeyID> vTxRelAccount;
            if (!scriptDBView.GetTxRelAccount(GetHash(), vTxRelAccount))
                return false;
            vAddr.insert(vTxRelAccount.begin(), vTxRelAccount.end());
        }
    }
    return true;
}
string CTransaction::ToString(CAccountViewCache &view) const {
    string str;
    string desId;
    if (desUserId.type() == typeid(CKeyID)) {
        desId = boost::get<CKeyID>(desUserId).ToString();
    } else if (desUserId.type() == typeid(CRegID)) {
        desId = boost::get<CRegID>(desUserId).ToString();
    }
    str += strprintf("txType=%s, hash=%s, ver=%d, srcId=%s desId=%s, llValues=%ld, llFees=%ld, vContract=%s, nValidHeight=%d\n",
    txTypeArray[nTxType], GetHash().ToString().c_str(), nVersion, boost::get<CRegID>(srcRegId).ToString(), desId.c_str(), llValues, llFees, HexStr(vContract).c_str(), nValidHeight);
    return str;
}

Object CTransaction::ToJSON(const CAccountViewCache &AccountView) const{
    Object result;
    CAccountViewCache view(AccountView);
    CKeyID keyid;

    auto getregidstring = [&](CUserID const &userId) {
        if(userId.type() == typeid(CRegID))
            return boost::get<CRegID>(userId).ToString();
        return string(" ");
    };

    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", txTypeArray[nTxType]));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid",  getregidstring(srcRegId)));
    view.GetKeyId(srcRegId, keyid);
    result.push_back(Pair("addr",  keyid.ToAddress()));
    result.push_back(Pair("desregid", getregidstring(desUserId)));
    view.GetKeyId(desUserId, keyid);
    result.push_back(Pair("desaddr", keyid.ToAddress()));
    result.push_back(Pair("money", llValues));
    result.push_back(Pair("fees", llFees));
    result.push_back(Pair("height", nValidHeight));
    result.push_back(Pair("contract", HexStr(vContract)));
    return result;
}
bool CTransaction::CheckTransaction(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {

    if(srcRegId.type() != typeid(CRegID)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CTransaction srcRegId must be CRegID"), REJECT_INVALID, "srcaddr-type-error");
    }

    if((desUserId.type() != typeid(CRegID)) && (desUserId.type() != typeid(CKeyID))) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CTransaction desUserId must be CRegID or CKeyID"), REJECT_INVALID, "desaddr-type-error");
    }

    if (!MoneyRange(llFees)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CTransaction CheckTransaction, appeal tx fee out of range"), REJECT_INVALID,
                "bad-appeal-fee-toolarge");
    }

    CAccount acctInfo;
    if (!view.GetAccount(boost::get<CRegID>(srcRegId), acctInfo)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() :CTransaction CheckTransaction, read account falied, regid=%s", boost::get<CRegID>(srcRegId).ToString()), REJECT_INVALID, "bad-getaccount");
    }
    if (!acctInfo.IsRegistered()) {
        return state.DoS(100, ERRORMSG("CheckTransaction(): CTransaction CheckTransaction, account have not registed public key"), REJECT_INVALID,
                "bad-no-pubkey");
    }

    uint256 sighash = SignatureHash();
    if (!CheckSignScript(sighash, signature, acctInfo.PublicKey)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CTransaction CheckTransaction, CheckSignScript failed"), REJECT_INVALID,
                "bad-signscript-check");
    }

    return true;
}
uint256 CTransaction::GetHash() const {
    return SignatureHash();
}
uint256 CTransaction::SignatureHash() const {
    CHashWriter ss(SER_GETHASH, 0);
    CID srcId(srcRegId);
    CID desId(desUserId);
    ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << srcId << desId << VARINT(llFees) << VARINT(llValues) << vContract;
    return ss.GetHash();
}


bool CRewardTransaction::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {

    CID id(account);
    if (account.type() != typeid(CRegID)) {
        return state.DoS(100,
                ERRORMSG("ExecuteTx() : CRewardTransaction ExecuteTx, account %s error, data type must be either CRegID", HexStr(id.GetID())),
                UPDATE_ACCOUNT_FAIL, "bad-account");
    }
    CAccount acctInfo;
    if (!view.GetAccount(account, acctInfo)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRewardTransaction ExecuteTx, read source addr %s account info error", HexStr(id.GetID())),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
//  LogPrint("op_account", "before operate:%s\n", acctInfo.ToString());
    CAccountLog acctInfoLog(acctInfo);
    if(0 == nIndex) {   //current block reward tx, need to clear coindays
//      acctInfo.ClearAccPos(nHeight);
    }
    else if(-1 == nIndex){ //maturity reward tx,only update values
        acctInfo.llValues += rewardValue;
    }
    else {  //never go into this step
        return ERRORMSG("nIndex type error!");
//      assert(0);
    }

    CUserID userId = acctInfo.keyID;
    if (!view.SetAccount(userId, acctInfo))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRewardTransaction ExecuteTx, write secure account info error"), UPDATE_ACCOUNT_FAIL,
                "bad-save-accountdb");
    txundo.Clear();
    txundo.vAccountLog.push_back(acctInfoLog);
    txundo.txHash = GetHash();
    if(SysCfg().GetAddressToTxFlag() && 0 == nIndex) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(account, sendKeyId)) {
            return ERRORMSG("ExecuteTx() : CRewardTransaction ExecuteTx, get keyid by account error!");
        }
        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
//  LogPrint("op_account", "after operate:%s\n", acctInfo.ToString());
    return true;
}
bool CRewardTransaction::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (account.type() == typeid(CRegID)) {
        if (!view.GetKeyId(account, keyId))
            return false;
        vAddr.insert(keyId);
    } else if (account.type() == typeid(CPubKey)) {
        CPubKey pubKey = boost::get<CPubKey>(account);
        if (!pubKey.IsFullyValid())
            return false;
        vAddr.insert(pubKey.GetKeyID());
    }
    return true;
}
string CRewardTransaction::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(account, keyId);
    CRegID regId;
    view.GetRegId(account, regId);
    str += strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyid=%s, rewardValue=%ld\n", txTypeArray[nTxType], GetHash().ToString().c_str(), nVersion, regId.ToString(), keyId.GetHex(), rewardValue);
    return str;
}
Object CRewardTransaction::ToJSON(const CAccountViewCache &AccountView) const{
    Object result;
    CAccountViewCache view(AccountView);
    CKeyID keyid;
    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", txTypeArray[nTxType]));
    result.push_back(Pair("ver", nVersion));
    if(account.type() == typeid(CRegID)) {
        result.push_back(Pair("regid", boost::get<CRegID>(account).ToString()));
    }
    if(account.type() == typeid(CPubKey)) {
        result.push_back(Pair("pubkey", boost::get<CPubKey>(account).ToString()));
    }
    view.GetKeyId(account, keyid);
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("money", rewardValue));
    result.push_back(Pair("height", nHeight));
    return std::move(result);
}
bool CRewardTransaction::CheckTransaction(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    return true;
}
uint256 CRewardTransaction::GetHash() const
{
    return SignatureHash();
}
uint256 CRewardTransaction::SignatureHash() const {
    CHashWriter ss(SER_GETHASH, 0);
    CID accId(account);
    ss << VARINT(nVersion) << nTxType << accId << VARINT(rewardValue) << VARINT(nHeight);
    return ss.GetHash();
}

bool CRegisterContractTx::ExecuteTx(int nIndex, CAccountViewCache &view,CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CID id(regAcctId);
    CAccount acctInfo;
    CScriptDBOperLog operLog;
    if (!view.GetAccount(regAcctId, acctInfo)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, read regist addr %s account info error", HexStr(id.GetID())),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccount acctInfoLog(acctInfo);
    uint64_t minusValue = llFees;
    if (minusValue > 0) {
        if(!acctInfo.OperateAccount(MINUS_FREE, minusValue, nHeight))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, operate account failed ,regId=%s", boost::get<CRegID>(regAcctId).ToString()),
                    UPDATE_ACCOUNT_FAIL, "operate-account-failed");
        txundo.vAccountLog.push_back(acctInfoLog);
    }
    txundo.txHash = GetHash();

    CVmScript vmScript;
    CDataStream stream(script, SER_DISK, CLIENT_VERSION);
    try {
        stream >> vmScript;
    } catch (exception& e) {
        return state.DoS(100, ERRORMSG(("ExecuteTx() :CRegisterContractTx ExecuteTx, Unserialize to vmScript error:" + string(e.what())).c_str()),
                UPDATE_ACCOUNT_FAIL, "unserialize-script-error");
    }
    if(!vmScript.IsValid())
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, vmScript invalid"), UPDATE_ACCOUNT_FAIL, "script-check-failed");

    CRegID regId(nHeight, nIndex);
    //create script account
    CKeyID keyId = Hash160(regId.GetVec6());
    CAccount account;
    account.keyID = keyId;
    account.regID = regId;
    //save new script content
    if(!scriptDB.SetScript(regId, script)){
        return state.DoS(100,
                ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, save script id %s script info error", regId.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }
    if (!view.SaveAccountInfo(regId, keyId, account)) {
        return state.DoS(100,
                ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx create new account script id %s script info error",
                        regId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }

    nRunStep = script.size();

    if(!operLog.vKey.empty()) {
        txundo.vScriptOperLog.push_back(operLog);
    }
    CUserID userId = acctInfo.keyID;
    if (!view.SetAccount(userId, acctInfo))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, save account info error"), UPDATE_ACCOUNT_FAIL,
                "bad-save-accountdb");

    if(SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(regAcctId, sendKeyId)) {
            return ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, get regAcctId by account error!");
        }
        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    return true;
}
bool CRegisterContractTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CID id(regAcctId);
    CAccount account;
    CUserID userId;
    if (!view.GetAccount(regAcctId, account)) {
        return state.DoS(100, ERRORMSG("UndoUpdateAccount() : CRegisterContractTx UndoExecuteTx, read regist addr %s account info error", HexStr(id.GetID())),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }

    if(script.size() != 6) {

        CRegID scriptId(nHeight, nIndex);
        //delete script content
        if (!scriptDB.EraseScript(scriptId)) {
            return state.DoS(100, ERRORMSG("UndoUpdateAccount() : CRegisterContractTx UndoExecuteTx, erase script id %s error", scriptId.ToString()),
                    UPDATE_ACCOUNT_FAIL, "erase-script-failed");
        }
        //delete account
        if(!view.EraseId(scriptId)){
            return state.DoS(100, ERRORMSG("UndoUpdateAccount() : CRegisterContractTx UndoExecuteTx, erase script account %s error", scriptId.ToString()),
                                UPDATE_ACCOUNT_FAIL, "erase-appkeyid-failed");
        }
        CKeyID keyId = Hash160(scriptId.GetVec6());
        userId = keyId;
        if(!view.EraseAccount(userId)){
            return state.DoS(100, ERRORMSG("UndoUpdateAccount() : CRegisterContractTx UndoExecuteTx, erase script account %s error", scriptId.ToString()),
                                UPDATE_ACCOUNT_FAIL, "erase-appaccount-failed");
        }
//      LogPrint("INFO", "Delete regid %s app account\n", scriptId.ToString());
    }

    for(auto &itemLog : txundo.vAccountLog){
        if(itemLog.keyID == account.keyID) {
            if(!account.UndoOperateAccount(itemLog))
                return state.DoS(100, ERRORMSG("UndoUpdateAccount: CRegisterContractTx UndoExecuteTx, undo operate account error, keyId=%s", account.keyID.ToString()),
                        UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    for(; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if(!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100,
                    ERRORMSG("ExecuteTx() : CRegisterContractTx UndoExecuteTx, undo scriptdb data error"), UPDATE_ACCOUNT_FAIL, "undo-scriptdb-failed");
    }
    userId = account.keyID;
    if (!view.SetAccount(userId, account))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx UndoExecuteTx, save account error"), UPDATE_ACCOUNT_FAIL,
                "bad-save-accountdb");
    return true;
}
bool CRegisterContractTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(regAcctId, keyId))
        return false;
    vAddr.insert(keyId);
    return true;
}
string CRegisterContractTx::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(regAcctId, keyId);
    str += strprintf("txType=%s, hash=%s, ver=%d, accountId=%s, keyid=%s, llFees=%ld, nValidHeight=%d\n",
    txTypeArray[nTxType], GetHash().ToString().c_str(), nVersion,boost::get<CRegID>(regAcctId).ToString(), keyId.GetHex(), llFees, nValidHeight);
    return str;
}
Object CRegisterContractTx::ToJSON(const CAccountViewCache &AccountView) const{
    Object result;
    CAccountViewCache view(AccountView);
    CKeyID keyid;
    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", txTypeArray[nTxType]));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid",  boost::get<CRegID>(regAcctId).ToString()));
    view.GetKeyId(regAcctId, keyid);
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("script", "script_content"));
    result.push_back(Pair("fees", llFees));
    result.push_back(Pair("height", nValidHeight));
    return result;
}
bool CRegisterContractTx::CheckTransaction(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {

    if (regAcctId.type() != typeid(CRegID)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx regAcctId must be CRegID"), REJECT_INVALID,
                "regacctid-type-error");
    }

    if (!MoneyRange(llFees)) {
            return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx CheckTransaction, tx fee out of range"), REJECT_INVALID,
                    "fee-too-large");
    }

    uint64_t llFuel = ceil(script.size()/100) * GetFuelRate(scriptDB);
    if (llFuel < 1 * COIN) {
        llFuel = 1 * COIN;
    }

    if( llFees < llFuel) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx CheckTransaction, register app tx fee too litter (actual:%lld vs need:%lld)", llFees, llFuel),
                REJECT_INVALID, "fee-too-litter");
    }

    CAccount acctInfo;
    if (!view.GetAccount(boost::get<CRegID>(regAcctId), acctInfo)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx CheckTransaction, get account falied"),
                REJECT_INVALID, "bad-getaccount");
    }
    if (!acctInfo.IsRegistered()) {
        return state.DoS(100, ERRORMSG("CheckTransaction(): CRegisterContractTx CheckTransaction, account have not registed public key"),
                REJECT_INVALID, "bad-no-pubkey");
    }
    uint256 signhash = SignatureHash();
    if (!CheckSignScript(signhash, signature, acctInfo.PublicKey)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx CheckTransaction, CheckSignScript failed"),
                REJECT_INVALID, "bad-signscript-check");
    }
    return true;
}
uint256 CRegisterContractTx::GetHash() const
{
    return SignatureHash();
}
uint256 CRegisterContractTx::SignatureHash() const {
    CHashWriter ss(SER_GETHASH, 0);
    CID regAccId(regAcctId);
    ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << regAccId << script << VARINT(llFees);
    return ss.GetHash();
}

bool CDelegateTransaction::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
    CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CID id(userId);
    CAccount acctInfo;
    if (!view.GetAccount(userId, acctInfo)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, read regist addr %s account info error", HexStr(id.GetID())),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccount acctInfoLog(acctInfo);
    uint64_t minusValue = llFees;
    if (minusValue > 0) {
        if(!acctInfo.OperateAccount(MINUS_FREE, minusValue, nHeight))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, operate account failed ,regId=%s", boost::get<CRegID>(userId).ToString()),
                UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }
    if (!acctInfo.ProcessDelegateVote(operVoteFunds, nHeight)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, operate delegate vote failed ,regId=%s", boost::get<CRegID>(userId).ToString()),
            UPDATE_ACCOUNT_FAIL, "operate-delegate-failed");
    }
    if (!view.SaveAccountInfo(acctInfo.regID, acctInfo.keyID, acctInfo)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx create new account script id %s script info error", acctInfo.regID.ToString()),
                    UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }
    txundo.vAccountLog.push_back(acctInfoLog);
    txundo.txHash = GetHash();

    for (auto iter = operVoteFunds.begin(); iter != operVoteFunds.end(); ++iter) {
        CAccount delegate;
        if (!view.GetAccount(CUserID(iter->fund.pubKey), delegate)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, read regist addr %s account info error", iter->fund.pubKey.GetKeyID().ToAddress()),
                    UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccount delegateAcctLog(delegate);
        if (!delegate.OperateVote(VoteOperType(iter->operType), iter->fund.value)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, operate delegate address %s vote fund error", iter->fund.pubKey.GetKeyID().ToAddress()),
                    UPDATE_ACCOUNT_FAIL, "operate-vote-error");
        }
        txundo.vAccountLog.push_back(delegateAcctLog);
        // set the new value and erase the old value
        CScriptDBOperLog operDbLog;
        if (!scriptDB.SetDelegateData(delegate, operDbLog)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, erase account id %s vote info error", delegate.regID.ToString()),
                    UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
        txundo.vScriptOperLog.push_back(operDbLog);

        CScriptDBOperLog eraseDbLog;
        if (delegateAcctLog.llVotes > 0) {
            if(!scriptDB.EraseDelegateData(delegateAcctLog, eraseDbLog)) {
                return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, erase account id %s vote info error", delegateAcctLog.regID.ToString()),
                        UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            }
        }
        txundo.vScriptOperLog.push_back(eraseDbLog);

        if (!view.SaveAccountInfo(delegate.regID, delegate.keyID, delegate)) {
                return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx create new account script id %s script info error", acctInfo.regID.ToString()),
                        UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
    }

    if(SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(userId, sendKeyId)) {
            return ERRORMSG("ExecuteTx() : CDelegateTransaction ExecuteTx, get regAcctId by account error!");
        }
        if(!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    return true;
}

string CDelegateTransaction::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(userId, keyId);
    str += strprintf("txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n", txTypeArray[nTxType],
        GetHash().ToString().c_str(), nVersion, keyId.ToAddress(), keyId.ToString());
    str += "vote:\n";
    for(auto item=operVoteFunds.begin(); item!=operVoteFunds.end(); ++item) {
        str += strprintf("%s", item->ToString());
    }
    return str;
}

Object CDelegateTransaction::ToJSON(const CAccountViewCache &AccountView) const {
    Object result;
    CAccountViewCache view(AccountView);
    CKeyID keyid;
    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", txTypeArray[nTxType]));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid", boost::get<CRegID>(userId).ToString()));
    view.GetKeyId(userId, keyid);
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("fees", llFees));
    Array operVoteFundArray;
    for(auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        operVoteFundArray.push_back(item->ToJson(true));
    }
    result.push_back(Pair("operVoteFundList", operVoteFundArray));
    return std::move(result);
}

bool CDelegateTransaction::CheckTransaction(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CID id(userId);
    if(userId.type() != typeid(CRegID)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction send account is not CRegID type"), REJECT_INVALID, "deletegate-tx-error");
    }
    if(0 == operVoteFunds.size()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction the deletegate oper fund empty"), REJECT_INVALID,
                           "oper-fund-empty-error");
    }
    if(operVoteFunds.size() > IniCfg().GetDelegatesNum()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction the deletegates number a transaction can't exceeds maximum"), REJECT_INVALID,
                    "deletegates-number-error");
    }
    if (!MoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction CheckTransaction, delegate tx fee out of range"), REJECT_INVALID,
                "bad-regtx-fee-toolarge");
    CKeyID sendTxKeyID;
    if(!view.GetKeyId(userId, sendTxKeyID)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction get keyId error by CUserID =%s", HexStr(id.GetID())), REJECT_INVALID, "");
    }

    CAccount sendAcctInfo;
    if (!view.GetAccount(userId, sendAcctInfo)) {
     return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction get account info error, userid=%s", HexStr(id.GetID())),
             REJECT_INVALID, "bad-read-accountdb");
    }

    //check account delegates number;
    set<CKeyID> setTotalOperVoteKeyID;
    for(auto operItem : sendAcctInfo.vVoteFunds) {
        setTotalOperVoteKeyID.insert(operItem.pubKey.GetKeyID());
    }

    //check delegate duplication
    set<CKeyID> setOperVoteKeyID;
    uint64_t totalVotes = 0;
    for(auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        if (0 >= item->fund.value || (uint64_t)GetMaxMoney() < item->fund.value )
            return ERRORMSG("votes:%lld too larger than MaxVote or less than 0", item->fund.value);
        setOperVoteKeyID.insert(item->fund.pubKey.GetKeyID());
        setTotalOperVoteKeyID.insert(item->fund.pubKey.GetKeyID());
        CAccount acctInfo;
        if (!view.GetAccount(CUserID(item->fund.pubKey), acctInfo)) {
            return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction get account info error, address=%s", item->fund.pubKey.GetKeyID().ToAddress()),
                    REJECT_INVALID, "bad-read-accountdb");
        }
        if(item->fund.value > totalVotes)
            totalVotes = item->fund.value;
    }

    if(setTotalOperVoteKeyID.size() > IniCfg().GetDelegatesNum()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction the delegates number of account can't exceeds maximum"), REJECT_INVALID,
                           "account-delegates-number-error");
    }

    if(setOperVoteKeyID.size() != operVoteFunds.size()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction duplication vote fund"), REJECT_INVALID,
                           "deletegates-duplication fund-error");
    }
    if(totalVotes > sendAcctInfo.llValues) {
       return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTransaction delegate votes exceeds than account balance, userid=%s", HexStr(id.GetID())),
                      REJECT_INVALID, "insufficient balance for votes");
    }
    return true;
}

bool CDelegateTransaction::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(userId, keyId))
        return false;
    vAddr.insert(keyId);
    for(auto iter = operVoteFunds.begin(); iter != operVoteFunds.end(); ++iter) {
        vAddr.insert(iter->fund.pubKey.GetKeyID());
    }
    return true;
}

uint256 CDelegateTransaction::GetHash() const
{
    return SignatureHash();
}

uint256 CDelegateTransaction::SignatureHash() const {
    CHashWriter ss(SER_GETHASH, 0);
    CID regAccId(userId);
    ss << VARINT(nVersion) << nTxType << VARINT(nValidHeight) << regAccId << operVoteFunds << VARINT(llFees);
    return ss.GetHash();
}

string CAccountLog::ToString() const {
    string str("");
    str += strprintf("    Account log: keyId=%d llValues=%lld nVoteHeight=%lld llVotes=%lld \n",
            keyID.GetHex(), llValues, nVoteHeight, llVotes);
     str += string("    vote fund:");
    for(auto it =  vVoteFunds.begin(); it != vVoteFunds.end(); ++it) {
        str += strprintf("    address=%s, vote=%lld\n", it->pubKey.GetKeyID().ToAddress(), it->value);
    }
    return str;
}

string CTxUndo::ToString() const {
    vector<CAccountLog>::const_iterator iterLog = vAccountLog.begin();
    string strTxHash("txHash:");
    strTxHash += txHash.GetHex();
    strTxHash += "\n";
    string str("  list account Log:\n");
    for (; iterLog != vAccountLog.end(); ++iterLog) {
        str += iterLog->ToString();
    }
    strTxHash += str;
    vector<CScriptDBOperLog>::const_iterator iterDbLog = vScriptOperLog.begin();
    string strDbLog(" list script db Log:\n");
    for (; iterDbLog !=  vScriptOperLog.end(); ++iterDbLog) {
        strDbLog += iterDbLog->ToString();
    }
    strTxHash += strDbLog;
    return strTxHash;
}

bool CTxUndo::GetAccountOperLog(const CKeyID &keyId, CAccountLog &accountLog) {
    vector<CAccountLog>::iterator iterLog = vAccountLog.begin();
    for (; iterLog != vAccountLog.end(); ++iterLog) {
        if (iterLog->keyID == keyId) {
            accountLog = *iterLog;
            return true;
        }
    }
    return false;
}

bool CAccount::UndoOperateAccount(const CAccountLog & accountLog) {
    LogPrint("undo_account", "after operate:%s\n", ToString());
    llValues    = accountLog.llValues;
    nVoteHeight = accountLog.nVoteHeight;
    vVoteFunds  = accountLog.vVoteFunds;
    llVotes     = accountLog.llVotes;
    LogPrint("undo_account", "before operate:%s\n", ToString().c_str());
    return true;
}

uint64_t CAccount::GetAccountProfit(uint64_t nCurHeight) {
     if (vVoteFunds.empty()) {
        LogPrint("DEBUG", "1st-time vote for the account, hence no minting of interest.");
        nVoteHeight = nCurHeight; //record the 1st-time vote block height into account
        return 0; // 0 profit for 1st-time vote
    }

    // 先判断计算分红的上下限区块高度是否落在同一个分红率区间
    uint64_t nBeginHeight = nVoteHeight;
    uint64_t nEndHeight = nCurHeight;
    uint64_t nBeginSubsidy = IniCfg().GetBlockSubsidyCfg(nVoteHeight);
    uint64_t nEndSubsidy = IniCfg().GetBlockSubsidyCfg(nCurHeight);
    uint64_t nValue = vVoteFunds.begin()->value;
    LogPrint("profits", "nBeginSubsidy:%lld nEndSubsidy:%lld nBeginHeight:%d nEndHeight:%d\n",
        nBeginSubsidy, nEndSubsidy, nBeginHeight, nEndHeight);

    // 计算分红
    auto calculateProfit = [](uint64_t nValue, uint64_t nSubsidy, int nBeginHeight, int nEndHeight) -> uint64_t {
        int64_t nHoldHeight = nEndHeight - nBeginHeight;
        int64_t nYearHeight = SysCfg().GetSubsidyHalvingInterval();
        uint64_t llProfits =  (uint64_t)(nValue * ((long double)nHoldHeight * nSubsidy / nYearHeight / 100));
        LogPrint("profits", "nValue:%lld nSubsidy:%lld nBeginHeight:%d nEndHeight:%d llProfits:%lld\n",
            nValue, nSubsidy, nBeginHeight, nEndHeight, llProfits);
        return llProfits;
    };

    // 如果属于同一个分红率区间，分红=区块高度差（持有高度）* 分红率；如果不属于同一个分红率区间，则需要根据分段函数累加每一段的分红
    uint64_t llProfits = 0;
    uint64_t nSubsidy = nBeginSubsidy;
    while (nSubsidy != nEndSubsidy) {
        int nJumpHeight = IniCfg().GetBlockSubsidyJumpHeight(nSubsidy - 1);
        llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nJumpHeight);
        nBeginHeight = nJumpHeight;
        nSubsidy -= 1;
    }

    llProfits += calculateProfit(nValue, nSubsidy, nBeginHeight, nEndHeight);
    LogPrint("profits", "updateHeight:%d curHeight:%d freeze value:%lld\n",
        nVoteHeight, nCurHeight, vVoteFunds.begin()->value);

    nVoteHeight = nCurHeight;
    return llProfits;
}

uint64_t CAccount::GetRawBalance() {
    return llValues;
}

uint64_t CAccount::GetTotalBalance() {
    if(!vVoteFunds.empty())
        return vVoteFunds.begin()->value + llValues;
    return llValues;
}

uint64_t CAccount::GetFrozenBalance() {
    uint64_t votes = 0;
    for (auto it = vVoteFunds.begin(); it != vVoteFunds.end(); it++) {
      if(it->value > votes) {
          votes = it->value;
      }
    }
    return votes;
}

Object CAccount::ToJsonObj(bool isAddress) const
{
    Array voteFundArray;
    for(auto & fund : vVoteFunds) { voteFundArray.push_back(fund.ToJson(true)); }
    Object obj;
    obj.push_back(Pair("Address",       keyID.ToAddress()));
    obj.push_back(Pair("KeyID",         keyID.ToString()));
    obj.push_back(Pair("PublicKey",     PublicKey.ToString()));
    obj.push_back(Pair("MinerPKey",     MinerPKey.ToString()));
    obj.push_back(Pair("RegID",         regID.ToString()));
    obj.push_back(Pair("Balance",       llValues));
    obj.push_back(Pair("UpdateHeight",  nVoteHeight));
    obj.push_back(Pair("Votes",         llVotes));
    obj.push_back(Pair("voteFundList",  voteFundArray));
    return obj;
}

string CAccount::ToString(bool isAddress) const {
    string str;
    str += strprintf("regID=%s, keyID=%s, publicKey=%s, minerpubkey=%s, values=%ld updateHeight=%d llVotes=%lld\n",
        regID.ToString(), keyID.GetHex().c_str(), PublicKey.ToString().c_str(),
        MinerPKey.ToString().c_str(), llValues, nVoteHeight, llVotes);
    str += "vVoteFunds list: \n";
    for (auto & fund : vVoteFunds) {
        str += fund.ToString(isAddress);
    }
    return str;
}

bool CAccount::IsMoneyOverflow(uint64_t nAddMoney) {
    if (!MoneyRange(nAddMoney))
        return ERRORMSG("money:%lld too larger than MaxMoney");
    return true;
}

bool CAccount::OperateAccount(OperType type, const uint64_t &value, const uint64_t nCurHeight) {
    LogPrint("op_account", "before operate:%s\n", ToString());
    if (!IsMoneyOverflow(value))
        return false;
    if (keyID == uint160()) {
        return ERRORMSG("operate account's keyId is 0 error");
    }
    if (!value)
        return true;
    switch (type) {
    case ADD_FREE: {
        llValues += value;
        if (!IsMoneyOverflow(llValues))
            return false;
        break;
    }
    case MINUS_FREE: {
        if (value > llValues)
            return false;
        llValues -= value;
        break;
    }
    default:
        return ERRORMSG("operate account type error!");
    }
    LogPrint("op_account", "after operate:%s\n", ToString());
    return true;
}

bool CAccount::ProcessDelegateVote(vector<COperVoteFund> & operVoteFunds, const uint64_t nCurHeight)
{
    if (nCurHeight < nVoteHeight) {
        LogPrint("ERROR", "current vote tx height (%d) can't be smaller than the last nVoteHeight (%d)",
            nCurHeight, nVoteHeight);
        return false;
    }

    uint64_t llProfit = GetAccountProfit(nCurHeight);
    if (!IsMoneyOverflow(llProfit)) return false;

    for (auto operVote = operVoteFunds.begin(); operVote != operVoteFunds.end(); ++operVote) {
        CPubKey pubKey = operVote->fund.pubKey;
        vector<CVoteFund>::iterator itfund = find_if(vVoteFunds.begin(), vVoteFunds.end(),
            [pubKey](CVoteFund fund){ return fund.pubKey == pubKey; });

        int voteType = VoteOperType(operVote->operType);
        if (ADD_FUND == voteType) {
            if (itfund != vVoteFunds.end()) {
                if (!IsMoneyOverflow(operVote->fund.value))
                     return ERRORMSG("ProcessDelegateVote() : oper fund value exceed maximum ");
                itfund->value += operVote->fund.value;
                if (!IsMoneyOverflow(itfund->value))
                     return ERRORMSG("ProcessDelegateVote() : fund value exceeds maximum");
            } else {
               vVoteFunds.push_back(operVote->fund);
               if(vVoteFunds.size() > IniCfg().GetDelegatesNum()) {
                   return ERRORMSG("ProcessDelegateVote() : fund number exceeds maximum");
               }
            }
        } else if(MINUS_FUND == voteType) {
            if  (itfund != vVoteFunds.end()) {
                if (!IsMoneyOverflow(operVote->fund.value))
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceed maximum ");
                if(itfund->value < operVote->fund.value) {
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceed delegate fund value");
                }
                itfund->value -= operVote->fund.value;
                if(0 == itfund->value) {
                    vVoteFunds.erase(itfund);
                }
            } else {
                return ERRORMSG("ProcessDelegateVote() : CDelegateTransaction ExecuteTx AccountVoteOper revocation votes are not exist");
            }
        } else {
            return ERRORMSG("ProcessDelegateVote() : operType: %d invalid", voteType);
        }
    }

    std::sort(vVoteFunds.begin(),vVoteFunds.end(), [](CVoteFund fund1, CVoteFund fund2) {
        return fund1.value > fund2.value;
    });

    int64_t newTotalVotes = 0;
    if (!vVoteFunds.empty())
        newTotalVotes = vVoteFunds.begin()->value;

    int64_t totalVotes = vVoteFunds.empty() ? 0 : (int64_t) vVoteFunds.begin()->value;
    if (llValues + (uint64_t) totalVotes < (uint64_t) newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVote() : delegate value exceed account value");
    }
    llValues += totalVotes - newTotalVotes;
    llValues += llProfit;
    LogPrint("profits", "receive profits:%lld\n", llProfit);
    return true;
}

bool CAccount::OperateVote(VoteOperType type, const uint64_t & values) {
    if(ADD_FUND == type) {
        llVotes += values;
        if(!IsMoneyOverflow(llVotes)) {
            return ERRORMSG("OperateVote() : delegates total votes exceed maximum ");
        }
    } else if (MINUS_FUND == type) {
        if(llVotes < values) {
            return ERRORMSG("OperateVote() : delegates total votes less than revocation vote value");
        }
        llVotes -= values;
    } else {
        return ERRORMSG("OperateVote() : CDelegateTransaction ExecuteTx AccountVoteOper revocation votes are not exist");
    }
    return true;
}

string COperVoteFund::ToString(bool isAddress) const {
    string str = strprintf("operVoteType=%s %s", voteOperTypeArray[operType], fund.ToString(isAddress));
    return str;
}

Object COperVoteFund::ToJson(bool isAddress) const {
    Object obj;
    string sOperType;
    if (operType >= 3) {
        sOperType = "INVALID_OPER_TYPE";
        LogPrint("ERROR", "Delegate Vote Tx contains invalid operType: %d", operType);
    } else {
        sOperType = voteOperTypeArray[operType];
    }
    obj.push_back(Pair("operType", sOperType));
    obj.push_back(Pair("voteFund", fund.ToJson(isAddress)));
    return obj;
}

Object CVoteFund::ToJson(bool isAddress) const{
    Object obj;
    if(isAddress) {
        obj.push_back(Pair("address", pubKey.GetKeyID().ToAddress()));
    } else {
        obj.push_back(Pair("pubkey", pubKey.ToString()));
    }
    obj.push_back(Pair("value", value));
    return obj;
}
