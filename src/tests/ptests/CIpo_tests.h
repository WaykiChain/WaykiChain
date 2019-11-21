/*
 * CBlackHalo_tests.h
 *
 *  Created on: 2015-04-24
 *      Author: frank.shi
 */

#ifndef CANONY_TESTS_H
#define CANONY_TESTS_H

#include "CycleTestBase.h"
#include "../test/systestbase.h"
#include "../rpc/core/rpcclient.h"
#include "tx/tx.h"

using namespace std;
using namespace boost;
using namespace json_spirit;

//#define SEND_A    "DmtzzT99HYUGAV6ejkWTWXF8pcYXtkpU4g"
//#define SEND_A    "DmtzzT99HYUGAV6ejkWTWXF8pcYXtkpU4g"
#define SEND_A    "hSxQahzj65tLhvXDNKAGBbukUgeeiRJiiH"  // 0-20  100001392001815000,
                                                        //       100001391601814999

class CIpoTest: public CycleTestBase {
	int nNum;
	int nStep;
	string strTxHash;
	string strAppRegId;//注册应用后的Id
public:
	CIpoTest();
	~CIpoTest(){};
	virtual TEST_STATE Run() ;
	bool RegistScript();
	bool CreateIpoTx(string contact,int64_t llSendTotal);
	bool SendIpoTx(unsigned char type);
	void RunIpo(unsigned char type);
	void SendErrorIopTx();

};

#endif /* CANONY_TESTS_H */
