// Copyright (c) 2011-2014 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <stdlib.h>
#include <time.h>
#include "main.h"
#include "miner/miner.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>

#include "json/json_spirit_writer_template.h"
#include "./rpc/core/rpcclient.h"
using namespace std;
using namespace boost;
using namespace json_spirit;

extern Object CallRPC(const string& strMethod, const Array& params);

int CommandLineRPC_GetValue(int argc, const char *argv[],Value &value)
{
    string strPrint;
    int nRet = 0;
    try
    {
        // Skip switches
        while (argc > 1 && IsSwitchChar(argv[1][0]))
        {
            argc--;
            argv++;
        }

        // Method
        if (argc < 2)
            throw runtime_error("too few parameters");
        string strMethod = argv[1];

        // Parameters default to strings
        std::vector<std::string> strParams(&argv[2], &argv[argc]);
        Array params = RPCConvertValues(strMethod, strParams);

        // Execute
        Object reply = CallRPC(strMethod, params);

        // Parse reply
        const Value& result = find_value(reply, "result");
        const Value& error  = find_value(reply, "error");

        if (error.type() != null_type)
        {
            // Error
            strPrint = "error: " + write_string(error, false);
            int code = find_value(error.get_obj(), "code").get_int();
            nRet = abs(code);
        }
        else
        {
        	value = result;
            // Result
            if (result.type() == null_type)
                strPrint = "";
            else if (result.type() == str_type)
                strPrint = result.get_str();
            else
                strPrint = write_string(result, true);
        }
    }
    catch (boost::thread_interrupted) {
        throw;
    }
    catch (std::exception& e) {
        strPrint = string("error: ") + e.what();
        nRet = abs(-1);
    }
    catch (...) {
        PrintExceptionContinue(NULL, "CommandLineRPC()");
        throw;
    }

    if (strPrint != "")
    {
//      fprintf((nRet == 0 ? stdout : stderr), "%s\n", strPrint.c_str());
    }

    return nRet;
}
const static int nFrozenHeight = 100;
const static int nMatureHeight = 100;
struct AccState{
	int64_t dUnmatureMoney;
	int64_t dFreeMoney;
	int64_t dFrozenMoney;
	AccState()
	{
		dUnmatureMoney = 0;
		dFreeMoney = 0;
		dFrozenMoney = 0;
	}
	AccState(int64_t a,int64_t b,int64_t c)
	{
		dUnmatureMoney = a;
		dFreeMoney = b;
		dFrozenMoney = c;
	}

	bool operator==(AccState &a)
	{
		if(dUnmatureMoney == a.dUnmatureMoney &&
		   dFreeMoney == a.dFreeMoney &&
		   dFrozenMoney == a.dFrozenMoney)
		{
			return true;
		}
		return false;
	}

	bool SumEqual(AccState &a)
	{
		int64_t l = dUnmatureMoney+dFreeMoney+dFrozenMoney;
		int64_t r = a.dUnmatureMoney+a.dFreeMoney+a.dFrozenMoney;

		return l==r;
	}

	AccState& operator+=(AccState &a)
	{
		dUnmatureMoney += a.dUnmatureMoney;
		dFreeMoney += a.dFreeMoney;
		dFrozenMoney += a.dFrozenMoney;
		return *this;
	}
};

struct AccOperLog{
	std::map<int,AccState> mapAccState;
	AccOperLog()
	{
		mapAccState.clear();
	}
	bool Add(int &height,AccState &accstate)
	{
		mapAccState[height] = accstate;
		return true;
	}

	void MergeAcc(int height)
	{
		for(auto &item:mapAccState)
		{
			if(height > item.first+nFrozenHeight)
			{
				item.second.dFreeMoney += item.second.dFrozenMoney;
				item.second.dFrozenMoney = 0.0;
			}

			if(height > item.first+nMatureHeight)
			{
				item.second.dFreeMoney += item.second.dUnmatureMoney;
				item.second.dUnmatureMoney = 0.0;
			}
		}
	}
};

