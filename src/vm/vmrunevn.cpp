/*
 * ScriptCheck.cpp
 *
 *  Created on: Sep 2, 2014
 *      Author: ranger.shi
 */
#include "vmrunevn.h"
#include "tx.h"
#include "util.h"
#include<algorithm>
#include <boost/foreach.hpp>
#include "SafeInt3.hpp"

 //CVmRunEvn *pVmRunEvn = NULL;

#define MAX_OUTPUT_COUNT 100
CVmRunEvn::CVmRunEvn() {
    RawAccont.clear();
    NewAccont.clear();
    RawAppUserAccout.clear();
    NewAppUserAccout.clear();
    RunTimeHeight = 0;
    m_ScriptDBTip = NULL;
    m_view = NULL;
    m_dblog = std::make_shared<std::vector<CScriptDBOperLog> >();
    isCheckAccount = false;
}
vector<shared_ptr<CAccount> > &CVmRunEvn::GetRawAccont() {
    return RawAccont;
}
vector<shared_ptr<CAccount> > &CVmRunEvn::GetNewAccont() {
    return NewAccont;
}
vector<shared_ptr<CAppUserAccout>> &CVmRunEvn::GetNewAppUserAccount()
{
    return NewAppUserAccout;
}

vector<shared_ptr<CAppUserAccout>> &CVmRunEvn::GetRawAppUserAccount()
{
    return RawAppUserAccout;
}


bool CVmRunEvn::intial(shared_ptr<CBaseTransaction> & Tx, CAccountViewCache& view, int nheight) {

    m_output.clear();
    listTx = Tx;
    RunTimeHeight = nheight;
    m_view = &view;
    vector<unsigned char> vScript;

    if (Tx.get()->nTxType != CONTRACT_TX) {
        LogPrint("ERROR", "%s\r\n", "err param");
//      assert(0);
        return false;
    }

    CTransaction* secure = static_cast<CTransaction*>(Tx.get());
    if (!m_ScriptDBTip->GetScript(boost::get<CRegID>(secure->desUserId), vScript)) {
        LogPrint("ERROR", "Script is not Registed %s\r\n", boost::get<CRegID>(secure->desUserId).ToString());
        return false;
    }

    CDataStream stream(vScript, SER_DISK, CLIENT_VERSION);
    try {
        stream >> vmScript;
    } catch (exception& e) {
        LogPrint("ERROR", "%s\n", "CVmScriptRun::intial() Unserialize to vmScript error");
        throw runtime_error("CVmScriptRun::intial() Unserialize to vmScript error:" + string(e.what()));
    }

    if (vmScript.IsValid() == false){
        LogPrint("ERROR", "%s\n", "CVmScriptRun::intial() vmScript.IsValid error");
        return false;
    }
    isCheckAccount = vmScript.IsCheckAccount();
    if(secure->vContract.size() >=4*1024 ){
        LogPrint("ERROR", "%s\n", "CVmScriptRun::intial() vContract context size lager 4096");
        return false;
    }

    try {
        pLua = std::make_shared<CVmlua>(vmScript.Rom, secure->vContract);
    } catch (exception& e) {
        LogPrint("ERROR", "%s\n", "CVmScriptRun::intial() CVmlua init error");
        return false;
    }

    //pVmRunEvn = this; //传CVmRunEvn对象指针给lmylib.cpp库使用
    LogPrint("vm", "%s\n", "CVmScriptRun::intial() LUA");

    return true;
}

