#include <boost/foreach.hpp>
#include <boost/test/unit_test.hpp>
#include "txdb.h"
#include "database.h"
#include <iostream>
#include  "boost/filesystem/operations.hpp"
#include  "boost/filesystem/path.hpp"
using namespace std;

#define VECTOR_SIZE 1000
class CAccountViewTest {
public:
	CAccountViewTest() {
//		pAccountViewDB = new CAccountViewDB("test",500, false, false);
		pViewTip1 = new CAccountViewCache(*pAccountViewTip,true);
		pViewTip2 = new CAccountViewCache(*pViewTip1,true);
		Init();
	}
	~CAccountViewTest() {
//		if(pAccountViewDB != NULL) {
//			delete pAccountViewDB;
//		}
		if(pViewTip2 != NULL) {
			delete pViewTip2;
		}
		if(pViewTip1 != NULL) {
				delete pViewTip1;
			}
//		const boost::filesystem::path p=GetDataDir() / "blocks" / "test";
//		boost::filesystem::remove_all(p);
//		boost::filesystem::remove_all(GetDataDir() / "blocks" / "test");
	}
	bool EraseAccount();
	bool EraseKeyID();
	bool HaveAccount();
	bool CheckKeyMap(bool bCheckExist);
	bool SetKeyID();
	bool TestGetAccount(bool bCheckExist);
	void Init();

public:
	vector<CKeyID> vRandomKeyID;
	vector<CRegID> vRandomRegID;
	vector<CAccount> vAccount;
	CAccountViewCache* pViewTip1;
	CAccountViewCache* pViewTip2;
};

bool CAccountViewTest::EraseKeyID() {
	for (int i = 0; i < VECTOR_SIZE; i++) {
		pViewTip2->EraseId(vRandomRegID.at(i));
	}

	return true;
}

bool CAccountViewTest::EraseAccount() {
	for (int i = 0; i < VECTOR_SIZE; i++) {
		CUserID userId = vRandomKeyID.at(i);
		pViewTip2->EraseAccount(userId);
	}

	return true;
}

bool CAccountViewTest::TestGetAccount(bool bCheckExist) {
	CAccount account;

	//get account by keyID
	for (int i = 0; i < VECTOR_SIZE; i++) {
		CUserID userId = vRandomKeyID.at(i);
		if (bCheckExist) {
			if (!pViewTip2->GetAccount(userId, account)) {
				return false;
			}
		} else {
			if (pViewTip2->GetAccount(userId, account)) {
				return false;
			}
		}
	}

	//get account by accountID
	for (int i = 0; i < VECTOR_SIZE; i++) {
		CUserID userId = vRandomKeyID.at(i);
		if (bCheckExist) {
			if (!pViewTip2->GetAccount(userId, account)) {
				return false;
			}
		} else {
			if (pViewTip2->GetAccount(userId, account)) {
				return false;
			}
		}
	}

	return true;
}

bool CAccountViewTest::SetKeyID() {
	for (int i = 0; i < VECTOR_SIZE; i++) {
		pViewTip2->SetKeyId(vRandomRegID.at(i), vRandomKeyID.at(i));
	}

	return true;
}

void CAccountViewTest::Init() {
	vRandomKeyID.reserve(VECTOR_SIZE);
	vRandomRegID.reserve(VECTOR_SIZE);
	vAccount.reserve(VECTOR_SIZE);

	for (int i = 0; i < VECTOR_SIZE; i++) {
		CKey key;
		key.MakeNewKey();
		CPubKey pubkey = key.GetPubKey();
		CKeyID keyID = pubkey.GetKeyID();
		vRandomKeyID.push_back(keyID);
	}

	for (int j = 0; j < VECTOR_SIZE; j++) {
		CRegID accountId(10000 + j, j);
		vRandomRegID.push_back(accountId);
	}

	for (int k = 0; k < VECTOR_SIZE; k++) {
		CAccount account;
		account.llValues = k + 1;
		account.keyID = vRandomKeyID.at(k);
		vAccount.push_back(account);
	}
}

bool CAccountViewTest::CheckKeyMap(bool bCheckExist) {
	CKeyID keyID;
	for (int i = 0; i < VECTOR_SIZE; i++) {
		if (bCheckExist) {
			if (!pViewTip2->GetKeyId(vRandomRegID.at(i), keyID)) {
				return false;
			}
		} else {
			if (pViewTip2->GetKeyId(vRandomRegID.at(i), keyID)) {
				return false;
			}
		}
	}

	return true;
}

bool CAccountViewTest::HaveAccount() {
	for (int i = 0; i < VECTOR_SIZE; i++) {
		CUserID userId = vRandomKeyID.at(i);
		if (pViewTip2->HaveAccount(userId))
			return false;
	}

	return true;
}
BOOST_FIXTURE_TEST_SUITE(accountview_tests,CAccountViewTest)



BOOST_FIXTURE_TEST_CASE(regid_test,CAccountViewTest)
{
	BOOST_CHECK(pViewTip2);
	BOOST_CHECK(CheckKeyMap(false) );
	BOOST_CHECK(SetKeyID() );
	BOOST_CHECK(HaveAccount() );

	BOOST_CHECK(CheckKeyMap(true) );
	BOOST_CHECK(pViewTip2->Flush() );
	BOOST_CHECK(CheckKeyMap(true) );

	EraseKeyID();
	BOOST_CHECK(pViewTip2->Flush() );
	BOOST_CHECK(CheckKeyMap(false) );
}

BOOST_FIXTURE_TEST_CASE(setaccount_test1,CAccountViewTest)
{
	BOOST_CHECK(SetKeyID() );
	for (int i = 0; i < VECTOR_SIZE; i++) {
		CUserID userId = vRandomKeyID.at(i);
		BOOST_CHECK(pViewTip2->SetAccount(userId, vAccount.at(i)) );
	}

	BOOST_CHECK(TestGetAccount(true) );
	BOOST_CHECK(pViewTip2->Flush() );
	BOOST_CHECK(TestGetAccount(true) );

	EraseAccount();
	EraseKeyID();
	BOOST_CHECK(pViewTip2->Flush() );
	BOOST_CHECK(TestGetAccount(false) );
}

BOOST_FIXTURE_TEST_CASE(setaccount_test2,CAccountViewTest)
{
	BOOST_CHECK(SetKeyID());
	for (int i = 0; i < VECTOR_SIZE; i++) {
		CUserID userId = vRandomRegID.at(i);
		BOOST_CHECK(pViewTip2->SetAccount(userId, vAccount.at(i)));
	}

	BOOST_CHECK(TestGetAccount(true));
	BOOST_CHECK(pViewTip2->Flush());
	BOOST_CHECK(TestGetAccount(true));

	EraseAccount();
	EraseKeyID();
	BOOST_CHECK(pViewTip2->Flush());
	BOOST_CHECK(TestGetAccount(false));
}

BOOST_FIXTURE_TEST_CASE(BatchWrite_test,CAccountViewTest)
{
	BOOST_CHECK(SetKeyID() );
	pViewTip2->BatchWrite(vAccount);

	BOOST_CHECK(TestGetAccount(true));
	BOOST_CHECK(pViewTip2->Flush());
	BOOST_CHECK(TestGetAccount(true));

	EraseAccount();
	EraseKeyID();
	BOOST_CHECK(pViewTip2->Flush());
	BOOST_CHECK(TestGetAccount(false));
}


BOOST_AUTO_TEST_SUITE_END()
