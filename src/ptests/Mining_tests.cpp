#include "./rpc/core/rpcserver.h"
#include "./rpc/core/rpcclient.h"
#include "commons/util.h"
#include "config/chainparams.h"
#include <boost/test/unit_test.hpp>
#include <boost/assign/list_of.hpp>
#include "tx/tx.h"
using namespace std;
using namespace boost;
using namespace json_spirit;
#include "../test/systestbase.h"
map<string, string> mapDesAddress[] = {
        boost::assign::map_list_of
        ("000000000900",	"dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem")
        ("000000000500",	"dsGb9GyDGYnnHdjSvRfYbj9ox2zPbtgtpo")
        ("000000000300",	"dejEcGCsBkwsZaiUH1hgMapjbJqPdPNV9U")
        ("000000000800",	"e21rEzVwkPFQYfgxcg7xLp7DKeYrW4Fpoz")
        ("000000000700",	"dd936HZcwj9dQkefHPqZpxzUuKZZ2QEsbN")
        ("000000000400",	"dkoEsWuW3aoKaGduFbmjVDbvhmjxFnSbyL")
        ("000000000100",	"dggsWmQ7jH46dgtA5dEZ9bhFSAK1LASALw")
        ("000000000600",	"doym966kgNUKr2M9P7CmjJeZdddqvoU5RZ")
        ("000000000200",	"dtKsuK9HUvLLHtBQL8Psk5fUnTLTFC83GS"),

        boost::assign::map_list_of
        ("010000003300",	"mm8f5877wY4u2WhhX2JtGWPTPKwLbGJi37")
        ("010000002d00",	"n31DG5wjP1GcKyVMupGBvjvweNkM75MPuR")
        ("010000002a00",	"mkeict2uyvmb4Gjx3qXh6vReoTw1A2gkLZ")
        ("010000002600",	"mqwRHqjQZcJBqJP46E256Z2VuqqRAkZkKH")
        ("010000004300",	"mvV1fW4NMv9MGoRwHw583TDi8gnqqjQovZ")
        ("010000003500",	"mzGkgfAkWtkQ4AP2Ut4yiAzCNx6EDzWjW8")
        ("010000002500",	"n3wo9Ts6AUmHdGM1PixpRnLRFrG5G8a5QA")
        ("010000004600",	"mxrmM6qNswgZHmp1u2HTu2soncQLkud7tF")
        ("010000004500",	"mmBBV47uFguukjceTXkPsB3izndht2YXx7"),

        boost::assign::map_list_of
        ("010000003200",	"n3eyjajBMwXiK56ohkzvA2Xu53W9E6jj8K")
        ("010000002900",	"muDs5TAdk6n8rSyLdq6HTzBkJT2XxPF1wP")
        ("010000003700",	"mxZqVtfao3A6dbwymKtn6oE4GacXoJNsac")
        ("010000004000",	"mspw67fn4KGwUrG9oo9mvLJQAPrTYwxQ6w")
        ("010000004700",	"mvNmNnB98GDSeYqg2jH2gSU557XEivs3N5")
        ("010000003a00",	"mjEGztB67nfscqSg5ryUGtzyTGwwEASZeQ")
        ("010000004100",	"mmCvt8WZzF27VGBMWWVkED3vsDRdpnigGV")
        ("010000003100",	"miUVkZNCDaKLTveqT3uWcy8kkpkA94gNvS")
        ("010000004200",	"mxx4MohV2ZfifQiZnmU4yVUVf2QUVM2grx"),

		boost::assign::map_list_of
        ("010000003f00",	"mjuZWVqVQ2cmFoB8pJRj7XWVCPkeoiWJAq")
        ("010000002b00",	"n2tTaaF8xoWWYvaxSDkfQP5GeEcCCsjq1t")
        ("010000002700",	"mw1XUknDsVtb68BUJNj25rKAikYG8qELHJ")
        ("010000002f00",	"mgE3hASaGCRPxJdZruAsydr2ygQz2UBWZM")
        ("010000003400",	"n1vNXyu2GNypJGdZYxzCBCeQFVt1Fd42Qn")
        ("010000003d00",	"n4Cti65cSeufvfxStKUozHNGX3fQSHsDe5")
        ("010000003c00",	"mnZUhyb83ZTQWc9TXXFfhjJEu65q4cFj4S")
        ("010000004800",	"muGiULSeqi2FQ2ypzU7aP8Uu1SWC5kRBki")
        ("010000003900",	"mrMFs4kk8sqZ7iE8DquqPLL8udyGNDUZ8T"),

        boost::assign::map_list_of
        ("010000003800",	"mogX7FTZ9Yuu6gYscKaEf2oxroeRuNDi76")
        ("010000002e00",	"mgs1mDsaXuj16aJ5YMHqLx7xsQ88snsZmB")
        ("010000003b00",	"mzUKrawp7a7LNB7D7kKzKEpgAStsAAHz18")
        ("010000002800",	"miNou7awKXUPN9wbzVP32zTXcWvPsZBpYg")
        ("010000003e00",	"mnnd1QQx2dM5yfp1j8Vp7Dcq7BhiS6bNEQ")
        ("010000004400",	"muS2Nxtva88d45uN6up7WeHszi3oWAcadK")
        ("010000003600",	"mw8yB7Pp7GYiDHhLQT2GNsLc439rfJ3Fai")
        ("010000002c00",	"miRVDrwxtJJh4XnZFnYR6YbdqpAuirVDzZ")
        ("010000003000",	"mvqUh3LR4R7cDWfw4AW7mRUSxfZbvonQ8v")};


