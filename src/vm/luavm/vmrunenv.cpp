/*
 * ScriptCheck.cpp
 *
 *  Created on: Sep 2, 2014
 *      Author: ranger.shi
 */
#include "vmrunenv.h"
#include <algorithm>
#include <boost/foreach.hpp>
#include "commons/SafeInt3.hpp"
#include "tx/tx.h"
#include "commons/util.h"

// CVmRunEnv *pVmRunEnv = NULL;

#define MAX_OUTPUT_COUNT 100

CVmRunEnv::CVmRunEnv() {
    rawAccount.clear();
    newAccount.clear();
    rawAppUserAccount.clear();
    newAppUserAccount.clear();
    runTimeHeight      = 0;
    pContractCache = nullptr;
    pAccountCache  = nullptr;
    pDBOpLogsMap   = nullptr;
    isCheckAccount     = false;
}

vector<shared_ptr<CAccount>>& CVmRunEnv::GetRawAccont() { return rawAccount; }

vector<shared_ptr<CAccount>>& CVmRunEnv::GetNewAccount() { return newAccount; }

vector<shared_ptr<CAppUserAccount>>& CVmRunEnv::GetNewAppUserAccount() { return newAppUserAccount; }

vector<shared_ptr<CAppUserAccount>>& CVmRunEnv::GetRawAppUserAccount() { return rawAppUserAccount; }

bool CVmRunEnv::Initialize(shared_ptr<CBaseTx>& tx, CAccountDBCache& view, int nHeight) {
    vmOperateOutput.clear();
    pBaseTx       = tx;
    runTimeHeight = nHeight;
    pAccountCache = &view;
    string contractScript;

    if (tx.get()->nTxType != CONTRACT_INVOKE_TX) {
        LogPrint("ERROR", "%s\n", "err param");
        return false;
    }

    CContractInvokeTx* contractTx = static_cast<CContractInvokeTx*>(tx.get());
    if (!pContractCache->GetContractScript(contractTx->appUid.get<CRegID>(), contractScript)) {
        LogPrint("ERROR", "contract not found: %s\n",
                 contractTx->appUid.get<CRegID>().ToString());
        return false;
    }

    CDataStream stream(contractScript, SER_DISK, CLIENT_VERSION);
    try {
        stream >> vmScript;
    } catch (exception& e) {
        LogPrint("ERROR", "%s\n", "CVmScriptRun::Initialize() Unserialize to vmScript error");
        throw runtime_error("CVmScriptRun::Initialize() Unserialize to vmScript error:" +
                            string(e.what()));
    }

    if (!vmScript.IsValid()) {
        LogPrint("ERROR", "%s\n", "CVmScriptRun::Initialize() vmScript.IsValid error");
        return false;
    }
    isCheckAccount = vmScript.IsCheckAccount();
    if (contractTx->arguments.size() >= kContractArgumentMaxSize) {
        LogPrint("ERROR", "%s\n", "CVmScriptRun::Initialize() arguments context size too large");
        return false;
    }

    try {
        pLua = std::make_shared<CVmlua>(vmScript.GetRom(), contractTx->arguments);
    } catch (exception& e) {
        LogPrint("ERROR", "%s\n", "CVmScriptRun::Initialize() CVmlua init error");
        return false;
    }

    // pVmRunEnv = this; //传CVmRunEnv对象指针给lmylib.cpp库使用
    LogPrint("vm", "%s\n", "CVmScriptRun::Initialize() LUA");

    return true;
}

CVmRunEnv::~CVmRunEnv() {}

