/*
 * CycleTestBase.h
 *
 *  Created on: 2014��12��30��
 *      Author: ranger.shi
 */

#ifndef CYCLETESTBASE_H_
#define CYCLETESTBASE_H_
#include "../test/systestbase.h"
enum TEST_STATE{
	this_state,
	next_state,
	end_state,
};

class CycleTestBase {
protected:
    SysTestBase basetest;
    static int totalsend;
    static vector<string> vAccount;

public:
    CycleTestBase();
    bool IncSentTotal() {
        basetest.ShowProgressTotal("Send Cycle:", ++totalsend);
        return true;
    }
    static bool SelectAccounts(vector<string> &vAccount);
    bool SelectOneAccount(string &selectAddr, bool flag = false);
    bool WaitComfirmed(string &strTxHash, string &regId);
    int Str2Int(string &strValue);
    virtual TEST_STATE Run();
    virtual ~CycleTestBase();
};

#endif /* CYCLETESTBASE_H_ */
