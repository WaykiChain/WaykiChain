// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include <string>
#include <type_traits>
#include <boost/test/unit_test.hpp>
#include "persistence/dbcache.h"

using namespace std;

BOOST_AUTO_TEST_SUITE(commons_leb128_tests)

void CheckUint32Value(uint32_t val, const string& serializedStr) {
    string msg = "check uint32_t=" + std::to_string(val);
    CFixedUInt32 value1(val);
    CDataStream dsWrite(1, 1);
    dsWrite << value1;
    string serStr = dsWrite.str();
    BOOST_CHECK_MESSAGE(serStr.size() == CFixedUInt32::SIZE && serStr.size() == value1.GetSerializeSize(1, 1),
        msg + " size error");
    BOOST_CHECK_MESSAGE(serStr == ParseHexStr(serializedStr),
        msg + " serialize content error: " + HexStr(serStr) + " vs expected=" + serializedStr);

    CFixedUInt32 value2(~value1.value);
    CDataStream dsRead(serStr, 1, 1);
    dsRead >> value2;
    BOOST_CHECK_MESSAGE(value2 == value1, msg + " unserialize content error: " +
        "v2=" + to_string(value2.value) + " vs v1=" + to_string(value1.value));
}


BOOST_AUTO_TEST_CASE(fixed_uint32_test)
{
    vector<std::pair<uint32_t, string>> values {
        {0,             "0000000000"},
        {1,             "0000000001"},
        {15,            "000000000F"},
        {127,           "000000007F"},
        {128,           "0000000100"},
        {255,           "000000017F"},
        {256,           "0000000200"},
        {0x7FFFFFFF,    "077F7F7F7F"},
        {0x80000000,    "0800000000"},
        {0xFFFFFFFF,    "0F7F7F7F7F"},
    };
    for (size_t i = 0; i < values.size(); i++) {
        if (i > 0) {
            BOOST_CHECK_MESSAGE(values[i].first > values[i-1].first,
                to_string(values[i].first) + " must > " + to_string(values[i-1].first));
            BOOST_CHECK_MESSAGE(values[i].second > values[i-1].second,
                values[i].second + " must > " + values[i-1].second);
        }
        CheckUint32Value(values[i].first, values[i].second);
    }

}

BOOST_AUTO_TEST_SUITE_END()

