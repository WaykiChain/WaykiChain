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
bool AppInit2(boost::thread_group& threadGroup);

/* The help message mode determines what help message to show */
enum HelpMessageMode
{
    HMM_BITCOIND,
    HMM_BITCOIN_QT
};

string HelpMessage(HelpMessageMode mode);

#endif
