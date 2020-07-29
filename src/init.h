// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_INIT_H
#define COIN_INIT_H

#include <string>
#include <boost/thread/thread.hpp>

using std::string;

class CWallet;

extern CWallet* pWalletMain;

void RequestShutdown();
#define StartShutdown()                                                                            \
    {                                                                                              \
        LogPrint(BCLog::INFO, "Request shutdown by %s()\n", __func__);                             \
        RequestShutdown();                                                                         \
    }

bool ShutdownRequested();
void Shutdown();
bool AppInit(boost::thread_group& threadGroup);
//!Initialize the logging infrastructure
bool InitLogging();
void FinalLogging();
void Interrupt();
string HelpMessage();
void StartCommonGeneration(const int64_t period, const int64_t batchSize);
void StartContractGeneration(const string &regid, const int64_t period, const int64_t batchSize);

#endif
