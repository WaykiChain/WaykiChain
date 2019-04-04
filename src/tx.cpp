#include "serialize.h"
#include <boost/foreach.hpp>
#include "hash.h"
#include "util.h"
#include "database.h"
#include "main.h"
#include <algorithm>
#include "txdb.h"
#include "vm/vmrunenv.h"
#include "core.h"
#include "miner.h"
#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
using namespace json_spirit;

string CBaseTx::txTypeArray[7] = { "NULL_TXTYPE", "REWARD_TX", "REG_ACCT_TX", "COMMON_TX", "CONTRACT_TX", "REG_APP_TX", "DELEGATE_TX"};
string COperVoteFund::voteOperTypeArray[3] = {"NULL_OPER", "ADD_FUND", "MINUS_FUND"};

static bool GetKeyId(const CAccountViewCache &view, const vector<unsigned char> &ret, CKeyID &KeyId)
{
    if (ret.size() == 6) {
        CRegID regId(ret);
        KeyId = regId.GetKeyID(view);
    } else if (ret.size() == 34) {
        string addr(ret.begin(), ret.end());
        KeyId = CKeyID(addr);
    } else {
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

bool CRegID::Clean()
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

    keyId = regId.GetKeyID(*pAccountViewTip);
    return !keyId.IsEmpty();
}

bool CRegID::IsRegIdStr(const string & str)
{
    bool ret = IsSimpleRegIdStr(str) || (str.length() == 12);
    return ret;
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
    } else if (strRegID.length() == 12) {
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

CKeyID CRegID::GetKeyID(const CAccountViewCache &view)const
{
    CKeyID ret;
    CAccountViewCache(view).GetKeyId(*this, ret);
    return ret;
}

void CRegID::SetRegIDByCompact(const vector<unsigned char> &vIn)
{
    if (vIn.size() > 0) {
        CDataStream ds(vIn, SER_DISK, CLIENT_VERSION);
        ds >> *this;
    } else {
        Clean();
    }
}

bool CBaseTx::IsValidHeight(int nCurrHeight, int nTxCacheHeight) const
{
    if(REWARD_TX == nTxType)
        return true;
    if (nValidHeight > nCurrHeight + nTxCacheHeight / 2)
        return false;
    if (nValidHeight < nCurrHeight - nTxCacheHeight / 2)
        return false;
    return true;
}

bool CBaseTx::UndoExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                            CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                            CScriptDBViewCache &scriptDB) {
    vector<CAccountLog>::reverse_iterator rIterAccountLog = txundo.vAccountLog.rbegin();
    for (; rIterAccountLog != txundo.vAccountLog.rend(); ++rIterAccountLog) {
        CAccount account;
        CUserID userId = rIterAccountLog->keyID;
        if (!view.GetAccount(userId, account)) {
            return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, read account info error"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        if (!account.UndoOperateAccount(*rIterAccountLog)) {
            return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, undo operate account failed"),
                             UPDATE_ACCOUNT_FAIL, "undo-operate-account-failed");
        }

        if (COMMON_TX == nTxType &&
            (account.IsEmptyValue() &&
             (!account.pubKey.IsFullyValid() || account.pubKey.GetKeyID() != account.keyID))) {
            view.EraseAccount(userId);
        } else if (COMMON_TX == nTxType && account.regID == CRegID(nHeight, nIndex)) {
            // If the CRegID was generated by this COMMON_TX, need to remove CRegID.
            CPubKey empPubKey;
            account.pubKey      = empPubKey;
            account.minerPubKey = empPubKey;
            account.regID.Clean();
            if (!view.SetAccount(userId, account)) {
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }

            view.EraseId(CRegID(nHeight, nIndex));
        } else {
            if (!view.SetAccount(userId, account)) {
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, write account info error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");
            }
        }
    }

    if (DELEGATE_TX == nTxType) {
        vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog =
            txundo.vScriptOperLog.rbegin();
        if (SysCfg().GetAddressToTxFlag() && txundo.vScriptOperLog.size() > 0) {
            if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, undo scriptdb data error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            ++rIterScriptDBLog;
        }

        for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
            // Recover the old value and erase the new value.
            if (!scriptDB.SetDelegateData(rIterScriptDBLog->vKey))
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, set delegate data error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");

            ++rIterScriptDBLog;
            if (!scriptDB.EraseDelegateData(rIterScriptDBLog->vKey))
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, erase delegate data error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
    } else {
        vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog =
            txundo.vScriptOperLog.rbegin();
        for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
            if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
                return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, undo scriptdb data error"),
                                 UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
    }

    if (CONTRACT_TX == nTxType) {
        if (!scriptDB.EraseTxRelAccout(GetHash()))
            return state.DoS(100, ERRORMSG("CBaseTx::UndoExecuteTx, erase tx rel account error"),
                             UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }
    return true;
}

uint64_t CBaseTx::GetFuel(int nfuelRate) {
    uint64_t llFuel = ceil(nRunStep/100.0f) * nfuelRate;
    if (REG_CONT_TX == nTxType) {
        if (llFuel < 1 * COIN) {
            llFuel = 1 * COIN;
        }
    }
    return llFuel;
}

int CBaseTx::GetFuelRate(CScriptDBViewCache &scriptDB) {
    if (nFuelRate > 0)
        return nFuelRate;

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
    } else {
        nFuelRate = GetElementForBurn(chainActive.Tip());
    }

    return nFuelRate;
}

// check the fees must be more than nMinTxFee
bool CBaseTx::CheckMinTxFee(uint64_t llFees) {
    NET_TYPE networkID = SysCfg().NetworkID();
    if ( (networkID == MAIN_NET && nValidHeight > kCheckTxFeeForkHeight) //for mainnet, need hardcode here, compatible with old data
        || (networkID == TEST_NET && nValidHeight > 100000) // for testnet, need hardcode here, compatible with old data
        || (networkID == REGTEST_NET) ) {  // for regtest net, must do the check
        return llFees >= nMinTxFee;
    }
    // else  no need to check MinTxFee
    return true;
}

// transactions should check the signagure size before verifying signature
bool CBaseTx::CheckSignatureSize(vector<unsigned char> &signature) {
    return signature.size() > 0 && signature.size() < MAX_BLOCK_SIGNATURE_SIZE;
}

