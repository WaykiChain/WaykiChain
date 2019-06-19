#ifdef TODO
#include "main.h"
#include <iostream>
#include <boost/test/unit_test.hpp>
#include  "boost/filesystem/operations.hpp"
#include  "boost/filesystem/path.hpp"
using namespace std;
CContractCache *pscriptDBView = NULL;
CContractCache *pTestView = NULL;
CContractDB *pTestDB = NULL;
vector< vector<unsigned char> > arrKey;
vector<unsigned char> vKey1 = {0x01, 0x02, 0x01};
vector<unsigned char> vKey2 = {0x01, 0x02, 0x02};
vector<unsigned char> vKey3 = {0x01, 0x02, 0x03};
vector<unsigned char> vKey4 = {0x01, 0x02, 0x04};
vector<unsigned char> vKey5 = {0x01, 0x02, 0x05};
vector<unsigned char> vKey6 = {0x01, 0x02, 0x06};
vector<unsigned char> vKeyValue = {0x06, 0x07, 0x08};

int nCount = 0;
void init() {
	 pTestDB = new CContractDB("testdb",size_t(4<<20), false , true);
	 pTestView =  new CContractCache(*pTestDB, false);
	 //穷举数据分别位于scriptDBView pTestView, pTestDB 三级分布数据测试数据库中
	 pscriptDBView = new CContractCache(*pTestView, true);

	 arrKey.push_back(vKey1);
	 arrKey.push_back(vKey2);
	 arrKey.push_back(vKey3);
	 arrKey.push_back(vKey4);
	 arrKey.push_back(vKey5);
	 arrKey.push_back(vKey6);
}

void closedb() {

	if (pTestView != NULL) {
		delete pTestView;
		pTestView = NULL;
	}
	if (pTestDB != NULL) {
		delete pTestDB;
		pTestDB = NULL;
	}
	const boost::filesystem::path p=GetDataDir() / "blocks" / "testdb";
	boost::filesystem::remove_all(p);

}
void testscriptdb() {
	vector<unsigned char> vScriptId = {0x01,0x00,0x00,0x00,0x01,0x00};
	vector<unsigned char> vScriptId1 = {0x01,0x00,0x00,0x00,0x02,0x00};
	vector<unsigned char> vScriptContent = {0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01};
	vector<unsigned char> vScriptContent1 = {0x01,0x02,0x03,0x04,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01};
	CRegID regScriptId(vScriptId);
	CRegID regScriptId1(vScriptId1);
	//write script content to db
	BOOST_CHECK(pTestView->SetScript(regScriptId, vScriptContent));
	BOOST_CHECK(pTestView->SetScript(regScriptId1, vScriptContent1));
	//write all data in caches to db
	BOOST_CHECK(pTestView->Flush());
	//test if the script id is exist in db
	BOOST_CHECK(pTestView->HaveScript(regScriptId));
	vector<unsigned char> vScript;
	//read script content from db by scriptId
	BOOST_CHECK(pTestView->GetScript(regScriptId,vScript));
	// if the readed script content equals with original
	BOOST_CHECK(vScriptContent == vScript);
	int nCount;
	//get script numbers from db
	BOOST_CHECK(pTestView->GetScriptCount(nCount));
	//if the number is one
	BOOST_CHECK_EQUAL(nCount, 2);
	//get index 0 script from db
	vScript.clear();
	vector<unsigned char> vId;

	CRegID regId;
	BOOST_CHECK(pTestView->GetScript(0, regId, vScript));
	BOOST_CHECK(vScriptId == regId.GetRegIdRaw());
	BOOST_CHECK(vScriptContent == vScript);
	BOOST_CHECK(pTestView->GetScript(1, regId, vScript));
	BOOST_CHECK(vScriptId1 == regId.GetRegIdRaw());
	BOOST_CHECK(vScriptContent1 == vScript);
	//delete script from db
	BOOST_CHECK(pTestView->EraseScript(regScriptId));
	BOOST_CHECK(pTestView->GetScriptCount(nCount));
	BOOST_CHECK_EQUAL(nCount, 1);
	//write all data in caches to db
	BOOST_CHECK(pTestView->Flush());
}