class CMinerTest{
public:
	std::map<string,AccState> mapAccState;
	std::map<string,AccOperLog> mapAccOperLog;
	int nCurHeight;
	int64_t nCurMoney;
	int64_t nCurFee;
private:
	int GetRandomFee()
	{
		srand(time(NULL));
		int r =(rand()%1000000)+1000000;
		return r;
	}
	int GetRandomMoney()
	{
		srand(time(NULL));
		int r = (rand() % 1000) + 1000;
		return r;
	}
public:

	bool GetOneAddr(std::string &addr, const char *pStrMinMoney, const char *bpBoolReg)
	{
		//CommanRpc
		const char *argv[] = {"rpctest", "getoneaddr",pStrMinMoney,bpBoolReg};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc,argv,value);
		if(!ret)
		{
			addr = value.get_str();
			LogPrint("test_miners","GetOneAddr:%s\r\n",addr.c_str());
			return true;
		}
		return false;
	}

	bool GetOneScriptId(std::string &regscriptid)
	{
		//CommanRpc
		const char *argv[] = {"rpctest", "listcontractregid"};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc,argv,value);
		if(!ret)
		{
			Object &Oid = value.get_obj();
			regscriptid = Oid[0].value_.get_str();
			LogPrint("test_miners","GetOneAddr:%s\r\n",regscriptid.c_str());
			return true;
		}
		return false;
	}

	bool GetNewAddr(std::string &addr)
	{
		//CommanRpc
		const char *argv[] = {"rpctest", "getnewaddr"};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			addr = value.get_str();
			LogPrint("test_miners","getnewaddr:%s\r\n",addr.c_str());
			return true;
		}
		return false;
	}

	bool GetAccState(const std::string &addr,AccState &accstate)
	{
		//CommanRpc
		char temp[64] = {0};
		strncpy(temp,addr.c_str(),sizeof(temp)-1);

		const char *argv[] = {"rpctest", "getaddramount",temp};
		int argc = sizeof(argv)/sizeof(char*);
		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			Object obj = value.get_obj();
			double dfree = find_value(obj,"free amount").get_real();
			double dmature = find_value(obj,"mature amount").get_real();
			double dfrozen = find_value(obj,"frozen amount").get_real();
			accstate.dFreeMoney = roundint64(dfree*COIN);
			accstate.dUnmatureMoney = roundint64(dmature*COIN);
			accstate.dFrozenMoney = roundint64(dfrozen*COIN);
			LogPrint("test_miners","addr:%s GetAccState FreeMoney:%0.8lf matureMoney:%0.8lf FrozenMoney:%0.8lf\r\n",
					addr.c_str(), dfree, dmature, dfrozen);
			return true;
		}
		return false;
	}

	bool GetBlockHeight(int &height)
	{
		height = 0;
		const char *argv[] = {"rpctest", "getinfo",};
		int argc = sizeof(argv) / sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			Object obj = value.get_obj();

			height = find_value(obj,"blocks").get_int();
			LogPrint("test_miners","GetBlockHeight:%d\r\n",height);
			return true;
		}
		return false;
	}

	bool CreateNormalTx(const std::string &srcAddr,const std::string &desAddr,const int height)
	{
		//CommanRpc
		char src[64] = { 0 };
		strncpy(src, srcAddr.c_str(), sizeof(src)-1);

		char dest[64] = { 0 };
		strncpy(dest, desAddr.c_str(), sizeof(dest)-1);

		char money[64] = {0};
		int nmoney = GetRandomMoney();
		sprintf(money,"%d00000000",nmoney);
		nCurMoney = nmoney*COIN;

		char fee[64] = {0};
		int nfee = GetRandomFee();
		sprintf(fee,"%d",nfee);
		nCurFee = nfee;

		char height[16] = {0};
		sprintf(height,"%d",height);

		const char *argv[] = { "rpctest", "createnormaltx", src,dest,money,fee,height};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			LogPrint("test_miners","CreateNormalTx:%s\r\n",value.get_str().c_str());
			return true;
		}
		return false;
	}

	bool CreateFreezeTx(const std::string &addr,const int height)
	{
		//CommanRpc
		char caddr[64] = { 0 };
		strncpy(caddr, addr.c_str(), sizeof(caddr)-1);

		char money[64] = {0};
		int nmoney = GetRandomMoney();
		sprintf(money, "%d00000000", nmoney);
		nCurMoney = nmoney * COIN;

		char fee[64] = { 0 };
		int nfee = GetRandomFee();
		sprintf(fee, "%d", nfee);
		nCurFee = nfee;

		char height[16] = {0};
		sprintf(height,"%d",height);

		char freeheight[16] = {0};
		sprintf(freeheight,"%d",height+100);

		const char *argv[] = { "rpctest", "createfreezetx", caddr,money,fee,height,freeheight};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			LogPrint("test_miners","CreateFreezeTx:%s\r\n",value.get_str().c_str());
			return true;
		}
		return false;
	}

	bool registeraccounttx(const std::string &addr,const int height)
	{
		//CommanRpc
		char caddr[64] = { 0 };
		strncpy(caddr, addr.c_str(), sizeof(caddr)-1);

		char fee[64] = { 0 };
		int nfee = GetRandomFee();
		sprintf(fee, "%d", nfee);
		nCurFee = nfee;

		char height[16] = {0};
		sprintf(height,"%d",height);


		const char *argv[] = { "rpctest", "registeraccounttx", caddr, fee, height};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			LogPrint("test_miners","RegisterSecureTx:%s\r\n",value.get_str().c_str());
			return true;
		}
		return false;
	}

	bool CallContractTx(const std::string &scriptid, const std::string &addrs, const std::string &contract, const int height)
	{
//		char cscriptid[1024] = { 0 };
//		vector<char> te(scriptid.begin(),scriptid.end());
//		&te[0]
//		strncpy(cscriptid, scriptid.c_str(), sizeof(scriptid)-1);

//		char caddr[1024] = { 0 };
//		strncpy(caddr, addrs.c_str(), sizeof(addrs)-1);

//		char ccontract[128*1024] = { 0 };
//		strncpy(ccontract, contract.c_str(), sizeof(contract)-1);

		char fee[64] = { 0 };
		int nfee = GetRandomFee();
		sprintf(fee, "%d", nfee);
		nCurFee = nfee;

		char height[16] = {0};
		sprintf(height,"%d",height);

		 const char *argv[] = { "rpctest", "CallContractTx", (char *)(scriptid.c_str()), (char *)(addrs.c_str()), (char *)(contract.c_str()), fee, height};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			LogPrint("test_miners","CallContractTx:%s\r\n",value.get_str().c_str());
			return true;
		}
		return false;
	}

	bool DeployContractTx(const std::string &addr, const std::string &script, const int height)
	{
		//CommanRpc
		char caddr[64] = { 0 };
		strncpy(caddr, addr.c_str(), sizeof(caddr)-1);

		char csript[128*1024] = {0};
		strncpy(csript, script.c_str(), sizeof(csript)-1);

		char fee[64] = { 0 };
		int nfee = GetRandomFee();
		sprintf(fee, "%d", nfee);
		nCurFee = nfee;

		char height[16] = {0};
		sprintf(height,"%d",height);


		const char *argv[] = { "rpctest", "deploycontracttx", caddr, csript, fee, height};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret) {
			LogPrint("test_miners","RegisterSecureTx:%s\r\n",value.get_str().c_str());
			return true;
		}

		return false;
	}

	bool CreateSecureTx(const string &scriptid,const vector<string> &obaddrs,
			const vector<string> &addrs,const string&contract,const int height)
	{
		//CommanRpc
		char cscriptid[64] = { 0 };
		strncpy(cscriptid, scriptid.c_str(), sizeof(cscriptid)-1);

		char cobstr[512] = {0};
		{
			Array array;
			array.clear();
			for (const auto &str : obaddrs)
			{
				array.push_back(str);
			}
			string arraystr = write_string(Value(array),false);
			strncpy(cobstr, arraystr.c_str(), sizeof(cobstr)-1);
		}
		char addrstr[512] = {0};
		{
			Array array;
			array.clear();
			for (const auto &str : addrs) {
				array.push_back(str);
			}
			string arraystr = write_string(Value(array), false);
			strncpy(addrstr, arraystr.c_str(), sizeof(addrstr) - 1);
		}

		char ccontract[10*1024] = { 0 };
		strncpy(ccontract, contract.c_str(), sizeof(ccontract)-1);

		char height[16] = {0};
		sprintf(height,"%d",height);


		const char *argv[] = { "rpctest", "createsecuretx", cscriptid,cobstr,addrstr,ccontract,"1000000",height};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			LogPrint("test_miners","CreateSecureTx:%s\r\n",value.get_str().c_str());
			return true;
		}
		return false;
	}

	bool SignSecureTx(const string &securetx)
	{
		//CommanRpc
		char csecuretx[10*1024] = { 0 };
		strncpy(csecuretx, securetx.c_str(), sizeof(csecuretx)-1);


		const char *argv[] = { "rpctest", "signsecuretx", csecuretx};
		int argc = sizeof(argv)/sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			LogPrint("test_miners","SignSecureTx:%s\r\n",value.get_str().c_str());
			return true;
		}
		return false;
	}

	bool IsAllTxInBlock()
	{
		const char *argv[] = { "rpctest", "listunconfirmedtx" };
		int argc = sizeof(argv) / sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret) {
			Array array = value.get_array();
			if(array.size() == 0)
			return true;
		}
		return false;
	}

	bool GetBlockHash(const int height,std::string &blockhash)
	{
		char height[16] = {0};
		sprintf(height,"%d",height);

		const char *argv[] = {"rpctest", "getblockhash",height};
		int argc = sizeof(argv) / sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			blockhash = value.get_str();
			LogPrint("test_miners","GetBlockHash:%s\r\n",blockhash.c_str());
			return true;
		}
		return false;
	}

	bool GetBlockMinerAddr(const std::string &blockhash,std::string &addr)
	{
		char cblockhash[80] = {0};
		strncpy(cblockhash,blockhash.c_str(),sizeof(cblockhash)-1);

		const char *argv[] = {"rpctest", "getblock",cblockhash};
		int argc = sizeof(argv) / sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret)
		{
			Array txs = find_value(value.get_obj(),"tx").get_array();
			addr = find_value(txs[0].get_obj(),"addr").get_str();
			LogPrint("test_miners","GetBlockMinerAddr:%s\r\n",addr.c_str());
			return true;
		}
		return false;
	}

	bool GenerateOneBlock()
	{
		const char *argv[] = {"rpctest", "setgenerate","true"};
		int argc = sizeof(argv) / sizeof(char*);

		Value value;
		int ret = CommandLineRPC_GetValue(argc, argv, value);
		if (!ret) {
			return true;
		}
		return false;
	}

