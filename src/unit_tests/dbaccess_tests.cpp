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

BOOST_AUTO_TEST_SUITE(dbaccess_tests)


BOOST_AUTO_TEST_CASE(dbaccess_test)
{
    bool isWipe = true;
    shared_ptr<CDBAccess> pDBAccess = make_shared<CDBAccess>(
        DBNameType::ACCOUNT, 100000, false, isWipe);
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


BOOST_AUTO_TEST_SUITE(dbcache_tests)

BOOST_AUTO_TEST_CASE(dbcache_multi_value_Level1_test)
{
    const bool isWipe = true;
    const dbk::PrefixType prefix = dbk::REGID_KEYID;
    shared_ptr<CDBAccess> pDBAccess = make_shared<CDBAccess>(
        DBNameType::ACCOUNT, 100000, false, isWipe);

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
        DBNameType::ACCOUNT, 100000, false, isWipe);

    auto pDBCache1 = make_shared< CCompositeKVCache<prefix, string, string> >(pDBAccess.get());
    auto pDBCache2 = make_shared< CCompositeKVCache<prefix, string, string> >(pDBCache1.get());
    auto pDBCache3 = make_shared< CCompositeKVCache<prefix, string, string> >(pDBCache2.get());
    auto pDbOpLogMap = make_shared<CDBOpLogMap>();
    pDBCache3->SetData("regid-1", "keyid-1", *pDbOpLogMap);
    pDBCache3->SetData("regid-2", "keyid-2", *pDbOpLogMap);
    pDBCache3->SetData("regid-3", "keyid-3", *pDbOpLogMap);
    assert(pDbOpLogMap->GetDbOpLogs(prefix).size() == 3);
    string opKey3, opValue3;
    pDbOpLogMap->GetDbOpLogs(prefix).at(2).Get(opKey3, opValue3);
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
        DBNameType::ACCOUNT, 100000, false, isWipe);

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