bool CRegisterAccountTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                                   CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                                   CScriptDBViewCache &scriptDB) {
    CAccount account;
    CRegID regId(nHeight, nIndex);
    CKeyID keyId = boost::get<CPubKey>(userId).GetKeyID();
    if (!view.GetAccount(userId, account))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, read source keyId %s account info error",
            keyId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccountLog acctLog(account);
    if (account.pubKey.IsFullyValid() && account.pubKey.GetKeyID() == keyId)
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, read source keyId %s duplicate register",
            keyId.ToString()), UPDATE_ACCOUNT_FAIL, "duplicate-register-account");

    account.pubKey = boost::get<CPubKey>(userId);
    if (llFees > 0)
        if (!account.OperateAccount(MINUS_FREE, llFees, nHeight))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, not sufficient funds in account, keyid=%s",
                keyId.ToString()), UPDATE_ACCOUNT_FAIL, "not-sufficiect-funds");

    account.regID = regId;
    if (typeid(CPubKey) == minerId.type()) {
        account.minerPubKey = boost::get<CPubKey>(minerId);
        if (account.minerPubKey.IsValid() && !account.minerPubKey.IsFullyValid()) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, minerPubKey:%s Is Invalid",
                account.minerPubKey.ToString()), UPDATE_ACCOUNT_FAIL, "MinerPKey Is Invalid");
        }
    }

    if (!view.SaveAccountInfo(regId, keyId, account))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, write source addr %s account info error",
            regId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

    txundo.vAccountLog.push_back(acctLog);
    txundo.txHash = GetHash();
    if(SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if(!view.GetKeyId(userId, sendKeyId))
            return ERRORMSG("ExecuteTx() : CRegisterAccountTx ExecuteTx, get keyid by userId error!");

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
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterAccountTx UndoExecuteTx, read secure account=%s info error",
            accountId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");

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
        oldAccount.pubKey = empPubKey;
        oldAccount.minerPubKey = empPubKey;
        oldAccount.regID.Clean();
        CUserID userId(keyId);
        view.SetAccount(userId, oldAccount);
    } else {
        view.EraseAccount(userId);
    }
    view.EraseId(accountId);
    return true;
}

bool CRegisterAccountTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    if (!boost::get<CPubKey>(userId).IsFullyValid())
        return false;

    vAddr.insert(boost::get<CPubKey>(userId).GetKeyID());
    return true;
}

string CRegisterAccountTx::ToString(CAccountViewCache &view) const {
    string str;
    str += strprintf("txType=%s, hash=%s, ver=%d, pubkey=%s, llFees=%ld, keyid=%s, nValidHeight=%d\n",
        txTypeArray[nTxType], GetHash().ToString().c_str(), nVersion, boost::get<CPubKey>(userId).ToString(),
        llFees, boost::get<CPubKey>(userId).GetKeyID().ToAddress(), nValidHeight);

    return str;
}

Object CRegisterAccountTx::ToJson(const CAccountViewCache &AccountView) const {
    CID userCID(userId);
    CID minerCID(minerId);
    string address = boost::get<CPubKey>(userId).GetKeyID().ToAddress();
    string userPubKey = HexStr(userCID.GetID());
    string userMinerPubKey = HexStr(minerCID.GetID());

    Object result;
    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("txtype",         txTypeArray[nTxType]));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("addr",           address));
    result.push_back(Pair("pubkey",         userPubKey));
    result.push_back(Pair("minerpubkey",    userMinerPubKey));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("height",         nValidHeight));
    return result;
}

bool CRegisterAccountTx::CheckTransaction(CValidationState &state, CAccountViewCache &view,
                                          CScriptDBViewCache &scriptDB) {
    if (userId.type() != typeid(CPubKey))
        return state.DoS(100, ERRORMSG("CRegisterContractTx::CheckTransaction : userId must be CPubKey"),
            REJECT_INVALID, "userid-type-error");

    if ((minerId.type() != typeid(CPubKey)) && (minerId.type() != typeid(CNullID)))
        return state.DoS(100, ERRORMSG("CRegisterContractTx::CheckTransaction : minerId must be CPubKey or CNullID"),
            REJECT_INVALID, "minerid-type-error");

    if (!boost::get<CPubKey>(userId).IsFullyValid())
        return state.DoS(100, ERRORMSG("CRegisterAccountTx::CheckTransaction, register tx public key is invalid"),
            REJECT_INVALID, "bad-regtx-publickey");

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CRegisterAccountTx::CheckTransaction, register tx fee out of range"),
            REJECT_INVALID, "bad-regtx-fee-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CRegisterAccountTx::CheckTransaction, register tx fee smaller than MinTxFee"),
            REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    if (!CheckSignatureSize(signature)) {
        return state.DoS(100, ERRORMSG("CRegisterAccountTx::CheckTransaction, signature size invalid"),
            REJECT_INVALID, "bad-tx-sig-size");
    }

    // check signature script
    uint256 sighash = SignatureHash();
    if (!CheckSignScript(sighash, signature, boost::get<CPubKey>(userId)))
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterAccountTx CheckTransaction, register tx signature error "),
            REJECT_INVALID, "bad-regtx-signature");

    return true;
}

