// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2020 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "main.h"

#include <string>
#include <type_traits>
#include <boost/test/unit_test.hpp>
#include "commons/lrucache.hpp"

using namespace std;

BOOST_AUTO_TEST_SUITE(commons_lrucache_tests)

BOOST_AUTO_TEST_CASE(lrucache_test)
{
    typedef CLruCache<std::string, uint32_t> Cache;
    Cache cache(3);
    const auto& queue = cache.GetQueue();
    cache.Insert("1", 1U);
    cache.Insert("2", 2);
    cache.Insert("3", 3);
    BOOST_CHECK(cache.GetSize() == 3);
    BOOST_CHECK(queue.front() == Cache::Item("3", 3));
    BOOST_CHECK(queue.back() == Cache::Item("1", 1));
    cache.Insert("4", 4);
    BOOST_CHECK(cache.GetSize() == 3);
    BOOST_CHECK(queue.front() == Cache::Item("4", 4));
    BOOST_CHECK(queue.back() == Cache::Item("2", 2));

    auto pData = cache.Get("2", true /* touch */);
    BOOST_CHECK(pData != nullptr);
    BOOST_CHECK(pData == &queue.front().second);
    BOOST_CHECK(queue.front() == Cache::Item("2", 2));
    BOOST_CHECK(queue.back() == Cache::Item("3", 3));

    BOOST_CHECK(cache.Get("1") == nullptr);
}

BOOST_AUTO_TEST_SUITE_END()

