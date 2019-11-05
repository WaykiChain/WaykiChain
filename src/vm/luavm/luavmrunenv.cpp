// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "luavmrunenv.h"
#include "commons/SafeInt3.hpp"
#include "tx/tx.h"
#include "commons/util.h"
#include "vm/luavm/lua/lua.hpp"
#include "vm/luavm/lua/lburner.h"

#define MAX_OUTPUT_COUNT 100

CLuaVMRunEnv::CLuaVMRunEnv():
    p_context(nullptr),
	pLua(nullptr),
	rawAppUserAccount(),
	newAppUserAccount(),
    receipts(),
	vmOperateOutput(),
    transfer_count(0),
    isCheckAccount(false) {}

vector<shared_ptr<CAppUserAccount>>& CLuaVMRunEnv::GetNewAppUserAccount() { return newAppUserAccount; }
vector<shared_ptr<CAppUserAccount>>& CLuaVMRunEnv::GetRawAppUserAccount() { return rawAppUserAccount; }

CLuaVMRunEnv::~CLuaVMRunEnv() {}

std::shared_ptr<string>  CLuaVMRunEnv::ExecuteContract(CLuaVMContext *pContextIn, uint64_t& uRunStep) {
    p_context = pContextIn;

    assert(p_context->p_arguments->size() <= MAX_CONTRACT_ARGUMENT_SIZE);
    assert(p_context->fuel_limit > 0);

    pLua = std::make_shared<CLuaVM>(p_context->p_contract->code, *p_context->p_arguments);

    LogPrint("vm", "CVmScriptRun::ExecuteContract(), prepare to execute tx. txid=%s, fuelLimit=%llu\n",
             p_context->p_base_tx->GetHash().GetHex(), p_context->fuel_limit);

    tuple<uint64_t, string> ret = pLua.get()->Run(p_context->fuel_limit, this);

    int64_t step = std::get<0>(ret);
    if (0 == step) {
        return make_shared<string>("VmScript run Failed");
    } else if (-1 == step) {
        return make_shared<string>(std::get<1>(ret));
    } else {
        uRunStep = step;
    }

    LogPrint("vm", "txid:%s, step:%ld\n", p_context->p_base_tx->ToString(p_context->p_cw->accountCache), uRunStep);

    if (!CheckOperate()) {
        return make_shared<string>("VmScript CheckOperate Failed");
    }

    if (!OperateAccount(vmOperateOutput)) {
        return make_shared<string>("VmScript OperateAccount Failed");
    }

    LogPrint("vm", "isCheckAccount: %d\n", isCheckAccount);
    // CheckAppAcctOperate only support to check when the transfer symbol is WICC
    if (isCheckAccount && p_context->transfer_symbol == SYMB::WICC && !CheckAppAcctOperate()) {
        return make_shared<string>("VmScript CheckAppAcct Failed");
    }

    if (!OperateAppAccount(mapAppFundOperate)) {
        return make_shared<string>("OperateAppAccount Failed");
    }

    return nullptr;
}

UnsignedCharArray CLuaVMRunEnv::GetAccountID(const CVmOperate& value) {
    UnsignedCharArray accountId;
    if (value.accountType == AccountType::REGID) {
        accountId.assign(value.accountId, value.accountId + 6);
    } else if (value.accountType == AccountType::BASE58ADDR) {
        string addr(value.accountId, value.accountId + sizeof(value.accountId));
        CKeyID keyid = CKeyID(addr);
        CRegID regid;
        if (p_context->p_cw->accountCache.GetRegId(CUserID(keyid), regid)) {
            accountId.assign(regid.GetRegIdRaw().begin(), regid.GetRegIdRaw().end());
        } else {
            accountId.assign(value.accountId, value.accountId + 34);
        }
    }

    return accountId;
}

