/*
 * CBlackHalo_tests.cpp
 *
 *  Created on: 2014��12��30��
 *      Author: ranger.shi
 */

#include "CBlackHalo_tests.h"

CBlackHalo::CBlackHalo() {
	 step =0 ;
	 sritpthash = "";
	 buyerhash = "";
	 sellerhash= "";
	 buyerconfiredhash = "";
	 buyercancelhash = "";
	 scriptid = "";
	 sendmonye = 0;
}

CBlackHalo::~CBlackHalo() {
	// todo Auto-generated destructor stub
}

TEST_STATE CBlackHalo::Run()
{
     switch(step)
     {
     case 0:
    	 RegistScript();
    	 break;
     case 1:
    	 WaitRegistScript();
    	 break;
     case 2:
    	 SendBuyerPackage();
    	 break;
     case 3:
    	 WaitSendBuyerPackage();
    	 break;
     case 4:
    	 SendSellerPackage();
    	 break;
     case 5:
    	 WaitSendSellerPackage();
    	 break;
     case 6:
    	 SendBuyerConfirmedPackage();
          break;
     case 7:
          WaitSendBuyerConfirmedPackage();

          break;
     }
	return next_state;
}

bool CBlackHalo::RegistScript(){
	const char* pKey[] = { "cNcJkU44oG3etbWoEvY46i5qWPeE8jVb7K44keXxEQxsXUZ85MKU",
			"cVFWoy8jmJVVSNnMs3YRizkR7XEekMTta4MzvuRshKuQEEJ4kbNg", };
	int nCount = sizeof(pKey) / sizeof(char*);
	basetest.ImportWalletKey(pKey, nCount);

	string strFileName("darksecure.bin");
	int nFee = GetRandomFee();
	int nCurHight;
	basetest.GetBlockHeight(nCurHight);
	//ע��ԶĽű�
	Value regscript = basetest.DeployContractTx(BUYER_A, strFileName, nCurHight, nFee+COIN);
	if(basetest.GetHashFromCreatedTx(regscript, sritpthash)){
		step++;
		return true;
	}
	return true;
}
bool CBlackHalo::WaitRegistScript(){
	if (basetest.GetTxConfirmedRegID(sritpthash, scriptid)) {
			step++;
			return true;
	}
	return true;
}

bool CBlackHalo::SendBuyerPackage(){
	if(scriptid == "")
		return false;

		FIRST_CONTRACT senddata;
		senddata.dnType = 0x01;
		string strregid = "";
		string selleraddr = SELLER_B;
		BOOST_CHECK(basetest.GetRegID(selleraddr,strregid));
		CRegID Sellerregid(strregid);
		memcpy(senddata.seller,&Sellerregid.GetRegIdRaw().at(0),sizeof(senddata.seller));

		CDataStream scriptData(SER_DISK, CLIENT_VERSION);
		scriptData << senddata;
		string sendcontract = HexStr(scriptData);
//		cout <<sendcontract<<endl;
		sendmonye = GetPayMoney();
		Value  buyerpack= basetest.CallContractTx(scriptid,BUYER_A,sendcontract,0,0,sendmonye);

		if(basetest.GetHashFromCreatedTx(buyerpack, buyerhash)){
			step++;
			return true;
		}
		return true;
}
bool CBlackHalo::WaitSendBuyerPackage(){
	string index = "";
	if (basetest.GetTxConfirmedRegID(buyerhash, index)) {
		step++;
		return true;
	}
	return true;
}

bool CBlackHalo::SendSellerPackage(){
	if(scriptid == "")
		return false;

	NEXT_CONTRACT Seller;

	Seller.dnType = 0x02;
	memcpy(Seller.hash, uint256S(buyerhash).begin(), sizeof(Seller.hash));

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << Seller;
	string sendcontract = HexStr(scriptData);
//	cout<<"size:"<<Size<<" " <<sendcontract<<endl;

	Value  Sellerpack= basetest.CallContractTx(scriptid,SELLER_B,sendcontract,0,0,sendmonye/2);

	if(basetest.GetHashFromCreatedTx(Sellerpack, sellerhash)){
		step++;
		return true;
	}

	return true;
}
bool CBlackHalo::WaitSendSellerPackage(){
	string index = "";
	if (basetest.GetTxConfirmedRegID(sellerhash, index)) {
		step++;
			return true;
	}
	return true;
}

bool CBlackHalo::SendBuyerConfirmedPackage(){
	if(scriptid == "")
		return false;

	NEXT_CONTRACT Seller;
	Seller.dnType = 0x03;
	memcpy(Seller.hash, uint256S(buyerhash).begin(), sizeof(Seller.hash));

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << Seller;
	string sendcontract = HexStr(scriptData);
	Value  Sellerpack= basetest.CallContractTx(scriptid,BUYER_A,sendcontract,0);

	if(basetest.GetHashFromCreatedTx(Sellerpack, buyerconfiredhash)){
		step++;
		return true;
	}

	return true;
}
bool CBlackHalo::WaitSendBuyerConfirmedPackage(){
	string index = "";
	if (basetest.GetTxConfirmedRegID(buyerconfiredhash, index)) {
		step = 1;
        IncSentTotal();
			return true;
	}
	return true;
}

bool CBlackHalo::SendBuyerCancelPackage(){
	if(scriptid == "")
		return false;

	NEXT_CONTRACT Seller;
	Seller.dnType = 0x04;
	memcpy(Seller.hash, uint256S(buyerhash).begin(), sizeof(Seller.hash));

	CDataStream scriptData(SER_DISK, CLIENT_VERSION);
	scriptData << Seller;
	string sendcontract = HexStr(scriptData);
	Value  Sellerpack= basetest.CallContractTx(scriptid,BUYER_A,sendcontract,0);

	if(basetest.GetHashFromCreatedTx(Sellerpack, buyercancelhash)){
		step++;
		return true;
	}
	return true;
}
bool CBlackHalo::WaitSendBuyerCancelPackage(){
	string index = "";
	if (basetest.GetTxConfirmedRegID(buyercancelhash, index)) {
		step = 1;
		return true;
	}
	return true;
}