int64_t sendValues[] = {1000000000, 2000000000, 3000000000, 4000000000, 5000000000, 6000000000, 7000000000, 8000000000, 9000000000, 10000000000};


void SubmitBlock(vector<string> &param) {
	if(1 != param.size())
			return;
	param.insert(param.begin(), "submitblock");
	param.insert(param.begin(), "rpctest");

	char *argv[param.size()];
	for(size_t i=0; i<param.size(); ++i) {
		argv[i] = const_cast<char *>(param[i].c_str());
		++i;
	}
	CommandLineRPC(param.size(), argv);
}

bool readblock(const string &filePath)
{
	CBlock block;
    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) return false;

    fseek(fp, 8, SEEK_SET); // skip msgheader/size

    CAutoFile filein = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
    if (!filein) return false;
    while(!feof(fp)) {
    	filein >> block;
		CDataStream ds(SER_DISK, CLIENT_VERSION);
		ds << block;
    	vector<string> param;
    	param.push_back(HexStr(ds));
    	SubmitBlock(param);
    }
    return true;
}

class CMiningTest {
public:
	//????????л?????????Block???
	CMiningTest() {

	}
	~CMiningTest() {};

};


class CSendItem:public SysTestBase{
private:
	string m_strRegId;
	string m_strAddress;
	int64_t m_llSendValue;
public:
	CSendItem(){
	};
	CSendItem(const string &strRegId, const string &strDesAddr, const int64_t &llSendValue)
	{
		m_strRegId = strRegId;
		m_strAddress = strDesAddr;
		m_llSendValue = llSendValue;
	}
	void GetContranctData(vector<unsigned char> &vContranct ) {
		//vector<unsigned char> temp = ParseHex(m_strRegId);
		CRegID reg(m_strRegId);
		vContranct.insert(vContranct.end(), reg.GetRegIdRaw().begin(), reg.GetRegIdRaw().end());
		//temp.clear();
		CDataStream ds(SER_DISK, CLIENT_VERSION);
		ds << m_llSendValue;
		vector<unsigned char> temp(ds.begin(), ds.end());
		//string strSendValue = HexStr(temp);
		vContranct.insert(vContranct.end(), temp.begin(), temp.end());
	}
	//index ????Χ1~5?????1~5???????
	static CSendItem GetRandomSendItem(int index) {
		int randAddr = std::rand() % 9;
		int randSendValue = std::rand() % 10;
		map<string, string>::iterator iterAddr = mapDesAddress[index - 1].begin();
		map<string, string>::iterator iterLast = mapDesAddress[index - 1].end();
		--iterLast;
		do {
			iterAddr++;
		} while (--randAddr > 0 && iterAddr != iterLast);
		return CSendItem(iterAddr->first, iterAddr->second, sendValues[randSendValue]);
	}
	string GetRegID() {
		return m_strRegId;
	}

	string GetInvolvedKeyIds() {
		return m_strAddress;
	}

	uint64_t GetSendValue() {
		char cSendValue[12] = {0};
		sprintf(&cSendValue[0], "%ld", m_llSendValue);
		string strSendValue(cSendValue);
		cout << "GetSendValue:" << strSendValue << endl;
		return m_llSendValue;
	}


};

/**
 *构建普通交易
 * @param param
 * param[0]:源地址
 * param[1]:目的地址
 * param[2]:转账金额
 * param[3]:手续费
 * param[4]:有效期高度
 */
void CreateNormalTx(vector<string> &param) {
	if(3 != param.size())
		return;
	param.insert(param.begin(), "sendtoaddress");
	param.insert(param.begin(), "rpctest");
	char *argv[param.size()];
	for(size_t i=0; i<param.size();++i) {
	     argv[i] = const_cast<char *>(param[i].c_str());
	}
	CommandLineRPC(param.size(), argv);
}