shared_ptr<CAppUserAccount> CLuaVMRunEnv::GetAppAccount(shared_ptr<CAppUserAccount>& appAccount) {
    if (rawAppUserAccount.size() == 0)
        return nullptr;

    vector<shared_ptr<CAppUserAccount>>::iterator Iter;
    for (Iter = rawAppUserAccount.begin(); Iter != rawAppUserAccount.end(); Iter++) {
        shared_ptr<CAppUserAccount> temp = *Iter;
        if (appAccount.get()->GetAccUserId() == temp.get()->GetAccUserId())
            return temp;
    }
    return nullptr;
}

bool CLuaVMRunEnv::CheckOperate() {
    // judge contract rule
    uint64_t addMoney = 0, minusMoney = 0;
    uint64_t operValue = 0;
    for (auto& it : vmOperateOutput) {
        if (it.accountType != REGID && it.accountType != AccountType::BASE58ADDR)
            return false;

        if (it.opType == BalanceOpType::ADD_FREE) {
            memcpy(&operValue, it.money, sizeof(it.money));
            /*
            uint64_t temp = addMoney;
            temp += operValue;
            if(temp < operValue || temp<addMoney) {
                return false;
            }
            */
            uint64_t temp = 0;
            if (!SafeAdd(addMoney, operValue, temp)) {
                return false;
            }
            addMoney = temp;
        } else if (it.opType == BalanceOpType::SUB_FREE) {
            UnsignedCharArray accountId = GetAccountID(it);
            if (accountId.size() != 6)
                return false;
            CRegID regId(accountId);
            // allow to minus current contract's account only.
            assert(!p_context->p_app_account->regid.IsEmpty());
            if (regId != p_context->p_app_account->regid) {
                return false;
            }

            memcpy(&operValue, it.money, sizeof(it.money));
            /*
            uint64_t temp = minusMoney;
            temp += operValue;
            if(temp < operValue || temp < minusMoney) {
                return false;
            }
            */
            uint64_t temp = 0;
            if (!SafeAdd(minusMoney, operValue, temp))
                return false;

            minusMoney = temp;
        } else {
            // Assert(0);
            return false;  // 输入数据错误
        }
    }

    if (addMoney != minusMoney)
        return false;

    return true;
}

bool CLuaVMRunEnv::CheckAppAcctOperate() {
    assert(p_context->transfer_symbol == SYMB::WICC);
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
                if (!SafeAdd(appFund.mMoney, addValue, temp))
                    return false;

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
                if (!SafeAdd(appFund.mMoney, minusValue, temp))
                    return false;

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
    if (!SafeSubtract(addValue, minusValue, sumValue))
        return false;

    uint64_t sysContractAcct(0);
    for (auto item : vmOperateOutput) {
        UnsignedCharArray vAccountId = GetAccountID(item);
        if (vAccountId == p_context->p_app_account->regid.GetRegIdRaw() &&
            item.opType == BalanceOpType::SUB_FREE) {
            uint64_t value;
            memcpy(&value, item.money, sizeof(item.money));
            int64_t temp = value;
            if (temp < 0)
                return false;

            /*
            temp += sysContractAcct;
            if(temp < sysContractAcct || temp < (int64_t)value)
                return false;
            */
            uint64_t tempValue = temp;
            uint64_t tempOut   = 0;
            if (!SafeAdd(tempValue, sysContractAcct, tempOut))
                return false;

            sysContractAcct = tempOut;
        }
    }
    /*
        int64_t sysAcctSum = tx->coin_amount - sysContractAcct;
        if(sysAcctSum > (int64_t)tx->coin_amount) {
            return false;
        }
    */
    int64_t sysAcctSum = 0;
    if (!SafeSubtract((int64_t)p_context->transfer_amount, (int64_t)sysContractAcct, sysAcctSum))
        return false;

    if (sumValue != sysAcctSum) {
        LogPrint("vm",
                 "CheckAppAcctOperate: addValue=%lld, minusValue=%lld, txValue[%s]=%lld, "
                 "sysContractAcct=%lld, sumValue=%lld, sysAcctSum=%lld\n",
                 addValue, minusValue, SYMB::WICC, p_context->transfer_amount, sysContractAcct, sumValue, sysAcctSum);

        return false;
    }

    return true;
}