CVmRunEvn::~CVmRunEvn() {

}
tuple<bool, uint64_t, string> CVmRunEvn::run(shared_ptr<CBaseTransaction>& Tx, CAccountViewCache& view, CScriptDBViewCache& VmDB, int nHeight,
        uint64_t nBurnFactor, uint64_t &uRunStep) {

    if(nBurnFactor == 0)
    {
//      assert(0);
        return std::make_tuple (false, 0, string("VmScript nBurnFactor == 0 \n"));
    }
    m_ScriptDBTip = &VmDB;

    CTransaction* tx = static_cast<CTransaction*>(Tx.get());
    if(tx->llFees < CBaseTransaction::nMinTxFee) {
        return std::make_tuple (false, 0, string("CVmRunEvn: Contract Tx fee too small\n"));
    }
    uint64_t maxstep = ((tx->llFees-CBaseTransaction::nMinTxFee)/ nBurnFactor) * 100;

    if(maxstep > MAX_BLOCK_RUN_STEP){
        maxstep = MAX_BLOCK_RUN_STEP;
    }
    LogPrint("vm", "tx hash:%s fees=%lld fuelrate=%lld maxstep:%d\n", Tx->GetHash().GetHex(), tx->llFees, nBurnFactor, maxstep);
    if (!intial(Tx, view, nHeight)) {
        return std::make_tuple (false, 0, string("VmScript inital Failed\n"));
    }

    int64_t step = 0;

    tuple<uint64_t, string> ret = pLua.get()->run(maxstep,this);
    LogPrint("vm", "%s\r\n", "CVmScriptRun::run() LUA");
    step = std::get<0>(ret);
    if (0 == step) {
        return std::make_tuple(false, 0, string("VmScript run Failed\n"));
    } else if (-1 == step) {
        return std::make_tuple(false, 0, std::get<1>(ret));
    }else{
        uRunStep = step;
    }

    LogPrint("vm", "tx:%s,step:%ld\n", tx->ToString(view), uRunStep);

    if (!CheckOperate(m_output)) {
        return std::make_tuple (false, 0, string("VmScript CheckOperate Failed \n"));
    }

    if (!OpeatorAccount(m_output, view, nHeight)) {
        return std::make_tuple (false, 0, string("VmScript OpeatorAccount Failed\n"));
    }

    LogPrint("vm", "isCheckAccount:%d\n", isCheckAccount);
    if(isCheckAccount) {
        LogPrint("vm","isCheckAccount is true\n");
        if(!CheckAppAcctOperate(tx))
            return std::make_tuple (false, 0, string("VmScript CheckAppAcct Failed\n"));
    }

    if(!OpeatorAppAccount(MapAppOperate, *m_ScriptDBTip))
    {
        return std::make_tuple (false, 0, string("OpeatorApp Account Failed\n"));
    }

    if(SysCfg().GetOutPutLog() && m_output.size() > 0) {
        CScriptDBOperLog operlog;
        uint256 txhash = GetCurTxHash();
        if(!m_ScriptDBTip->WriteTxOutPut(txhash, m_output, operlog))
            return std::make_tuple (false, 0, string("write tx out put Failed \n"));
        m_dblog->push_back(operlog);
    }
/*
    uint64_t spend = uRunStep * nBurnFactor;
        if((spend < uRunStep) || (spend < nBurnFactor)){
        return std::make_tuple (false, 0, string("mul error\n"));
    }
*/
    uint64_t spend = 0;
    if(!SafeMultiply(uRunStep, nBurnFactor, spend))
    {
        return std::make_tuple (false, 0, string("mul error\n"));
    }
    return std::make_tuple (true, spend, string("VmScript Sucess\n"));

}

