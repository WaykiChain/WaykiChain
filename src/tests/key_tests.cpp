// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifdef TODO
#include "entities/key.h"

#include "commons/base58.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include "crypto/hash.h"
#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>

using namespace std;

//static const string strSecret1C    ("Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw");
//static const string strSecret2C    ("L3Hq7a8FEQwJkW1M2GNKDW28546Vp5miewcCzSqUD9kCAXrJdS3g");
//static const CCoinAddress addr1C("1NoJrossxPBKfCHuJXT4HadJrXRE9Fxiqs");
//static const CCoinAddress addr2C("1CRj2HyM1CXWzHAXLQtiGLyggNT9WQqsDs");

//static const string strSecret1C    ("cPRVcTPYmbwiZaLt4mez6v2a3sHKL3np5pc781BYUTYQEUue1j4K");
//static const string strSecret2C    ("cRNvsxg5RGLnp8UDf86TK4uRZZ2qZjNDFHSiAos3rdvB6SX2VC4G");
//static const CCoinAddress addr1C("n4PwAoA9zQ3SrndjB4wp8QWb9xPpNdtLsW");
//static const CCoinAddress addr2C("mfu2Z1UfUBkWQACSXVDr25h5UVU7LmC3xB");


static const string strAddressBad("1HV9Lc3sNHZxwj4Zk6fB38tEmBryq2cBiF");


#ifdef KEY_TESTS_DUMPINFO
void dumpKeyInfo(uint256 privkey)
{
    CKey key;
    key.resize(32);
    memcpy(&secret[0], &privkey, 32);
    vector<unsigned char> sec;
    sec.resize(32);
    memcpy(&sec[0], &secret[0], 32);
    printf("  * secret (hex): %s\n", HexStr(sec).c_str());

    for (int nCompressed=0; nCompressed<2; nCompressed++)
    {
        bool fCompressed = nCompressed == 1;
        printf("  * %s:\n", fCompressed ? "compressed" : "uncompressed");
        CCoinSecret bsecret;
        bsecret.SetSecret(secret, fCompressed);
        printf("    * secret (base58): %s\n", bsecret.ToString().c_str());
        CKey key;
        key.SetSecret(secret, fCompressed);
        vector<unsigned char> vchPubKey = key.GetPubKey();
        printf("    * pubkey (hex): %s\n", HexStr(vchPubKey).c_str());
        printf("    * address (base58): %s\n", CCoinAddress(vchPubKey).ToString().c_str());
    }
}
#endif


BOOST_AUTO_TEST_SUITE(key_tests)

