// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "appaccount.h"

#include "commons/SafeInt3.hpp"
#include "commons/serialize.h"
#include "commons/util.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "vm/luavm/luavmrunenv.h"

#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_writer_template.h"

#include <algorithm>
#include <boost/foreach.hpp>

using namespace json_spirit;

CAppCFund::CAppCFund() {
    vTag.clear();
    value         = 0;
    timeoutHeight = 0;
}

CAppCFund::CAppCFund(const CAppCFund& fund) {
    vTag          = fund.GetTag();
    value         = fund.GetValue();
    timeoutHeight = fund.GetHeight();
}

CAppCFund::CAppCFund(const vector<uint8_t>& tag, uint64_t val, int32_t height) {
    vTag          = tag;
    value         = val;
    timeoutHeight = height;
}

inline bool CAppCFund::MergeCFund(const CAppCFund& fund) {
    assert(fund.GetTag() == this->GetTag());
    assert(fund.GetHeight() == this->GetHeight() && fund.GetValue() > 0);

    uint64_t tempValue = 0;
    if (!SafeAdd(fund.GetValue(), value, tempValue)) return ERRORMSG("Operate overflow !");

    value = tempValue;

    return true;
}

CAppCFund::CAppCFund(const CAppFundOperate& operate) {
    // assert(operate.opType == ADD_TAG_OP || ADD_TAG_OP == operate.opType);
    assert(operate.timeoutHeight > 0);

    vTag          = operate.GetFundTagV();
    value         = operate.GetUint64Value();  //!< amount of money
    timeoutHeight = operate.timeoutHeight;
}

CAppUserAccount::CAppUserAccount() {
    user_id.clear();
    bcoins = 0;
    frozen_funds.clear();
}

CAppUserAccount::CAppUserAccount(const string& userId) {
    user_id = userId;
    bcoins  = 0;
    frozen_funds.clear();
}

CAppUserAccount::~CAppUserAccount() {}

bool CAppUserAccount::GetAppCFund(CAppCFund& fundOut, const vector<uint8_t>& tag, int32_t height) {
    auto it = find_if(frozen_funds.begin(), frozen_funds.end(),
                      [&](const CAppCFund& fundIn) { return height == fundIn.GetHeight() && fundIn.GetTag() == tag; });

    if (it != frozen_funds.end()) {
        fundOut = *it;
        return true;
    }

    return false;
}

bool CAppUserAccount::AddAppCFund(const CAppCFund& appFund) {
    //需要找到超时高度和tag 都相同的才可以合并
    auto it = find_if(frozen_funds.begin(), frozen_funds.end(), [&](const CAppCFund& fundIn) {
        return fundIn.GetTag() == appFund.GetTag() && fundIn.GetHeight() == appFund.GetHeight();
    });

    if (it != frozen_funds.end()) {  //如果找到了
        return it->MergeCFund(appFund);
        // return true;
    }
    //没有找到就加一个新的
    frozen_funds.insert(frozen_funds.end(), appFund);
    return true;
}

uint64_t CAppUserAccount::GetAllFreezedValues() {
    uint64_t total = 0;
    for (auto& fund : frozen_funds) {
        total += fund.GetValue();
    }

    return total;
}

bool CAppUserAccount::AutoMergeFreezeToFree(int32_t height) {
    bool needRemove = false;
    for (auto& fund : frozen_funds) {
        if (fund.GetHeight() <= height) {
            // bcoins += fund.getvalue();
            uint64_t tempValue = 0;
            if (!SafeAdd(bcoins, fund.GetValue(), tempValue)) {
                return ERRORMSG("Operate overflow !");
            }
            bcoins     = tempValue;
            needRemove = true;
        }
    }

    if (needRemove) {
        frozen_funds.erase(remove_if(frozen_funds.begin(), frozen_funds.end(),
                                     [&](const CAppCFund& fundIn) { return (fundIn.GetHeight() <= height); }),
                           frozen_funds.end());
    }

    return true;
}

bool CAppUserAccount::ChangeAppCFund(const CAppCFund& appFund) {
    //需要找到超时高度和tag 都相同的才可以合并
    assert(appFund.GetHeight() > 0);
    auto it = find_if(frozen_funds.begin(), frozen_funds.end(), [&](const CAppCFund& fundIn) {
        return fundIn.GetTag() == appFund.GetTag() && fundIn.GetHeight() == appFund.GetHeight();
    });
    if (it != frozen_funds.end()) {  //如果找到了
        *it = appFund;
        return true;
    }
    return false;
}

