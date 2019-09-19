// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CREATEMINTERKEY_TESTS_H
#define CREATEMINTERKEY_TESTS_H


class CCreateMinerkey :public SysTestBase{
public:
	void CreateAccount();
	bool SelectAccounts();
	string GetOneAccount();
	CCreateMinerkey(){};
	virtual ~CCreateMinerkey();
private:
	vector<string> vAccount;
	map<string, uint64_t> mapSendValue;

};

#endif /* CREATEMINTERKEY_TESTS_H */
