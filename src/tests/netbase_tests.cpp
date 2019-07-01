// Copyright (c) 2012-2013 The Bitcoin Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "netbase.h"

#include <string>

#include <boost/test/unit_test.hpp>

using namespace std;

BOOST_AUTO_TEST_SUITE(netbase_tests)



BOOST_AUTO_TEST_CASE(netbase_networks)
{
	BOOST_CHECK(CNetAddr("127.0.0.1").GetNetwork() == NET_UNROUTABLE);
	BOOST_CHECK(CNetAddr("::1").GetNetwork() == NET_UNROUTABLE);
	BOOST_CHECK(CNetAddr("8.8.8.8").GetNetwork() == NET_IPV4);
	BOOST_CHECK(CNetAddr("2001::8888").GetNetwork() == NET_IPV6);
	BOOST_CHECK(CNetAddr("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").GetNetwork() == NET_TOR);
}

BOOST_AUTO_TEST_CASE(netbase_properties)
{
	BOOST_CHECK(CNetAddr("127.0.0.1").IsIPv4());
	BOOST_CHECK(CNetAddr("::FFFF:192.168.1.1").IsIPv4());
	BOOST_CHECK(CNetAddr("::1").IsIPv6());
	BOOST_CHECK(CNetAddr("10.0.0.1").IsRFC1918());
	BOOST_CHECK(CNetAddr("192.168.1.1").IsRFC1918());
	BOOST_CHECK(CNetAddr("172.31.255.255").IsRFC1918());
	BOOST_CHECK(CNetAddr("2001:0DB8::").IsRFC3849());
	BOOST_CHECK(CNetAddr("169.254.1.1").IsRFC3927());
	BOOST_CHECK(CNetAddr("2002::1").IsRFC3964());
	BOOST_CHECK(CNetAddr("FC00::").IsRFC4193());
	BOOST_CHECK(CNetAddr("2001::2").IsRFC4380());
	BOOST_CHECK(CNetAddr("2001:10::").IsRFC4843());
	BOOST_CHECK(CNetAddr("FE80::").IsRFC4862());
	BOOST_CHECK(CNetAddr("64:FF9B::").IsRFC6052());
	BOOST_CHECK(CNetAddr("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca").IsTor());
	BOOST_CHECK(CNetAddr("127.0.0.1").IsLocal());
	BOOST_CHECK(CNetAddr("::1").IsLocal());
	BOOST_CHECK(CNetAddr("8.8.8.8").IsRoutable());
	BOOST_CHECK(CNetAddr("2001::1").IsRoutable());
	BOOST_CHECK(CNetAddr("127.0.0.1").IsValid());
}

bool static TestSplitHost(string test, string host, int port)
{
	string hostOut;
	int portOut = -1;
	SplitHostPort(test, portOut, hostOut);
	return hostOut == host && port == portOut;
}

BOOST_AUTO_TEST_CASE(netbase_splithost)
{
	BOOST_CHECK(TestSplitHost("www.bitcoin.org", "www.bitcoin.org", -1));
	BOOST_CHECK(TestSplitHost("[www.bitcoin.org]", "www.bitcoin.org", -1));
	BOOST_CHECK(TestSplitHost("www.bitcoin.org:80", "www.bitcoin.org", 80));
	BOOST_CHECK(TestSplitHost("[www.bitcoin.org]:80", "www.bitcoin.org", 80));
	BOOST_CHECK(TestSplitHost("127.0.0.1", "127.0.0.1", -1));
	BOOST_CHECK(TestSplitHost("127.0.0.1:8333", "127.0.0.1", 8333));
	BOOST_CHECK(TestSplitHost("[127.0.0.1]", "127.0.0.1", -1));
	BOOST_CHECK(TestSplitHost("[127.0.0.1]:8333", "127.0.0.1", 8333));
	BOOST_CHECK(TestSplitHost("::ffff:127.0.0.1", "::ffff:127.0.0.1", -1));
	BOOST_CHECK(TestSplitHost("[::ffff:127.0.0.1]:8333", "::ffff:127.0.0.1", 8333));
	BOOST_CHECK(TestSplitHost("[::]:8333", "::", 8333));
	BOOST_CHECK(TestSplitHost("::8333", "::8333", -1));
	BOOST_CHECK(TestSplitHost(":8333", "", 8333));
	BOOST_CHECK(TestSplitHost("[]:8333", "", 8333));
	BOOST_CHECK(TestSplitHost("", "", -1));
}

bool static TestParse(string src, string canon)
{
	CService addr;
	if (!LookupNumeric(src.c_str(), addr, 65535))
	return canon == "";
	return canon == addr.ToString();
}

BOOST_AUTO_TEST_CASE(netbase_lookupnumeric)
{
	BOOST_CHECK(TestParse("127.0.0.1", "127.0.0.1:65535"));
	BOOST_CHECK(TestParse("127.0.0.1:8333", "127.0.0.1:8333"));
	BOOST_CHECK(TestParse("::ffff:127.0.0.1", "127.0.0.1:65535"));
	BOOST_CHECK(TestParse("::", "[::]:65535"));
	BOOST_CHECK(TestParse("[::]:8333", "[::]:8333"));
	BOOST_CHECK(TestParse("[127.0.0.1]", "127.0.0.1:65535"));
	BOOST_CHECK(TestParse(":::", ""));
}

BOOST_AUTO_TEST_CASE(onioncat_test)
{
	// values from https://web.archive.org/web/20121122003543/http://www.cypherpunk.at/onioncat/wiki/OnionCat
	CNetAddr addr1("5wyqrzbvrdsumnok.onion");
	CNetAddr addr2("FD87:D87E:EB43:edb1:8e4:3588:e546:35ca");
	BOOST_CHECK(addr1 == addr2);
	BOOST_CHECK(addr1.IsTor());
	BOOST_CHECK(addr1.ToStringIP() == "5wyqrzbvrdsumnok.onion");
	BOOST_CHECK(addr1.IsRoutable());
}



BOOST_AUTO_TEST_SUITE_END()
