/*
 * createminterkey_tests.h
 *
 *  Created on: 2015��4��13��
 *      Author: ranger.shi
 */

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
