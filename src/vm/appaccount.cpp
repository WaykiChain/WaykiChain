/*
 * appuseraccout.cpp
 *
 *  Created on: 2015年3月30日
 *      Author: ranger.shi
 */

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
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"
#include "appaccount.h"
#include "SafeInt3.hpp"
using namespace json_spirit;


CAppCFund::CAppCFund() {
	vTag.clear();
	value = 0;
	nHeight = 0;
}

CAppCFund::CAppCFund(const CAppCFund &fund) {
	vTag = fund.GetTag();
	value = fund.getvalue();
	nHeight = fund.getheight();
}

CAppCFund::CAppCFund(const vector<unsigned char>& vtag, uint64_t val, int nhight) {
	vTag = vtag;
	value = val;
	nHeight = nhight;
}

inline bool CAppCFund::MergeCFund(const CAppCFund &fund) {
	assert(fund.GetTag() == this->GetTag());
	assert(fund.getheight() == this->getheight() && fund.getvalue() > 0);
	//value = fund.getvalue()+value;
	uint64_t tempValue = 0;
	if(!SafeAdd(fund.getvalue(), value, tempValue)) {
		return ERRORMSG("Operate overflow !");
	}
	value = tempValue;
	return true;
}



CAppCFund::CAppCFund(const CAppFundOperate& Op) {
	//	assert(Op.opeatortype == ADD_TAG_OP || ADD_TAG_OP == Op.opeatortype);
	assert(Op.outheight > 0);
	vTag = Op.GetFundTagV();
	value = Op.GetUint64Value();					//!< amount of money
	nHeight = Op.outheight;
}


CAppUserAccout::CAppUserAccout() {
	mAccUserID.clear();
	llValues = 0;

	vFrozenFunds.clear();
}
CAppUserAccout::CAppUserAccout(const vector<unsigned char> &userId)
{
	mAccUserID.clear();
	mAccUserID = userId;
	llValues = 0;

	vFrozenFunds.clear();
}
bool CAppUserAccout::GetAppCFund(CAppCFund& outFound, const vector<unsigned char>& vtag , int hight) {

	auto it = find_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
		return hight ==CfundIn.getheight() && CfundIn.GetTag()== vtag  ;});
	if (it != vFrozenFunds.end()) {
		outFound = *it;
		return true;
	}
	return false;
}

bool CAppUserAccout::AddAppCFund(const CAppCFund& inFound) {
	//需要找到超时高度和tag 都相同的才可以合并
	auto it = find_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
		return CfundIn.GetTag()== inFound.GetTag() && CfundIn.getheight() ==inFound.getheight() ;});
	if (it != vFrozenFunds.end()) { //如果找到了
		return it->MergeCFund(inFound);
		//return true;
	}
	//没有找到就加一个新的
	vFrozenFunds.insert(vFrozenFunds.end(),inFound);
	return true;
}

uint64_t CAppUserAccout::GetAllFreezedValues()
{
	uint64_t total = 0;
	for (auto &Fund : vFrozenFunds) {
		total += Fund.getvalue();
	}

	return total;
}

bool CAppUserAccout::AutoMergeFreezeToFree(int hight) {

	bool isneedremvoe = false;
	for (auto &Fund : vFrozenFunds) {
		if (Fund.getheight() <= hight) {
			//llValues += Fund.getvalue();
			uint64_t tempValue = 0;
			if(!SafeAdd(llValues, Fund.getvalue(), tempValue)) {
				printf("Operate overflow !\n");
				return ERRORMSG("Operate overflow !");
			}
			llValues = tempValue;
			isneedremvoe = true;
		}
	}
	if (isneedremvoe) {
		vFrozenFunds.erase(remove_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
			return (CfundIn.getheight() <= hight);}), vFrozenFunds.end());
	}
	return true;

}

bool CAppUserAccout::ChangeAppCFund(const CAppCFund& inFound) {
	//需要找到超时高度和tag 都相同的才可以合并
	assert(inFound.getheight() > 0);
	auto it = find_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
		return CfundIn.GetTag()== inFound.GetTag() && CfundIn.getheight() ==inFound.getheight() ;});
	if (it != vFrozenFunds.end()) { //如果找到了
		*it= inFound;
		return true;
	}
	return false;
}