tuple<bool, uint64_t, string> CVmRunEnv::ExecuteContract(shared_ptr<CBaseTx>& pBaseTx, int nHeight, CCacheWrapper& cw,
                                                         uint64_t nBurnFactor, uint64_t& uRunStep) {
    if (nBurnFactor == 0) return std::make_tuple(false, 0, string("VmScript nBurnFactor == 0"));

    pContractCache = &cw.contractCache;
    pDBOpLogsMap   = &cw.txUndo.dbOpLogMap;

    CContractInvokeTx* tx = static_cast<CContractInvokeTx*>(pBaseTx.get());
    if (tx->llFees < CBaseTx::nMinTxFee)
        return std::make_tuple(false, 0, string("CVmRunEnv: Contract pBaseTx fee too small"));

    uint64_t fuelLimit = ((tx->llFees - CBaseTx::nMinTxFee) / nBurnFactor) * 100;
    if (fuelLimit > MAX_BLOCK_RUN_STEP) {
        fuelLimit = MAX_BLOCK_RUN_STEP;
    }

    LogPrint("vm", "tx hash:%s fees=%lld fuelrate=%lld fuelLimit:%d\n", pBaseTx->GetHash().GetHex(), tx->llFees,
             nBurnFactor, fuelLimit);

    if (fuelLimit == 0) {
        return std::make_tuple(false, 0, string("CVmRunEnv::ExecuteContract, fees too low"));
    }

    if (!Initialize(pBaseTx, cw.accountCache, nHeight)) {
        return std::make_tuple(false, 0, string("VmScript inital Failed"));
    }

    tuple<uint64_t, string> ret = pLua.get()->Run(fuelLimit, this);
    LogPrint("vm", "CVmScriptRun::ExecuteContract() LUA\n");

    int64_t step = std::get<0>(ret);
    if (0 == step) {
        return std::make_tuple(false, 0, string("VmScript run Failed"));
    } else if (-1 == step) {
        return std::make_tuple(false, 0, std::get<1>(ret));
    } else {
        uRunStep = step;
    }

    LogPrint("vm", "tx:%s,step:%ld\n", tx->ToString(cw.accountCache), uRunStep);

    if (!CheckOperate(vmOperateOutput)) {
        return std::make_tuple(false, 0, string("VmScript CheckOperate Failed"));
    }

    if (!OperateAccount(vmOperateOutput, cw.accountCache, nHeight)) {
        return std::make_tuple(false, 0, string("VmScript OperateAccount Failed"));
    }

    LogPrint("vm", "isCheckAccount:%d\n", isCheckAccount);
    if (isCheckAccount) {
        LogPrint("vm", "isCheckAccount is true\n");
        if (!CheckAppAcctOperate(tx))
            return std::make_tuple(false, 0, string("VmScript CheckAppAcct Failed"));
    }

    if (!OperateAppAccount(mapAppFundOperate, *pContractCache)) {
        return std::make_tuple(false, 0, string("OperateAppAccount Account Failed"));
    }

    if (SysCfg().IsContractLogOn() && vmOperateOutput.size() > 0) {
        uint256 txid = GetCurTxHash();
        if (!pContractCache->WriteTxOutput(txid, vmOperateOutput, *pDBOpLogsMap))
            return std::make_tuple(false, 0, string("write tx out put Failed"));
    }
    /*
        uint64_t spend = uRunStep * nBurnFactor;
            if((spend < uRunStep) || (spend < nBurnFactor)){
            return std::make_tuple (false, 0, string("mul error\n"));
        }
    */
    uint64_t spend = 0;
    if (!SafeMultiply(uRunStep, nBurnFactor, spend)) {
        return std::make_tuple(false, 0, string("mul error"));
    }
    return std::make_tuple(true, spend, string("VmScript Success"));
}

bool CVmRunEnv::UndoDatas(CCacheWrapper &cw) {

    if (!cw.contractCache.UndoTxOutput(cw.txUndo.dbOpLogMap)) {
        return ERRORMSG("CVmRunEnv::UndoDatas UndoTxOutput failed");
    }
    if (!cw.contractCache.UndoContractAccount(cw.txUndo.dbOpLogMap)) {
        return ERRORMSG("CVmRunEnv::UndoDatas UndoContractAccount failed");
    }
    if (!cw.contractCache.UndoContractData(cw.txUndo.dbOpLogMap)) {
        return ERRORMSG("CVmRunEnv::UndoDatas UndoContractData failed");
    }
    return true;
}

