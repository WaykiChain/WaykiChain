// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

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
