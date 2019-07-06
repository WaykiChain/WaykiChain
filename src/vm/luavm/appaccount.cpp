/*
 * appuseraccout.cpp
 *
 *  Created on: 2015年3月30日
 *      Author: ranger.shi
 */

#include "appaccount.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "commons/util.h"
#include "main.h"
#include "vm/luavm/vmrunenv.h"
#include "miner/miner.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "commons/SafeInt3.hpp"

#include <boost/foreach.hpp>
#include <algorithm>

using namespace json_spirit;


CAppCFund::CAppCFund() {
    vTag.clear();
    value = 0;
    timeoutHeight = 0;
}

CAppCFund::CAppCFund(const CAppCFund &fund) {
    vTag = fund.GetTag();
    value = fund.GetValue();
    timeoutHeight = fund.GetHeight();
}

CAppCFund::CAppCFund(const vector<unsigned char>& tag, uint64_t val, int height) {
    vTag = tag;
    value = val;
    timeoutHeight = height;
}

inline bool CAppCFund::MergeCFund(const CAppCFund &fund) {
    assert(fund.GetTag() == this->GetTag());
    assert(fund.GetHeight() == this->GetHeight() && fund.GetValue() > 0);
    //value = fund.GetValue()+value;
    uint64_t tempValue = 0;
    if (!SafeAdd(fund.GetValue(), value, tempValue))
        return ERRORMSG("Operate overflow !");

    value = tempValue;
    return true;
}


CAppCFund::CAppCFund(const CAppFundOperate& op) {
    //	assert(Op.opType == ADD_TAG_OP || ADD_TAG_OP == Op.opType);
    assert(op.timeoutHeight > 0);

    vTag = op.GetFundTagV();
    value = op.GetUint64Value();					//!< amount of money
    timeoutHeight = op.timeoutHeight;
}


CAppUserAccount::CAppUserAccount() {
    mAccUserID.clear();
    bcoins = 0;

    vFrozenFunds.clear();
}

CAppUserAccount::CAppUserAccount(const string &userId)
{
    mAccUserID.clear();
    mAccUserID = userId;
    bcoins = 0;

    vFrozenFunds.clear();
}

bool CAppUserAccount::GetAppCFund(CAppCFund& outFound, const vector<unsigned char>& vtag , int height)
{
    auto it = find_if( vFrozenFunds.begin(), vFrozenFunds.end(),
        [&](const CAppCFund& CfundIn) { return height == CfundIn.GetHeight() && CfundIn.GetTag() == vtag; } );

    if (it != vFrozenFunds.end()) {
        outFound = *it;
        return true;
    }

    return false;
}

bool CAppUserAccount::AddAppCFund(const CAppCFund& inFound) {
    //需要找到超时高度和tag 都相同的才可以合并
    auto it = find_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
        return CfundIn.GetTag()== inFound.GetTag() && CfundIn.GetHeight() == inFound.GetHeight() ;});

    if (it != vFrozenFunds.end()) { //如果找到了
        return it->MergeCFund(inFound);
        //return true;
    }
    //没有找到就加一个新的
    vFrozenFunds.insert(vFrozenFunds.end(),inFound);
    return true;
}

uint64_t CAppUserAccount::GetAllFreezedValues()
{
    uint64_t total = 0;
    for (auto &Fund : vFrozenFunds) {
        total += Fund.GetValue();
    }

    return total;
}

bool CAppUserAccount::AutoMergeFreezeToFree(int height)
{
    bool needRemove = false;
    for (auto &Fund : vFrozenFunds) {
        if (Fund.GetHeight() <= height) {
            //bcoins += Fund.getvalue();
            uint64_t tempValue = 0;
            if(!SafeAdd(bcoins, Fund.GetValue(), tempValue)) {
                printf("Operate overflow !\n");
                return ERRORMSG("Operate overflow !");
            }
            bcoins = tempValue;
            needRemove = true;
        }
    }

    if (needRemove) {
        vFrozenFunds.erase(remove_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
            return (CfundIn.GetHeight() <= height);}), vFrozenFunds.end());
    }

    return true;
}

bool CAppUserAccount::ChangeAppCFund(const CAppCFund& inFound) {
    //需要找到超时高度和tag 都相同的才可以合并
    assert(inFound.GetHeight() > 0);
    auto it = find_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
        return CfundIn.GetTag()== inFound.GetTag() && CfundIn.GetHeight() ==inFound.GetHeight() ;});
    if (it != vFrozenFunds.end()) { //如果找到了
        *it= inFound;
        return true;
    }
    return false;
}