shared_ptr<CAccount> CVmRunEnv::GetNewAccount(shared_ptr<CAccount>& vOldAccount) {
    if (newAccount.size() == 0) return NULL;
    vector<shared_ptr<CAccount>>::iterator Iter;
    for (Iter = newAccount.begin(); Iter != newAccount.end(); Iter++) {
        shared_ptr<CAccount> temp = *Iter;
        if (temp.get()->keyId == vOldAccount.get()->keyId) {
            newAccount.erase(Iter);
            return temp;
        }
    }
    return NULL;
}
shared_ptr<CAccount> CVmRunEnv::GetAccount(shared_ptr<CAccount>& Account) {
    if (rawAccount.size() == 0) return NULL;
    vector<shared_ptr<CAccount>>::iterator Iter;
    for (Iter = rawAccount.begin(); Iter != rawAccount.end(); Iter++) {
        shared_ptr<CAccount> temp = *Iter;
        if (Account.get()->keyId == temp.get()->keyId) {
            return temp;
        }
    }
    return NULL;
}

UnsignedCharArray CVmRunEnv::GetAccountID(CVmOperate value) {
    UnsignedCharArray accountId;
    if (value.accountType == regid) {
        accountId.assign(value.accountId, value.accountId + 6);
    } else if (value.accountType == base58addr) {
        string addr(value.accountId, value.accountId + sizeof(value.accountId));
        CKeyID KeyId = CKeyID(addr);
        CRegID regid;
        if (pAccountCache->GetRegId(CUserID(KeyId), regid)) {
            accountId.assign(regid.GetRegIdRaw().begin(), regid.GetRegIdRaw().end());
        } else {
            accountId.assign(value.accountId, value.accountId + 34);
        }
    }
    return accountId;
}

shared_ptr<CAppUserAccount> CVmRunEnv::GetAppAccount(shared_ptr<CAppUserAccount>& AppAccount) {
    if (rawAppUserAccount.size() == 0) return NULL;
    vector<shared_ptr<CAppUserAccount>>::iterator Iter;
    for (Iter = rawAppUserAccount.begin(); Iter != rawAppUserAccount.end(); Iter++) {
        shared_ptr<CAppUserAccount> temp = *Iter;
        if (AppAccount.get()->GetAccUserId() == temp.get()->GetAccUserId()) return temp;
    }
    return NULL;
}

bool CVmRunEnv::CheckOperate(const vector<CVmOperate>& listoperate) {
    // judge contract rulue
    uint64_t addmoey = 0, miusmoney = 0;
    uint64_t operValue = 0;
    if (listoperate.size() > MAX_OUTPUT_COUNT) return false;

    for (auto& it : listoperate) {
        if (it.accountType != regid && it.accountType != base58addr) return false;

        if (it.opType == ADD_BCOIN) {
            memcpy(&operValue, it.money, sizeof(it.money));
            /*
            uint64_t temp = addmoey;
            temp += operValue;
            if(temp < operValue || temp<addmoey) {
                return false;
            }
            */
            uint64_t temp = 0;
            if (!SafeAdd(addmoey, operValue, temp)) {
                return false;
            }
            addmoey = temp;
        } else if (it.opType == MINUS_BCOIN) {
            // vector<unsigned char > accountId(it.accountId,it.accountId+sizeof(it.accountId));
            UnsignedCharArray accountId = GetAccountID(it);
            if (accountId.size() != 6) return false;
            CRegID regId(accountId);
            CContractInvokeTx* tx = static_cast<CContractInvokeTx*>(pBaseTx.get());
            /// current tx's script cant't mius other script's regid
            if (pContractCache->HaveContractScript(regId) && regId != tx->appUid.get<CRegID>())
                return false;

            memcpy(&operValue, it.money, sizeof(it.money));
            /*
            uint64_t temp = miusmoney;
            temp += operValue;
            if(temp < operValue || temp < miusmoney) {
                return false;
            }
            */
            uint64_t temp = 0;
            if (!SafeAdd(miusmoney, operValue, temp)) return false;

            miusmoney = temp;
        } else {
            //          Assert(0);
            return false;  // 输入数据错误
        }

        // vector<unsigned char> accountId(it.accountId, it.accountId + sizeof(it.accountId));
        UnsignedCharArray accountId = GetAccountID(it);
        if (accountId.size() == 6) {
            CRegID regId(accountId);
            if (regId.IsEmpty() || regId.GetKeyId(*pAccountCache) == uint160()) return false;

            //  app only be allowed minus self money
            if (!pContractCache->HaveContractScript(regId) && it.opType == MINUS_BCOIN) return false;
        }
    }

    if (addmoey != miusmoney) return false;

    return true;
}