bool CCommonTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                          CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                          CScriptDBViewCache &scriptDB) {
    CAccount srcAcct;
    CAccount desAcct;
    CAccountLog desAcctLog;
    bool generateRegID = false;

    if (!view.GetAccount(srcUserId, srcAcct))
        return state.DoS(100, ERRORMSG("CCommonTx::ExecuteTx, read source addr account info error"),
                         READ_ACCOUNT_FAIL, "bad-read-accountdb");
    else {
        if (srcUserId.type() == typeid(CPubKey)) {
            srcAcct.pubKey = boost::get<CPubKey>(srcUserId);

            CRegID srcRegID;
            // If the source account has NO CRegID, need to generate a new CRegID.
            if (!view.GetRegId(srcUserId, srcRegID)) {
                srcAcct.regID = CRegID(nHeight, nIndex);
                generateRegID = true;
            }
        }
    }

    CAccountLog srcAcctLog(srcAcct);
    uint64_t minusValue = llFees + llValues;
    if (!srcAcct.OperateAccount(MINUS_FREE, minusValue, nHeight))
        return state.DoS(100, ERRORMSG("CCommonTx::ExecuteTx, account has insufficient funds"),
                         UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    if (generateRegID) {
        if (!view.SaveAccountInfo(srcAcct.regID, srcAcct.keyID, srcAcct))
            return state.DoS(100, ERRORMSG("CCommonTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    } else {
        if (!view.SetAccount(CUserID(srcAcct.keyID), srcAcct))
            return state.DoS(100, ERRORMSG("CCommonTx::ExecuteTx, save account info error"),
                             WRITE_ACCOUNT_FAIL, "bad-write-accountdb");
    }

    uint64_t addValue = llValues;
    if (!view.GetAccount(desUserId, desAcct)) {
        if (desUserId.type() == typeid(CKeyID)) {  // target account has NO CRegID
            desAcct.keyID    = boost::get<CKeyID>(desUserId);
            desAcctLog.keyID = desAcct.keyID;
        } else {
            return state.DoS(100, ERRORMSG("CCommonTx::ExecuteTx, get account info failed"),
                             READ_ACCOUNT_FAIL, "bad-read-accountdb");
        }
    } else {  // target account has NO CAccount(first involved in transacion)
        desAcctLog.SetValue(desAcct);
    }

    if (!desAcct.OperateAccount(ADD_FREE, addValue, nHeight))
        return state.DoS(100, ERRORMSG("CCommonTx::ExecuteTx, operate accounts error"),
                         UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");

    if (!view.SetAccount(desUserId, desAcct))
        return state.DoS(100,
                         ERRORMSG("CCommonTx::ExecuteTx, save account error, kyeId=%s",
                                  desAcct.keyID.ToString()),
                         UPDATE_ACCOUNT_FAIL, "bad-save-account");

    txundo.vAccountLog.push_back(srcAcctLog);
    txundo.vAccountLog.push_back(desAcctLog);
    txundo.txHash = GetHash();

    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        CKeyID revKeyId;
        if (!view.GetKeyId(srcUserId, sendKeyId))
            return ERRORMSG("CCommonTx::ExecuteTx, get keyid by srcUserId error!");

        if (!view.GetKeyId(desUserId, revKeyId))
            return ERRORMSG("CCommonTx::ExecuteTx, get keyid by desUserId error!");

        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                         operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);

        if (!scriptDB.SetTxHashByAddress(revKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                         operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CCommonTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view,
                           CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(srcUserId, keyId))
        return false;
    vAddr.insert(keyId);
    CKeyID desKeyId;
    if (!view.GetKeyId(desUserId, desKeyId))
        return false;
    vAddr.insert(desKeyId);
    return true;
}

string CCommonTx::ToString(CAccountViewCache &view) const {
    string srcId;
    if (srcUserId.type() == typeid(CPubKey)) {
        srcId = boost::get<CPubKey>(srcUserId).ToString();
    } else if (srcUserId.type() == typeid(CRegID)) {
        srcId = boost::get<CRegID>(srcUserId).ToString();
    }

    string desId;
    if (desUserId.type() == typeid(CKeyID)) {
        desId = boost::get<CKeyID>(desUserId).ToString();
    } else if (desUserId.type() == typeid(CRegID)) {
        desId = boost::get<CRegID>(desUserId).ToString();
    }

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, srcId=%s, desId=%s, llValues=%ld, llFees=%ld, memo=%s, "
        "nValidHeight=%d\n",
        txTypeArray[nTxType], GetHash().ToString().c_str(), nVersion, srcId.c_str(), desId.c_str(),
        llValues, llFees, HexStr(memo).c_str(), nValidHeight);

    return str;
}

Object CCommonTx::ToJson(const CAccountViewCache &AccountView) const {
    Object result;
    CAccountViewCache view(AccountView);

    auto GetRegIdString = [&](CUserID const &userId) {
        if (userId.type() == typeid(CRegID))
            return boost::get<CRegID>(userId).ToString();
        return string(" ");
    };

    CKeyID srcKeyId, desKeyId;
    view.GetKeyId(srcUserId, srcKeyId);
    view.GetKeyId(desUserId, desKeyId);

    result.push_back(Pair("hash",           GetHash().GetHex()));
    result.push_back(Pair("txtype",         txTypeArray[nTxType]));
    result.push_back(Pair("ver",            nVersion));
    result.push_back(Pair("regid",          GetRegIdString(srcUserId)));
    result.push_back(Pair("addr",           srcKeyId.ToAddress()));
    result.push_back(Pair("desregid",       GetRegIdString(desUserId)));
    result.push_back(Pair("desaddr",        desKeyId.ToAddress()));
    result.push_back(Pair("money",          llValues));
    result.push_back(Pair("fees",           llFees));
    result.push_back(Pair("height",         nValidHeight));
    result.push_back(Pair("memo",           HexStr(memo)));

    return result;
}

bool CCommonTx::CheckTransaction(CValidationState &state, CAccountViewCache &view,
                                 CScriptDBViewCache &scriptDB) {
    if ((srcUserId.type() != typeid(CRegID)) && (srcUserId.type() != typeid(CPubKey)))
        return state.DoS(100, ERRORMSG("CCommonTx::CheckTransaction, srcaddr type error"),
                         REJECT_INVALID, "srcaddr-type-error");

    if ((desUserId.type() != typeid(CRegID)) && (desUserId.type() != typeid(CKeyID)))
        return state.DoS(100, ERRORMSG("CCommonTx::CheckTransaction, desaddr type error"),
                         REJECT_INVALID, "desaddr-type-error");

    if ((srcUserId.type() == typeid(CPubKey)) && !boost::get<CPubKey>(srcUserId).IsFullyValid())
        return state.DoS(100,
                         ERRORMSG("CCommonTx::CheckTransaction, common tx public key is invalid"),
                         REJECT_INVALID, "bad-commontx-publickey");

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CCommonTx::CheckTransaction, tx fees out of money range"),
                         REJECT_INVALID, "bad-appeal-fees-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100,
                         ERRORMSG("CCommonTx::CheckTransaction, tx fees smaller than MinTxFee"),
                         REJECT_INVALID, "bad-tx-fees-toosmall");
    }

    CAccount srcAccount;
    if (!view.GetAccount(srcUserId, srcAccount))
        return state.DoS(100, ERRORMSG("CCommonTx::CheckTransaction, read account failed"),
                         REJECT_INVALID, "bad-getaccount");

    if (!CheckSignatureSize(signature)) {
            return state.DoS(100, ERRORMSG("CCommonTx::CheckTransaction, signature size invalid"),
                             REJECT_INVALID, "bad-tx-sig-size");
    }

    uint256 sighash = SignatureHash();
    CPubKey pubKey =
        srcUserId.type() == typeid(CPubKey) ? boost::get<CPubKey>(srcUserId) : srcAccount.pubKey;
    if (!CheckSignScript(sighash, signature, pubKey))
        return state.DoS(100, ERRORMSG("CCommonTx::CheckTransaction, CheckSignScript failed"),
                         REJECT_INVALID, "bad-signscript-check");

    return true;
}

bool CContractTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                            CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                            CScriptDBViewCache &scriptDB) {
    CAccount srcAcct;
    CAccount desAcct;
    CAccountLog desAcctLog;
    uint64_t minusValue = llFees + llValues;
    if (!view.GetAccount(srcRegId, srcAcct))
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, read source addr %s account info error",
            boost::get<CRegID>(srcRegId).ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");

    CAccountLog srcAcctLog(srcAcct);
    if (!srcAcct.OperateAccount(MINUS_FREE, minusValue, nHeight))
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, accounts insufficient funds"),
            UPDATE_ACCOUNT_FAIL, "operate-minus-account-failed");

    CUserID userId = srcAcct.keyID;
    if (!view.SetAccount(userId, srcAcct))
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, save account%s info error",
            boost::get<CRegID>(srcRegId).ToString()), WRITE_ACCOUNT_FAIL, "bad-write-accountdb");

    uint64_t addValue = llValues;
    if (!view.GetAccount(desUserId, desAcct)) {
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, get account info failed by regid:%s",
            boost::get<CRegID>(desUserId).ToString()), READ_ACCOUNT_FAIL, "bad-read-accountdb");
    } else {
        desAcctLog.SetValue(desAcct);
    }

    if (!desAcct.OperateAccount(ADD_FREE, addValue, nHeight))
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, operate accounts error"),
            UPDATE_ACCOUNT_FAIL, "operate-add-account-failed");

    if (!view.SetAccount(desUserId, desAcct))
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, save account error, kyeId=%s",
            desAcct.keyID.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");

    txundo.vAccountLog.push_back(srcAcctLog);
    txundo.vAccountLog.push_back(desAcctLog);

    vector<unsigned char> vScript;
    if (!scriptDB.GetScript(boost::get<CRegID>(desUserId), vScript))
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, read account faild, RegId=%s",
            boost::get<CRegID>(desUserId).ToString()), READ_ACCOUNT_FAIL, "bad-read-script");

    CVmRunEnv vmRunEnv;
    std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    uint64_t fuelRate = GetFuelRate(scriptDB);

    int64_t llTime = GetTimeMillis();
    tuple<bool, uint64_t, string> ret = vmRunEnv.ExecuteContract(pTx, view, scriptDB, nHeight, fuelRate, nRunStep);
    if (!std::get<0>(ret))
        return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, txid=%s run script error:%s",
            GetHash().GetHex(), std::get<2>(ret)), UPDATE_ACCOUNT_FAIL, "run-script-error: " + std::get<2>(ret));

    LogPrint("vm", "execute contract elapse:%lld, txhash=%s\n", GetTimeMillis() - llTime, GetHash().GetHex());

    set<CKeyID> vAddress;
    vector<std::shared_ptr<CAccount> > &vAccount = vmRunEnv.GetNewAccont();
    for (auto & itemAccount : vAccount) {  //更新对应的合约交易的账户信息
        vAddress.insert(itemAccount->keyID);
        userId = itemAccount->keyID;
        CAccount oldAcct;
        if (!view.GetAccount(userId, oldAcct)) {
            if (!itemAccount->keyID.IsNull()) {  //合约往未发生过转账记录地址转币
                oldAcct.keyID = itemAccount->keyID;
            } else
                return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, read account info error"),
                    UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccountLog oldAcctLog(oldAcct);
        if (!view.SetAccount(userId, *itemAccount))
            return state.DoS(100, ERRORMSG("CContractTx::ExecuteTx, write account info error"),
                UPDATE_ACCOUNT_FAIL, "bad-write-accountdb");

        txundo.vAccountLog.push_back(oldAcctLog);
    }
    txundo.vScriptOperLog.insert(txundo.vScriptOperLog.end(), vmRunEnv.GetDbLog()->begin(),
                                 vmRunEnv.GetDbLog()->end());
    vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
    for (auto & itemUserAccount : vAppUserAccount) {
        CKeyID itemKeyID;
        bool bValid = GetKeyId(view, itemUserAccount.get()->GetAccUserId(), itemKeyID);
        if (bValid)
            vAddress.insert(itemKeyID);
    }

    if (!scriptDB.SetTxRelAccout(GetHash(), vAddress))
        return ERRORMSG("CContractTx::ExecuteTx, save tx relate account info to script db error");

    txundo.txHash = GetHash();

    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        CKeyID revKeyId;
        if (!view.GetKeyId(srcRegId, sendKeyId))
            return ERRORMSG("CContractTx::ExecuteTx, get keyid by srcRegId error!");

        if (!view.GetKeyId(desUserId, revKeyId))
            return ERRORMSG("CContractTx::ExecuteTx, get keyid by desUserId error!");

        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                         operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);

        if (!scriptDB.SetTxHashByAddress(revKeyId, nHeight, nIndex + 1, txundo.txHash.GetHex(),
                                         operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }

    return true;
}