void settodb(int nType, vector<unsigned char> &vKey, vector<unsigned char> &vScriptData) {
	vector<unsigned char> vScriptId = {0x01,0x00,0x00,0x00,0x02,0x00};
	CDbOpLog operlog;
	CRegID regScriptId(vScriptId);
	if(0==nType) {
		BOOST_CHECK(pscriptDBView->SetContractData(regScriptId, vKey, vScriptData, operlog));
		BOOST_CHECK(pscriptDBView->Flush());
		BOOST_CHECK(pTestView->Flush());
	}else if(1==nType) {
		BOOST_CHECK(pscriptDBView->SetContractData(regScriptId, vKey, vScriptData, operlog));
		BOOST_CHECK(pscriptDBView->Flush());
	}else {
		BOOST_CHECK(pscriptDBView->SetContractData(regScriptId, vKey, vScriptData,  operlog));
	}
}

void cleandb(int nType, vector<unsigned char> vKey) {
	vector<unsigned char> vScriptId = {0x01,0x00,0x00,0x00,0x02,0x00};
	CRegID regScriptId(vScriptId);
	CDbOpLog operlog;
	if(0==nType) {
		BOOST_CHECK(pscriptDBView->EraseAppData(regScriptId, vKey, operlog));
		BOOST_CHECK(pscriptDBView->Flush());
		BOOST_CHECK(pTestView->Flush());
	}else if(1==nType) {
		BOOST_CHECK(pscriptDBView->EraseAppData(regScriptId, vKey, operlog));
		BOOST_CHECK(pscriptDBView->Flush());
	}else {
		BOOST_CHECK(pscriptDBView->EraseAppData(regScriptId, vKey, operlog));
	}
}