BOOST_AUTO_TEST_CASE(key_test1)
{

    CCoinSecret bsecret1, bsecret2, bsecret1C, bsecret2C, baddress1;
    string strSecret1C, strSecret2C;

    bool fRegTest = SysCfg().GetBoolArg("-regtest", false);
	bool fTestNet = SysCfg().GetBoolArg("-testnet", false);
	if (fTestNet && fRegTest) {
		fprintf(stderr, "Error: Invalid combination of -regtest and -testnet.\n");
		assert(0);
	}

	if (fRegTest || fTestNet) {
		strSecret1C = string("cSM6sUgD8Fkfpy6hZ9WSkeqhKYAkpudsTa3meyj8AZe1959YyehF");
		strSecret2C = string("cUVvSGPpKxRfuZSYtGFyB4T3gXTCT7tyZgHm5WAiqPQoFB4JVARe");
	} else {
		strSecret1C = string("Kwr371tjA9u2rFSMZjTNun2PXXP3WPZu2afRHTcta6KxEUdm1vEw");
		strSecret2C = string("L3Hq7a8FEQwJkW1M2GNKDW28546Vp5miewcCzSqUD9kCAXrJdS3g");
	}

//    BOOST_CHECK( bsecret1.SetString (strSecret1));
//    BOOST_CHECK( bsecret2.SetString (strSecret2));
    BOOST_CHECK( bsecret1C.SetString(strSecret1C));
    BOOST_CHECK( bsecret2C.SetString(strSecret2C));
    BOOST_CHECK(!baddress1.SetString(strAddressBad));

//    CKey key1  = bsecret1.GetKey();
//    BOOST_CHECK(key1.IsCompressed() == false);
//    CKey key2  = bsecret2.GetKey();
//    BOOST_CHECK(key2.IsCompressed() == false);
    CKey key1C = bsecret1C.GetKey();
    BOOST_CHECK(key1C.IsCompressed() == true);
    CKey key2C = bsecret2C.GetKey();
    BOOST_CHECK(key1C.IsCompressed() == true);

//    CPubKey pubkey1  = key1. GetPubKey();
//    CPubKey pubkey2  = key2. GetPubKey();
    CPubKey pubkey1C = key1C.GetPubKey();
    CPubKey pubkey2C = key2C.GetPubKey();

//    BOOST_CHECK(addr1.Get()  == CTxDestination(pubkey1.GetID()));
//    BOOST_CHECK(addr2.Get()  == CTxDestination(pubkey2.GetID()));
//    BOOST_CHECK(addr1C.Get() == CTxDestination(pubkey1C.GetID()));
//    BOOST_CHECK(addr2C.Get() == CTxDestination(pubkey2C.GetID()));
	if (fRegTest || fTestNet) {
		CCoinAddress addr1Ct("tQCmxQDFQdHmAZw1j3dteB4CTro2Ph5TYP");
		CCoinAddress addr2Ct("t7yXQfSAzsypLJvwnhuehAp3nbKYhv27qW");

		BOOST_CHECK(addr1Ct.Get() == CTxDestination(pubkey1C.GetKeyId()));
		BOOST_CHECK(addr2Ct.Get() == CTxDestination(pubkey2C.GetKeyId()));
	} else {
		CCoinAddress addr1Cm("1NoJrossxPBKfCHuJXT4HadJrXRE9Fxiqs");
		CCoinAddress addr2Cm("1CRj2HyM1CXWzHAXLQtiGLyggNT9WQqsDs");

		BOOST_CHECK(addr1Cm.Get() == CTxDestination(pubkey1C.GetKeyId()));
		BOOST_CHECK(addr2Cm.Get() == CTxDestination(pubkey2C.GetKeyId()));
	}

    for (int n=0; n<16; n++)
    {
        string strMsg = strprintf("Very secret message %i: 11", n);
        uint256 hashMsg = Hash(strMsg.begin(), strMsg.end());

        // normal signatures

//        vector<unsigned char> sign1, sign2, sign1C, sign2C;
        vector<unsigned char> sign1C, sign2C;

//        BOOST_CHECK(key1.Sign (hashMsg, sign1));
//        BOOST_CHECK(key2.Sign (hashMsg, sign2));
        BOOST_CHECK(key1C.Sign(hashMsg, sign1C));
        BOOST_CHECK(key2C.Sign(hashMsg, sign2C));

//        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1));
//        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2));
//        BOOST_CHECK( pubkey1.Verify(hashMsg, sign1C));
//        BOOST_CHECK(!pubkey1.Verify(hashMsg, sign2C));
//
//        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1));
//        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2));
//        BOOST_CHECK(!pubkey2.Verify(hashMsg, sign1C));
//        BOOST_CHECK( pubkey2.Verify(hashMsg, sign2C));

//        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1));
//        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2));
        BOOST_CHECK( pubkey1C.Verify(hashMsg, sign1C));
        BOOST_CHECK(!pubkey1C.Verify(hashMsg, sign2C));

//        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1));
//        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2));
        BOOST_CHECK(!pubkey2C.Verify(hashMsg, sign1C));
        BOOST_CHECK( pubkey2C.Verify(hashMsg, sign2C));

        // compact signatures (with key recovery)

//        vector<unsigned char> csign1, csign2, csign1C, csign2C;
        vector<unsigned char> csign1C, csign2C;

//        BOOST_CHECK(key1.SignCompact (hashMsg, csign1));
//        BOOST_CHECK(key2.SignCompact (hashMsg, csign2));
        BOOST_CHECK(key1C.SignCompact(hashMsg, csign1C));
        BOOST_CHECK(key2C.SignCompact(hashMsg, csign2C));

        CPubKey rkey1, rkey2, rkey1C, rkey2C;

//        BOOST_CHECK(rkey1.RecoverCompact (hashMsg, csign1));
//        BOOST_CHECK(rkey2.RecoverCompact (hashMsg, csign2));
        BOOST_CHECK(rkey1C.RecoverCompact(hashMsg, csign1C));
        BOOST_CHECK(rkey2C.RecoverCompact(hashMsg, csign2C));

//        BOOST_CHECK(rkey1  == pubkey1);
//        BOOST_CHECK(rkey2  == pubkey2);
        BOOST_CHECK(rkey1C == pubkey1C);
        BOOST_CHECK(rkey2C == pubkey2C);
    }

}

BOOST_AUTO_TEST_SUITE_END()

#endif //TODO