bool CContractTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
    CKeyID keyId;
    if (!view.GetKeyId(srcRegId, keyId))
        return false;

    vAddr.insert(keyId);
    CKeyID desKeyId;
    if (!view.GetKeyId(desUserId, desKeyId))
        return false;

    vAddr.insert(desKeyId);

    CVmRunEnv vmRunEnv;
    std::shared_ptr<CBaseTx> pTx = GetNewInstance();
    uint64_t fuelRate = GetFuelRate(scriptDB);
    CScriptDBViewCache scriptDBView(scriptDB, true);

    if (uint256() == pTxCacheTip->HasTx(GetHash())) {
        CAccountViewCache accountView(view, true);
        tuple<bool, uint64_t, string> ret = vmRunEnv.ExecuteContract(pTx, accountView, scriptDBView,
            chainActive.Height() + 1, fuelRate, nRunStep);

        if (!std::get<0>(ret))
            return ERRORMSG("CContractTx::GetAddress, %s", std::get<2>(ret));

        vector<shared_ptr<CAccount> > vpAccount = vmRunEnv.GetNewAccont();

        for (auto & item : vpAccount)
            vAddr.insert(item->keyID);

        vector<std::shared_ptr<CAppUserAccount> > &vAppUserAccount = vmRunEnv.GetRawAppUserAccount();
        for (auto & itemUserAccount : vAppUserAccount) {
            CKeyID itemKeyID;
            bool bValid = GetKeyId(view, itemUserAccount.get()->GetAccUserId(), itemKeyID);
            if (bValid)
                vAddr.insert(itemKeyID);
        }
    } else {
        set<CKeyID> vTxRelAccount;
        if (!scriptDBView.GetTxRelAccount(GetHash(), vTxRelAccount))
            return false;

        vAddr.insert(vTxRelAccount.begin(), vTxRelAccount.end());
    }
    return true;
}