shared_ptr<CAccount> CVmRunEvn::GetNewAccount(shared_ptr<CAccount>& vOldAccount) {
    if (NewAccont.size() == 0)
        return NULL;
    vector<shared_ptr<CAccount> >::iterator Iter;
    for (Iter = NewAccont.begin(); Iter != NewAccont.end(); Iter++) {
        shared_ptr<CAccount> temp = *Iter;
        if (temp.get()->keyID == vOldAccount.get()->keyID) {
            NewAccont.erase(Iter);
            return temp;
        }
    }
    return NULL;
}
shared_ptr<CAccount> CVmRunEvn::GetAccount(shared_ptr<CAccount>& Account) {
    if (RawAccont.size() == 0)
        return NULL;
    vector<shared_ptr<CAccount> >::iterator Iter;
    for (Iter = RawAccont.begin(); Iter != RawAccont.end(); Iter++) {
        shared_ptr<CAccount> temp = *Iter;
        if (Account.get()->keyID == temp.get()->keyID) {
            return temp;
        }
    }
    return NULL;
}
vector_unsigned_char CVmRunEvn::GetAccountID(CVmOperate value) {
    vector_unsigned_char accountid;
    if (value.nacctype == regid) {
        accountid.assign(value.accountid, value.accountid + 6);
    } else if (value.nacctype == base58addr) {
        string addr(value.accountid,value.accountid+sizeof(value.accountid));
        CKeyID KeyId = CKeyID(addr);
        CRegID regid;
        if(m_view->GetRegId(CUserID(KeyId), regid)){
            accountid.assign(regid.GetVec6().begin(),regid.GetVec6().end());
        }else{
            accountid.assign(value.accountid, value.accountid + 34);
        }
    }
    return accountid;
}

shared_ptr<CAppUserAccout> CVmRunEvn::GetAppAccount(shared_ptr<CAppUserAccout>& AppAccount) {
    if (RawAppUserAccout.size() == 0)
        return NULL;
    vector<shared_ptr<CAppUserAccout> >::iterator Iter;
    for (Iter = RawAppUserAccout.begin(); Iter != RawAppUserAccout.end(); Iter++) {
        shared_ptr<CAppUserAccout> temp = *Iter;
        if (AppAccount.get()->getaccUserId() == temp.get()->getaccUserId()) {
            return temp;
        }
    }
    return NULL;
}

bool CVmRunEvn::CheckOperate(const vector<CVmOperate> &listoperate) {
    // judge contract rulue
    uint64_t addmoey = 0, miusmoney = 0;
    uint64_t operValue = 0;
    if(listoperate.size() > MAX_OUTPUT_COUNT) {
        return false;
    }

    for (auto& it : listoperate) {

        if(it.nacctype != regid && it.nacctype != base58addr)
            return false;

        if (it.opeatortype == ADD_FREE ) {
            memcpy(&operValue,it.money,sizeof(it.money));
            /*
            uint64_t temp = addmoey;
            temp += operValue;
            if(temp < operValue || temp<addmoey) {
                return false;
            }
            */
            uint64_t temp = 0;
            if(!SafeAdd(addmoey, operValue, temp)) {
                return false;
            }
            addmoey = temp;
        }else if (it.opeatortype == MINUS_FREE) {

            //vector<unsigned char > accountid(it.accountid,it.accountid+sizeof(it.accountid));
            vector_unsigned_char accountid = GetAccountID(it);
            if(accountid.size() != 6)
                return false;
            CRegID regId(accountid);
            CTransaction* tx = static_cast<CTransaction*>(listTx.get());
            /// current tx's script cant't mius other script's regid
            if(m_ScriptDBTip->HaveScript(regId) && regId != boost::get<CRegID>(tx->desUserId))
            {
                return false;
            }

            memcpy(&operValue,it.money,sizeof(it.money));
            /*
            uint64_t temp = miusmoney;
            temp += operValue;
            if(temp < operValue || temp < miusmoney) {
                return false;
            }
            */
            uint64_t temp = 0;
            if(!SafeAdd(miusmoney, operValue, temp)) {
                return false;
            }
            miusmoney = temp;
        }
        else{
//          Assert(0);
            return false; // 输入数据错误
        }

        //vector<unsigned char> accountid(it.accountid, it.accountid + sizeof(it.accountid));
        vector_unsigned_char accountid = GetAccountID(it);
        if(accountid.size() == 6){
            CRegID regId(accountid);
            if(regId.IsEmpty() || regId.getKeyID( *m_view) == uint160())
                return false;
            //  app only be allowed minus self money
            if (!m_ScriptDBTip->HaveScript(regId) && it.opeatortype == MINUS_FREE) {
                return false;
            }
        }

    }
    if (addmoey != miusmoney)
    {
        return false;
    }
    return true;
}

