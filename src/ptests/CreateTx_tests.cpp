
#include "CreateTx_tests.h"
#include "CycleTestManger.h"


int CCreateTxTest::nCount = 0;

CCreateTxTest::CCreateTxTest():	nTxType(0), nNum(0), nStep(0), sendhash(""), newAddr("") {
	for (int i = 1; i < boost::unit_test::framework::master_test_suite().argc; ++i) {
		string strArgv = boost::unit_test::framework::master_test_suite().argv[i];
		string::size_type pos = strArgv.find("-number=");
		if (string::npos != pos) {
			string strNum = strArgv.substr(pos + strlen("-number="), string::npos);
			nNum = Str2Int(strNum);
			break;
		}
	}
}

void CCreateTxTest::Initialize(){
	CycleTestManger aCycleManager  = CycleTestManger::GetNewInstance();
	vector<std::shared_ptr<CycleTestBase> > vTest;
	if(0 == nNum) { //if don't have input param -number, default create 100 CCreateTxText instance defalue;
		nNum = 100;
	}
	for(int i=0; i< nNum; ++i)
	{
		vTest.push_back(std::make_shared<CCreateTxTest>());
	}
	aCycleManager.Initialize(vTest);
	aCycleManager.Run();
}


TEST_STATE CCreateTxTest::Run() {

	switch (nStep) {
		case 0: {
			if (!vAccount.empty() || SelectAccounts(vAccount))
				++nStep;
			break;
		}
		case 1: {
			if (CreateTx(nTxType)) {
				++nStep;
			}
			if (++nTxType > 3)
				nTxType = 0;
			break;
		}
		case 2: {
			string comfirmedPos;
			if (WaitComfirmed(sendhash, comfirmedPos) == true) {
				IncSentTotal();
				if (++nCount >= nNum) {
					nCount = 0;
					nStep = 0;
					vAccount.clear();
				} else {
					nStep = 1; //�ȴ�ȷ����� ����´����������ǳ�ʼ��
				}
			}
			break;
		}
	}
	return next_state;
}

bool CCreateTxTest::CreateTx(int nTxType)
{
	switch(nTxType) {
	case 0:
	{
		string srcAcct("");
		if(!SelectOneAccount(srcAcct))
			return false;
		string desAcct = srcAcct;
		if (!SelectOneAccount(desAcct, true))
			return false;
		Value value = basetest.CreateNormalTx(srcAcct, desAcct, 100 * COIN);
		if (basetest.GetHashFromCreatedTx(value, sendhash)) {
			cout << "src regId:" << srcAcct<<" create normal tx hash:" << sendhash << endl;
			return true;
		}
		break;
	}
	case 1:
	{
		string srcAcct("");
		if(!SelectOneAccount(srcAcct))
			return false;
		BOOST_CHECK(basetest.GetNewAddr(newAddr, false));
		Value value = basetest.CreateNormalTx(srcAcct, newAddr, 100 * COIN);
		if (basetest.GetHashFromCreatedTx(value, sendhash)) {
			cout << "src regId:" << srcAcct <<" create regist account step 1 tx hash:" << sendhash <<endl;
			return true;
		}

		break;
	}
	case 2:
	{
		Value value = basetest.RegisterAccountTx(newAddr, 100000);
		cout << "register address:" << newAddr << endl;
		if (basetest.GetHashFromCreatedTx(value, sendhash)) {
			cout << "register address:" << newAddr << " create regist account step 2 tx hash:" << sendhash <<endl;
			return true;
		}
		break;
	}
	case 3:
	{
		string regAddress("");
		if(!SelectOneAccount(regAddress))
			return false;
		uint64_t nFee = basetest.GetRandomFee() + 1 * COIN;
		Value value = basetest.DeployContractTx(regAddress, "unit_test.bin", 0, nFee);
		if (basetest.GetHashFromCreatedTx(value, sendhash)) {
			cout << "regid "<< regAddress <<" create regist app tx hash:" << sendhash << endl;
			return true;
		}
		break;
	}
	default:
		break;
	}
	return false;
}

BOOST_FIXTURE_TEST_SUITE(CreateTxTest,CCreateTxTest)

BOOST_FIXTURE_TEST_CASE(Test,CCreateTxTest)
{
	Initialize();
}

BOOST_AUTO_TEST_SUITE_END()