string CContractTx::ToString(CAccountViewCache &view) const {
    string desId;
    if (desUserId.type() == typeid(CKeyID)) {
        desId = boost::get<CKeyID>(desUserId).ToString();
    } else if (desUserId.type() == typeid(CRegID)) {
        desId = boost::get<CRegID>(desUserId).ToString();
    }

    string str = strprintf(
        "txType=%s, hash=%s, ver=%d, srcId=%s, desId=%s, llValues=%ld, llFees=%ld, arguments=%s, "
        "nValidHeight=%d\n",
        txTypeArray[nTxType], GetHash().ToString().c_str(), nVersion,
        boost::get<CRegID>(srcRegId).ToString(), desId.c_str(), llValues, llFees,
        HexStr(arguments).c_str(), nValidHeight);

    return str;
}

Object CContractTx::ToJson(const CAccountViewCache &AccountView) const {
    Object result;
    CAccountViewCache view(AccountView);

    auto GetRegIdString = [&](CUserID const &userId) {
        if (userId.type() == typeid(CRegID))
            return boost::get<CRegID>(userId).ToString();
        return string(" ");
    };

    CKeyID srcKeyId, desKeyId;
    view.GetKeyId(srcRegId, srcKeyId);
    view.GetKeyId(desUserId, desKeyId);

    result.push_back(Pair("hash",       GetHash().GetHex()));
    result.push_back(Pair("txtype",     txTypeArray[nTxType]));
    result.push_back(Pair("ver",        nVersion));
    result.push_back(Pair("regid",      GetRegIdString(srcRegId)));
    result.push_back(Pair("addr",       srcKeyId.ToAddress()));
    result.push_back(Pair("desregid",   GetRegIdString(desUserId)));
    result.push_back(Pair("desaddr",    desKeyId.ToAddress()));
    result.push_back(Pair("money",      llValues));
    result.push_back(Pair("fees",       llFees));
    result.push_back(Pair("height",     nValidHeight));
    result.push_back(Pair("arguments",   HexStr(arguments)));

    return result;
}

bool CContractTx::CheckTransaction(CValidationState &state, CAccountViewCache &view,
                                            CScriptDBViewCache &scriptDB) {
    if (arguments.size() >= kContractArgumentMaxSize)
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, arguments's size too large"),
                         REJECT_INVALID, "arguments-size-toolarge");

    if (srcRegId.type() != typeid(CRegID))
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, srcRegId must be CRegID"),
            REJECT_INVALID, "srcaddr-type-error");

    if ((desUserId.type() != typeid(CRegID)) && (desUserId.type() != typeid(CKeyID)))
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, desUserId must be CRegID or CKeyID"),
            REJECT_INVALID, "desaddr-type-error");

    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, tx fee out of money range"),
            REJECT_INVALID, "bad-appeal-fee-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, tx fee smaller than MinTxFee"),
            REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    CAccount srcAccount;
    if (!view.GetAccount(boost::get<CRegID>(srcRegId), srcAccount))
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, read account failed, regid=%s",
            boost::get<CRegID>(srcRegId).ToString()), REJECT_INVALID, "bad-getaccount");

    if (!srcAccount.IsRegistered())
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, account pubkey not registered"),
            REJECT_INVALID, "bad-account-unregistered");

    if (!CheckSignatureSize(signature)) {
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, signature size invalid"),
            REJECT_INVALID, "bad-tx-sig-size");
    }

    uint256 sighash = SignatureHash();
    if (!CheckSignScript(sighash, signature, srcAccount.pubKey))
        return state.DoS(100, ERRORMSG("CContractTx::CheckTransaction, CheckSignScript failed"),
            REJECT_INVALID, "bad-signscript-check");

    return true;
}

bool CRewardTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
                          CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache,
                          CScriptDBViewCache &scriptDB) {
    CID id(account);
    if (account.type() != typeid(CRegID)) {
        return state.DoS(100,
            ERRORMSG("ExecuteTx() : CRewardTx ExecuteTx, account %s error, data type must be either CRegID",
            HexStr(id.GetID())), UPDATE_ACCOUNT_FAIL, "bad-account");
    }

    CAccount acctInfo;
    if (!view.GetAccount(account, acctInfo)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRewardTx ExecuteTx, read source addr %s account info error",
            HexStr(id.GetID())), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    // LogPrint("op_account", "before operate:%s\n", acctInfo.ToString());
    CAccountLog acctInfoLog(acctInfo);
    if (0 == nIndex) {
        // nothing to do here
    } else if (-1 == nIndex) {  // maturity reward tx, only update values
        acctInfo.llValues += rewardValue;
    } else {  // never go into this step
        return ERRORMSG("nIndex type error!");
    }

    CUserID userId = acctInfo.keyID;
    if (!view.SetAccount(userId, acctInfo))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRewardTx ExecuteTx, write secure account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

    txundo.Clear();
    txundo.vAccountLog.push_back(acctInfoLog);
    txundo.txHash = GetHash();
    if (SysCfg().GetAddressToTxFlag() && 0 == nIndex) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if (!view.GetKeyId(account, sendKeyId))
            return ERRORMSG("ExecuteTx() : CRewardTx ExecuteTx, get keyid by account error!");

        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;

        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    // LogPrint("op_account", "after operate:%s\n", acctInfo.ToString());
    return true;
}

bool CRewardTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB) {
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

string CRewardTx::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(account, keyId);
    CRegID regId;
    view.GetRegId(account, regId);
    str += strprintf("txType=%s, hash=%s, ver=%d, account=%s, keyid=%s, rewardValue=%ld\n",
        txTypeArray[nTxType], GetHash().ToString().c_str(), nVersion, regId.ToString(), keyId.GetHex(), rewardValue);

    return str;
}

Object CRewardTx::ToJson(const CAccountViewCache &AccountView) const{
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
    return result;
}