bool CVmRunEvn::CheckAppAcctOperate(CTransaction* tx) {
    int64_t addValue(0), minusValue(0), sumValue(0);
    for(auto  vOpItem : MapAppOperate) {
        for(auto appFund : vOpItem.second) {
            if(ADD_FREE_OP == appFund.opeatortype || ADD_TAG_OP == appFund.opeatortype) {
                /*
                int64_t temp = appFund.mMoney;
                temp += addValue;
                if(temp < addValue || temp<appFund.mMoney) {
                    return false;
                }
                */
                int64_t temp = 0;
                if(!SafeAdd(appFund.mMoney, addValue, temp)) {
                    return false;
                }
                addValue = temp;
            }
            else if(SUB_FREE_OP == appFund.opeatortype || SUB_TAG_OP == appFund.opeatortype) {
                /*
                int64_t temp = appFund.mMoney;
                temp += minusValue;
                if(temp < minusValue || temp<appFund.mMoney) {
                    return false;
                }
                */
                int64_t temp = 0;
                if(!SafeAdd(appFund.mMoney, minusValue, temp)) {
                    return false;
                }
                minusValue = temp;
            }
        }
    }
    /*
    sumValue = addValue - minusValue;
    if(sumValue > addValue) {
        return false;
    }
    */
    if(!SafeSubtract(addValue, minusValue, sumValue)) {
        return false;
    }

    uint64_t sysContractAcct(0);
    for(auto item : m_output) {
        vector_unsigned_char vAccountId = GetAccountID(item);
        if(vAccountId == boost::get<CRegID>(tx->desUserId).GetVec6() && item.opeatortype == MINUS_FREE) {
            uint64_t value;
            memcpy(&value, item.money, sizeof(item.money));
            int64_t temp = value;
            if(temp < 0) {
                return false;
            }
            /*
            temp += sysContractAcct;
            if(temp < sysContractAcct || temp < (int64_t)value)
                return false;
            */
            uint64_t tempValue = temp;
            uint64_t tempOut = 0;
            if(!SafeAdd(tempValue, sysContractAcct, tempOut)) {
                return false;
            }
            sysContractAcct = tempOut;
        }
    }
/*
    int64_t sysAcctSum = tx->llValues - sysContractAcct;
    if(sysAcctSum > (int64_t)tx->llValues) {
        return false;
    }
*/
    int64_t sysAcctSum = 0;
    if (!SafeSubtract((int64_t)tx->llValues, (int64_t)sysContractAcct, sysAcctSum)) {
        return false;
    }

    if(sumValue != sysAcctSum){
        LogPrint("vm", "CheckAppAcctOperate:addValue=%lld, minusValue=%lld, txValue=%lld, sysContractAcct=%lld sumValue=%lld, sysAcctSum=%lld\n", addValue, minusValue, tx->llValues, sysContractAcct,sumValue, sysAcctSum);
        return false;
    }
    return true;
}

//shared_ptr<vector<CVmOperate>> CVmRunEvn::GetOperate() const {
//  auto tem = std::make_shared<vector<CVmOperate>>();
//  shared_ptr<vector<unsigned char>> retData = pMcu.get()->GetRetData();
//  CDataStream Contractstream(*retData.get(), SER_DISK, CLIENT_VERSION);
//  vector<CVmOperate> retvmcode;
//  ;
//  Contractstream >> retvmcode;
//  return tem;
//}

