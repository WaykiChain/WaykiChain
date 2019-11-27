// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include <string>
#include <vector>
#include <map>
#include <boost/test/unit_test.hpp>
#include "persistence/dbaccess.h"

using namespace std;

static const CRegID id; // to fix the link error: undefined reference to `CRegID ...

struct FDBAccessTests {
    FDBAccessTests() {
        BOOST_TEST_MESSAGE( "setup FDBAccessTests" );
        root_dir = "/tmp/coind_unit_test";
        if (boost::filesystem::exists(root_dir))
            BOOST_CHECK(boost::filesystem::is_directory(root_dir));
        else
            BOOST_CHECK_NO_THROW(boost::filesystem::create_directory(root_dir));

        db_dir = root_dir / "dbaccess_tests";
        BOOST_CHECK_MESSAGE(!boost::filesystem::exists(db_dir), "must remove dir " + db_dir.string() + " first");

        BOOST_CHECK_NO_THROW(boost::filesystem::create_directory(db_dir));
    }
    ~FDBAccessTests() {
        BOOST_TEST_MESSAGE( "teardown FDBAccessTests" );
        BOOST_CHECK_NO_THROW(boost::filesystem::remove_all(db_dir));
    }

    boost::filesystem::path root_dir;
    boost::filesystem::path db_dir;
};

BOOST_FIXTURE_TEST_SUITE(dbaccess_tests, FDBAccessTests)
//BOOST_AUTO_TEST_SUITE(dbaccess_tests)

BOOST_AUTO_TEST_CASE(dbaccess_test)
{
    bool isWipe = true;
    shared_ptr<CDBAccess> pDBAccess = make_shared<CDBAccess>(
        db_dir, DBNameType::ACCOUNT, false, isWipe);
    const dbk::PrefixType prefix = dbk::REGID_KEYID;
    map<string, string> mapData;
    mapData["regid-1"] = "keyid-1";
    mapData["regid-2"] = "keyid-2";
    mapData["regid-3"] = "keyid-3";
    pDBAccess->BatchWrite<string, string>(prefix, mapData);
    string value1;
    BOOST_CHECK(pDBAccess->GetData(prefix, string("regid-1"), value1));
    BOOST_CHECK( value1 == "keyid-1" );
    string value3;
    BOOST_CHECK(pDBAccess->GetData(prefix, string("regid-3"), value3));
    BOOST_CHECK( value3 == "keyid-3" );

}

BOOST_AUTO_TEST_SUITE_END()


BOOST_FIXTURE_TEST_SUITE(dbcache_tests, FDBAccessTests)
//BOOST_AUTO_TEST_SUITE(dbcache_tests)

BOOST_AUTO_TEST_CASE(dbcache_multi_value_Level1_test)
{
    const bool isWipe = true;
    const dbk::PrefixType prefix = dbk::REGID_KEYID;
    shared_ptr<CDBAccess> pDBAccess = make_shared<CDBAccess>(
        db_dir, DBNameType::ACCOUNT, false, isWipe);

    auto pDBCache = make_shared< CCompositeKVCache<prefix, string, string> >(pDBAccess.get());
    pDBCache->SetData("regid-1", "keyid-1");
    pDBCache->SetData("regid-2", "keyid-2");
    pDBCache->SetData("regid-3", "keyid-3");
    pDBCache->Flush();

    string value1;
    BOOST_CHECK(pDBCache->GetData(string("regid-1"), value1));
    BOOST_CHECK( value1 == "keyid-1" );
    string value3;
    BOOST_CHECK(pDBCache->GetData(string("regid-3"), value3));
    BOOST_CHECK( value3 == "keyid-3" );
}


BOOST_AUTO_TEST_CASE(dbcache_multi_value_Level3_test)
{
    const bool isWipe = true;
    const dbk::PrefixType prefix = dbk::REGID_KEYID;
    shared_ptr<CDBAccess> pDBAccess = make_shared<CDBAccess>(
        db_dir, DBNameType::ACCOUNT, false, isWipe);

    auto pDBCache1 = make_shared< CCompositeKVCache<prefix, string, string> >(pDBAccess.get());
    auto pDBCache2 = make_shared< CCompositeKVCache<prefix, string, string> >(pDBCache1.get());
    auto pDBCache3 = make_shared< CCompositeKVCache<prefix, string, string> >(pDBCache2.get());
    auto pDbOpLogMap = make_shared<CDBOpLogMap>();
    pDBCache3->SetDbOpLogMap(pDbOpLogMap.get());
    pDBCache3->SetData("regid-1", "keyid-1");
    pDBCache3->SetData("regid-2", "keyid-2");
    pDBCache3->SetData("regid-3", "keyid-3");
    assert(pDbOpLogMap->GetDbOpLogsPtr(prefix)->size() == 3);
    string opKey3, opValue3;
    pDbOpLogMap->GetDbOpLogsPtr(prefix)->at(2).Get(opKey3, opValue3);
    assert(opKey3 == "regid-3" && opValue3 == "");

    pDBCache3->Flush();
    pDBCache2->Flush();
    pDBCache1->Flush();

    string value1;
    BOOST_CHECK(pDBCache3->GetData(string("regid-1"), value1));
    BOOST_CHECK( value1 == "keyid-1" );
    string value3;
    BOOST_CHECK(pDBCache3->GetData(string("regid-3"), value3));
    BOOST_CHECK( value3 == "keyid-3" );

    string value4;
    BOOST_CHECK(!pDBCache3->GetData(string("regid-4"), value4));
}


BOOST_AUTO_TEST_CASE(dbcache_scalar_value_Level3_test)
{
    const bool isWipe = true;
    const dbk::PrefixType prefix = dbk::REGID_KEYID;
    shared_ptr<CDBAccess> pDBAccess = make_shared<CDBAccess>(
        db_dir, DBNameType::ACCOUNT, false, isWipe);

    auto pDBCache1 = make_shared< CSimpleKVCache<prefix, string> >(pDBAccess.get());
    auto pDBCache2 = make_shared< CSimpleKVCache<prefix, string> >(pDBCache1.get());
    auto pDBCache3 = make_shared< CSimpleKVCache<prefix, string> >(pDBCache2.get());
    pDBCache3->SetData("keyid-1");
    pDBCache3->Flush();
    pDBCache2->Flush();
    pDBCache1->Flush();

    string value1;
    BOOST_CHECK(pDBCache3->GetData(value1));
    BOOST_CHECK( value1 == "keyid-1" );
}

BOOST_AUTO_TEST_SUITE_END()