bool CRegisterContractTx::ExecuteTx(int nIndex, CAccountViewCache &view,CValidationState &state, CTxUndo &txundo,
        int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB) {
    CID id(regAcctId);
    CAccount acctInfo;
    CScriptDBOperLog operLog;
    if (!view.GetAccount(regAcctId, acctInfo)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, read regist addr %s account info error",
            HexStr(id.GetID())), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccount acctInfoLog(acctInfo);
    uint64_t minusValue = llFees;
    if (minusValue > 0) {
        if(!acctInfo.OperateAccount(MINUS_FREE, minusValue, nHeight))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, operate account failed ,regId=%s",
                boost::get<CRegID>(regAcctId).ToString()), UPDATE_ACCOUNT_FAIL, "operate-account-failed");

        txundo.vAccountLog.push_back(acctInfoLog);
    }
    txundo.txHash = GetHash();

    CVmScript vmScript;
    CDataStream stream(script, SER_DISK, CLIENT_VERSION);
    try {
        stream >> vmScript;
    } catch (exception& e) {
        return state.DoS(100, ERRORMSG(("ExecuteTx() :CRegisterContractTx ExecuteTx, Unserialize to vmScript error:" +
            string(e.what())).c_str()), UPDATE_ACCOUNT_FAIL, "unserialize-script-error");
    }
    if(!vmScript.IsValid())
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, vmScript invalid"),
            UPDATE_ACCOUNT_FAIL, "script-check-failed");

    CRegID regId(nHeight, nIndex);
    //create script account
    CKeyID keyId = Hash160(regId.GetVec6());
    CAccount account;
    account.keyID = keyId;
    account.regID = regId;
    //save new script content
    if(!scriptDB.SetScript(regId, script)){
        return state.DoS(100,
            ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, save script id %s script info error",
                regId.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
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
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx ExecuteTx, save account info error"),
            UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");

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

    CRegID scriptId(nHeight, nIndex);
    //delete script content
    if (!scriptDB.EraseScript(scriptId)) {
        return state.DoS(100, ERRORMSG("UndoUpdateAccount() : CRegisterContractTx UndoExecuteTx, erase script id %s error", scriptId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "erase-script-failed");
    }
    //delete account
    if (!view.EraseId(scriptId)) {
        return state.DoS(100, ERRORMSG("UndoUpdateAccount() : CRegisterContractTx UndoExecuteTx, erase script account %s error", scriptId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "erase-appkeyid-failed");
    }
    CKeyID keyId = Hash160(scriptId.GetVec6());
    userId       = keyId;
    if (!view.EraseAccount(userId)) {
        return state.DoS(100, ERRORMSG("UndoUpdateAccount() : CRegisterContractTx UndoExecuteTx, erase script account %s error", scriptId.ToString()),
                         UPDATE_ACCOUNT_FAIL, "erase-appaccount-failed");
    }
    // LogPrint("INFO", "Delete regid %s app account\n", scriptId.ToString());

    for (auto &itemLog : txundo.vAccountLog) {
        if (itemLog.keyID == account.keyID) {
            if (!account.UndoOperateAccount(itemLog))
                return state.DoS(100, ERRORMSG("UndoUpdateAccount: CRegisterContractTx UndoExecuteTx, undo operate account error, keyId=%s", account.keyID.ToString()),
                                 UPDATE_ACCOUNT_FAIL, "undo-account-failed");
        }
    }

    vector<CScriptDBOperLog>::reverse_iterator rIterScriptDBLog = txundo.vScriptOperLog.rbegin();
    for (; rIterScriptDBLog != txundo.vScriptOperLog.rend(); ++rIterScriptDBLog) {
        if (!scriptDB.UndoScriptData(rIterScriptDBLog->vKey, rIterScriptDBLog->vValue))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx UndoExecuteTx, undo scriptdb data error"), UPDATE_ACCOUNT_FAIL, "undo-scriptdb-failed");
    }
    userId = account.keyID;
    if (!view.SetAccount(userId, account))
        return state.DoS(100, ERRORMSG("ExecuteTx() : CRegisterContractTx UndoExecuteTx, save account error"), UPDATE_ACCOUNT_FAIL, "bad-save-accountdb");
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

Object CRegisterContractTx::ToJson(const CAccountViewCache &AccountView) const{
    Object result;
    CAccountViewCache view(AccountView);

    CKeyID keyid;
    view.GetKeyId(regAcctId, keyid);

    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", txTypeArray[nTxType]));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid",  boost::get<CRegID>(regAcctId).ToString()));
    result.push_back(Pair("addr", keyid.ToAddress()));
    result.push_back(Pair("script", "script_content"));
    result.push_back(Pair("fees", llFees));
    result.push_back(Pair("height", nValidHeight));
    return result;
}

bool CRegisterContractTx::CheckTransaction(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB)
{
    if (regAcctId.type() != typeid(CRegID)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx regAcctId must be CRegID"),
            REJECT_INVALID, "regacctid-type-error");
    }

    if (!CheckMoneyRange(llFees)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx CheckTransaction, tx fee out of range"),
            REJECT_INVALID, "fee-too-large");
    }

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CRegisterContractTx::CheckTransaction, tx fee smaller than MinTxFee"),
            REJECT_INVALID, "bad-tx-fee-toosmall");
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

    if (!CheckSignatureSize(signature)) {
        return state.DoS(100, ERRORMSG("CRegisterContractTx::CheckTransaction, signature size invalid"),
            REJECT_INVALID, "bad-tx-sig-size");
    }

    uint256 signhash = SignatureHash();
    if (!CheckSignScript(signhash, signature, acctInfo.pubKey)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CRegisterContractTx CheckTransaction, CheckSignScript failed"),
            REJECT_INVALID, "bad-signscript-check");
    }
    return true;
}

bool CDelegateTx::ExecuteTx(int nIndex, CAccountViewCache &view, CValidationState &state,
    CTxUndo &txundo, int nHeight, CTransactionDBCache &txCache, CScriptDBViewCache &scriptDB)
{
    CID id(userId);
    CAccount acctInfo;
    if (!view.GetAccount(userId, acctInfo)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, read regist addr %s account info error", HexStr(id.GetID())),
            UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
    }
    CAccount acctInfoLog(acctInfo);
    uint64_t minusValue = llFees;
    if (minusValue > 0) {
        if(!acctInfo.OperateAccount(MINUS_FREE, minusValue, nHeight))
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, operate account failed ,regId=%s", boost::get<CRegID>(userId).ToString()),
                UPDATE_ACCOUNT_FAIL, "operate-account-failed");
    }
    if (!acctInfo.ProcessDelegateVote(operVoteFunds, nHeight)) {
        return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, operate delegate vote failed ,regId=%s", boost::get<CRegID>(userId).ToString()),
            UPDATE_ACCOUNT_FAIL, "operate-delegate-failed");
    }
    if (!view.SaveAccountInfo(acctInfo.regID, acctInfo.keyID, acctInfo)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx create new account script id %s script info error", acctInfo.regID.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
    }
    txundo.vAccountLog.push_back(acctInfoLog);
    txundo.txHash = GetHash();

    for (auto iter = operVoteFunds.begin(); iter != operVoteFunds.end(); ++iter) {
        CAccount delegate;
        if (!view.GetAccount(CUserID(iter->fund.pubKey), delegate)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, read regist addr %s account info error", iter->fund.pubKey.GetKeyID().ToAddress()),
                UPDATE_ACCOUNT_FAIL, "bad-read-accountdb");
        }
        CAccount delegateAcctLog(delegate);
        if (!delegate.OperateVote(VoteOperType(iter->operType), iter->fund.value)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, operate delegate address %s vote fund error", iter->fund.pubKey.GetKeyID().ToAddress()),
                UPDATE_ACCOUNT_FAIL, "operate-vote-error");
        }
        txundo.vAccountLog.push_back(delegateAcctLog);
        // set the new value and erase the old value
        CScriptDBOperLog operDbLog;
        if (!scriptDB.SetDelegateData(delegate, operDbLog)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, erase account id %s vote info error", delegate.regID.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
        txundo.vScriptOperLog.push_back(operDbLog);

        CScriptDBOperLog eraseDbLog;
        if (delegateAcctLog.llVotes > 0) {
            if(!scriptDB.EraseDelegateData(delegateAcctLog, eraseDbLog)) {
                return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, erase account id %s vote info error", delegateAcctLog.regID.ToString()),
                    UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
            }
        }
        txundo.vScriptOperLog.push_back(eraseDbLog);

        if (!view.SaveAccountInfo(delegate.regID, delegate.keyID, delegate)) {
            return state.DoS(100, ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx create new account script id %s script info error", acctInfo.regID.ToString()),
                UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb");
        }
    }

    if (SysCfg().GetAddressToTxFlag()) {
        CScriptDBOperLog operAddressToTxLog;
        CKeyID sendKeyId;
        if (!view.GetKeyId(userId, sendKeyId)) {
            return ERRORMSG("ExecuteTx() : CDelegateTx ExecuteTx, get regAcctId by account error!");
        }
        if (!scriptDB.SetTxHashByAddress(sendKeyId, nHeight, nIndex+1, txundo.txHash.GetHex(), operAddressToTxLog))
            return false;
        txundo.vScriptOperLog.push_back(operAddressToTxLog);
    }
    return true;
}