public:
	CMinerTest()
	{
		mapAccState.clear();
		mapAccOperLog.clear();
		nCurHeight = 0;
	}
	bool CheckAccState(const string &addr,AccState &lastState,bool bAccurate = false)
	{
		AccState initState = mapAccState[addr];
		AccOperLog operlog = mapAccOperLog[addr];
		operlog.MergeAcc(nCurHeight);

		for(auto & item:operlog.mapAccState)
		{
			initState += item.second;
		}


		bool b = false;
		if(bAccurate)
		{
			b = (initState==lastState);
		}
		else
		{
			b = lastState.SumEqual(initState);
		}
		return b;
	}
};

BOOST_FIXTURE_TEST_SUITE(miner_tests,CMinerTest)

BOOST_FIXTURE_TEST_CASE(block_normaltx_and_regaccounttx,CMinerTest)
{
	//printf("\r\block_normaltx test start:\r\n");
	string srcaddr;
	string destaddr;
	BOOST_REQUIRE(GetOneAddr(srcaddr,"1100000000000","true"));

	AccState initState;
	BOOST_REQUIRE(GetAccState(srcaddr,initState));
	mapAccState[srcaddr] = initState;//insert

	int height = 0;
	BOOST_REQUIRE(GetBlockHeight(height));
	nCurHeight = height;
	uint64_t totalfee = 0;
	{
		BOOST_REQUIRE(GetNewAddr(destaddr));
		BOOST_REQUIRE(CreateNormalTx(srcaddr,destaddr,height));
		totalfee += nCurFee;
		LogPrint("test_miners","srcaddr:%s\r\ndestaddr:%s\r\nnCurFee:%I64d\r\n",
				srcaddr.c_str(), destaddr.c_str(), nCurFee);
		AccOperLog &operlog1 = mapAccOperLog[srcaddr];
		AccOperLog &operlog2 = mapAccOperLog[destaddr];
		AccState acc1(0,-(nCurMoney+nCurFee),0);
		AccState acc2(0,nCurMoney,0);
		operlog1.Add(height,acc1);
		operlog2.Add(height,acc2);
	}
	BOOST_REQUIRE(GenerateOneBlock());
	height++;

	BOOST_REQUIRE(IsAllTxInBlock());
	string mineraddr;
	string blockhash;
	BOOST_REQUIRE(GetBlockHash(height,blockhash));
	BOOST_REQUIRE(GetBlockMinerAddr(blockhash,mineraddr));

	if(mineraddr == srcaddr)
	{
		AccState acc(totalfee,0,0);
		mapAccOperLog[mineraddr].Add(height,acc);
	}
	else
	{
		BOOST_REQUIRE(GetAccState(mineraddr,initState));
		mapAccState[mineraddr] = initState;//insert
	}

	for(auto & item:mapAccOperLog)
	{
		AccState lastState;
		BOOST_REQUIRE(GetAccState(item.first,lastState));
		BOOST_REQUIRE(CheckAccState(item.first,lastState));
	}

	//test reg account
	{
		nCurHeight = height;
		BOOST_REQUIRE(registeraccounttx(destaddr, height));
		{
			AccOperLog &operlog1 = mapAccOperLog[destaddr];
			AccState acc1(0, -nCurFee, 0);
			operlog1.Add(height,acc1);
		}
		BOOST_REQUIRE(GenerateOneBlock());
		height++;

		BOOST_REQUIRE(IsAllTxInBlock());
		string mineraddr;
		string blockhash;
		BOOST_REQUIRE(GetBlockHash(height,blockhash));
		BOOST_REQUIRE(GetBlockMinerAddr(blockhash,mineraddr));

		if(mineraddr == destaddr)
		{
			AccState acc(nCurFee, 0, 0);
			mapAccOperLog[mineraddr].Add(height,acc);
		}
		else
		{
			BOOST_REQUIRE(GetAccState(mineraddr,initState));
			mapAccState[mineraddr] = initState;//insert
		}

		for(auto & item:mapAccOperLog)
		{
			AccState lastState;
			BOOST_REQUIRE(GetAccState(item.first,lastState));
			BOOST_REQUIRE(CheckAccState(item.first,lastState));
		}
	}
}

