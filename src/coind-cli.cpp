// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "commons/util.h"
#include "init.h"
#include "rpc/core/rpcclient.h"
#include "rpc/core/rpcprotocol.h"
#include "config/chainparams.h"

#include <boost/filesystem/operations.hpp>

//////////////////////////////////////////////////////////////////////////////
//
// Start
//
static bool AppInitRPC(int argc, char* argv[])
{
    //
    // Parameters
    //
	CBaseParams::InitializeParams(argc, argv);
	SysCfg().InitialConfig();

    if (argc<2 || SysCfg().IsArgCount("-?") || SysCfg().IsArgCount("--help")) {
        // First part of help message is specific to RPC client
        string strUsage = _("Coin Core RPC client version") + " " + FormatFullVersion() + "\n\n" +
            _("Usage:") + "\n" +
              "  coincli [options] <command> [params]  " + _("Send command to Coin Core") + "\n" +
              "  coincli [options] help                " + _("List commands") + "\n" +
              "  coincli [options] help <command>      " + _("Get help for a command") + "\n";

        strUsage += "\n" + HelpMessageCli(true);

        fprintf(stdout, "%s", strUsage.c_str());
        return false;
    }
    return true;
}

int main(int argc, char* argv[])
{
    SetupEnvironment();

    try {
        if(!AppInitRPC(argc, argv))
            return abs(RPC_MISC_ERROR);
    } catch (exception& e) {
        PrintExceptionContinue(&e, "AppInitRPC()");
        return abs(RPC_MISC_ERROR);
    } catch (...) {
        PrintExceptionContinue(NULL, "AppInitRPC()");
        return abs(RPC_MISC_ERROR);
    }

    int ret = abs(RPC_MISC_ERROR);
    try {
        ret = CommandLineRPC(argc, argv);
    } catch (exception& e) {
        PrintExceptionContinue(&e, "CommandLineRPC()");
    } catch (...) {
        PrintExceptionContinue(NULL, "CommandLineRPC()");
    }
    return ret;
}