/**
 * 构建合约交易
 * @param param
 * param[0]:脚本注册ID
 * param[1]:账户地址列表,json的数组格式
 * param[2]:合约内容
 * param[3]:手续费
 * param[4]:有效期高度
 */
void CallContractTx(vector<string> &param) {
	if(5 != param.size())
		return;
	param.insert(param.begin(), "CallContractTx");
	param.insert(param.begin(), "rpctest");
	char *argv[param.size()];
	for(size_t i=0; i<param.size();++i) {
	     argv[i] = const_cast<char *>(param[i].c_str());
	}
	CommandLineRPC(param.size(), argv);
}


/**
 * 构建注册脚本交易
 * @param param
 * param[0]:注册脚本的账户地址
 * param[1]:注册脚本标识位，0-标识脚本内容的文件路径，1-已注册脚本ID
 * param[2]:文件路径或注册脚本ID
 * param[3]:手续费
 * param[4]:有效期高度
 * param[5]:脚本描述 （针对新注册脚本,可选）
 * param[6]:脚本授权时间 （可选）
 * param[7]:授权脚本每次从账户中扣减金额上限 （可选）
 * param[8]:授权脚本总共扣钱金额上限 （可选）
 * param[9]:授权脚本每天扣钱金额上限 （可选）
 * param[10]:用户自定义数据
 *
 */
void CreateRegScriptTx(vector<string> &param) {
	if(5 > param.size())
		return;
	param.insert(param.begin(), "deploycontracttx");
	param.insert(param.begin(), "rpctest");

	char *argv[param.size()];
	for(size_t i=0; i<param.size();++i) {
	     argv[i] = const_cast<char *>(param[i].c_str());
	}
	CommandLineRPC(param.size(), argv);
}

time_t sleepTime = 500;     //每隔1秒发送一个交易
int64_t llTime = 24*60*60;   //每隔1秒发送一个交易

time_t string2time(const char * str,const char * formatStr)
{
  struct tm tm1;
  int year,mon,mday,hour,min,sec;
  if( -1 == sscanf(str,formatStr,&year,&mon,&mday,&hour,&min,&sec)) return -1;
  tm1.tm_year=year-1900;
  tm1.tm_mon=mon-1;
  tm1.tm_mday=mday;
  tm1.tm_hour=hour;
  tm1.tm_min=min;
  tm1.tm_sec=sec;
  return mktime(&tm1);
}
BOOST_FIXTURE_TEST_SUITE(auto_mining_test, CSendItem)
BOOST_FIXTURE_TEST_CASE(regscript,CSendItem) {
	//注册脚本交易
	SysTestBase::DeployContractTx("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin",0);
}
BOOST_FIXTURE_TEST_CASE(test1, CSendItem)
{
	const char *argv[] = {"progname", "-datadir=D:\\bitcoin\\1"};
	int argc = sizeof(argv) / sizeof(char*);
	CBaseParams::InitializeParams(argc, argv);
//	time_t t1 = string2time("2014-12-01 17:30:00","%d-%d-%d %d:%d:%d");

	Value resulut = DeployContractTx("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin",0);
	string scripthash = "";
	BOOST_CHECK(GetHashFromCreatedTx(resulut,scripthash));
	string scriptid = "";
	BOOST_CHECK(GetTxConfirmedRegID(scripthash,scriptid));

	int64_t runTime = GetTime()+llTime;
	vector<string> param;
	while (GetTime() < runTime) {
		//创建客户端1->客户端2的普通交易
		CSendItem sendItem = CSendItem::GetRandomSendItem(1);
		CSendItem recItem = CSendItem::GetRandomSendItem(2);
		CreateNormalTx(sendItem.GetInvolvedKeyIds(),recItem.GetInvolvedKeyIds(),recItem.GetSendValue());  //创建普通交易
		MilliSleep(sleepTime);

		//创建客户端1->客户端2的合约交易
		CSendItem sendItem1 = CSendItem::GetRandomSendItem(1);

		CallContractTx(scriptid,sendItem1.GetInvolvedKeyIds(),"01",0);  //创建合约交易
		MilliSleep(sleepTime);
	}
}
BOOST_AUTO_TEST_CASE(test2)
{
	const char *argv[] = {"progname", "-datadir=D:\\bitcoin\\2"};
	int argc = sizeof(argv) / sizeof(char*);
	CBaseParams::InitializeParams(argc, argv);
	int64_t runTime = GetTime()+llTime;
	Value resulut = DeployContractTx("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin",0);
	string scripthash = "";
	BOOST_CHECK(GetHashFromCreatedTx(resulut,scripthash));
	string scriptid = "";
	BOOST_CHECK(GetTxConfirmedRegID(scripthash,scriptid));

	while(GetTime()<runTime) {
		//创建客户端2->客户端3的普通交易
		CSendItem sendItem = CSendItem::GetRandomSendItem(2);
		CSendItem recItem = CSendItem::GetRandomSendItem(3);
		CreateNormalTx(sendItem.GetInvolvedKeyIds(),recItem.GetInvolvedKeyIds(),recItem.GetSendValue());
		MilliSleep(sleepTime);

		//创建客户端2->客户端3的合约交易
		CSendItem sendItem1 = CSendItem::GetRandomSendItem(2);
		CallContractTx(scriptid,sendItem1.GetInvolvedKeyIds(),"01",0); //创建合约交易
		MilliSleep(sleepTime);
	}
}