bool CVmRunEvn::OpeatorAccount(const vector<CVmOperate>& listoperate, CAccountViewCache& view, const int nCurHeight) {

    NewAccont.clear();
    for (auto& it : listoperate) {
//      CTransaction* tx = static_cast<CTransaction*>(listTx.get());
//      CFund fund;
//      memcpy(&fund.value,it.money,sizeof(it.money));
//      fund.nHeight = it.outheight;
        uint64_t value;
        memcpy(&value, it.money, sizeof(it.money));

        auto tem = std::make_shared<CAccount>();
//      vector_unsigned_char accountid = GetAccountID(it);
//      if (accountid.size() == 0) {
//          return false;
//      }
//      vector_unsigned_char accountid(it.accountid,it.accountid+sizeof(it.accountid));
        vector_unsigned_char accountid = GetAccountID(it);
        CRegID userregId;
        CKeyID userkeyid;

        if(accountid.size() == 6){
            userregId.SetRegID(accountid);
            if(!view.GetAccount(CUserID(userregId), *tem.get())){
                return false;                                           /// 账户不存在
            }
        }else{
            string popaddr(accountid.begin(), accountid.end());
            userkeyid = CKeyID(popaddr);
             if(!view.GetAccount(CUserID(userkeyid), *tem.get()))
             {
                 tem->keyID = userkeyid;
                //return false;                                           /// 未产生过交易记录的账户
             }
        }

        shared_ptr<CAccount> vmAccount = GetAccount(tem);
        if (vmAccount.get() == NULL) {
            RawAccont.push_back(tem);
            vmAccount = tem;
        }
        LogPrint("vm", "account id:%s\r\n", HexStr(accountid).c_str());
        LogPrint("vm", "befer account:%s\r\n", vmAccount.get()->ToString().c_str());
        bool ret = false;
//      vector<CScriptDBOperLog> vAuthorLog;
        //todolist
//      if(IsSignatureAccount(vmAccount.get()->regID) || vmAccount.get()->regID == boost::get<CRegID>(tx->appRegId))
        {
            ret = vmAccount.get()->OperateAccount((OperType)it.opeatortype, value, nCurHeight);
        }
//      else{
//          ret = vmAccount.get()->OperateAccount((OperType)it.opeatortype, fund, *m_ScriptDBTip, vAuthorLog,  height, &GetScriptRegID().GetVec6(), true);
//      }

//      LogPrint("vm", "after account:%s\r\n", vmAccount.get()->ToString().c_str());
        if (!ret) {
            return false;
        }
        NewAccont.push_back(vmAccount);
//      m_dblog->insert(m_dblog->end(), vAuthorLog.begin(), vAuthorLog.end());

    }
    return true;
}

const CRegID& CVmRunEvn::GetScriptRegID()
{   // 获取目的账户ID
    CTransaction* tx = static_cast<CTransaction*>(listTx.get());
    return boost::get<CRegID>(tx->desUserId);
}

const CRegID &CVmRunEvn::GetTxAccount() {
    CTransaction* tx = static_cast<CTransaction*>(listTx.get());
    return boost::get<CRegID>(tx->srcRegId);
}
uint64_t CVmRunEvn::GetValue() const{
    CTransaction* tx = static_cast<CTransaction*>(listTx.get());
        return tx->llValues;
}
const vector<unsigned char>& CVmRunEvn::GetTxContact()
{
    CTransaction* tx = static_cast<CTransaction*>(listTx.get());
        return tx->vContract;
}
int CVmRunEvn::GetComfirHeight()
{
    return RunTimeHeight;
}
uint256 CVmRunEvn::GetCurTxHash()
{
    return listTx.get()->GetHash();
}
CScriptDBViewCache* CVmRunEvn::GetScriptDB()
{
    return m_ScriptDBTip;
}
CAccountViewCache * CVmRunEvn::GetCatchView()
{
    return m_view;
}
void CVmRunEvn::InsertOutAPPOperte(const vector<unsigned char>& userId,const CAppFundOperate &source)
{
    if(MapAppOperate.count(userId))
    {
        MapAppOperate[userId].push_back(source);
    }
    else
    {
        vector<CAppFundOperate> it;
        it.push_back(source);
        MapAppOperate[userId] = it;
    }

}
bool CVmRunEvn::InsertOutputData(const vector<CVmOperate>& source)
{
    m_output.insert(m_output.end(),source.begin(),source.end());
    if(m_output.size() < MAX_OUTPUT_COUNT)
        return true;
    return false;
}
shared_ptr<vector<CScriptDBOperLog> > CVmRunEvn::GetDbLog()
{
    return m_dblog;
}

