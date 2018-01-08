#include "main.h"
#include "txdb.h"
#include "database.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include  "boost/filesystem/operations.hpp"
#include  "boost/filesystem/path.hpp"
using namespace std;

class CScriptDBTest
{
public:
	CScriptDBTest();
	~CScriptDBTest();
	void Init();
	void InsertData(CScriptDBViewCache* pViewCache);
	void CheckRecordCount(CScriptDBViewCache* pViewCache,size_t nComparCount);
	void CheckReadData(CScriptDBViewCache* pViewCache);
	void Flush(CScriptDBViewCache* pViewCache1,CScriptDBViewCache* pViewCache2,CScriptDBViewCache* pViewCache3);
	void Flush(CScriptDBViewCache* pViewCache);
	void EraseData(CScriptDBViewCache* pViewCache);
	void GetScriptData(CScriptDBViewCache* pViewCache);
protected:
	static const int TEST_SIZE=10000;
	CScriptDB* pTestDB;
	CScriptDBViewCache* pTestView;
	map<vector<unsigned char>,vector<unsigned char> > mapScript;
};

CScriptDBTest::CScriptDBTest() {
	 Init();
}


CScriptDBTest::~CScriptDBTest() {
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

void CScriptDBTest::Init() {
	pTestDB = new CScriptDB("testdb", size_t(4 << 20), false, true);
	pTestView = new CScriptDBViewCache(*pTestDB, false);

	vector<unsigned char> vScriptBase = { 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
				0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };

	for (int i = 0; i < TEST_SIZE; ++i) {
		CRegID regID(i,1);
		vector<unsigned char> vScriptData(vScriptBase.begin(),vScriptBase.end());
		vector<unsigned char> vRegV6 = regID.GetVec6();
		vScriptData.insert(vScriptData.end(),vRegV6.begin(),vRegV6.end());
		mapScript.insert(std::make_pair(vRegV6,vScriptData));
	}
}

void CScriptDBTest::CheckReadData(CScriptDBViewCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	vector<unsigned char> vScriptContent;
	for (const auto& item : mapScript) {
		CRegID regID(item.first);
		BOOST_CHECK(pViewCache->HaveScript(regID));
		BOOST_CHECK(pViewCache->GetScript(regID, vScriptContent));
		BOOST_CHECK(vScriptContent == item.second);
	}
}

void CScriptDBTest::InsertData(CScriptDBViewCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	for (const auto& item : mapScript) {
		CRegID regID(item.first);
		BOOST_CHECK(pViewCache->SetScript(regID, item.second));
	}
}

void CScriptDBTest::CheckRecordCount(CScriptDBViewCache* pViewCache,size_t nComparCount) {
	BOOST_CHECK(pViewCache);
	int nCount = 0;
	if( 0 == nComparCount) {
		BOOST_CHECK(!pViewCache->GetScriptCount(nCount));
	} else {
		BOOST_CHECK(pViewCache->GetScriptCount(nCount));
		BOOST_CHECK((unsigned int)nCount == nComparCount);
	}
}

void CScriptDBTest::Flush(CScriptDBViewCache* pViewCache1, CScriptDBViewCache* pViewCache2,
		CScriptDBViewCache* pViewCache3) {
	BOOST_CHECK(pViewCache1 && pViewCache2 && pViewCache3);
	BOOST_CHECK(pViewCache1->Flush());
	BOOST_CHECK(pViewCache2->Flush());
	BOOST_CHECK(pViewCache3->Flush());
}

void CScriptDBTest::Flush(CScriptDBViewCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	BOOST_CHECK(pViewCache->Flush());
}

void CScriptDBTest::EraseData(CScriptDBViewCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	for (const auto& item:mapScript) {
		CRegID regID(item.first);
		BOOST_CHECK(pViewCache->EraseScript(regID));
	}
}

void CScriptDBTest::GetScriptData(CScriptDBViewCache* pViewCache) {
	BOOST_CHECK(pViewCache);
	int nCount = 0;
//	int nHeight = 0;
	int nCurHeight = TEST_SIZE / 2;
	vector<unsigned char> vScriptData;
	vector<unsigned char> vScriptKey;
	set<CScriptDBOperLog> setOperLog;
	auto it = mapScript.begin();
	BOOST_CHECK(it != mapScript.end());

	BOOST_CHECK(pViewCache->GetScriptDataCount(CRegID(it->first), nCount));
	bool ret = pViewCache->GetScriptData(nCurHeight, CRegID(it->first), 0, vScriptKey, vScriptData);

	while (ret) {
		if (++it == mapScript.end()) {
			break;
		}
		ret = pViewCache->GetScriptData(nCurHeight, CRegID(it->first), 1, vScriptKey, vScriptData);
		pViewCache->GetScriptDataCount(CRegID(it->first), nCount);
	}
}


BOOST_FIXTURE_TEST_SUITE(scriptdbex_test,CScriptDBTest)
BOOST_AUTO_TEST_CASE(add_erase)
{
	CScriptDBViewCache cache2(*pTestView,true);
	CScriptDBViewCache cache3(cache2,true);
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
//	CScriptDBViewCache cache2(*pTestView,true);
//	CScriptDBViewCache cache3(cache2,true);
//	InsertData(&cache3);
//
//	Flush(&cache3,&cache2,pTestView);
//
//	GetScriptData(&cache3);
//
//	Flush(&cache3,&cache2,pTestView);
//
//	CheckRecordCount(pTestView,TEST_SIZE/2);
}

BOOST_AUTO_TEST_SUITE_END()









