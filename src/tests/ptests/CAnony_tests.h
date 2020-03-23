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


typedef struct  {
	unsigned char Sender[6];						//!<ת����ID������6�ֽڵ��˻�ID��
	int64_t nPayMoney;								//!<ת�˵���֧���Ľ��
	unsigned short len;             		        //!<����Ǯ�˻���Ϣ����
	IMPLEMENT_SERIALIZE
	(
			for(int i = 0;i < 6;i++)
			READWRITE(Sender[i]);
			READWRITE(nPayMoney);
			READWRITE(len);
	)
}CONTRACT;

typedef struct  {
	char  account[6];						    	//!<����Ǯ��ID������6�ֽڵ��˻�ID��
	int64_t nReciMoney;						    	//!<	�յ�Ǯ�Ľ��
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
	bool RegistContract();
	bool CreateAnonyTx();
	void Initialize();
};

#endif /* CANONY_TESTS_H */