string CDelegateTx::ToString(CAccountViewCache &view) const {
    string str;
    CKeyID keyId;
    view.GetKeyId(userId, keyId);
    str += strprintf("txType=%s, hash=%s, ver=%d, address=%s, keyid=%s\n", txTypeArray[nTxType],
        GetHash().ToString().c_str(), nVersion, keyId.ToAddress(), keyId.ToString());
    str += "vote:\n";
    for (auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        str += strprintf("%s", item->ToString());
    }
    return str;
}

Object CDelegateTx::ToJson(const CAccountViewCache &accountView) const {
    Object result;
    CAccountViewCache view(accountView);
    CKeyID keyId;
    result.push_back(Pair("hash", GetHash().GetHex()));
    result.push_back(Pair("txtype", txTypeArray[nTxType]));
    result.push_back(Pair("ver", nVersion));
    result.push_back(Pair("regid", boost::get<CRegID>(userId).ToString()));
    view.GetKeyId(userId, keyId);
    result.push_back(Pair("addr", keyId.ToAddress()));
    result.push_back(Pair("fees", llFees));
    Array operVoteFundArray;
    for (auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        operVoteFundArray.push_back(item->ToJson(true));
    }
    result.push_back(Pair("operVoteFundList", operVoteFundArray));
    return result;
}

bool CDelegateTx::CheckTransaction(CValidationState &state, CAccountViewCache &view, CScriptDBViewCache &scriptDB)
{
    CID id(userId);
    if (userId.type() != typeid(CRegID)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx send account is not CRegID type"),
            REJECT_INVALID, "deletegate-tx-error");
    }
    if (0 == operVoteFunds.size()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx the deletegate oper fund empty"),
            REJECT_INVALID, "oper-fund-empty-error");
    }
    if (operVoteFunds.size() > IniCfg().GetDelegatesNum()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx the deletegates number a transaction can't exceeds maximum"),
            REJECT_INVALID, "deletegates-number-error");
    }
    if (!CheckMoneyRange(llFees))
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx CheckTransaction, delegate tx fee out of range"),
            REJECT_INVALID, "bad-tx-fee-toolarge");

    if (!CheckMinTxFee(llFees)) {
        return state.DoS(100, ERRORMSG("CDelegateTx::CheckTransaction, tx fee smaller than MinTxFee"),
            REJECT_INVALID, "bad-tx-fee-toosmall");
    }

    CKeyID sendTxKeyID;
    if(!view.GetKeyId(userId, sendTxKeyID)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx get keyId error by CUserID =%s", HexStr(id.GetID())), REJECT_INVALID, "");
    }

    CAccount sendAcct;
    if (!view.GetAccount(userId, sendAcct)) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx get account info error, userid=%s", HexStr(id.GetID())),
            REJECT_INVALID, "bad-read-accountdb");
    }
    if (!sendAcct.IsRegistered()) {
        return state.DoS(100, ERRORMSG("CheckTransaction(): CDelegateTx CheckTransaction, pubkey not registed"),
            REJECT_INVALID, "bad-no-pubkey");
    }

    NET_TYPE netowrkID = SysCfg().NetworkID();
    if ( (netowrkID == MAIN_NET && nValidHeight > kCheckDelegateTxSignatureForkHeight) // for mainnet, need hardcode here, compatible with 7 old unsigned votes
        || (netowrkID == TEST_NET || netowrkID == REGTEST_NET) ) { // for testnet or regtest, must do the check

        if (!CheckSignatureSize(signature)) {
            return state.DoS(100, ERRORMSG("CDelegateTx::CheckTransaction, signature size invalid"),
                REJECT_INVALID, "bad-tx-sig-size");
        }

        uint256 signhash = SignatureHash();
        if (!CheckSignScript(signhash, signature, sendAcct.pubKey)) {
            return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx CheckTransaction, CheckSignScript failed"),
                REJECT_INVALID, "bad-signscript-check");
        }
    }

    //check account delegates number;
    set<CKeyID> setTotalOperVoteKeyID;
    for (auto operItem : sendAcct.vVoteFunds) {
        setTotalOperVoteKeyID.insert(operItem.pubKey.GetKeyID());
    }

    //check delegate duplication
    set<CKeyID> setOperVoteKeyID;
    uint64_t totalVotes = 0;
    for (auto item = operVoteFunds.begin(); item != operVoteFunds.end(); ++item) {
        if (0 >= item->fund.value || (uint64_t) GetMaxMoney() < item->fund.value )
            return ERRORMSG("votes: %lld not within (0 .. MaxVote)", item->fund.value);

        setOperVoteKeyID.insert(item->fund.pubKey.GetKeyID());
        setTotalOperVoteKeyID.insert(item->fund.pubKey.GetKeyID());
        CAccount acctInfo;
        if (!view.GetAccount(CUserID(item->fund.pubKey), acctInfo))
            return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx get account info error, address=%s",
                item->fund.pubKey.GetKeyID().ToAddress()), REJECT_INVALID, "bad-read-accountdb");

        if(item->fund.value > totalVotes)
            totalVotes = item->fund.value;
    }

    if (setTotalOperVoteKeyID.size() > IniCfg().GetDelegatesNum()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx the delegates number of account can't exceeds maximum"),
            REJECT_INVALID, "account-delegates-number-error");
    }

    if (setOperVoteKeyID.size() != operVoteFunds.size()) {
        return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx duplication vote fund"),
            REJECT_INVALID, "deletegates-duplication fund-error");
    }

    if (totalVotes > sendAcct.llValues) {
       return state.DoS(100, ERRORMSG("CheckTransaction() : CDelegateTx delegate votes (%d) exceeds account balance (%d), userid=%s",
            totalVotes, sendAcct.llValues, HexStr(id.GetID())), REJECT_INVALID, "insufficient balance for votes");
    }

    return true;
}