/**
 * 从脚本数据库中，取指定账户的 应用账户信息,同时解冻冻结金额到自由金额
 * @param vAppUserId   账户地址或regId
 * @param sptrAcc
 * @return
 */
bool CVmRunEvn::GetAppUserAccout(const vector<unsigned char> &vAppUserId, shared_ptr<CAppUserAccout> &sptrAcc) {
    assert(m_ScriptDBTip != NULL);
    shared_ptr<CAppUserAccout> tem = std::make_shared<CAppUserAccout>();
    if (!m_ScriptDBTip->GetScriptAcc(GetScriptRegID(), vAppUserId, *tem.get())) {
            tem = std::make_shared<CAppUserAccout>(vAppUserId);
            sptrAcc = tem;
            return true;
    }
    if (!tem.get()->AutoMergeFreezeToFree(RunTimeHeight)) {
        return false;
    }
    sptrAcc = tem;
    return true;
}

bool CVmRunEvn::OpeatorAppAccount(const map<vector<unsigned char >,vector<CAppFundOperate> > opMap, CScriptDBViewCache& view) {
    NewAppUserAccout.clear();
    if ((MapAppOperate.size() > 0)) {
        for (auto const tem : opMap) {
            shared_ptr<CAppUserAccout> sptrAcc;
            if (!GetAppUserAccout(tem.first, sptrAcc)) {
                LogPrint("vm", "GetAppUserAccout(tem.first, sptrAcc, true) failed \r\n appuserid :%s\r\n",
                        HexStr(tem.first));
                return false;
            }
            if (!sptrAcc.get()->AutoMergeFreezeToFree(RunTimeHeight)) {
                LogPrint("vm", "AutoMergeFreezeToFreefailed \r\n appuser :%s\r\n", sptrAcc.get()->toString());
                return false;

            }
            shared_ptr<CAppUserAccout> vmAppAccount = GetAppAccount(sptrAcc);
            if (vmAppAccount.get() == NULL) {
                RawAppUserAccout.push_back(sptrAcc);
                vmAppAccount = sptrAcc;
            }
            LogPrint("vm", "before user: %s\r\n", sptrAcc.get()->toString());
            if (!sptrAcc.get()->Operate(tem.second)) {

                int i = 0;
                for (auto const pint : tem.second) {
                    LogPrint("vm", "GOperate failed \r\n Operate %d : %s\r\n", i++, pint.toString());
                }
                LogPrint("vm", "GetAppUserAccout(tem.first, sptrAcc, true) failed \r\n appuserid :%s\r\n",
                        HexStr(tem.first));
                return false;
            }
            NewAppUserAccout.push_back(sptrAcc);
            LogPrint("vm", "after user: %s\r\n", sptrAcc.get()->toString());
            CScriptDBOperLog log;
            view.SetScriptAcc(GetScriptRegID(), *sptrAcc.get(), log);
            shared_ptr<vector<CScriptDBOperLog> > m_dblog = GetDbLog();
            m_dblog.get()->push_back(log);

        }
    }
    return true;
}

void CVmRunEvn::SetCheckAccount(bool bCheckAccount)
{
    isCheckAccount = bCheckAccount;
}

Object CVmOperate::ToJson() {
    Object obj;
    if(nacctype == regid) {
        vector<unsigned char> vRegId(accountid, accountid+6);
        CRegID regId(vRegId);
        obj.push_back(Pair("regid", regId.ToString()));
    }else if(nacctype == base58addr) {
        string addr(accountid,accountid+sizeof(accountid));
        obj.push_back(Pair("addr", addr));
    }
    if(opeatortype == ADD_FREE) {
        obj.push_back(Pair("opertype", "add"));
    }else if(opeatortype == MINUS_FREE) {
        obj.push_back(Pair("opertype", "minus"));
    }
    if(outheight > 0)
        obj.push_back(Pair("freezeheight", (int) outheight));
    uint64_t amount;
    memcpy(&amount, money, sizeof(money));
    obj.push_back(Pair("amount", amount));
    return obj;
}