BOOST_FIXTURE_TEST_CASE(block_regscripttx_and_contracttx,CMinerTest)
{
	string srcaddr;
	BOOST_REQUIRE(GetOneAddr(srcaddr,"1100000000000","true"));

	AccState initState;
	BOOST_REQUIRE(GetAccState(srcaddr,initState));
	mapAccState[srcaddr] = initState;//insert

	int height = 0;
	BOOST_REQUIRE(GetBlockHeight(height));
	nCurHeight = height;
	//test regscripttx
	{
		string script = "fd3e0102001d000000000022220000000000000000222202011112013512013a75d0007581bf750900750a0f020017250910af08f509400c150a8008f5094002150ad2af222509c582c0e0e50a34ffc583c0e0e509c3958224f910af0885830a858209800885830a858209d2afcef0a3e520f0a37808e608f0a3defaeff0a3e58124fbf8e608f0a3e608f0a30808e608f0a3e608f0a315811581d0e0fed0e0f815811581e8c0e0eec0e022850a83850982e0a3fee0a3f5207808e0a3f608dffae0a3ffe0a3c0e0e0a3c0e0e0a3c0e0e0a3c0e010af0885820985830a800885820985830ad2afd083d0822274f8120042e990fbfef01200087f010200a8c082c083ea90fbfef0eba3f012001202010cd083d0822274f812004274fe12002ceafeebff850982850a83eef0a3eff0aa09ab0a790112013d80ea79010200e80200142200";
		BOOST_REQUIRE(DeployContractTx(srcaddr, script, height));
		AccOperLog &operlog1 = mapAccOperLog[srcaddr];
		AccState acc1(0, -nCurFee, 0);
		operlog1.Add(height,acc1);

		BOOST_REQUIRE(GenerateOneBlock());
		height++;

		BOOST_REQUIRE(IsAllTxInBlock());
		string mineraddr;
		string blockhash;
		BOOST_REQUIRE(GetBlockHash(height,blockhash));
		BOOST_REQUIRE(GetBlockMinerAddr(blockhash,mineraddr));

		if(mineraddr == srcaddr)
		{
			AccState acc(nCurFee, 0, 0);
			mapAccOperLog[mineraddr].Add(height,acc);
		}
		else
		{
			BOOST_REQUIRE(GetAccState(mineraddr,initState));
			mapAccState[mineraddr] = initState;//insert
		}

		for(auto & item:mapAccOperLog)
		{
			AccState lastState;
			BOOST_REQUIRE(GetAccState(item.first,lastState));
			BOOST_REQUIRE(CheckAccState(item.first,lastState));
		}
	}

	//test contracttx
	{
		CRegID scriptId(height, (uint16_t)1);

		string conaddr;
		do {
			BOOST_REQUIRE(GetOneAddr(conaddr, "1100000000000", "true"));
		} while (conaddr == srcaddr);
		string vconaddr = "[\"" + conaddr + "\"] ";

		AccState initState;
		BOOST_REQUIRE(GetAccState(conaddr,initState));
		mapAccState[conaddr] = initState;//insert

		string scriptid;
		BOOST_REQUIRE(GetOneScriptId(scriptid));

		BOOST_REQUIRE(CallContractTx(scriptid, vconaddr, "010203040506070809", height));

		AccOperLog &operlog1 = mapAccOperLog[conaddr];
		AccState acc1(0, -nCurFee, 0);
		operlog1.Add(height,acc1);

		BOOST_REQUIRE(GenerateOneBlock());
		height++;

		BOOST_REQUIRE(IsAllTxInBlock());
		string mineraddr;
		string blockhash;
		BOOST_REQUIRE(GetBlockHash(height,blockhash));
		BOOST_REQUIRE(GetBlockMinerAddr(blockhash,mineraddr));

		if(mineraddr == conaddr)
		{
			AccState acc(nCurFee, 0, 0);
			mapAccOperLog[mineraddr].Add(height,acc);
		}
		else
		{
			BOOST_REQUIRE(GetAccState(mineraddr,initState));
			mapAccState[mineraddr] = initState;//insert
		}

		for(auto & item:mapAccOperLog)
		{
			AccState lastState;
			BOOST_REQUIRE(GetAccState(item.first,lastState));
			BOOST_REQUIRE(CheckAccState(item.first,lastState));
		}
	}
}

