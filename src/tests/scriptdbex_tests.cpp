#ifdef TODO
#include "main.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include  "boost/filesystem/operations.hpp"
#include  "boost/filesystem/path.hpp"
using namespace std;

class CContractDBTest
{
public:
	CContractDBTest();
	~CContractDBTest();
	void Init();
	void InsertData(CContractDBCache* pViewCache);
	void CheckRecordCount(CContractDBCache* pViewCache,size_t nComparCount);
	void CheckReadData(CContractDBCache* pViewCache);
	void Flush(CContractDBCache* pViewCache1,CContractDBCache* pViewCache2,CContractDBCache* pViewCache3);
	void Flush(CContractDBCache* pViewCache);
	void EraseData(CContractDBCache* pViewCache);
	void GetContractData(CContractDBCache* pViewCache);
protected:
	static const int TEST_SIZE=10000;
	CContractDB* pTestDB;
	CContractDBCache* pTestView;
	map<vector<unsigned char>,vector<unsigned char> > mapScript;
};

CContractDBTest::CContractDBTest() {
	 Init();
}


CContractDBTest::~CContractDBTest() {
	if (pTestView != NULL) {
		delete pTestView;
		pTestView = NULL;
	}
	if (pTestDB != NULL) {
		delete pTestDB;
		pTestDB = NULL;
	}
	const boost::filesystem::path p = GetDataDir() / "blocks" / "testdb";
	boost::filesystem::remove_all(p);
}

void CContractDBTest::Init() {
	pTestDB = new CContractDB("testdb", size_t(4 << 20), false, true);
	pTestView = new CContractDBCache(*pTestDB, false);

	vector<unsigned char> vScriptBase = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
				0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };

	for (int i = 0; i < TEST_SIZE; ++i) {
		CRegID regId(i,1);
		vector<unsigned char> vScriptData(vScriptBase.begin(),vScriptBase.end());
		vector<unsigned char> vRegV6 = regId.GetRegIdRaw();
		vScriptData.insert(vScriptData.end(),vRegV6.begin(),vRegV6.end());
		mapScript.insert(std::make_pair(vRegV6,vScriptData));
	}
}

void CContractDBTest::CheckReadData(CContractDBCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	vector<unsigned char> vScriptContent;
	for (const auto& item : mapScript) {
		CRegID regId(item.first);
		BOOST_CHECK(pViewCache->HaveContract(regId));
		BOOST_CHECK(pViewCache->GetContractScript(regId, vScriptContent));
		BOOST_CHECK(vScriptContent == item.second);
	}
}

void CContractDBTest::InsertData(CContractDBCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	for (const auto& item : mapScript) {
		CRegID regId(item.first);
		BOOST_CHECK(pViewCache->SetContractScript(regId, item.second));
	}
}

void CContractDBTest::CheckRecordCount(CContractDBCache* pViewCache,size_t nComparCount) {
	// BOOST_CHECK(pViewCache);
	// int nCount = 0;
	// if( 0 == nComparCount) {
	// 	BOOST_CHECK(!pViewCache->GetScriptCount(nCount));
	// } else {
	// 	BOOST_CHECK(pViewCache->GetScriptCount(nCount));
	// 	BOOST_CHECK((unsigned int)nCount == nComparCount);
	// }
}

void CContractDBTest::Flush(CContractDBCache* pViewCache1, CContractDBCache* pViewCache2,
		CContractDBCache* pViewCache3) {
	BOOST_CHECK(pViewCache1 && pViewCache2 && pViewCache3);
	BOOST_CHECK(pViewCache1->Flush());
	BOOST_CHECK(pViewCache2->Flush());
	BOOST_CHECK(pViewCache3->Flush());
}

void CContractDBTest::Flush(CContractDBCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	BOOST_CHECK(pViewCache->Flush());
}

void CContractDBTest::EraseData(CContractDBCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	for (const auto& item:mapScript) {
		CRegID regId(item.first);
		BOOST_CHECK(pViewCache->EraseContractScript(regId));
	}
}

void CContractDBTest::GetContractData(CContractDBCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	int nCount = 0;
//	int height = 0;
	int nCurHeight = TEST_SIZE / 2;
	vector<unsigned char> vScriptData;
	vector<unsigned char> vScriptKey;
	set<CDbOpLog> setOperLog;
	auto it = mapScript.begin();
	BOOST_CHECK(it != mapScript.end());
}


BOOST_FIXTURE_TEST_SUITE(scriptdbex_test,CContractDBTest)
BOOST_AUTO_TEST_CASE(add_erase)
{
	CContractDBCache cache2(*pTestView,true);
	CContractDBCache cache3(cache2,true);
	InsertData(&cache3);

	Flush(&cache3,&cache2,pTestView);

	CheckRecordCount(pTestView,mapScript.size());

	CheckReadData(pTestView);

	//test erase data
	EraseData(&cache3);

	Flush(&cache3,&cache2,pTestView);

	CheckRecordCount(pTestView,0);
}

BOOST_AUTO_TEST_CASE(overtime)
{
}

BOOST_AUTO_TEST_SUITE_END()
#endif //TODO