bool CLuaVMRunEnv::OperateAccount(const vector<CVmOperate>& operates) {

    for (auto& operate : operates) {
        uint64_t value;
        memcpy(&value, operate.money, sizeof(operate.money));

        auto pAccount = std::make_shared<CAccount>();
        UnsignedCharArray accountId = GetAccountID(operate);
        CUserID uid;

        if (accountId.size() == 6) {
            CRegID regid;
            regid.SetRegID(accountId);
            if (!p_context->p_cw->accountCache.GetAccount(CUserID(regid), *pAccount)) {
                LogPrint("vm", "[ERR]CLuaVMRunEnv::OperateAccount(), account not exist! regid=%s\n", regid.ToString());
                return false;
            }
            uid = regid;
        } else {
            CKeyID keyid;
            keyid = CKeyID(string(accountId.begin(), accountId.end()));
            if (!p_context->p_cw->accountCache.GetAccount(keyid, *pAccount)) {
                pAccount = make_shared<CAccount>(keyid);
                // TODO: new account fuel
            }
            uid = keyid;
        }

        LogPrint("vm", "uid=%s\nbefore account: %s\n", uid.ToString(),
                 pAccount->ToString());

        if (!pAccount->OperateBalance(SYMB::WICC, operate.opType, value)) {
            LogPrint("vm", "[ERR]CLuaVMRunEnv::OperateAccount(), operate account failed! uid=%s, operate=%s\n",
                uid.ToString(), GetBalanceOpTypeName(operate.opType));
            return false;
        }

        if (operate.opType == BalanceOpType::ADD_FREE) {
            receipts.emplace_back(nullId, uid, SYMB::WICC, value, ReceiptCode::CONTRACT_ACCOUNT_OPERATE_ADD);
        } else if (operate.opType == BalanceOpType::SUB_FREE) {
            receipts.emplace_back(uid, nullId, SYMB::WICC, value, ReceiptCode::CONTRACT_ACCOUNT_OPERATE_SUB);
        }

        if (!p_context->p_cw->accountCache.SetAccount(pAccount->keyid, *pAccount)) {
            LogPrint("vm",
                     "[ERR]CLuaVMRunEnv::OperateAccount(), save account failed, uid=%s\n", uid.ToString());
            return false;
        }

        LogPrint("vm", "after account:%s\n", pAccount->ToString());
    }

    return true;
}

