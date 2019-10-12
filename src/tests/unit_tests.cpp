// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#define BOOST_TEST_MODULE Unit Test Suite

#include <boost/test/unit_test.hpp>

// unit tests for basic units of coind
struct UnitTestingSetup {
    UnitTestingSetup() { }
    ~UnitTestingSetup() { }


};

BOOST_GLOBAL_FIXTURE(UnitTestingSetup);