bool CVmRunEnv::CheckAppAcctOperate(CContractInvokeTx* tx) {
    int64_t addValue(0), minusValue(0), sumValue(0);
    for (auto vOpItem : mapAppFundOperate) {
        for (auto appFund : vOpItem.second) {
            if (ADD_FREE_OP == appFund.opType || ADD_TAG_OP == appFund.opType) {
                /*
                int64_t temp = appFund.mMoney;
                temp += addValue;
                if(temp < addValue || temp<appFund.mMoney) {
                    return false;
                }
                */
                int64_t temp = 0;
                if (!SafeAdd(appFund.mMoney, addValue, temp)) return false;

                addValue = temp;
            } else if (SUB_FREE_OP == appFund.opType || SUB_TAG_OP == appFund.opType) {
                /*
                int64_t temp = appFund.mMoney;
                temp += minusValue;
                if(temp < minusValue || temp<appFund.mMoney) {
                    return false;
                }
                */
                int64_t temp = 0;
                if (!SafeAdd(appFund.mMoney, minusValue, temp)) return false;

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
    if (!SafeSubtract(addValue, minusValue, sumValue)) return false;

    uint64_t sysContractAcct(0);
    for (auto item : vmOperateOutput) {
        UnsignedCharArray vAccountId = GetAccountID(item);
        if (vAccountId == tx->appUid.get<CRegID>().GetRegIdRaw() &&
            item.opType == MINUS_BCOIN) {
            uint64_t value;
            memcpy(&value, item.money, sizeof(item.money));
            int64_t temp = value;
            if (temp < 0) return false;

            /*
            temp += sysContractAcct;
            if(temp < sysContractAcct || temp < (int64_t)value)
                return false;
            */
            uint64_t tempValue = temp;
            uint64_t tempOut   = 0;
            if (!SafeAdd(tempValue, sysContractAcct, tempOut)) return false;

            sysContractAcct = tempOut;
        }
    }
    /*
        int64_t sysAcctSum = tx->bcoins - sysContractAcct;
        if(sysAcctSum > (int64_t)tx->bcoins) {
            return false;
        }
    */
    int64_t sysAcctSum = 0;
    if (!SafeSubtract((int64_t)tx->bcoins, (int64_t)sysContractAcct, sysAcctSum)) return false;

    if (sumValue != sysAcctSum) {
        LogPrint("vm",
                 "CheckAppAcctOperate:addValue=%lld, minusValue=%lld, txValue=%lld, "
                 "sysContractAcct=%lld sumValue=%lld, sysAcctSum=%lld\n",
                 addValue, minusValue, tx->bcoins, sysContractAcct, sumValue, sysAcctSum);

        return false;
    }

    return true;
}

// shared_ptr<vector<CVmOperate>> CVmRunEnv::GetOperate() const {
//  auto tem = std::make_shared<vector<CVmOperate>>();
//  shared_ptr<vector<unsigned char>> retData = pMcu.get()->GetRetData();
//  CDataStream Contractstream(*retData.get(), SER_DISK, CLIENT_VERSION);
//  vector<CVmOperate> retvmcode;
//  ;
//  Contractstream >> retvmcode;
//  return tem;
//}

bool CVmRunEnv::OperateAccount(const vector<CVmOperate>& listoperate, CAccountDBCache& view,
                               const int nCurHeight) {
    newAccount.clear();
    for (auto& it : listoperate) {
        //      CTransaction* tx = static_cast<CTransaction*>(pBaseTx.get());
        //      CFund fund;
        //      memcpy(&fund.value,it.money,sizeof(it.money));
        //      fund.nHeight = it.timeoutHeight;
        uint64_t value;
        memcpy(&value, it.money, sizeof(it.money));

        auto tem = std::make_shared<CAccount>();
        //      UnsignedCharArray accountId = GetAccountID(it);
        //      if (accountId.size() == 0) {
        //          return false;
        //      }
        //      UnsignedCharArray accountId(it.accountId,it.accountId+sizeof(it.accountId));
        UnsignedCharArray accountId = GetAccountID(it);
        CRegID userregId;
        CKeyID userkeyid;

        if (accountId.size() == 6) {
            userregId.SetRegID(accountId);
            if (!view.GetAccount(CUserID(userregId), *tem.get())) {
                return false;  // 账户不存在
            }
        } else {
            string popaddr(accountId.begin(), accountId.end());
            userkeyid = CKeyID(popaddr);
            if (!view.GetAccount(CUserID(userkeyid), *tem.get())) {
                tem->keyId = userkeyid;
                // return false;
                // 未产生过交易记录的账户
            }
        }

        shared_ptr<CAccount> vmAccount = GetAccount(tem);
        if (vmAccount.get() == NULL) {
            rawAccount.push_back(tem);
            vmAccount = tem;
        }
        LogPrint("vm", "account id:%s\nbefore account: %s\n", HexStr(accountId).c_str(),
                 vmAccount.get()->ToString());

        bool ret = false;
        //      vector<CDbOpLog> vAuthorLog;
        // todolist
        //      if(IsSignatureAccount(vmAccount.get()->regId) || vmAccount.get()->regId ==
        //      tx->appRegId.get<CRegID>())
        { ret = vmAccount.get()->OperateBalance(CoinType::WICC, (BalanceOpType)it.opType, value); }
        //      else{
        //          ret = vmAccount.get()->OperateBalance((BalanceOpType)it.opType, fund,
        //          *pContractCache, vAuthorLog,  height, &GetScriptRegID().GetRegIdRaw(), true);
        //      }

        //      LogPrint("vm", "after account:%s\n", vmAccount.get()->ToString());
        if (!ret) return false;

        newAccount.push_back(vmAccount);
        //      pScriptDBOperLog->insert(pScriptDBOperLog->end(), vAuthorLog.begin(),
        //      vAuthorLog.end());
    }

    return true;
}

const CRegID& CVmRunEnv::GetScriptRegID() {  // 获取目的账户ID
    CContractInvokeTx* tx = static_cast<CContractInvokeTx*>(pBaseTx.get());
    return tx->appUid.get<CRegID>();
}

const CRegID& CVmRunEnv::GetTxAccount() {
    CContractInvokeTx* tx = static_cast<CContractInvokeTx*>(pBaseTx.get());
    return tx->txUid.get<CRegID>();
}

uint64_t CVmRunEnv::GetValue() const {
    CContractInvokeTx* tx = static_cast<CContractInvokeTx*>(pBaseTx.get());
    return tx->bcoins;
}

const vector<unsigned char>& CVmRunEnv::GetTxContract() {
    CContractInvokeTx* tx = static_cast<CContractInvokeTx*>(pBaseTx.get());
    return tx->arguments;
}

int CVmRunEnv::GetConfirmHeight() { return runTimeHeight; }

int CVmRunEnv::GetBurnVersion() {
    // the burn version belong to the Feature Fork Version
    return GetFeatureForkVersion(runTimeHeight);
}

uint256 CVmRunEnv::GetCurTxHash() { return pBaseTx.get()->GetHash(); }

CContractDBCache* CVmRunEnv::GetScriptDB() { return pContractCache; }

CAccountDBCache* CVmRunEnv::GetCatchView() { return pAccountCache; }

void CVmRunEnv::InsertOutAPPOperte(const vector<unsigned char>& userId,
                                   const CAppFundOperate& source) {
    if (mapAppFundOperate.count(userId)) {
        mapAppFundOperate[userId].push_back(source);
    } else {
        vector<CAppFundOperate> it;
        it.push_back(source);
        mapAppFundOperate[userId] = it;
    }
}

bool CVmRunEnv::InsertOutputData(const vector<CVmOperate>& source) {
    vmOperateOutput.insert(vmOperateOutput.end(), source.begin(), source.end());
    if (vmOperateOutput.size() < MAX_OUTPUT_COUNT) return true;
    return false;
}

CDBOpLogMap* CVmRunEnv::GetDbLog() { return pDBOpLogsMap; }

/**
 * 从脚本数据库中，取指定账户的 应用账户信息,同时解冻冻结金额到自由金额
 * @param vAppUserId   账户地址或regId
 * @param sptrAcc
 * @return
 */
bool CVmRunEnv::GetAppUserAccount(const vector<unsigned char>& vAppUserId,
                                  shared_ptr<CAppUserAccount>& sptrAcc) {
    assert(pContractCache);
    shared_ptr<CAppUserAccount> tem = std::make_shared<CAppUserAccount>();
    string appUserId(vAppUserId.begin(), vAppUserId.end());
    if (!pContractCache->GetContractAccount(GetScriptRegID(), appUserId, *tem.get())) {
        tem     = std::make_shared<CAppUserAccount>(appUserId);
        sptrAcc = tem;
        return true;
    }
    if (!tem.get()->AutoMergeFreezeToFree(runTimeHeight)) {
        return false;
    }
    sptrAcc = tem;
    return true;
}

bool CVmRunEnv::OperateAppAccount(const map<vector<unsigned char>, vector<CAppFundOperate>> opMap,
                                  CContractDBCache& view) {
    newAppUserAccount.clear();
    if ((mapAppFundOperate.size() > 0)) {
        for (auto const tem : opMap) {
            shared_ptr<CAppUserAccount> sptrAcc;
            if (!GetAppUserAccount(tem.first, sptrAcc)) {
                LogPrint("vm",
                         "GetAppUserAccount(tem.first, sptrAcc, true) failed \n appuserid :%s\n",
                         HexStr(tem.first));
                return false;
            }

            if (!sptrAcc.get()->AutoMergeFreezeToFree(runTimeHeight)) {
                LogPrint("vm", "AutoMergeFreezeToFreefailed \n appuser :%s\n",
                         sptrAcc.get()->ToString());
                return false;
            }

            shared_ptr<CAppUserAccount> vmAppAccount = GetAppAccount(sptrAcc);
            if (vmAppAccount.get() == NULL) {
                rawAppUserAccount.push_back(sptrAcc);
                vmAppAccount = sptrAcc;
            }

            LogPrint("vm", "before user: %s\n", sptrAcc.get()->ToString());
            if (!sptrAcc.get()->Operate(tem.second)) {
                int i = 0;
                for (auto const pint : tem.second) {
                    LogPrint("vm", "GOperate failed \n Operate %d : %s\n", i++, pint.ToString());
                }
                LogPrint("vm",
                         "GetAppUserAccount(tem.first, sptrAcc, true) failed \n appuserid :%s\n",
                         HexStr(tem.first));
                return false;
            }
            newAppUserAccount.push_back(sptrAcc);
            LogPrint("vm", "after user: %s\n", sptrAcc.get()->ToString());
            view.SetContractAccount(GetScriptRegID(), *sptrAcc.get(), *pDBOpLogsMap);
        }
    }
    return true;
}

void CVmRunEnv::SetCheckAccount(bool bCheckAccount) { isCheckAccount = bCheckAccount; }
