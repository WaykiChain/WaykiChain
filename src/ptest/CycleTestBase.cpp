/*
 * CycleTestBase.cpp
 *
 *  Created on: 2014Äê12ÔÂ30ÈÕ
 *      Author: ranger.shi
 */

#include "CycleTestBase.h"
#include <ctype.h>

int CycleTestBase::totalsend = 0;
vector<string> CycleTestBase::vAccount;

CycleTestBase::CycleTestBase() {

}

TEST_STATE CycleTestBase::Run() {
	return end_state;
}

CycleTestBase::~CycleTestBase() {

}


bool CycleTestBase::SelectAccounts(vector<string> &vAccounts) {
	const char *argv[] = { "rpctest", "listaddr"};
		int argc = sizeof(argv) / sizeof(char*);

		Value value;
		if (SysTestBase::CommandLineRPC_GetValue(argc, argv, value)) {
			if (value.type() == null_type) {
				return false;
			}
			for(auto & item : value.get_array()) {
				const Value& balance = find_value(item.get_obj(), "balance");
				if(balance.get_real() > 5000.0) {
					const Value& regId = find_value(item.get_obj(), "regid");
					if("" != regId.get_str())
						vAccounts.push_back(regId.get_str());
				}
			}
		}
	return !vAccounts.empty();
}

bool CycleTestBase::SelectOneAccount(string &selectAddr, bool flag) {
	srand(time(NULL));
	if(vAccount.empty() || (flag && vAccount.size() < 2))
	{
		if(!SelectAccounts(vAccount))
			return false;
	}
	int r = (rand() % vAccount.size());
	if(flag) {
		string strAddr("");
		while(true){
		strAddr = vAccount.at(r);
		if(selectAddr != strAddr)
			break;
		r = (rand() % vAccount.size());
		}
		selectAddr = strAddr;
	}else {
		selectAddr = vAccount.at(r);
	}
	return true;
}

bool CycleTestBase::WaitComfirmed(string &strTxHash, string &regId){
	if (basetest.GetTxConfirmedRegID(strTxHash, regId)) {
			return true;
	}
	return false;
}

int CycleTestBase::Str2Int(string &strValue) {
	if("" == strValue) {
		return 0;
	}
	int iRet(0);
	int len = strValue.length();
	for(int i=0; i<len; ++i) {
		if(!isdigit(strValue[i])){
			return 0;
		}
		iRet += (strValue[i]-'0') * pow(10, len-1-i);
	}
	return iRet;
}