BOOST_AUTO_TEST_CASE(test3)
{
	const char *argv[] = {"progname", "-datadir=D:\\bitcoin\\3"};
	int argc = sizeof(argv) / sizeof(char*);
	CBaseParams::InitializeParams(argc, argv);
	int64_t runTime = GetTime()+llTime;

	Value resulut = DeployContractTx("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin",0);
	string scripthash = "";
	BOOST_CHECK(GetHashFromCreatedTx(resulut,scripthash));
	string scriptid = "";
	BOOST_CHECK(GetTxConfirmedRegID(scripthash,scriptid));
	while(GetTime()<runTime) {
		//创建客户端3->客户端4的普通交易
		CSendItem sendItem = CSendItem::GetRandomSendItem(3);
		CSendItem recItem = CSendItem::GetRandomSendItem(4);
		CreateNormalTx(sendItem.GetInvolvedKeyIds(),recItem.GetInvolvedKeyIds(),recItem.GetSendValue());
		MilliSleep(sleepTime);

		//创建客户端3->客户端4的合约交易
		CSendItem sendItem1 = CSendItem::GetRandomSendItem(3);
		CallContractTx(scriptid,sendItem1.GetInvolvedKeyIds(),"01",0);
		MilliSleep(sleepTime);
	}
}

BOOST_AUTO_TEST_CASE(test4)
{

	const char *argv[] = {"progname", "-datadir=D:\\bitcoin\\4"};
	int argc = sizeof(argv) / sizeof(char*);
	CBaseParams::InitializeParams(argc, argv);
	int64_t runTime = GetTime()+llTime;

	Value resulut = DeployContractTx("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem","unit_test.bin",0);
	string scripthash = "";
	BOOST_CHECK(GetHashFromCreatedTx(resulut,scripthash));
	string scriptid = "";
	BOOST_CHECK(GetTxConfirmedRegID(scripthash,scriptid));
	while(GetTime()<runTime) {
		//创建客户端4->客户端5的普通交易
		CSendItem sendItem = CSendItem::GetRandomSendItem(4);
		CSendItem recItem = CSendItem::GetRandomSendItem(5);
		CreateNormalTx(sendItem.GetInvolvedKeyIds(),recItem.GetInvolvedKeyIds(),recItem.GetSendValue());  //创建普通交易
		MilliSleep(sleepTime);

		//创建客户端4->客户端5的合约交易
		CSendItem sendItem1 = CSendItem::GetRandomSendItem(4);
		CallContractTx(scriptid,sendItem1.GetInvolvedKeyIds(),"01",0);
		MilliSleep(sleepTime);
	}
}
BOOST_AUTO_TEST_CASE(test5)
{
	const char *argv[] = {"progname", "-datadir=D:\\bitcoin\\5"};
	int argc = sizeof(argv) / sizeof(char*);
	CBaseParams::InitializeParams(argc, argv);

	int64_t runTime = GetTime()+llTime;
	Value resulut = DeployContractTx("dsjkLDFfhenmx2JkFMdtJ22TYDvSGgmJem", "unit_test.bin",0);
	string scripthash = "";
	BOOST_CHECK(GetHashFromCreatedTx(resulut,scripthash));
	string scriptid = "";
	BOOST_CHECK(GetTxConfirmedRegID(scripthash,scriptid));
	while(GetTime()<runTime) {
		//创建客户端5->客户端1的普通交易
		CSendItem sendItem = CSendItem::GetRandomSendItem(5);
		CSendItem recItem = CSendItem::GetRandomSendItem(1);
		CreateNormalTx(sendItem.GetInvolvedKeyIds(),recItem.GetInvolvedKeyIds(),recItem.GetSendValue());
		MilliSleep(sleepTime);

		//创建客户端5->客户端1的合约交易
		CSendItem sendItem1 = CSendItem::GetRandomSendItem(5);
		CallContractTx(scriptid,sendItem1.GetInvolvedKeyIds(),"01",0);
		MilliSleep(sleepTime);
	}

}

BOOST_AUTO_TEST_SUITE_END()
