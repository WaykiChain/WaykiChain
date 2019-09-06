/*
 * CycleP2PBet_test.h
 *
 *  Created on: 2015��1��15��
 *      Author: spark.huang
 */

#ifndef CYCLEP2PBET_TEST_H_
#define CYCLEP2PBET_TEST_H_

#include "../test/systestbase.h"
#include "CycleTestBase.h"

#pragma pack(push)
#pragma pack(1)

typedef struct {
	unsigned char type;  /*TX_SENDBET = 0x01,TX_ACCEPTBET = 0x02,TX_OPENBET = 0x03*/
	unsigned char noperateType;   //��������ֵ0 -1
	uint64_t money;
	unsigned short hight;  //��ʱ�߶� 20
	unsigned char dhash[32];   	IMPLEMENT_SERIALIZE
	(
			READWRITE(type);
			READWRITE(noperateType);
			READWRITE(money);
			READWRITE(hight);
			for(int i = 0; i < 32; i++)
			{
				READWRITE(dhash[i]);
			}
	)
} SEND_DATA;

typedef struct {
	unsigned char type;
	unsigned char noperateType;
	uint64_t money;
	unsigned char data;
	unsigned char txid[32];		//����ԶĵĹ�ϣ��Ҳ�ǶԶ����ݵĹؼ���
	IMPLEMENT_SERIALIZE
	(
			READWRITE(type);
			READWRITE(noperateType);
			READWRITE(money);
			READWRITE(data);
			for(int i = 0; i < 32; i++)
			{
				READWRITE(txid[i]);
			}
	)
} ACCEPT_DATA;

typedef struct {
	unsigned char type;
	unsigned char noperateType;
	unsigned char txid[32];		//����ԶĵĹ�ϣ��Ҳ�ǶԶ����ݵĹؼ���
	unsigned char dhash[33];IMPLEMENT_SERIALIZE
	(
			READWRITE(type);
			READWRITE(noperateType);
			for(int i = 0; i < 32; i++)
			{
				READWRITE(txid[i]);
			}
			for(int ii = 0; ii < 33; ii++)
			{
				READWRITE(dhash[ii]);
			}
	)
} OPEN_DATA;

#pragma pack(pop)

#define ADDR_A    "doym966kgNUKr2M9P7CmjJeZdddqvoU5RZ"   // 0-6
#define VADDR_A   "[\"doym966kgNUKr2M9P7CmjJeZdddqvoU5RZ\"]"
#define ADDR_B    "dd936HZcwj9dQkefHPqZpxzUuKZZ2QEsbN"
#define VADDR_B   "[\"dd936HZcwj9dQkefHPqZpxzUuKZZ2QEsbN\"]"  //0-7


class CTestBetTx:public CycleTestBase,public SysTestBase
{
	bool RegScript(void);
	bool WaiteRegScript(void);
	bool ASendP2PBet(void);
	bool WaitASendP2PBet(void);
	bool BAcceptP2PBet(void);
	bool WaitBAcceptP2PBet(void);
	bool AOpenP2PBet(void);
	bool WaitAOpenP2PBet(void);
public:
	CTestBetTx();
	virtual TEST_STATE Run();
	~CTestBetTx();
	uint64_t GetRandomBetAmount() {
		srand(time(NULL));
		int r = (rand() % 1000000) + 500000;
		return r;
	}

	bool GetRandomData(unsigned char *buf, int num)
	{
		RAND_bytes(buf, num);
		return true;
	}
	int GetBetData()
	{
		unsigned char buf;
		RAND_bytes(&buf, 1);
		int num = buf;

		if(num>0&&num<=6)
			return num;
		num = num%6 +1;
		return num;
	}
	unsigned char GetRanOpType(){
		unsigned char cType;
		RAND_bytes(&cType, sizeof(cType));
		unsigned char  gussnum = cType % 2;
		//cout<<"type:"<<(int)gussnum<<endl;
		return gussnum;
	}
private:
	unsigned char nSdata[33];  //ǰ32�ֽڵ������,��1���ֽڵ�1-6֮�����
	int mCurStep;
	string strRegScriptHash;
	string strAsendHash;
	string strBacceptHash;
	string strAopenHash;
	string scriptid;
	uint64_t betamount;
};

#endif /* CYCLEP2PBET_TEST_H_ */
