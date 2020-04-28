// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "luavmrunenv.h"
#include "commons/SafeInt3.hpp"
#include "tx/tx.h"
#include "commons/util/util.h"
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

bool CLuaVMRunEnv::ExecuteContract(CLuaVMContext *pContextIn, uint64_t& fuel, string &errMsg) {
    p_context = pContextIn;

    assert(p_context->p_arguments->size() <= MAX_CONTRACT_ARGUMENT_SIZE);
    assert(p_context->fuel_limit > 0);

    pLua = std::make_shared<CLuaVM>(p_context->p_contract->code, *p_context->p_arguments);

    LogPrint(BCLog::LUAVM, "prepare to execute tx. txid=%s, fuelLimit=%llu\n",
                            p_context->p_base_tx->GetHash().GetHex(), p_context->fuel_limit);

    tuple<uint64_t, string> ret = pLua.get()->Run(p_context->fuel_limit, this);

    int64_t fuelRet = std::get<0>(ret);
    if (0 == fuelRet) {
        errMsg = "VmScript run Failed";
        return false;
    } else if (-1 == fuelRet) {
        errMsg = std::get<1>(ret);
        return false;
    } else {
        fuel = fuelRet;
    }

    LogPrint(BCLog::LUAVM, "{%s} used_fuel=%ld\n", p_context->p_base_tx->ToString(p_context->p_cw->accountCache), fuel);

    if (!CheckOperate()) {
        errMsg = "VmScript CheckOperate Failed";
        return false;
    }

    if (!OperateAccount(vmOperateOutput)) {
        errMsg = "VmScript OperateAccount Failed";
        return false;
    }

    LogPrint(BCLog::LUAVM, "isCheckAccount: %d\n", isCheckAccount);

    // CheckAppAcctOperate only support to check when the transfer symbol is WICC
    if (isCheckAccount && p_context->transfer_symbol == SYMB::WICC && !CheckAppAcctOperate()) {
        errMsg = "VmScript CheckAppAcct Failed";
        return false;
    }

    if (!OperateAppAccount(mapAppFundOperate)) {
        errMsg = "OperateAppAccount Failed";
        return false;
    }

    return true;
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
            accountId = regid.GetRegIdRaw();
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
            assert(!p_context->sp_app_account->regid.IsEmpty());
            if (regId != p_context->sp_app_account->regid) {
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
        if (vAccountId == p_context->sp_app_account->regid.GetRegIdRaw() &&
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
        LogPrint(BCLog::LUAVM,
                 "CheckAppAcctOperate: addValue=%lld, minusValue=%lld, txValue[%s]=%lld, "
                 "sysContractAcct=%lld, sumValue=%lld, sysAcctSum=%lld\n",
                 addValue, minusValue, SYMB::WICC, p_context->transfer_amount, sysContractAcct, sumValue, sysAcctSum);

        return false;
    }

    return true;
}

bool CLuaVMRunEnv::OperateAccount(const vector<CVmOperate>& operates) {

    FeatureForkVersionEnum version = GetFeatureForkVersion(p_context->height);
    for (auto& operate : operates) {
        uint64_t value;
        memcpy(&value, operate.money, sizeof(operate.money));

        UnsignedCharArray accountId = GetAccountID(operate);
        CUserID uid;

        if (accountId.size() == 6) {
            uid = CRegID(accountId);
        } else {
            uid = CKeyID(string(accountId.begin(), accountId.end()));
        }

        shared_ptr<CAccount> spAccount = nullptr;
        if (uid.IsEmpty()) {
            if (version <= MAJOR_VER_R2 && accountId.size() != 6) {
                LogPrint(BCLog::LUAVM, "[WARNING] the keyid of vm_operate is empty! "
                        "accountId=%s, height=%u, txid=%s\n", HexStr(accountId),
                        p_context->height, p_context->p_base_tx->GetHash().ToString());
                // ignore the empty keyid and get an empty account to compatible for old data
                // the empty account will not be saved,
                // but the operate receipt will be saved
                spAccount = make_shared<CAccount>();
            } else {
                LogPrint(BCLog::LUAVM, "[ERR] the uid is empty! accountId=%s\n", HexStr(accountId));
                return false;
            }
        } else {
            spAccount = GetAccount(uid);
            if (!spAccount) {
                if (!uid.is<CKeyID>()) {
                    LogPrint(BCLog::LUAVM, "[ERR]%s(), get to_account failed! to_uid=%s\n", __func__,
                            uid.ToDebugString());
                    return false;
                }

                // create new user
                spAccount = p_context->p_base_tx->NewAccount(*p_context->p_cw, uid.get<CKeyID>());
                LogPrint(BCLog::LUAVM, "%s(), create new user! to_uid=%s\n", __func__,
                        uid.ToDebugString());
            }
        }


        LogPrint(BCLog::LUAVM, "uid=%s\nbefore account: %s\n", uid.ToString(),
                spAccount->ToString());

        ReceiptType code = (operate.opType == BalanceOpType::ADD_FREE) ? ReceiptType::CONTRACT_ACCOUNT_OPERATE_ADD :
                            ReceiptType::CONTRACT_ACCOUNT_OPERATE_SUB;

        if (!spAccount->OperateBalance(SYMB::WICC, operate.opType, value, code, receipts)) {
            LogPrint(BCLog::LUAVM, "[ERR]%s(), operate account failed! uid=%s, operate=%s\n",
                __func__, uid.ToDebugString(), GetBalanceOpTypeName(operate.opType));
            return false;
        }
        LogPrint(BCLog::LUAVM, "after account:%s\n", spAccount->ToString());
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
        shared_ptr<CAccount> spFromAccount =
            transfer.isContractAccount ? p_context->sp_app_account : p_context->sp_tx_account;

        auto &toUid = transfer.toUid;
        shared_ptr<CAccount> spToAccount = GetAccount(toUid);
        if (!spToAccount) {
            if (!toUid.is<CKeyID>()) {
                LogPrint(BCLog::LUAVM, "[ERR] get to_uid=%s's account failed! \n", transfer.toUid.ToDebugString());
                ret = false;
                break;
            }
            // create new user
            spToAccount = p_context->p_base_tx->NewAccount(*p_context->p_cw, toUid.get<CKeyID>());
            isNewAccount = true;
            LogPrint(BCLog::LUAVM, "create new user! to_uid=%s\n", transfer.toUid.ToDebugString());
        }

        if (!spFromAccount->OperateBalance(transfer.tokenType, SUB_FREE, transfer.tokenAmount,
                                        ReceiptType::CONTRACT_ACCOUNT_TRANSFER_ASSET, receipts, spToAccount.get())) {
            LogPrint(BCLog::LUAVM, "[ERR]CLuaVMRunEnv::TransferAccountAsset(), operate SUB_FREE in from_account failed! "
                "from_regid=%s, isContractAccount=%d, symbol=%s, amount=%llu\n",
                spFromAccount->regid.ToString(), (int)transfer.isContractAccount, transfer.tokenType,
                transfer.tokenAmount);
            ret = false;
            break;
        }

        if (spToAccount && !p_context->p_cw->accountCache.SetAccount(spToAccount->keyid, *spToAccount)) {
            LogPrint(BCLog::LUAVM,
                     "[ERR]CLuaVMRunEnv::TransferAccountAsset(), save to_account failed, to_uid=%s\n",
                     transfer.toUid.ToString(), (int)transfer.isContractAccount);
            ret = false;
            break;
        }

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
    assert(!p_context->sp_app_account->regid.IsEmpty());
    return p_context->sp_app_account->regid;
}

const CRegID& CLuaVMRunEnv::GetTxAccountRegId() {
    assert(!p_context->sp_tx_account->regid.IsEmpty());
    return p_context->sp_tx_account->regid;
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

CAccountDBCache* CLuaVMRunEnv::GetAccountCache() { return &p_context->p_cw->accountCache; }


shared_ptr<CAccount> CLuaVMRunEnv::GetAccount(const CUserID &uid) {
    if (p_context->sp_app_account->IsSelfUid(uid)) {
        return p_context->sp_app_account;
    } else if (p_context->sp_tx_account->IsSelfUid(uid)) {
        return p_context->sp_tx_account;
    } else {
        return p_context->p_base_tx->GetAccount(*p_context->p_cw, uid);
    }
}

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
        LogPrint(BCLog::LUAVM, "[ERR]CLuaVMRunEnv::CheckOperateAccountLimit(), operate account count=%d excceed "
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
                LogPrint(BCLog::LUAVM, "GetAppUserAccount(tem.first, pAppUserAccount, true) failed \n appuserid :%s\n",
                         HexStr(tem.first));
                return false;
            }

            if (!pAppUserAccount.get()->AutoMergeFreezeToFree(p_context->height)) {
                LogPrint(BCLog::LUAVM, "AutoMergeFreezeToFree failed\nappuser :%s\n", pAppUserAccount.get()->ToString());
                return false;
            }

            shared_ptr<CAppUserAccount> vmAppAccount = GetAppAccount(pAppUserAccount);
            if (vmAppAccount.get() == nullptr) {
                rawAppUserAccount.push_back(pAppUserAccount);
            }

            LogPrint(BCLog::LUAVM, "before user: %s\n", pAppUserAccount.get()->ToString());

            vector<CReceipt> appOperateReceipts;
            if (!pAppUserAccount.get()->Operate(tem.second, appOperateReceipts)) {
                int32_t i = 0;
                for (auto const appFundOperate : tem.second) {
                    LogPrint(BCLog::LUAVM, "Operate failed\nOperate %d: %s\n", i++, appFundOperate.ToString());
                }
                LogPrint(BCLog::LUAVM, "GetAppUserAccount(tem.first, pAppUserAccount, true) failed\nappuserid: %s\n",
                         HexStr(tem.first));
                return false;
            }

            receipts.insert(receipts.end(), appOperateReceipts.begin(), appOperateReceipts.end());

            newAppUserAccount.push_back(pAppUserAccount);

            LogPrint(BCLog::LUAVM, "after user: %s\n", pAppUserAccount.get()->ToString());

            p_context->p_cw->contractCache.SetContractAccount(GetContractRegID(), *pAppUserAccount.get());
        }
    }

    return true;
}

void CLuaVMRunEnv::SetCheckAccount(bool bCheckAccount) { isCheckAccount = bCheckAccount; }
