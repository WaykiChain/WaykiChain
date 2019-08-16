// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include "init.h"
#include "main.h"
#include "rpc/core/rpcclient.h"
#include "rpc/core/rpcserver.h"
#include "commons/util.h"

/* Introduction text for doxygen: */

/*! \mainpage Developer documentation
 *
 * \section intro_sec Introduction
 *
 * This is the developer documentation of the reference client for a new crypto currency called WICC
 * (http://www.waykichain.com), which enables instant payments to anyone, anywhere in the world. WICC uses peer-to-peer
 * technology to operate with no central authority: managing transactions and issuing money are carried out collectively
 * by the network.
 *
 * The software is a community-driven open source project, released under the MIT license.
 *
 * \section Navigation
 * Use the buttons <code>Namespaces</code>, <code>Classes</code> or <code>Files</code> at the top of the page to start
 * navigating the code.
 */

static bool fDaemon = false;

void DetectShutdownThread(boost::thread_group* threadGroup) {
    bool fShutdown = ShutdownRequested();
    // Tell the main threads to shutdown.
    while (!fShutdown) {
        MilliSleep(200);
        fShutdown = ShutdownRequested();
    }
    if (threadGroup) {
        threadGroup->interrupt_all();
        threadGroup->join_all();
    }

    Interrupt();
}

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
bool AppInit(int argc, char* argv[], boost::thread_group& threadGroup) {
    //	boost::thread* detectShutdownThread = nullptr;

    bool fRet = false;
    try {
        //
        // Parameters
        //
        // If Qt is used, parameters/coin.conf are parsed in qt/Coin.cpp's main()
        CBaseParams::InitializeParams(argc, argv);
        SysCfg().InitialConfig();

        if (SysCfg().IsArgCount("-?") || SysCfg().IsArgCount("--help")) {
            // First part of help message is specific to coind / RPC client
            std::string strUsage = _("WaykiChain Coin Daemon") + " " + _("version") + " " + FormatFullVersion() +
                                   "\n\n" + _("Usage:") + "\n" + "  coind [options]                     " +
                                   _("Start Coin Core Daemon") + "\n" + _("Usage (deprecated, use Coin-cli):") + "\n" +
                                   "  coin [options] <command> [params]  " + _("Send command to Coin Core") + "\n" +
                                   "  coin [options] help                " + _("List commands") + "\n" +
                                   "  coin [options] help <command>      " + _("Get help for a command") + "\n";

            strUsage += "\n" + HelpMessage();
            strUsage += "\n" + HelpMessageCli(false);

            fprintf(stdout, "%s", strUsage.c_str());
            return false;
        }

        // Command-line RPC
        bool fCommandLine = false;
        for (int i = 1; i < argc; i++)
            if (!IsSwitchChar(argv[i][0]) && !boost::algorithm::istarts_with(argv[i], "Coin:"))
                fCommandLine = true;

        if (fCommandLine) {
            int ret = CommandLineRPC(argc, argv);
            exit(ret);
        }
#ifndef WIN32
        fDaemon = SysCfg().GetBoolArg("-daemon", false);
        if (fDaemon) {
            fprintf(stdout, "Coin server starting\n");

            // Daemonize
            pid_t pid = fork();
            if (pid < 0) {
                fprintf(stderr, "Error: fork() returned %d errno %d\n", pid, errno);
                return false;
            }
            if (pid > 0)  // Parent process, pid is child process id
            {
                CreatePidFile(GetPidFile(), pid);
                return true;
            }
            // Child process falls through to rest of initialization

            pid_t sid = setsid();
            if (sid < 0) fprintf(stderr, "Error: setsid() returned %d errno %d\n", sid, errno);
        }
#endif
        SysCfg().SoftSetBoolArg("-server", true);

        fRet = AppInit(threadGroup);
    } catch (std::exception& e) {
        PrintExceptionContinue(&e, "AppInit()");
    } catch (...) {
        PrintExceptionContinue(nullptr, "AppInit()");
    }

    return fRet;
}
std::tuple<bool, boost::thread*> RunCoin(int argc, char* argv[]) {
    boost::thread* detectShutdownThread = nullptr;
    static boost::thread_group threadGroup;
    SetupEnvironment();

    bool fRet = AppInit(argc, argv, threadGroup);

    detectShutdownThread = new boost::thread(boost::bind(&DetectShutdownThread, &threadGroup));

    if (!fRet) {
        if (detectShutdownThread)
            detectShutdownThread->interrupt();
        Interrupt();
        threadGroup.interrupt_all();

        // threadGroup.join_all(); was left out intentionally here, because we didn't re-test all of
        // the startup-failure cases to make sure they don't result in a hang due to some
        // thread-blocking-waiting-for-another-thread-during-startup case
    }
    return std::make_tuple(fRet, detectShutdownThread);
}

int main(int argc, char* argv[]) {
    std::tuple<bool, boost::thread*> ret = RunCoin(argc, argv);

    boost::thread* detectShutdownThread = std::get<1>(ret);

    bool fRet = std::get<0>(ret);

    if (detectShutdownThread) {
        detectShutdownThread->join();
        delete detectShutdownThread;
        detectShutdownThread = nullptr;
    }

    Shutdown();

#ifndef WIN32
    fDaemon = SysCfg().GetBoolArg("-daemon", false);
#endif

    if (fRet && fDaemon) return 0;

    return (fRet ? 0 : 1);
}