bool CAppUserAccout::MinusAppCFund(const CAppCFund& inFound) {
	assert(inFound.getheight() > 0);
	auto it = find_if(vFrozenFunds.begin(), vFrozenFunds.end(), [&](const CAppCFund& CfundIn) {
		return CfundIn.GetTag()== inFound.GetTag() && CfundIn.getheight() ==inFound.getheight() ;});

	if (it != vFrozenFunds.end()) { //如果找到了
		if (it->getvalue() >= inFound.getvalue()) {
			if(it->getvalue() == inFound.getvalue()) {
				vFrozenFunds.erase(it);
				return true;
			}
			it->setValue(it->getvalue()  - inFound.getvalue());
			return true;
		}
	}
	
	return false;
}

bool CAppUserAccout::MinusAppCFund(const vector<unsigned char> &vtag,uint64_t val,int nhight) {
	CAppCFund fund(vtag,val,nhight);
	return MinusAppCFund(fund);
}

bool CAppUserAccout::AddAppCFund(const vector<unsigned char>& vtag, uint64_t val, int nhight) {
	CAppCFund fund(vtag,val,nhight);
	return AddAppCFund(fund);
}

CAppUserAccout::~CAppUserAccout() {

}
bool CAppUserAccout::Operate(const vector<CAppFundOperate> &Op) {
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



bool CAppUserAccout::Operate(const CAppFundOperate& Op) {
	//LogPrint("acc","Op:%s",Op.toString());
	if (Op.opeatortype == ADD_FREE_OP) {
		//llValues += Op.GetUint64Value();
		uint64_t tempValue = 0;
		if(!SafeAdd(llValues, Op.GetUint64Value(), tempValue)) {
			return ERRORMSG("Operate overflow !");
		}
		llValues = tempValue;
		return true;
	} else if (Op.opeatortype == SUB_FREE_OP) {
		uint64_t tem = Op.GetUint64Value();
		if (llValues >= tem) {
			llValues -= tem;
			return true;
		}
	} else if (Op.opeatortype == ADD_TAG_OP) {
		CAppCFund tep(Op);
		return AddAppCFund(tep);
	} else if (Op.opeatortype == SUB_TAG_OP) {
		CAppCFund tep(Op);
		return MinusAppCFund(tep);
	} else {
		return ERRORMSG("CAppUserAccout operate type error!");
//		assert(0);
	}
	return false;
}

CAppFundOperate::CAppFundOperate() {
	FundTaglen = 0;
	appuserIDlen = 0;
	opeatortype = 0;
	outheight = 0;
	mMoney = 0;
}

Object CAppCFund::toJSON()const {
	Object result;
	result.push_back(Pair("value", value));
	result.push_back(Pair("nHeight", nHeight));
	result.push_back(Pair("vTag", HexStr(vTag)));
	return std::move(result);
}

string CAppCFund::toString()const {
	return write_string(Value(toJSON()), true);
}

Object CAppUserAccout::toJSON() const {
	Object result;
	result.push_back(Pair("mAccUserID", HexStr(mAccUserID)));
	result.push_back(Pair("FreeValues", llValues));
	
	Array arry;
	for (auto const te : vFrozenFunds) {
		arry.push_back(te.toJSON());
	}
	result.push_back(Pair("FrozenFunds", arry));
	
	return std::move(result);
}

string CAppUserAccout::toString() const {
	return write_string(Value(toJSON()), true);
}

Object CAppFundOperate::toJSON() const {
	Object result;
	int timout = outheight;
	string tep[] ={"error type","ADD_FREE_OP ","SUB_FREE_OP","ADD_TAG_OP","SUB_TAG_OP"};
	result.push_back(Pair("userid", HexStr(GetAppUserV())));
	result.push_back(Pair("vTag", HexStr(GetFundTagV())));
	result.push_back(Pair("opeatortype", tep[opeatortype]));
	result.push_back(Pair("outheight", timout));
//	result.push_back(Pair("outheight", outheight));
	result.push_back(Pair("mMoney", mMoney));
	return std::move(result);

}

string CAppFundOperate::toString() const {
	return write_string(Value(toJSON()), true);
}