BOOST_FIXTURE_TEST_CASE(block_frozentx,CMinerTest)
{
	//printf("\r\nblock_frozentx test start:\r\n");
	string srcaddr;
	BOOST_REQUIRE(GetOneAddr(srcaddr,"1100000000000","true"));

	AccState initState;
	BOOST_REQUIRE(GetAccState(srcaddr,initState));
	mapAccState[srcaddr] = initState;//insert

	int height = 0;
	BOOST_REQUIRE(GetBlockHeight(height));
	nCurHeight = height;
	uint64_t totalfee = 0;
	{
		BOOST_REQUIRE(CreateFreezeTx(srcaddr,height));
		totalfee += nCurFee;
		AccOperLog &operlog1 = mapAccOperLog[srcaddr];
		AccState acc(0,-(nCurMoney+nCurFee),nCurMoney);
		operlog1.Add(height,acc);
	}
	BOOST_REQUIRE(GenerateOneBlock());
	height++;
	BOOST_REQUIRE(IsAllTxInBlock());
	string mineraddr;
	string blockhash;
	BOOST_REQUIRE(GetBlockHash(height,blockhash));
	BOOST_REQUIRE(GetBlockMinerAddr(blockhash,mineraddr));

	if(mineraddr == srcaddr)
	{
		AccState acc(totalfee,0,0);
		mapAccOperLog[mineraddr].Add(height,acc);
	}
	else
	{
		BOOST_REQUIRE(GetAccState(mineraddr,initState));
		mapAccState[mineraddr] = initState;//insert
	}

	for(auto & item:mapAccOperLog)
	{
		AccState lastState;
		BOOST_REQUIRE(GetAccState(item.first,lastState));
		BOOST_REQUIRE(CheckAccState(item.first,lastState));
	}
}



BOOST_AUTO_TEST_SUITE_END()