bool CLuaVMRunEnv::TransferAccountAsset(lua_State *L, const vector<AssetTransfer> &transfers) {

    transfer_count += transfers.size();
    if (!CheckOperateAccountLimit())
        return false;

    bool ret = true;
    for (auto& transfer : transfers) {
        bool isNewAccount = false;
        CUserID fromUid;
        if (transfer.isContractAccount) {
            fromUid = GetContractRegID();
        } else {
            fromUid = GetTxUserRegid();
        }

        // TODO: need to cache the from account?
        auto pFromAccount = make_shared<CAccount>();
        if (!p_context->p_cw->accountCache.GetAccount(fromUid, *pFromAccount)) {
            LogPrint("vm", "[ERR]CLuaVMRunEnv::TransferAccountAsset(), get from_account failed! from_uid=%s, "
                     "isContractAccount=%d\n", transfer.toUid.ToString(), (int)transfer.isContractAccount);
            ret = false; break;
        }

        auto pToAccount = make_shared<CAccount>();
        if (!p_context->p_cw->accountCache.GetAccount(transfer.toUid, *pToAccount)) {
            if (!transfer.toUid.is<CKeyID>()) {
                LogPrint("vm", "[ERR]CLuaVMRunEnv::TransferAccountAsset(), get to_account failed! to_uid=%s\n",
                    transfer.toUid.ToString());
                ret = false; break;
            }
            // create new user
            pToAccount = make_shared<CAccount>(transfer.toUid.get<CKeyID>());
            isNewAccount = true;
            LogPrint("vm", "CLuaVMRunEnv::TransferAccountAsset(), create new user! to_uid=%s\n",
                transfer.toUid.ToString());
        }

        if (!pFromAccount->OperateBalance(transfer.tokenType, SUB_FREE, transfer.tokenAmount)) {
            LogPrint("vm", "[ERR]CLuaVMRunEnv::TransferAccountAsset(), operate SUB_FREE in from_account failed! "
                "from_uid=%s, isContractAccount=%d, symbol=%s, amount=%llu, account_amount=%llu\n",
                fromUid.ToString(), (int)transfer.isContractAccount, transfer.tokenType,
                transfer.tokenAmount, pFromAccount->GetBalance(transfer.tokenType, BalanceType::FREE_VALUE));
            ret = false; break;
        }

        if (!pToAccount->OperateBalance(transfer.tokenType, ADD_FREE, transfer.tokenAmount)) {
            LogPrint("vm", "[ERR]CLuaVMRunEnv::TransferAccountAsset(), operate ADD_FREE in to_account failed! "
                     "to_uid=%s, symbol=%s, amount=%llu\n",
                     transfer.toUid.ToString(), transfer.tokenType, transfer.tokenAmount);
            ret = false; break;
        }

        if (!p_context->p_cw->accountCache.SetAccount(pFromAccount->keyid, *pFromAccount)) {
            LogPrint("vm",
                     "[ERR]CLuaVMRunEnv::TransferAccountAsset(), save from_account failed, from_uid=%s, "
                     "isContractAccount=%d\n",
                     fromUid.ToString(), (int)transfer.isContractAccount);
            ret = false; break;
        }

        if (!p_context->p_cw->accountCache.SetAccount(pToAccount->keyid, *pToAccount)) {
            LogPrint("vm",
                     "[ERR]CLuaVMRunEnv::TransferAccountAsset(), save to_account failed, to_uid=%s\n",
                     transfer.toUid.ToString(), (int)transfer.isContractAccount);
            ret = false; break;
        }

        receipts.emplace_back(fromUid, transfer.toUid, transfer.tokenType, transfer.tokenAmount, ReceiptCode::CONTRACT_ACCOUNT_TRANSFER_ASSET);

        if (isNewAccount) {
            LUA_BurnAccount(L, FUEL_ACCOUNT_NEW, BURN_VER_R2);

        } else {
            LUA_BurnAccount(L, FUEL_ACCOUNT_OPERATE, BURN_VER_R2);
        }
    }
    if (!ret) {
        LUA_BurnAccount(L, FUEL_ACCOUNT_GET_VALUE, BURN_VER_R2);
    }
    return ret;
}

const CRegID& CLuaVMRunEnv::GetContractRegID() {
    assert(!p_context->p_app_account->regid.IsEmpty());
    return p_context->p_app_account->regid;
}

const CRegID& CLuaVMRunEnv::GetTxUserRegid() {
    assert(!p_context->p_tx_user_account->regid.IsEmpty());
    return p_context->p_tx_user_account->regid;
}

uint64_t CLuaVMRunEnv::GetValue() const {
    return p_context->transfer_amount;
}

const string& CLuaVMRunEnv::GetTxContract() {
    return *p_context->p_arguments;
}

int32_t CLuaVMRunEnv::GetConfirmHeight() { return p_context->height; }

int32_t CLuaVMRunEnv::GetBurnVersion() {
    // the burn version belong to the Feature Fork Version
    return GetFeatureForkVersion(p_context->height);
}

uint256 CLuaVMRunEnv::GetCurTxHash() { return p_context->p_base_tx->GetHash(); }


CCacheWrapper* CLuaVMRunEnv::GetCw() {
    return p_context->p_cw;
}

CContractDBCache* CLuaVMRunEnv::GetScriptDB() { return &p_context->p_cw->contractCache; }

CAccountDBCache* CLuaVMRunEnv::GetCatchView() { return &p_context->p_cw->accountCache; }

void CLuaVMRunEnv::InsertOutAPPOperte(const vector<uint8_t>& userId,
                                   const CAppFundOperate& source) {
    if (mapAppFundOperate.count(userId)) {
        mapAppFundOperate[userId].push_back(source);
    } else {
        vector<CAppFundOperate> it;
        it.push_back(source);
        mapAppFundOperate[userId] = it;
    }
}


