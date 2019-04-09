// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_INIT_H
#define COIN_INIT_H

#include <string>
using std::string;
class CWallet;

namespace boost {
    class thread_group;
};

extern CWallet* pwalletMain;

void StartShutdown();
bool ShutdownRequested();
void Shutdown();
bool AppInit(boost::thread_group& threadGroup);
void Interrupt();
string HelpMessage();
void StartGeneration(const int64_t period, const int64_t batchSize);

#endif