void traversaldb(CContractCache *pScriptDB, bool needEqual) {
	assert(pScriptDB!=NULL);
	vector< vector<unsigned char> > traversalKey;
//	int height(0);
	int curheight(0);
	vector<unsigned char> vKey;
	vector<unsigned char> vScript;
//	int nValidHeight(0);
	vector<unsigned char> vScriptId = {0x01,0x00,0x00,0x00,0x02,0x00};
	CRegID regScriptId(vScriptId);
	bool ret = pScriptDB->GetContractData(curheight,regScriptId, 0, vKey, vScript);
//	int nType(0);
	if(ret)  {
		traversalKey.push_back(vKey);
		vector<unsigned char> dataKey = { 'd', 'a', 't', 'a' };
		dataKey.insert(dataKey.end(), regScriptId.GetRegIdRaw().begin(), regScriptId.GetRegIdRaw().end());
		dataKey.push_back('_');
		dataKey.insert(dataKey.end(), vKey.begin(), vKey.end());
		if(pscriptDBView->mapContractDb.count(dataKey)) {
//			nType = 0;
		}else if(pTestView->mapContractDb.count(dataKey)) {
//			nType = 1;
		}else {
//			nType = 2;
		}
//		cout << "script key:" << HexStr(vKey) <<" data at level:"<< nType<< endl;
	}

	while(ret) {
		ret = pScriptDB->GetContractData(curheight,regScriptId, 1, vKey, vScript);
		if(ret) {
			vector<unsigned char> dataKey = { 'd', 'a', 't', 'a' };
			dataKey.insert(dataKey.end(), regScriptId.GetRegIdRaw().begin(), regScriptId.GetRegIdRaw().end());
			dataKey.push_back('_');
			dataKey.insert(dataKey.end(), vKey.begin(), vKey.end());
			if(pscriptDBView->mapContractDb.count(dataKey)) {
//				nType = 0;
			}else if(pTestView->mapContractDb.count(dataKey)) {
//				nType = 1;
			}else {
//				nType = 2;
			}
			traversalKey.push_back(vKey);
//			cout << "script key:" << HexStr(vKey) <<" data at level:"<< nType<< endl;
		}
	}
	if(needEqual)
		BOOST_CHECK(traversalKey == arrKey);
//	cout << "=======================traversaldb end======="<<++nCount<<"==============================="<<endl;
}
void testscriptdatadb() {
	vector<unsigned char> vScriptId = {0x01,0x00,0x00,0x00,0x02,0x00};
//  vector<unsigned char> vScriptKey = {0x01,0x00,0x02,0x03,0x04,0x05,0x06,0x07};
//	vector<unsigned char> vScriptKey1 = {0x01,0x00,0x02,0x03,0x04,0x05,0x07,0x06};
	vector<unsigned char> vScriptKey = {0x01,0x00,0x01};
	vector<unsigned char> vScriptKey1 = {0x01,0x00,0x02};
	vector<unsigned char> vScriptKey2 = {0x01,0x00,0x03};
	vector<unsigned char> vScriptKey3 = {0x01,0x00,0x04};
	vector<unsigned char> vScriptKey4 = {0x01,0x00,0x05};
	vector<unsigned char> vScriptKey5 = {0x01,0x00,0x06};
	vector<unsigned char> vScriptData = {0x01,0x01,0x01,0x01,0x01};
	vector<unsigned char> vScriptData1 = {0x01,0x01,0x01,0x00,0x00};
	CDbOpLog operlog;
	CRegID regScriptId(vScriptId);

	//测试数据库中有vScriptKey1， vScriptKey3，在缓存中有vScriptKey， vScriptKey2，是否能正确遍历脚本数据库
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey1, vScriptData,operlog));
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey3, vScriptData, operlog));
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey5, vScriptData,operlog));
	BOOST_CHECK(pTestView->Flush());
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey, vScriptData,  operlog));
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey2, vScriptData,  operlog));
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey4, vScriptData,  operlog));
	BOOST_CHECK(pTestView->EraseAppData(regScriptId, vScriptKey3, operlog));
	BOOST_CHECK(pTestView->EraseAppData(regScriptId, vScriptKey2, operlog));

	traversaldb(pTestView, false);

	BOOST_CHECK(pTestView->EraseAppData(regScriptId, vScriptKey, operlog));
	BOOST_CHECK(pTestView->EraseAppData(regScriptId, vScriptKey1, operlog));
	BOOST_CHECK(pTestView->EraseAppData(regScriptId, vScriptKey4, operlog));
	BOOST_CHECK(pTestView->EraseAppData(regScriptId, vScriptKey5, operlog));
	BOOST_CHECK(pTestView->Flush());

//	for(int a1=0; a1<3; ++a1) {
//		settodb(a1, vKey1, vKeyValue);
//		for(int a2=0; a2<3; ++a2) {
//			settodb(a2, vKey2, vKeyValue);
//			for(int a3=0; a3<3; ++a3) {
//				settodb(a3, vKey3, vKeyValue);
//				for(int a4=0; a4<3; ++a4) {
//					settodb(a4, vKey4, vKeyValue);
//					for(int a5=0; a5<3; ++a5) {
//						settodb(a5, vKey5, vKeyValue);
//						for(int a6=0; a6<3; ++a6){
//							settodb(a6, vKey6, vKeyValue);
//							traversaldb(pscriptDBView, true);
//							cleandb(a6, vKey6);
//						}
//						cleandb(a5, vKey5);
//					}
//					cleandb(a4, vKey4);
//				}
//				cleandb(a3, vKey3);
//			}
//			cleandb(a2, vKey2);
//		}
//		cleandb(a1, vKey1);
//	}
	for(int i=0; i<5; ++i) {
		for(int ii= i+1; ii<6; ++ii) {
			settodb(0, arrKey[i], vKeyValue);
			settodb(0, arrKey[ii], vKeyValue);
			for(int j=0; j<5; ++j) {
				for(int jj=j+1; jj<6; ++jj) {
					if(i!=j && i!=jj && ii!=j && ii != jj) {
						settodb(1, arrKey[j], vKeyValue);
						settodb(1, arrKey[jj], vKeyValue);
						for(int k=0; k<5; ++k) {
							for(int kk=k+1; kk<6; ++kk) {
								if(k != i && k != ii &&
										kk != i && kk !=ii
										&& k !=j && k != jj
											&& kk != j && kk != jj) {
									settodb(2, arrKey[k], vKeyValue);
									settodb(2, arrKey[kk], vKeyValue);
									traversaldb(pscriptDBView, true);
									cleandb(2, arrKey[k]);
									cleandb(2, arrKey[kk]);
								}
							}
						}
						cleandb(1, arrKey[j]);
						cleandb(1, arrKey[jj]);
						}
					}
				}
			cleandb(0, arrKey[i]);
			cleandb(0, arrKey[ii]);
			}
	}



	int height(0);
	int curheight(0);
	vector<unsigned char> vKey;
	vector<unsigned char> vScript;
	set<CDbOpLog> setOperLog;


	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey, vScriptData,  operlog));
