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
#include "../rpc/rpcclient.h"
#include "tx.h"

using namespace std;
using namespace boost;
using namespace json_spirit;


typedef struct  {
	unsigned char Sender[6];						//!<转账人ID（采用6字节的账户ID）
	int64_t nPayMoney;								//!<转账的人支付的金额
	unsigned short len;             		        //!<接受钱账户信息长度
	IMPLEMENT_SERIALIZE
	(
			for(int i = 0;i < 6;i++)
			READWRITE(Sender[i]);
			READWRITE(nPayMoney);
			READWRITE(len);
	)
}CONTRACT;

typedef struct  {
	char  account[6];						    	//!<接受钱的ID（采用6字节的账户ID）
	int64_t nReciMoney;						    	//!<	收到钱的金额
	IMPLEMENT_SERIALIZE
	(
			for(int i = 0;i < 6;i++)
			READWRITE(account[i]);
			READWRITE(nReciMoney);
	)
}ACCOUNT_INFO;


class CAnonyTest: public CycleTestBase {
	int nNum;
	int nStep;
	string strTxHash;
	string strAppRegId;
	string regId;
public:
	CAnonyTest();
	~CAnonyTest(){};
	virtual TEST_STATE Run() ;
	bool RegistScript();
	bool CreateAnonyTx();
	void Initialize();
};

#endif /* CANONY_TESTS_H */