bool CAppUserAccount::MinusAppCFund(const CAppCFund& appFund) {
    assert(appFund.GetHeight() > 0);
    auto it = find_if(frozen_funds.begin(), frozen_funds.end(), [&](const CAppCFund& fundIn) {
        return fundIn.GetTag() == appFund.GetTag() && fundIn.GetHeight() == appFund.GetHeight();
    });

    if (it != frozen_funds.end()) {  //如果找到了
        if (it->GetValue() >= appFund.GetValue()) {
            if (it->GetValue() == appFund.GetValue()) {
                frozen_funds.erase(it);
                return true;
            }
            it->SetValue(it->GetValue() - appFund.GetValue());
            return true;
        }
    }

    return false;
}

bool CAppUserAccount::MinusAppCFund(const vector<uint8_t>& tag, uint64_t val, int32_t height) {
    CAppCFund fund(tag, val, height);
    return MinusAppCFund(fund);
}

bool CAppUserAccount::AddAppCFund(const vector<uint8_t>& tag, uint64_t val, int32_t height) {
    CAppCFund fund(tag, val, height);
    return AddAppCFund(fund);
}

bool CAppUserAccount::Operate(const vector<CAppFundOperate>& operate, vector<CReceipt> &receipts) {
    assert(operate.size() > 0);
    // LogPrint("acc","before:%s",toString());
    for (auto const op : operate) {
        if (!Operate(op, receipts)) {
            return false;
        }
    }
    // LogPrint("acc","after:%s",toString());
    return true;
}

bool CAppUserAccount::Operate(const CAppFundOperate& operate, vector<CReceipt> &receipts) {
    // LogPrint("acc","operate:%s", operate.toString());
    if (operate.opType == ADD_FREE_OP) {
        // bcoins += operate.GetUint64Value();
        uint64_t tempValue = 0;
        if (!SafeAdd(bcoins, operate.GetUint64Value(), tempValue))
            return ERRORMSG("Operate overflow!");

        bcoins = tempValue;

        receipts.emplace_back(nullId, operate.GetUserID(), "", operate.GetUint64Value(), ReceiptCode::CONTRACT_TOKEN_OPERATE_ADD);

        return true;
    } else if (operate.opType == SUB_FREE_OP) {
        uint64_t tem = operate.GetUint64Value();
        if (bcoins >= tem) {
            bcoins -= tem;

            receipts.emplace_back(operate.GetUserID(), nullId, "", operate.GetUint64Value(), ReceiptCode::CONTRACT_TOKEN_OPERATE_SUB);

            return true;
        }
    } else if (operate.opType == ADD_TAG_OP) {
        receipts.emplace_back(nullId, operate.GetUserID(), operate.GetFundTag(), operate.GetUint64Value(), ReceiptCode::CONTRACT_TOKEN_OPERATE_TAG_ADD);

        CAppCFund tep(operate);
        return AddAppCFund(tep);
    } else if (operate.opType == SUB_TAG_OP) {
        receipts.emplace_back(operate.GetUserID(), nullId, operate.GetFundTag(), operate.GetUint64Value(), ReceiptCode::CONTRACT_TOKEN_OPERATE_TAG_SUB);

        CAppCFund tep(operate);
        return MinusAppCFund(tep);
    } else {
        return ERRORMSG("CAppUserAccount operate type error!");
    }

    return false;
}

CAppFundOperate::CAppFundOperate() {
    fundTagLen    = 0;
    appuserIDlen  = 0;
    opType        = 0;
    timeoutHeight = 0;
    mMoney        = 0;
}

Object CAppCFund::ToJson() const {
    Object result;
    result.push_back(Pair("value",          value));
    result.push_back(Pair("timeout_height", timeoutHeight));
    result.push_back(Pair("tag",            HexStr(vTag)));

    return result;
}

string CAppCFund::ToString() const { return write_string(Value(ToJson()), true); }

Object CAppUserAccount::ToJson() const {
    Object result;
    result.push_back(Pair("account_uid",    HexStr(user_id)));
    result.push_back(Pair("free_value",     bcoins));

    Array array;
    for (auto const te : frozen_funds) {
        array.push_back(te.ToJson());
    }
    result.push_back(Pair("frozen_funds",   array));

    return result;
}

string CAppUserAccount::ToString() const { return write_string(Value(ToJson()), true); }

Object CAppFundOperate::ToJson() const {
    Object result;

    static string opTypes[] = {
        "NULL_OP",
        "ADD_FREE_OP",
        "SUB_FREE_OP",
        "ADD_TAG_OP",
        "SUB_TAG_OP"
    };

    result.push_back(Pair("userid",     HexStr(GetAppUserV())));
    result.push_back(Pair("vTag",       HexStr(GetFundTagV())));
    result.push_back(Pair("opType",     opTypes[opType]));
    result.push_back(Pair("outHeight",  (int32_t)timeoutHeight));
    result.push_back(Pair("mMoney",     mMoney));

    return result;
}

string CAppFundOperate::ToString() const { return write_string(Value(ToJson()), true); }
