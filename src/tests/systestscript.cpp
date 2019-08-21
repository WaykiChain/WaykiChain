// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifdef TODO
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "init.h"
#include "miner/miner.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include <boost/test/unit_test.hpp>
#include "rpc/rpcclient.h"
#include "tx/tx.h"
#include "wallet/wallet.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include "vm/luavm/script.h"
#include "rpc/core/rpcserver.h"
#include "systestbase.h"
#include <boost/algorithm/string/predicate.hpp>
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_reader.h"
#include "json/json_spirit_writer.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_stream_reader.h"
#include "tx/tx.h"
using namespace std;
using namespace boost;
std::string txid("");
const int nNewAddrs = 1420;

class CSysScriptTest:public SysTestBase
{
public:
	CSysScriptTest() {
		StartServer();
	}

	~CSysScriptTest(){
		StopServer();
	}
private:
	void StartServer() {

	}

	void StopServer() {
	}
public:
	uint64_t GetValue(Value val,string compare)
	{
		if (val.type() != obj_type)
		{
			return 0;
		}
		json_spirit::Value::Object obj=  val.get_obj();
		string ret;
		for(size_t i = 0; i < obj.size(); ++i)
		{
			const json_spirit::Pair& pair = obj[i];
			const std::string& str_name = pair.name_;
			const json_spirit::Value& val_val = pair.value_;

			if(str_name.compare(compare) == 0)
			{
				return val_val.get_int64();
			}

			if(compare == "value")
			{
				if(str_name =="FreedomFund")
					{
						json_spirit::Value::Array narray = val_val.get_array();
						if(narray.size() == 0)
							return false;
						json_spirit::Value::Object obj1 = narray[0].get_obj();
						for(size_t j = 0; j < obj1.size(); ++j)
						{
							const json_spirit::Pair& pair = obj1[j];
							const std::string& str_name = pair.name_;
							const json_spirit::Value& val_val = pair.value_;
							if(str_name == "value")
							{
								return val_val.get_int64();
							}
						}

					}
			}



		}
		return 0;
	}