bool CAppUserAccount::MinusAppCFund(const CAppCFund& inFound) {
    assert(inFound.GetHeight() > 0);
    auto it = find_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
        return CfundIn.GetTag()== inFound.GetTag() && CfundIn.GetHeight() ==inFound.GetHeight() ;});

    if (it != vFrozenFunds.end()) { //如果找到了
        if (it->GetValue() >= inFound.GetValue()) {
            if(it->GetValue() == inFound.GetValue()) {
                vFrozenFunds.erase(it);
                return true;
            }
            it->SetValue(it->GetValue()  - inFound.GetValue());
            return true;
        }
    }

    return false;
}

bool CAppUserAccount::MinusAppCFund(const vector<unsigned char> &vtag,uint64_t val,int nhight) {
    CAppCFund fund(vtag,val,nhight);
    return MinusAppCFund(fund);
}

bool CAppUserAccount::AddAppCFund(const vector<unsigned char>& vtag, uint64_t val, int nhight) {
    CAppCFund fund(vtag,val,nhight);
    return AddAppCFund(fund);
}

CAppUserAccount::~CAppUserAccount() {

}

bool CAppUserAccount::Operate(const vector<CAppFundOperate> &Op) {
    assert(Op.size() > 0);
    //LogPrint("acc","before:%s",toString());
    for (auto const op : Op) {
        if (!Operate(op)) {
            return false;
        }
    }
    //LogPrint("acc","after:%s",toString());
    return true;
}


bool CAppUserAccount::Operate(const CAppFundOperate& Op) {
    //LogPrint("acc","Op:%s",Op.toString());
    if (Op.opType == ADD_FREE_OP) {
        //bcoins += Op.GetUint64Value();
        uint64_t tempValue = 0;
        if (!SafeAdd(bcoins, Op.GetUint64Value(), tempValue))
            return ERRORMSG("Operate overflow !");

        bcoins = tempValue;
        return true;

    } else if (Op.opType == SUB_FREE_OP) {
        uint64_t tem = Op.GetUint64Value();
        if (bcoins >= tem) {
            bcoins -= tem;
            return true;
        }

    } else if (Op.opType == ADD_TAG_OP) {
        CAppCFund tep(Op);
        return AddAppCFund(tep);

    } else if (Op.opType == SUB_TAG_OP) {
        CAppCFund tep(Op);
        return MinusAppCFund(tep);

    } else {
        return ERRORMSG("CAppUserAccount operate type error!");
    }

    return false;
}

CAppFundOperate::CAppFundOperate() {
    fundTagLen = 0;
    appuserIDlen = 0;
    opType = 0;
    timeoutHeight = 0;
    mMoney = 0;
}

Object CAppCFund::ToJson()const {
    Object result;
    result.push_back(Pair("value", value));
    result.push_back(Pair("outHeight", timeoutHeight));
    result.push_back(Pair("vTag", HexStr(vTag)));
    return result;
}

string CAppCFund::ToString()const {
    return write_string(Value(ToJson()), true);
}

Object CAppUserAccount::ToJson() const {
    Object result;
    result.push_back(Pair("mAccUserID", HexStr(mAccUserID)));
    result.push_back(Pair("FreeValues", bcoins));

    Array arry;
    for (auto const te : vFrozenFunds) {
        arry.push_back(te.ToJson());
    }
    result.push_back(Pair("FrozenFunds", arry));

    return result;
}

string CAppUserAccount::ToString() const {
    return write_string(Value(ToJson()), true);
}

Object CAppFundOperate::ToJson() const {
    Object result;
    // int timout = outheight;
    string optypes[] = {"error type", "ADD_FREE_OP ","SUB_FREE_OP","ADD_TAG_OP","SUB_TAG_OP"};

    result.push_back(Pair("userid", HexStr(GetAppUserV())));
    result.push_back(Pair("vTag", HexStr(GetFundTagV())));
    result.push_back(Pair("opType", optypes[opType]));
    result.push_back(Pair("outHeight", (int) timeoutHeight));
    result.push_back(Pair("mMoney", mMoney));

    return result;
}

string CAppFundOperate::ToString() const {
    return write_string(Value(ToJson()), true);
}