bool CDelegateTx::GetAddress(set<CKeyID> &vAddr, CAccountViewCache &view, CScriptDBViewCache &scriptDB)
{
    CKeyID keyId;
    if (!view.GetKeyId(userId, keyId))
        return false;

    vAddr.insert(keyId);
    for (auto iter = operVoteFunds.begin(); iter != operVoteFunds.end(); ++iter) {
        vAddr.insert(iter->fund.pubKey.GetKeyID());
    }
    return true;
}

string CAccountLog::ToString() const {
    string str("");
    str += strprintf("    Account log: keyId=%d llValues=%lld nVoteHeight=%lld llVotes=%lld \n",
        keyID.GetHex(), llValues, nVoteHeight, llVotes);
    str += string("    vote fund:");

    for (auto it =  vVoteFunds.begin(); it != vVoteFunds.end(); ++it) {
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
    if (!vVoteFunds.empty())
        return vVoteFunds.begin()->value + llValues;

    return llValues;
}

uint64_t CAccount::GetFrozenBalance() {
    uint64_t votes = 0;
    for (auto it = vVoteFunds.begin(); it != vVoteFunds.end(); it++) {
      if (it->value > votes) {
          votes = it->value;
      }
    }
    return votes;
}

Object CAccount::ToJsonObj(bool isAddress) const {
    Array voteFundArray;
    for (auto &fund : vVoteFunds) {
        voteFundArray.push_back(fund.ToJson(true));
    }

    Object obj;
    bool isMature = regID.GetHeight() > 0 && chainActive.Height() - (int)regID.GetHeight() >
                                                 kRegIdMaturePeriodByBlock
                        ? true
                        : false;
    obj.push_back(Pair("address",       keyID.ToAddress()));
    obj.push_back(Pair("keyID",         keyID.ToString()));
    obj.push_back(Pair("publicKey",     pubKey.ToString()));
    obj.push_back(Pair("minerPKey",     minerPubKey.ToString()));
    obj.push_back(Pair("regID",         regID.ToString()));
    obj.push_back(Pair("regIDMature",   isMature));
    obj.push_back(Pair("balance",       llValues));
    obj.push_back(Pair("updateHeight",  nVoteHeight));
    obj.push_back(Pair("votes",         llVotes));
    obj.push_back(Pair("voteFundList",  voteFundArray));
    return obj;
}

string CAccount::ToString(bool isAddress) const {
    string str;
    str += strprintf("regID=%s, keyID=%s, publicKey=%s, minerpubkey=%s, values=%ld updateHeight=%d llVotes=%lld\n",
        regID.ToString(), keyID.GetHex().c_str(), pubKey.ToString().c_str(),
        minerPubKey.ToString().c_str(), llValues, nVoteHeight, llVotes);
    str += "vVoteFunds list: \n";
    for (auto & fund : vVoteFunds) {
        str += fund.ToString(isAddress);
    }
    return str;
}

bool CAccount::IsMoneyOverflow(uint64_t nAddMoney) {
    if (!CheckMoneyRange(nAddMoney))
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

bool CAccount::ProcessDelegateVote(vector<COperVoteFund> & operVoteFunds, const uint64_t nCurHeight) {
    if (nCurHeight < nVoteHeight) {
        LogPrint("ERROR", "current vote tx height (%d) can't be smaller than the last nVoteHeight (%d)",
            nCurHeight, nVoteHeight);
        return false;
    }

    uint64_t llProfit = GetAccountProfit(nCurHeight);
    if (!IsMoneyOverflow(llProfit)) return false;

    uint64_t totalVotes = vVoteFunds.empty() ? 0 : vVoteFunds.begin()->value; /* totalVotes before vVoteFunds upate */

    for (auto operVote = operVoteFunds.begin(); operVote != operVoteFunds.end(); ++operVote) {
        CPubKey pubKey = operVote->fund.pubKey;
        vector<CVoteFund>::iterator itfund = find_if(vVoteFunds.begin(), vVoteFunds.end(),
                                                     [pubKey](CVoteFund fund) { return fund.pubKey == pubKey; });

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
               if (vVoteFunds.size() > IniCfg().GetDelegatesNum()) {
                   return ERRORMSG("ProcessDelegateVote() : fund number exceeds maximum");
               }
            }
        } else if (MINUS_FUND == voteType) {
            if  (itfund != vVoteFunds.end()) {
                if (!IsMoneyOverflow(operVote->fund.value))
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceed maximum ");
                if (itfund->value < operVote->fund.value) {
                    return ERRORMSG("ProcessDelegateVote() : oper fund value exceed delegate fund value");
                }
                itfund->value -= operVote->fund.value;
                if (0 == itfund->value) {
                    vVoteFunds.erase(itfund);
                }
            } else {
                return ERRORMSG("ProcessDelegateVote() : revocation votes not exist");
            }
        } else {
            return ERRORMSG("ProcessDelegateVote() : operType: %d invalid", voteType);
        }
    }

    // sort account votes after the operations against the new votes
    std::sort(vVoteFunds.begin(), vVoteFunds.end(), [](CVoteFund fund1, CVoteFund fund2) {
        return fund1.value > fund2.value;
    });

    // get the maximum one as the vote amount
    uint64_t newTotalVotes = vVoteFunds.empty() ? 0 : vVoteFunds.begin()->value;

    if (llValues + totalVotes < newTotalVotes) {
        return  ERRORMSG("ProcessDelegateVote() : delegate value exceed account value");
    }
    llValues = (llValues + totalVotes) - newTotalVotes;
    llValues += llProfit;
    LogPrint("profits", "received profits: %lld\n", llProfit);
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
        return ERRORMSG("OperateVote() : CDelegateTx ExecuteTx AccountVoteOper revocation votes are not exist");
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