	void CheckSdk()
	{
		string param ="01";
		Value resut =CallContractTx("010000000100", "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param, 10, 10000000);
		BOOST_CHECK(GetHashFromCreatedTx(resut,txid));
		BOOST_CHECK(GenerateOneBlock());
		uint256 hash(uint256S(txid.c_str()));
		param ="02";
		param += HexStr(hash);
		string temp;
		resut =CallContractTx("010000000100", "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param, 10, 10000000);
		BOOST_CHECK(GetHashFromCreatedTx(resut,temp));
		BOOST_CHECK(GenerateOneBlock());

		param ="03";
		resut =CallContractTx("010000000100", "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param, 10, 10000000);
		BOOST_CHECK(GetHashFromCreatedTx(resut,temp));
		BOOST_CHECK(GenerateOneBlock());

		param ="05";
		param += HexStr(hash);

		resut =CallContractTx("010000000100", "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param, 10, 10000000);
		BOOST_CHECK(GetHashFromCreatedTx(resut,temp));
		BOOST_CHECK(GenerateOneBlock());
	}

	string CreateRegScript(const char* strAddr, const char* sourceCode)
	{
		int nFee = 1*COIN + 10000000;
		string strTxHash;
		string strFileName(sourceCode);
		Value valueRes = DeployContractTx(strAddr,strFileName , 100, nFee);
		BOOST_CHECK(GetHashFromCreatedTx(valueRes,strTxHash));
		BOOST_CHECK(GenerateOneBlock());
		return strTxHash;
	}
	string CreateContactTx(int param)
	{
		char buffer[3] = {0};
		sprintf(buffer,"%02x",param);
		string temp;
		//Value resut =CallContractTx("010000000100", "5yNhSL7746VV5qWHHDNLkSQ1RYeiheryk9uzQG6C5d", buffer,10);
		Value resut =CallContractTx("010000000100", "ddMuEBkAwhcb5K5QJ83MqQHrgHRn4EbRdh", buffer, 10, 1*COIN);
		BOOST_CHECK(GetHashFromCreatedTx(resut,temp));
		BOOST_CHECK(GenerateOneBlock());
		return temp;
	}

	void CheckRollBack()
	{
		CreateContactTx(6);    //新增脚本数据
		//cout<<6<<endl;
		CreateContactTx(7);;   //修改脚本数据
		//cout<<7<<endl;
		CreateContactTx(8);    //删除脚本数据
//		cout<<8<<endl;
//		DisConnectBlock(1);           //删除1个block
//		mempool.mapTx.clear();
//		CreateContactTx(9);    //check删除的脚本是否恢复
//	//	cout<<9<<endl;
//		DisConnectBlock(2);
//		mempool.mapTx.clear();
//		CreateContactTx(10);    //check修改的脚本数据是否恢复
////		cout<<10<<endl;
//		DisConnectBlock(2);
//		mempool.mapTx.clear();
//		CreateContactTx(11);   //check新增的脚本数据是否恢复
	}
	bool CheckScriptid(Value val,string scriptid)
	{
		if (val.type() != obj_type)
		{
			return false;
		}
		const Value& value = val.get_obj();

		json_spirit::Value::Object obj= value.get_obj();
		for(size_t i = 0; i < obj.size(); ++i)
		{
			const json_spirit::Pair& pair = obj[i];
			const std::string& str_name = pair.name_;
			const json_spirit::Value& val_val = pair.value_;
			if(str_name =="PublicKey")
			{
				if(val_val.get_str() != "")
				{
					return false;
				}
			}
			else if(str_name =="KeyID")
			{
				CRegID regId(scriptid);
				CKeyID keyId = Hash160(regId.GetRegIdRaw());
				string key = HexStr(keyId.begin(), keyId.end()).c_str();
				if(val_val.get_str() != key)
				{
					return false;
				}
			}

		}
		return true;
	}
	bool CreateScriptAndCheck()
	{
		CreateRegScript("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin");

		CreateRegScript("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin");

		string strRegID = "010000000100";

		Value temp1 = GetAccountInfo(strRegID);
		if(!CheckScriptid(temp1,strRegID))
		{
			return false;
		}

		strRegID = "020000000100";
		temp1 = GetAccountInfo(strRegID);
		if(!CheckScriptid(temp1,strRegID))
		{
			return false;
		}
		return true;
	}

	void CheckScriptAccount()
	{
		// 检查脚本账户创建的合法性
		BOOST_CHECK_EQUAL(CreateScriptAndCheck(),true);

		//// 给脚本账户打钱
		string accountId = "010000000100";
		int param = 13;
		string temp = "";
		temp += tinyformat::format("%02x%s",param,accountId);
		Value resut =CallContractTx("010000000100", "dsGb9GyDGYnnHdjSvRfYbj9ox2zPbtgtpo", temp,10,100000000,10000);
		BOOST_CHECK(GetHashFromCreatedTx(resut,temp));

		BOOST_CHECK(SetAddrGenerteBlock("dggsWmQ7jH46dgtA5dEZ9bhFSAK1LASALw"));
		Value temp1 = GetAccountInfo("010000000100");
		temp1 = GetAccountInfo("dsGb9GyDGYnnHdjSvRfYbj9ox2zPbtgtpo");
		BOOST_CHECK_EQUAL(GetValue(temp1,"Balance"),99999999899990000);


		/// 脚本账户给普通账户打钱
		param = 14;
		temp = "";
		temp += tinyformat::format("%02x%s",param,accountId);
		resut =CallContractTx("010000000100", "dsGb9GyDGYnnHdjSvRfYbj9ox2zPbtgtpo", temp,10,100000000);
		BOOST_CHECK(GetHashFromCreatedTx(resut,temp));

		BOOST_CHECK(SetAddrGenerteBlock("dps9hqUmBAVGVg7ijLGPcD9CJz9HHiTw6H"));
		temp1 = GetAccountInfo("010000000100");
		BOOST_CHECK_EQUAL(GetValue(temp1,"Balance"),0);
		temp1 = GetAccountInfo("dsGb9GyDGYnnHdjSvRfYbj9ox2zPbtgtpo");
		BOOST_CHECK_EQUAL(GetValue(temp1,"Balance"),99999999800000000);

		//测试不能从其他脚本打钱到本APP脚本账户中
		accountId = "020000000100";
		param = 19;
		temp = "";
		temp += tinyformat::format("%02x%s",param,accountId);
		resut =CallContractTx("010000000100", "dsGb9GyDGYnnHdjSvRfYbj9ox2zPbtgtpo", temp,10);
		BOOST_CHECK(!GetHashFromCreatedTx(resut,temp));

	}

	bool CheckScriptDB(int nheigh,string srcipt,string hash,int flag)
	{
		int curtiph = chainActive.Height();

		string  hash2 = "txid";

		CRegID regid(srcipt);
		if (regid.IsEmpty() == true) {
			return false;
		}

		CContractDBCache contractScriptTemp(*pScriptDBTip, true);
		if (!contractScriptTemp.HaveContract(regid)) {
			return false;
		}

		vector<unsigned char> value;
		vector<unsigned char> vScriptKey;

		uint256 hash1(value);
		string pvalue(value.begin(),value.end());
		if(flag)
		BOOST_CHECK(hash==hash1.GetHex()|| pvalue == hash2);
		else{
			BOOST_CHECK(hash==hash1.GetHex());
		}
		unsigned short key = 0;
		memcpy(&key,  &vScriptKey.at(0), sizeof(key));

		int count = dbsize - 1;
		while (count--) {
			uint256 hash3(value);
			string pvalue(value.begin(), value.end());
			if (flag)
				BOOST_CHECK(hash == hash3.GetHex() || pvalue == hash2);
			else {
				BOOST_CHECK(hash == hash1.GetHex());
			}
		}
		return true;
	}

	void CreateTx(string pcontact,string addr)
	{
		string temp =addr;
		Value resut =CallContractTx("010000000100", temp, pcontact, 10, 1*COIN);
		string strReturn;
		BOOST_CHECK(GetHashFromCreatedTx(resut,strReturn));
		BOOST_CHECK(GenerateOneBlock());
		return ;
	}
	bool GetContractData(string srcipt,vector<unsigned char> key)
	{
		CRegID regid(srcipt);
			if (regid.IsEmpty() == true) {
				return false;
			}
			CContractDBCache contractScriptTemp(*pScriptDBTip, true);
			if (!contractScriptTemp.HaveContract(regid)) {
				return false;
			}
			vector<unsigned char> value;
			int tipH = chainActive.Height();
			CDbOpLog operLog;
			if (!contractScriptTemp.GetContractData(tipH,regid,key, value)) {
				return false;
			}
			return true;
	}
	string CreatWriteTx(string &hash)
	{
		string shash = CreateRegScript("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin");
		string scriptid;
		GetTxConfirmedRegID(shash, scriptid);
		BOOST_CHECK(scriptid !="");
		//// first tx
		string phash = CreateContactTx(15);
		int  height = chainActive.Height();

		BOOST_CHECK(CheckScriptDB(height,scriptid,phash,false));

		hash = phash;
		return scriptid;
	}
	void testdb()
	{
		string phash = "";
		string scriptid =  CreatWriteTx(phash);
		int height = chainActive.Height();
		int circle = 4;
		while(circle--)
		{
			BOOST_CHECK(GenerateOneBlock());
		}

		int count = 15;
		while(count > 1)
		{
			//// second tx
				uint256 hash(uint256S(phash.c_str()));
				int param =16;
				string temp = "";
				temp += tinyformat::format("%02x%s%02x",param,HexStr(hash),height);
			//	cout<<"cont:"<<temp<<endl;
				CreateTx(temp,"e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz");

				vector<unsigned char> key;
				const char *key1="2_error";
				key.insert(key.begin(),key1, key1 + strlen(key1) +1);
				BOOST_CHECK(!GetContractData(scriptid,key));

				CheckScriptDB((height),scriptid,phash,false);
				count--;
		}
	}

	void testdeletmodifydb()
	{
		string  writetxhash= "";
		string scriptid =  CreatWriteTx(writetxhash);
		int height = chainActive.Height();

		///// 修改删除包
		int param =17;
		string temp = "";
		temp += tinyformat::format("%02x%02x",param,11);
		CreateTx(temp,"e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz");
		vector<unsigned char> key;
		const char *key1="3_error";
		key.insert(key.begin(),key1, key1 + strlen(key1) +1);
		BOOST_CHECK(!GetContractData(scriptid,key));
		CheckScriptDB(height,scriptid,writetxhash,true);
		int modHeight = chainActive.Height();

	//	cout<<"end:"<<endl;
		//// 遍历
		int count = 15;
        while(count > 1)
        {
    		int param =18;
    		string temp = "";
    		temp += tinyformat::format("%02x",param);
    		CreateTx(temp,"e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz");
  //  		cout<<"cont:"<<endl;
   // 		cout<<chainActive.Height()<<endl;
        	CheckScriptDB(height,scriptid,writetxhash,true);
        	count--;
        }
	}
	void TestMinner()
	{
		string hash = "";
		vector<string> vNewAddress;
		string strAddress("0-8");
		for(int i=0; i < nNewAddrs; i++)
		{
			string newaddr;
			BOOST_CHECK(GetNewAddr(newaddr,false));
			vNewAddress.push_back(newaddr);
			uint64_t nMoney = 10000 * COIN;
			if( i == 800 ) {
				strAddress = "0-7";
			}
			Value value = CreateNormalTx(strAddress, newaddr, nMoney);
			BOOST_CHECK(GetHashFromCreatedTx(value,hash));
		}
		cout << "create new address completed!" << endl;
		while (!IsMemoryPoolEmpty()) {
			BOOST_CHECK(GenerateOneBlock());
		}
		cout << "new transaction have been confirmed, current height:" << chainActive.Height() << endl;
		for(size_t i=0; i < vNewAddress.size(); i++) {
			int nfee = GetRandomFee();
			Value value1 = RegisterAccountTx(vNewAddress[i], nfee);
			BOOST_CHECK(GetHashFromCreatedTx(value1,hash));

		//	BOOST_CHECK(GenerateOneBlock());
		}
		cout << "new address register account transactions have been created completed!" << endl;
		while (!IsMemoryPoolEmpty()) {
			BOOST_CHECK(GenerateOneBlock());
		}
       int totalhigh = 20;
		while(chainActive.Height() != totalhigh) {
			BOOST_CHECK(GenerateOneBlock());
			ShowProgress("GenerateOneBlock progress: ",((float)chainActive.Height()/(float)totalhigh)*100);
			MilliSleep(1500);
		}

//		BOOST_CHECK(DisConnectBlock(chainActive.Height()-1));
//		BOOST_CHECK(GenerateOneBlock());
//		BOOST_CHECK(GenerateOneBlock());
	}
};


BOOST_FIXTURE_TEST_SUITE(sysScript_test,CSysScriptTest)

BOOST_FIXTURE_TEST_CASE(script_test,CSysScriptTest)
{



	//// pass
	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	CreateRegScript("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin");
	CheckSdk();





	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	CreateRegScript("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin");
	CheckRollBack();

	//// pass
	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	CheckScriptAccount();

	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	testdb();

	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	testdeletmodifydb();
}

// 测试各种地址挖矿
BOOST_FIXTURE_TEST_CASE(minier,CSysScriptTest)
{
	ResetEnv();
	TestMinner();
}



BOOST_FIXTURE_TEST_CASE(appacc,CSysScriptTest){

	ResetEnv();
	BOOST_CHECK(0==chainActive.Height());
	string shash = CreateRegScript("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin");
	string sriptid ="";
	BOOST_CHECK(GetTxConfirmedRegID(shash,sriptid));
	int temp = 22;

	string param = strprintf("%02x",temp);
	string hash ="";
	uint64_t nMoney = 1000000000;
	Value resut =CallContractTx(sriptid, "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param,10,10000000,nMoney);
	BOOST_CHECK(GetHashFromCreatedTx(resut,hash));
	BOOST_CHECK(GenerateOneBlock());

	nMoney = nMoney/5;
	vector<unsigned char> vtemp;
	vtemp.assign((char*)&nMoney,(char*)&nMoney+sizeof(nMoney));
	for(int i = 1;i<5;i++){
		temp += 1;
		param = strprintf("%02x%s",temp,HexStr(vtemp));
		//cout<<i<<endl;
		resut =CallContractTx(sriptid, "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param,10,10000000,0);
		BOOST_CHECK(GetHashFromCreatedTx(resut,hash));
		BOOST_CHECK(GenerateOneBlock());
	}

	temp += 1;
	param = strprintf("%02x%s",temp,HexStr(vtemp));
	resut =CallContractTx(sriptid, "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param,10,10000000,0);
	BOOST_CHECK(GetHashFromCreatedTx(resut,hash));
	BOOST_CHECK(GenerateOneBlock());

	temp += 1;
	param = strprintf("%02x%s",temp,HexStr(vtemp));
	resut =CallContractTx(sriptid, "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz", param,10,10000000,0);
	BOOST_CHECK(!GetHashFromCreatedTx(resut,hash));


	BOOST_CHECK(DisConnectBlock(5));



	CContractDBCache contractScriptTemp(*pScriptDBTip, true);
	CRegID script(sriptid);
	CRegID strreg;
	string address = "e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz";

	BOOST_CHECK(SysTestBase::GetRegID(address,strreg));
	std::shared_ptr<CAppUserAccount> tem = std::make_shared<CAppUserAccount>();
	contractScriptTemp.GetContractAccount(script,strreg.GetRegIdRaw(),*tem.get());
	BOOST_CHECK(tem.get()->GetBcoins() == nMoney);
}

BOOST_AUTO_TEST_SUITE_END()
#endif //TODO