//	int height = 0;
//	int curheight = 0;
	BOOST_CHECK(pTestView->GetContractData(curheight,regScriptId,vScriptKey,vScriptData));
	pTestView->GetScriptCount(height);

	BOOST_CHECK(pTestView->GetContractData(curheight,regScriptId, 0, vScriptKey, vScriptData));
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey, vScriptData, operlog));

	//write script data to db
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey, vScriptData,  operlog));
	//write all data in caches to db
	BOOST_CHECK(pTestView->Flush());
	BOOST_CHECK(pTestView->SetContractData(regScriptId, vScriptKey1, vScriptData1, operlog));
	//test if the script id is exist in db
	BOOST_CHECK(pTestView->HaveScriptData(regScriptId, vScriptKey));
	vScript.clear();


	//read script content from db by appregid
	BOOST_CHECK(pTestView->GetContractData(curheight,regScriptId, vScriptKey, vScript));
	// if the readed script content equals with original
	BOOST_CHECK(vScriptData == vScript);
	int nCount;
	//get app item count from db
	BOOST_CHECK(pTestView->GetContractItemCount(regScriptId, nCount));
	//if the number is one
	BOOST_CHECK_EQUAL(nCount, 2);
	//get index 0 script from db
	vScript.clear();
	vKey.clear();
	BOOST_CHECK(pTestView->GetContractData(curheight,regScriptId, 0, vKey, vScript));
	BOOST_CHECK(vKey == vScriptKey);
	BOOST_CHECK(vScript == vScriptData);
	BOOST_CHECK(pTestView->GetContractData(curheight,regScriptId, 1, vKey, vScript));
	BOOST_CHECK(vKey == vScriptKey1);
	BOOST_CHECK(vScript == vScriptData1);
	//delete script from db
	BOOST_CHECK(pTestView->EraseAppData(regScriptId, vScriptKey, operlog));
	vKey.clear();
	vScript.clear();
	BOOST_CHECK(pTestView->GetContractData(curheight,regScriptId, 0, vKey, vScript));
	BOOST_CHECK(vKey == vScriptKey1);
	BOOST_CHECK(vScript == vScriptData1);
	BOOST_CHECK(pTestView->GetContractItemCount(regScriptId, nCount));
	BOOST_CHECK_EQUAL(nCount, 1);
	//write all data in caches to db
	BOOST_CHECK(pTestView->Flush());
}

BOOST_AUTO_TEST_SUITE(scriptdb_test)
BOOST_AUTO_TEST_CASE(test)
{
//	BOOST_ERROR("THE SUITE NEED TO MODIFY!");
	init();
	testscriptdb();
	testscriptdatadb();
	closedb();
}
BOOST_AUTO_TEST_SUITE_END()
#endif //TODO
