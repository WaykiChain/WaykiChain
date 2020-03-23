/*
 * CreateTx_tests.h
 *
 *  Created on: 2015-04-22
 *      Author: frank
 */

#ifndef CREATETX_TESTS_H_
#define CREATETX_TESTS_H_
#include "CycleTestBase.h"
#include "../test/systestbase.h"
#include "../rpc/core/rpcclient.h"
#include "tx/tx.h"
#include <vector>
using namespace std;

class CCreateTxTest : public CycleTestBase{
public:
	CCreateTxTest();
	 ~CCreateTxTest(){};
	 bool  CreateTx(int nTxType);
	 void Initialize();
	 TEST_STATE Run();
private:
	 static  int nCount ;
	 int nTxType;
	 int nNum;
	 int nStep ;
	 string sendhash;
	 string newAddr;

};
#endif /* CDARKANDANONY_H_ */
