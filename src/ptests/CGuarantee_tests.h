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

#define	TX_REGISTER   0x01   //注册仲裁账户
#define TX_MODIFYREGISTER  0x02 // 修改仲裁者注册信息
#define TX_ARBIT_ON     0x03 //仲裁开启
#define TX_ARBIT_OFF    0x04 //仲裁暂停
#define	TX_UNREGISTER  0x05 //注销仲裁账户
#define	TX_SEND  0x06 //挂单
#define	TX_CANCEL  0x07 //取消挂单
#define	TX_ACCEPT  0x08 //接单
#define TX_DELIVERY 0x09//发货
#define	TX_BUYERCONFIRM  0x0a //买家确认收货
#define	TX_ARBITRATION  0x0b //申请仲裁
#define	TX_FINALRESULT  0x0c //裁决结果






#define	SEND_TYPE_BUY   0x00   //!<挂单 买
#define	SEND_TYPE_SELL  0x01  //!<挂单 卖


typedef struct {
	unsigned char systype;               //0xff
	unsigned char type;            // 0x01 提?现?  02 充?值μ  03 提?现?一?定¨的?金e额?
	unsigned char typeaddr;            // 0x01 regid 0x02 base58
	uint64_t     money;

	IMPLEMENT_SERIALIZE
	(
		READWRITE(systype);
		READWRITE(type);
		READWRITE(typeaddr);
		READWRITE(money);
	)
} APPACC_money;

typedef struct {
	unsigned char systype;               //0xff
	unsigned char type;            // 0x01 提?现?  02 充?值μ  03 提?现?一?定¨的?金e额?
	unsigned char typeaddr;            // 0x01 regid 0x02 base58
//	uint64_t     money;

	IMPLEMENT_SERIALIZE
	(
		READWRITE(systype);
		READWRITE(type);
		READWRITE(typeaddr);
//		READWRITE(money);
	)
} APPACC;

enum GETDAWEL{
	TX_REGID = 0x01,
	TX_BASE58 = 0x02,
};



typedef struct {
	unsigned char type;            //!<交易类型
	uint64_t arbiterMoneyX;             //!<仲裁费用X
	uint64_t overtimeMoneyYmax;  //!<超时未判决的最大赔偿费用Y
	uint64_t configMoneyZ;              //!<无争议裁决费用Z
	unsigned int  overtimeheightT;  //!<判决期限时间T
	char  comment[220];             //!<备注说明 字符串以\0结束，长度不足后补0
	IMPLEMENT_SERIALIZE
	(
			READWRITE(type);
			READWRITE(arbiterMoneyX);
			READWRITE(overtimeMoneyYmax);
			READWRITE(configMoneyZ);
			READWRITE(overtimeheightT);
			for(int i = 0; i < 220; i++)
			{
				READWRITE(comment[i]);
			}
	)

}TX_REGISTER_CONTRACT;  //!<注册仲裁账户

typedef struct {
	unsigned char type;            //!<交易类型
	unsigned char sendType;         //!<挂单类型:0 买  1卖
	char arbitationID[6];        //!<仲裁者ID（采用6字节的账户ID）
	uint64_t moneyM;                   //!<交易金额
	unsigned int height;           //!<每个交易环节的超时高度

	char goods[20];               //!<商品信息  字符串以\0结束，长度不足后补0
	char  comment[200];             //!<备注说明 字符串以\0结束，长度不足后补0
	IMPLEMENT_SERIALIZE
	(
			READWRITE(type);
			READWRITE(sendType);
			for(int i = 0; i < 6; i++)
			{
				READWRITE(arbitationID[i]);
			}
			READWRITE(moneyM);
			READWRITE(height);
			for(int i = 0; i < 20; i++)
			{
				READWRITE(goods[i]);
			}
			for(int i = 0; i < 200; i++)
			{
				READWRITE(comment[i]);
			}
	)
}TX_SNED_CONTRACT;                  //!<挂单

typedef struct {
	unsigned char type;            //!<交易类型
	unsigned char txid[32];       //!<挂单的交易hash
	unsigned int height;          //!<每个交易环节的超时高度
	IMPLEMENT_SERIALIZE
	(
			READWRITE(type);
			for(int i = 0; i < 32; i++)
			{
				READWRITE(txid[i]);
			}
			READWRITE(height);
	)
} TX_CONTRACT;
typedef struct {
	unsigned char type;            //!<交易类型
	unsigned char txid[32];       //!<挂单的交易hash
	unsigned int height;          //!<每个交易环节的超时高度
	char  arbitationID[6];       //!<仲裁者ID（采用6字节的账户ID）
	IMPLEMENT_SERIALIZE
	(
			READWRITE(type);
			for(int i = 0; i < 32; i++)
			{
				READWRITE(txid[i]);
			}
			READWRITE(height);
			for(int i = 0; i < 6; i++)
			{
				READWRITE(arbitationID[i]);
			}
	)
} TX_Arbitration;  //!<申请仲裁

typedef struct {
	unsigned char type;            //!<交易类型
	unsigned char arbitHash[32];      //!<申请仲裁的交易hash
	unsigned int overtimeheightT;//!<判决期限时间T
	char 	winner[6];      	//!<赢家ID（采用6字节的账户ID）
	uint64_t winnerMoney;            //!<最终获得的金额
	char  loser[6];       //!<输家ID（采用6字节的账户ID）
	uint64_t loserMoney;            //!<最终获得的金额
	IMPLEMENT_SERIALIZE
	(
		READWRITE(type);
		for(int i = 0; i < 32; i++)
		{
			READWRITE(arbitHash[i]);
		}
		READWRITE(overtimeheightT);
		for(int i = 0; i < 6; i++)
		{
			READWRITE(winner[i]);
		}
		READWRITE(winnerMoney);
		for(int i = 0; i < 6; i++)
		{
			READWRITE(loser[i]);
		}
		READWRITE(loserMoney);
	)
}TX_FINALRESULT_CONTRACT;        //!<最终裁决


class CGuaranteeTest: public CycleTestBase {
	int nNum;
	int nStep;
	string strTxHash;
	string strAppRegId;//注册应用后的Id
public:
	CGuaranteeTest();
	~CGuaranteeTest(){};
	virtual TEST_STATE Run() ;
	bool RegistScript();

	bool Recharge(void);
	bool Withdraw(void);
	bool WithdrawSomemoney(void);

	bool Register(unsigned char type);
	bool ArbitONOrOFF(unsigned char type);
	bool UnRegister(void);
	bool SendStartTrade(void);
	bool SendCancelTrade(void);
	bool AcceptTrade(void);
	bool DeliveryTrade(void);
	bool BuyerConfirm(void);
    bool Arbitration(void);
    bool RunFinalResult(void);

};

#endif /* CANONY_TESTS_H */