bool CLuaVMRunEnv::CheckOperateAccountLimit() {
    if (vmOperateOutput.size() + transfer_count > MAX_OUTPUT_COUNT) {
        LogPrint("vm", "[ERR]CLuaVMRunEnv::CheckOperateAccountLimit(), operate account count=%d excceed "
            "the max limit=%d\n", vmOperateOutput.size() + transfer_count, MAX_OUTPUT_COUNT);
        return false;
    }
    return true;
}

bool CLuaVMRunEnv::InsertOutputData(const vector<CVmOperate>& source) {
    vmOperateOutput.insert(vmOperateOutput.end(), source.begin(), source.end());
    if (!CheckOperateAccountLimit())
        return false;

    return true;
}

/**
 * 从脚本数据库中，取指定账户的应用账户信息, 同时解冻冻结金额到自由金额
 * @param vAppUserId   账户地址或regId
 * @param pAppUserAccount
 * @return
 */
bool CLuaVMRunEnv::GetAppUserAccount(const vector<uint8_t>& vAppUserId, shared_ptr<CAppUserAccount>& pAppUserAccount) {

    shared_ptr<CAppUserAccount> tem = std::make_shared<CAppUserAccount>();
    string appUserId(vAppUserId.begin(), vAppUserId.end());
    if (!p_context->p_cw->contractCache.GetContractAccount(GetContractRegID(), appUserId, *tem.get())) {
        tem             = std::make_shared<CAppUserAccount>(appUserId);
        pAppUserAccount = tem;

        return true;
    }

    if (!tem.get()->AutoMergeFreezeToFree(p_context->height)) {
        return false;
    }

    pAppUserAccount = tem;

    return true;
}

bool CLuaVMRunEnv::OperateAppAccount(const map<vector<uint8_t>, vector<CAppFundOperate>> opMap) {
    newAppUserAccount.clear();

    if (!mapAppFundOperate.empty()) {
        for (auto const tem : opMap) {
            shared_ptr<CAppUserAccount> pAppUserAccount;
            if (!GetAppUserAccount(tem.first, pAppUserAccount)) {
                LogPrint("vm", "GetAppUserAccount(tem.first, pAppUserAccount, true) failed \n appuserid :%s\n",
                         HexStr(tem.first));
                return false;
            }

            if (!pAppUserAccount.get()->AutoMergeFreezeToFree(p_context->height)) {
                LogPrint("vm", "AutoMergeFreezeToFree failed\nappuser :%s\n", pAppUserAccount.get()->ToString());
                return false;
            }

            shared_ptr<CAppUserAccount> vmAppAccount = GetAppAccount(pAppUserAccount);
            if (vmAppAccount.get() == nullptr) {
                rawAppUserAccount.push_back(pAppUserAccount);
            }

            LogPrint("vm", "before user: %s\n", pAppUserAccount.get()->ToString());

            vector<CReceipt> appOperateReceipts;
            if (!pAppUserAccount.get()->Operate(tem.second, appOperateReceipts)) {
                int32_t i = 0;
                for (auto const appFundOperate : tem.second) {
                    LogPrint("vm", "Operate failed\nOperate %d: %s\n", i++, appFundOperate.ToString());
                }
                LogPrint("vm", "GetAppUserAccount(tem.first, pAppUserAccount, true) failed\nappuserid: %s\n",
                         HexStr(tem.first));
                return false;
            }

            receipts.insert(receipts.end(), appOperateReceipts.begin(), appOperateReceipts.end());

            newAppUserAccount.push_back(pAppUserAccount);

            LogPrint("vm", "after user: %s\n", pAppUserAccount.get()->ToString());

            p_context->p_cw->contractCache.SetContractAccount(GetContractRegID(), *pAppUserAccount.get());
        }
    }

    return true;
}

void CLuaVMRunEnv::SetCheckAccount(bool bCheckAccount) { isCheckAccount = bCheckAccount; }
