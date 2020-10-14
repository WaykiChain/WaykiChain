// Copyright (c) 2017-2020 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "entities/key.h"
#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(pub_key_tests)

BOOST_AUTO_TEST_CASE(pub_key_verify_test)
{
    ECC_Start();
    std::unique_ptr<ECCVerifyHandle> handle = std::make_unique<ECCVerifyHandle>();

    // Randomly generate a private key
    CKey  key;
    key.MakeNewKey();
    BOOST_CHECK(key.IsCompressed() == true);

    // get publicKey from key
    CPubKey pubKey = key.GetPubKey();

    // hash
    string strMsg = "hello world";
    uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

    // sign
    vector<unsigned char> signature;
    BOOST_CHECK(key.SignCompact(hashMsg, signature));

    // get public key from hash and sign
    CPubKey rKey;
    BOOST_CHECK(rKey.RecoverCompact(hashMsg, signature));
    BOOST_CHECK(rKey == pubKey);

    handle.reset();
    ECC_Stop();
}

BOOST_AUTO_TEST_SUITE_END()