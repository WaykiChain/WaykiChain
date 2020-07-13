// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#if defined(HAVE_CONFIG_H)
#include "config/coin-config.h"
#endif

#include "logging.h"
#include "init.h"
#include "config/configuration.h"
#include "p2p/addrman.h"

#include "rpc/core/rpcserver.h"
#include "vm/luavm/lua/lua.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "main.h"
#include "miner/miner.h"
#include "net.h"
#include "p2p/node.h"
#include "persistence/blockdb.h"
#include "persistence/accountdb.h"
#include "persistence/txdb.h"
#include "persistence/contractdb.h"
#include "tx/tx.h"
#include "commons/util/util.h"
#include "commons/util/time.h"
#ifdef USE_UPNP
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/miniwget.h>
#include <miniupnpc/upnpcommands.h>
#include <miniupnpc/upnperrors.h>

//#include "vm/wasm/wasm_interface.hpp"

#ifndef MINIUPNPC_VERSION
#define MINIUPNPC_VERSION "1.9"
#endif

#ifndef MINIUPNPC_API_VERSION
#define MINIUPNPC_API_VERSION 10
#endif

#endif

#include <stdint.h>
#include <stdio.h>

#ifndef WIN32
#include <signal.h>
#endif

#include <openssl/crypto.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/assign/list_of.hpp>

#include "wasm/modules/wasm_native_dispatch.hpp"

using namespace std;
using namespace boost::assign;
using namespace boost;

#define USE_LUA 1

CWallet *pWalletMain;

static std::unique_ptr<ECCVerifyHandle> globalVerifyHandle;

extern void wasm_code_cache_free();
//extern void wasm_load_native_modules_and_register_routes();

#ifdef WIN32
// Win32 LevelDB doesn't use filedescriptors, and the ones used for
// accessing block files, don't count towards to fd_set size limit
// anyway.
#define MIN_CORE_FILEDESCRIPTORS 0
#else
#define MIN_CORE_FILEDESCRIPTORS 150
#endif

// Used to pass flags to the Bind() function
enum BindFlags {
    BF_NONE         = 0,
    BF_EXPLICIT     = (1U << 0),
    BF_REPORT_ERROR = (1U << 1)
};

//////////////////////////////////////////////////////////////////////////////
//
// Shutdown
//

//
// Thread management and startup/shutdown:
//
// The network-processing threads are all part of a thread group
// created by AppInit() or the Qt main() function.
//
// A clean exit happens when StartShutdown() or the SIGTERM
// signal handler sets fRequestShutdown, which triggers
// the DetectShutdownThread(), which interrupts the main thread group.
// DetectShutdownThread() then exits, which causes AppInit() to
// continue (it .joins the shutdown thread).
// Shutdown() is then
// called to clean up database connections, and stop other
// threads that should only be stopped after the main network-processing
// threads have exited.
//
// Note that if running -daemon the parent process returns from AppInit
// before adding any threads to the threadGroup, so .join_all() returns
// immediately and the parent exits from main().
//
// Shutdown for Qt is very similar, only it uses a QTimer to detect
// fRequestShutdown getting set, and then does the normal Qt
// shutdown thing.
//

volatile bool fRequestShutdown = false;

void StartShutdown() { fRequestShutdown = true; }

bool ShutdownRequested() { return fRequestShutdown; }

void Interrupt() { InterruptRPCServer(); }

void Shutdown() {
    LogPrint(BCLog::INFO, "Shutdown() : In progress...\n");
    static CCriticalSection cs_Shutdown;
    TRY_LOCK(cs_Shutdown, lockShutdown);
    if (!lockShutdown)
        return;

    RenameThread("coin-shutoff");

    StopRPCServer();

    GenerateProduceBlockThread(false, nullptr, 0);
    StartCommonGeneration(0, 0);
    StartContractGeneration("", 0, 0);

    StopNode();
    UnregisterNodeSignals(GetNodeSignals());

    {
        LOCK(cs_main);

        if (pWalletMain) {
            pWalletMain->SetBestChain(chainActive.GetLocator());
            bitdb.Flush(true);
        }

        if (pCdMan != nullptr) {
            pCdMan->Flush();
            delete pCdMan;
            pCdMan = nullptr;
        }
    }

    boost::filesystem::remove(GetPidFile());
    UnregisterAllWallets();

    delete pWalletMain;

    // Uninitialize elliptic curve code
    globalVerifyHandle.reset();
    ECC_Stop();

    wasm_code_cache_free();

    LogPrint(BCLog::INFO, "Shutdown() : done\n");
}

//
// Signal handlers are very limited in what they are allowed to do, so:
//
void HandleSIGTERM(int32_t) {
    fRequestShutdown = true;
}

void HandleSIGHUP(int32_t) {
    fReopenDebugLog = true;
}

bool static InitError(const string &str) {
    LogPrint(BCLog::ERROR, "%s\n", str);
    fprintf(stderr, "Initialize error: %s\n", str.c_str());
    return false;
}

bool static Bind(const CService &addr, uint32_t flags) {
    if (!(flags & BF_EXPLICIT) && IsLimited(addr))
        return false;
    string strError;
    if (!BindListenPort(addr, strError)) {
        if (flags & BF_REPORT_ERROR)
            return InitError(strError);
        return false;
    }
    return true;
}

// Core-specific options shared between UI, daemon and RPC client
string HelpMessage() {
    string strUsage = _("Options:") + "\n";
    strUsage += "  -?                     " + _("This help message") + "\n";
    strUsage += "  -alertnotify=<cmd>     " + _("Execute command when a relevant alert is received or we see a really long fork (%s in cmd is replaced by message)") + "\n";
    strUsage += "  -blocknotify=<cmd>     " + _("Execute command when the best block changes (%s in cmd is replaced by block hash)") + "\n";
    strUsage += "  -checkblocks=<n>       " + _("How many blocks to check at startup (default: 288, 0 = all)") + "\n";
    strUsage += "  -checklevel=<n>        " + _("How thorough the block verification of -checkblocks is (0-4, default: 3)") + "\n";
    strUsage += "  -conf=<file>           " + _("Specify configuration file (default: ") + IniCfg().GetCoinName() + ".conf)" + "\n";
#if !defined(WIN32)
    strUsage += "  -daemon                " + _("Run in the background as a daemon and accept commands") + "\n";
#endif
    strUsage += "  -datadir=<dir>         " + _("Specify data directory") + "\n";
    strUsage += "  -dbcache=<n>           " + strprintf(_("Set database cache size in megabytes (%d to %d, default: %d)"), MIN_DB_CACHE, MAX_DB_CACHE, DEFAULT_DB_CACHE) + "\n";
    strUsage += "  -loadblock=<file>      " + _("Imports blocks from external blk000??.dat file") + " " + _("on startup") + "\n";
    strUsage += "  -pid=<file>            " + _("Specify pid file (default: coin.pid)") + "\n";
    strUsage += "  -reindex               " + _("Rebuild block chain index from current blk000??.dat files") + " " + _("on startup") + "\n";
    strUsage += "  -txindex               " + _("Maintain a full transaction index (default: 0)") + "\n";
    strUsage += "  -txtrace               " + _("Maintain trace of transaction (default: 1)") + "\n";
    strUsage += "  -logfailures           " + _("Log failures into level db in detail (default: 0)") + "\n";
    strUsage += "  -genreceipt            " + _("Whether generate receipt(default: 0)") + "\n";

    strUsage += "\n" + _("Connection options:") + "\n";
    strUsage += "  -addnode=<ip>          " + _("Add a node to connect to and attempt to keep the connection open") + "\n";
    strUsage += "  -banscore=<n>          " + _("Threshold for disconnecting misbehaving peers (default: 100)") + "\n";
    strUsage += "  -bantime=<n>           " + _("Number of seconds to keep misbehaving peers from reconnecting (default: 86400)") + "\n";
    strUsage += "  -bind=<addr>           " + _("Bind to given address and always listen on it. Use [host]:port notation for IPv6") + "\n";
    strUsage += "  -connect=<ip>          " + _("Connect only to the specified node(s)") + "\n";
    strUsage += "  -discover              " + _("Discover own IP address (default: 1 when listening and no -externalip)") + "\n";
    strUsage += "  -dns                   " + _("Allow DNS lookups for -addnode, -seednode and -connect") + " " + _("(default: 1)") + "\n";
    strUsage += "  -dnsseed               " + _("Query for peer addresses via DNS lookup, if low on addresses (default: 1 unless -connect)") + "\n";
    strUsage += "  -forcednsseed          " + _("Always query for peer addresses via DNS lookup (default: 0)") + "\n";
    strUsage += "  -externalip=<ip>       " + _("Specify your own public address") + "\n";
    strUsage += "  -listen                " + _("Accept connections from outside (default: 1 if no -proxy or -connect)") + "\n";
    strUsage += "  -maxconnections=<n>    " + _("Maintain at most <n> connections to peers (default: 125)") + "\n";
    strUsage += "  -maxreceivebuffer=<n>  " + _("Maximum per-connection receive buffer, <n>*1000 bytes (default: 5000)") + "\n";
    strUsage += "  -maxsendbuffer=<n>     " + _("Maximum per-connection send buffer, <n>*1000 bytes (default: 1000)") + "\n";
    strUsage += "  -onion=<ip:port>       " + _("Use separate SOCKS5 proxy to reach peers via Tor hidden services (default: -proxy)") + "\n";
    strUsage += "  -onlynet=<net>         " + _("Only connect to nodes in network <net> (IPv4, IPv6 or Tor)") + "\n";
    strUsage += "  -port=<port>           " + _("Listen for connections on <port> (default: 8333 or testnet: 18333)") + "\n";
    strUsage += "  -proxy=<ip:port>       " + _("Connect through SOCKS proxy") + "\n";
    strUsage += "  -ipserver=<server>     " + _("IP Reporting Service") + "\n";
    strUsage += "  -seednode=<ip>         " + _("Connect to a node to retrieve peer addresses, and disconnect") + "\n";
    strUsage += "  -socks=<n>             " + _("Select SOCKS version for -proxy (4 or 5, default: 5)") + "\n";
    strUsage += "  -timeout=<n>           " + _("Specify connection timeout in milliseconds (default: 5000)") + "\n";
#ifdef USE_UPNP
#if USE_UPNP
    strUsage += "  -upnp                  " + _("Use UPnP to map the listening port (default: 1 when listening)") + "\n";
#else
    strUsage += "  -upnp                  " + _("Use UPnP to map the listening port (default: 0)") + "\n";
#endif
#endif

#ifdef ENABLE_WALLET
    strUsage += "\n" + _("Wallet options:") + "\n";
    strUsage += "  -disablewallet         " + _("Do not load the wallet and disable wallet RPC calls") + "\n";
    strUsage += "  -genblock              " + _("Generate blocks (default: 0)") + "\n";
    strUsage += "  -genblocklimit=<n>     " + _("Set the processor limit for when generation is on (-1 = unlimited, default: -1)") + "\n";
    strUsage += "  -keypool=<n>           " + _("Set key pool size to <n> (default: 100)") + "\n";
    strUsage += "  -paytxfee=<amt>        " + _("Fee per kB to add to transactions you send") + "\n";
    strUsage += "  -rescan                " + _("Rescan the block chain for missing wallet transactions") + " " + _("on startup") + "\n";
    strUsage += "  -salvagewallet         " + _("Attempt to recover private keys from a corrupt wallet.dat") + " " + _("on startup") + "\n";
    strUsage += "  -spendzeroconfchange   " + _("Spend unconfirmed change when sending transactions (default: 1)") + "\n";
    strUsage += "  -upgradewallet         " + _("Upgrade wallet to latest format") + " " + _("on startup") + "\n";
    strUsage += "  -wallet=<file>         " + _("Specify wallet file (within data directory)") + " " + _("(default: wallet.dat)") + "\n";
    strUsage += "  -walletnotify=<cmd>    " + _("Execute command when a wallet transaction changes (%s in cmd is replaced by TxID)") + "\n";
#endif

    strUsage += "\n" + _("Debugging/Testing options:") + "\n";
    if (SysCfg().GetBoolArg("-help-debug", false)) {
        strUsage += "  -benchmark             " + _("Show benchmark information (default: 0)") + "\n";
        strUsage += "  -dblogsize=<n>         " + _("Flush database activity from memory pool to disk log every <n> megabytes (default: 100)") + "\n";
        strUsage += "  -disablesafemode       " + _("Disable safemode, override a real safe mode event (default: 0)") + "\n";
        strUsage += "  -testsafemode          " + _("Force safe mode (default: 0)") + "\n";
        strUsage += "  -dropmessagestest=<n>  " + _("Randomly drop 1 of every <n> network messages") + "\n";
        strUsage += "  -fuzzmessagestest=<n>  " + _("Randomly fuzz 1 of every <n> network messages") + "\n";
        strUsage += "  -flushwallet           " + _("Run a thread to flush wallet periodically (default: 1)") + "\n";
    }
    strUsage += "  -debug=<category>      " + _("Output debugging information (default: 0, supplying <category> is optional)") + "\n";
    strUsage += "                         " + _("If <category> is not supplied, output all debugging information.") + "\n";
    strUsage += "                         " + _("<category> can be:");
    strUsage += " addrman, alert, coindb, db, lock, rand, rpc, selectcoins, mempool, net";
    strUsage += "  -help-debug            " + _("Show all debugging options (usage: --help -help-debug)") + "\n";
    strUsage += "  -logtimestamps         " + _("Prepend debug output with timestamp (default: 1)") + "\n";
    if (SysCfg().GetBoolArg("-help-debug", false)) {
        strUsage += "  -limitfreerelay=<n>    " + _("Continuously rate-limit free transactions to <n>*1000 bytes per minute (default:15)") + "\n";
        strUsage += "  -maxsigcachesize=<n>   " + _("Limit size of signature cache to <n> entries (default: 50000)") + "\n";
    }
    strUsage += "  -logprinttoconsole     " + _("Send trace/debug info to console instead of debug.log file") + "\n";
    if (SysCfg().GetBoolArg("-help-debug", false)) {
        strUsage += "  -printblock=<hash>     " + _("Print block on startup, if found in block index") + "\n";
        strUsage += "  -printblocktree        " + _("Print block tree on startup (default: 0)") + "\n";
        strUsage += "  -printpriority         " + _("Log transaction priority and fee per kB when mining blocks (default: 0)") + "\n";
        strUsage += "  -privdb                " + _("Sets the DB_PRIVATE flag in the wallet db environment (default: 1)") + "\n";
        strUsage += "  -regtest               " + _("Enter regression test mode, which uses a special chain in which blocks can be solved instantly.") + "\n";
        strUsage += "                         " + _("This is intended for regression testing tools and app development.") + "\n";
        strUsage += "                         " + _("In this mode -genblocklimit controls how many blocks are generated immediately.") + "\n";
    }
    strUsage += "  -shrinkdebugfile       " + _("Shrink debug.log file on client startup (default: 1 when no -debug)") + "\n";
    strUsage += "  -nettype=<network>     " + _("Specify network type: main/test/regtest (default: main)") + "\n";

    strUsage += "\n" + _("Block creation options:") + "\n";
    strUsage += "  -blockmaxsize=<n>      " + strprintf(_("Set maximum block size in bytes (default: %d)"), DEFAULT_BLOCK_MAX_SIZE) + "\n";

    strUsage += "\n" + _("RPC server options:") + "\n";
    strUsage += "  -rpcserver             " + _("Accept command line and JSON-RPC commands") + "\n";
    strUsage += "  -rpcuser=<user>        " + _("Username for JSON-RPC connections") + "\n";
    strUsage += "  -rpcpassword=<pw>      " + _("Password for JSON-RPC connections") + "\n";
    strUsage += "  -rpcport=<port>        " + _("Listen for JSON-RPC connections on <port> (default: 8332 or testnet: 18332)") + "\n";
    strUsage += "  -rpcallowip=<ip>       " + _("Allow JSON-RPC connections from specified IP address") + "\n";
    strUsage += "  -rpcthreads=<n>        " + _("Set the number of threads to service RPC calls (default: 4)") + "\n";

    strUsage += "\n" + _("RPC SSL options: (see the Coin Wiki for SSL setup instructions)") + "\n";
    strUsage += "  -rpcssl                                  " + _("Use OpenSSL (https) for JSON-RPC connections") + "\n";
    strUsage += "  -rpcsslcertificatechainfile=<file.cert>  " + _("Server certificate file (default: server.cert)") + "\n";
    strUsage += "  -rpcsslprivatekeyfile=<file.pem>         " + _("Server private key (default: server.pem)") + "\n";
    strUsage += "  -rpcsslciphers=<ciphers>                 " + _("Acceptable ciphers (default: TLSv1.2+HIGH:TLSv1+HIGH:!SSLv2:!aNULL:!eNULL:!3DES:@STRENGTH)") + "\n";

    strUsage += "  -rpcwhitelistcmd=<method>          " + _("Add permitted RPC method to whitelist") + "\n";
    strUsage += "  -rpcblacklistcmd=<method>          " + _("Add Banned RPC method to blacklist") + "\n";
    return strUsage;
}

struct CImportingNow {
    CImportingNow() {
        assert(SysCfg().IsImporting() == false);
        SysCfg().SetImporting(true);
    }

    ~CImportingNow() {
        assert(SysCfg().IsImporting() == true);
        SysCfg().SetImporting(false);
    }
};

void ThreadImport(vector<boost::filesystem::path> vImportFiles) {
    RenameThread("coin-loadblk");

    // -reindex
    if (SysCfg().IsReindex()) {

        CImportingNow imp;
        int32_t nFile = 0;
        while (true) {
            CDiskBlockPos pos(nFile, 0);
            FILE *file = OpenBlockFile(pos, true);
            if (!file)
                break;

            LogPrint(BCLog::INFO, "Reindexing block file blk%05u.dat...\n", (uint32_t)nFile);
            LoadExternalBlockFile(file, &pos);
            nFile++;
        }
        pCdMan->pBlockCache->WriteReindexing(false);
        SysCfg().SetReIndex(false);
        LogPrint(BCLog::INFO, "Reindexing finished\n");
        // To avoid ending up in a situation without genesis block, re-try initializing (no-op if reindexing worked):
        InitBlockIndex();
        pWalletMain->ResendWalletTransactions();
    }

    // hardcoded $DATADIR/bootstrap.dat
    filesystem::path pathBootstrap = GetDataDir() / "bootstrap.dat";
    if (filesystem::exists(pathBootstrap)) {
        FILE *file = fopen(pathBootstrap.string().c_str(), "rb");
        if (file) {
            CImportingNow imp;
            filesystem::path pathBootstrapOld = GetDataDir() / "bootstrap.dat.old";
            LogPrint(BCLog::INFO, "Importing bootstrap.dat...\n");
            LoadExternalBlockFile(file);
            RenameOver(pathBootstrap, pathBootstrapOld);
        } else {
            LogPrint(BCLog::INFO, "Warning: Could not open bootstrap file %s\n", pathBootstrap.string());
        }
    }

    // -loadblock=
    for (const auto &path : vImportFiles) {
        FILE *file = fopen(path.string().c_str(), "rb");
        if (file) {
            CImportingNow imp;
            LogPrint(BCLog::INFO, "Importing blocks file %s...\n", path.string());
            LoadExternalBlockFile(file);
        } else {
            LogPrint(BCLog::INFO, "Warning: Could not open blocks file %s\n", path.string());
        }
    }
}

/** Initialize native_modules
 *  register routes for native tx(inline_transaction) dispatch
 */
void wasm_load_native_modules_and_register_routes() {
    auto& modules = get_wasm_native_modules();
    if(modules.size() == 0){
        modules.push_back(std::make_shared<wasm_native_module>());
        modules.push_back(std::make_shared<wasm_bank_native_module>());
    }

    auto& abi_router = get_wasm_abi_route();
    auto& act_router = get_wasm_act_route();

    for(auto& module: modules){
        module->register_routes(abi_router, act_router);
    }

}

/** Initialize Coin.
 *  @pre Parameters should be parsed and config file should be read.
 */
bool AppInit(boost::thread_group &threadGroup) {
#ifdef _MSC_VER
    // Turn off Microsoft heap dump noise
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, CreateFileA("NUL", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, 0));
#endif
#if _MSC_VER >= 1400
    // Disable confusing "helpful" text message on abort, Ctrl-C
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
#ifdef WIN32
    // Enable Data Execution Prevention (DEP)
    // Minimum supported OS versions: WinXP SP3, WinVista >= SP1, Win Server 2008
    // A failure is non-critical and needs no further attention!
#ifndef PROCESS_DEP_ENABLE
    // We define this here, because GCCs winbase.h limits this to _WIN32_WINNT >= 0x0601 (Windows 7),
    // which is not correct. Can be removed, when GCCs winbase.h is fixed!
#define PROCESS_DEP_ENABLE 0x00000001
#endif
    typedef BOOL(WINAPI * PSETPROCDEPPOL)(DWORD);
    PSETPROCDEPPOL setProcDEPPol = (PSETPROCDEPPOL)GetProcAddress(GetModuleHandleA("Kernel32.dll"), "SetProcessDEPPolicy");
    if (setProcDEPPol != nullptr) setProcDEPPol(PROCESS_DEP_ENABLE);

    // Initialize Windows Sockets
    WSADATA wsadata;
    int32_t ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
    if (ret != NO_ERROR || LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2)
        return InitError(strprintf("Error: Winsock library failed to start (WSAStartup returned error %d)", ret));

#endif
#ifndef WIN32
    umask(077);

    // Clean shutdown on SIGTERM
    struct sigaction sa;
    sa.sa_handler = HandleSIGTERM;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGINT, &sa, nullptr);

    // Reopen debug.log on SIGHUP
    struct sigaction sa_hup;
    sa_hup.sa_handler = HandleSIGHUP;
    sigemptyset(&sa_hup.sa_mask);
    sa_hup.sa_flags = 0;
    sigaction(SIGHUP, &sa_hup, nullptr);

    // Initialize elliptic curve code
    ECC_Start();
    globalVerifyHandle.reset(new ECCVerifyHandle());

    // Sanity check
    if (!ECC_InitSanityCheck())
        return fprintf(stderr, "Elliptic curve cryptography sanity check failure. Aborting.");

    //register wasm routes for native inline_transaction dispatch
    wasm_load_native_modules_and_register_routes();

#if defined(__SVR4) && defined(__sun)
    // ignore SIGPIPE on Solaris
    signal(SIGPIPE, SIG_IGN);
#endif
#endif

    if (SysCfg().IsArgCount("-bind")) {
        // when specifying an explicit binding address, you want to listen on it
        // even when -connect or -proxy is specified
        if (SysCfg().SoftSetBoolArg("-listen", true))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -bind set -> setting -listen=1\n");
    }

    if (SysCfg().IsArgCount("-connect") && SysCfg().GetMultiArgs("-connect").size() > 0) {
        // when only connecting to trusted nodes, do not seed via DNS, or listen by default
        if (SysCfg().SoftSetBoolArg("-dnsseed", false))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -connect set -> setting -dnsseed=0\n");

        if (SysCfg().SoftSetBoolArg("-listen", false))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -connect set -> setting -listen=0\n");
    }

    if (SysCfg().IsArgCount("-proxy")) {
        // to protect privacy, do not listen by default if a default proxy server is specified
        if (SysCfg().SoftSetBoolArg("-listen", false))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -proxy set -> setting -listen=0\n");
    }

    if (!SysCfg().GetBoolArg("-listen", true)) {
        // do not map ports or try to retrieve public IP when not listening (pointless)
        if (SysCfg().SoftSetBoolArg("-upnp", false))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -listen=0 -> setting -upnp=0\n");

        if (SysCfg().SoftSetBoolArg("-discover", false))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -listen=0 -> setting -discover=0\n");
    }

    if (SysCfg().IsArgCount("-externalip")) {
        // if an explicit public IP is specified, do not try to find others
        if (SysCfg().SoftSetBoolArg("-discover", false))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -externalip set -> setting -discover=0\n");
    }

    if (SysCfg().GetBoolArg("-salvagewallet", false)) {
        // Rewrite just private keys: rescan to find transactions
        if (SysCfg().SoftSetBoolArg("-rescan", true))
            LogPrint(BCLog::INFO, "AppInit : parameter interaction: -salvagewallet=1 -> setting -rescan=1\n");
    }

    SysCfg().SetTxTrace(SysCfg().GetBoolArg("-txtrace", true));

    // Make sure enough file descriptors are available
    int32_t nBind   = max((int32_t)SysCfg().IsArgCount("-bind"), 1);
    nMaxConnections = SysCfg().GetArg("-maxconnections", 125);
    nMaxConnections = max(min(nMaxConnections, (int32_t)(FD_SETSIZE - nBind - MIN_CORE_FILEDESCRIPTORS)), 0);
    int32_t nFD     = RaiseFileDescriptorLimit(nMaxConnections + MIN_CORE_FILEDESCRIPTORS);
    if (nFD < MIN_CORE_FILEDESCRIPTORS)
        return InitError(_("Not enough file descriptors available."));

    if (nFD - MIN_CORE_FILEDESCRIPTORS < nMaxConnections)
        nMaxConnections = nFD - MIN_CORE_FILEDESCRIPTORS;

    SysCfg().SetBenchMark(SysCfg().GetBoolArg("-benchmark", false));
    mempool.SetSanityCheck(SysCfg().GetBoolArg("-checkmempool", RegTest()));

    setvbuf(stdout, nullptr, _IOLBF, 0);

    string strDataDir = GetDataDir().string();

    // Make sure only a single Coin process is using the data directory.
    boost::filesystem::path pathLockFile = GetDataDir() / ".lock";
    FILE *file                           = fopen(pathLockFile.string().c_str(), "a");  // empty lock file; created if it doesn't exist.
    if (file) {
        fclose(file);
    }
    static boost::interprocess::file_lock lock(pathLockFile.string().c_str());
    if (!lock.try_lock()) {
        return InitError(strprintf(_("Cannot obtain a lock on data directory %s. coin Core is probably already running."), strDataDir));
    }

    LogPrint(BCLog::INFO, "Default data directory %s\n", GetDefaultDataDir().string());
    LogPrint(BCLog::INFO, "Using data directory %s\n", GetDataDir().string());

    LogPrint(BCLog::INFO, "%s version %s (%s)\n", IniCfg().GetCoinName().c_str(), FormatFullVersion().c_str(), CLIENT_DATE);
    LogPrint(BCLog::INFO, "Using OpenSSL version %s\n", SSLeay_version(SSLEAY_VERSION));
#ifdef USE_LUA
    LogPrint(BCLog::INFO, "Using Lua version %s\n", LUA_RELEASE);
#endif
    string boost_version = BOOST_LIB_VERSION;
    StringReplace(boost_version, "_", ".");
    LogPrint(BCLog::INFO, "Using Boost version %s\n", boost_version);
    string leveldb_version = strprintf("%d.%d", leveldb::kMajorVersion, leveldb::kMinorVersion);
    LogPrint(BCLog::INFO, "Using Level DB version %s\n", leveldb_version);
    LogPrint(BCLog::INFO, "Using Berkeley DB version %s\n", DB_VERSION_STRING);

#ifdef USE_UPNP
    LogPrint(BCLog::INFO, "Using miniupnpc version %s,API version %d\n", MINIUPNPC_VERSION, MINIUPNPC_API_VERSION);
#endif
    LogPrint(BCLog::INFO, "Startup time: %s\n", DateTimeStrFormat("%Y-%m-%d %H:%M:%S", GetTime()));
    LogPrint(BCLog::INFO, "Default data directory %s\n", GetDefaultDataDir().string());
    LogPrint(BCLog::INFO, "Using data directory %s\n", strDataDir);
    LogPrint(BCLog::INFO, "Using at most %i connections (%i file descriptors available)\n", nMaxConnections, nFD);

    RegisterNodeSignals(GetNodeSignals());

    int32_t nSocksVersion = SysCfg().GetArg("-socks", 5);
    if (nSocksVersion != 4 && nSocksVersion != 5) {
        return InitError(strprintf(_("Unknown -socks proxy version requested: %i"), nSocksVersion));
    }

    if (SysCfg().IsArgCount("-onlynet")) {
        set<enum Network> nets;
        vector<string> tmp = SysCfg().GetMultiArgs("-onlynet");
        for (auto &snet : tmp) {
            enum Network net = ParseNetwork(snet);
            if (net == NET_UNROUTABLE) {
                return InitError(strprintf(_("Unknown network specified in -onlynet: '%s'"), snet));
            }
            nets.insert(net);
        }
        for (int32_t n = 0; n < NET_MAX; n++) {
            enum Network net = (enum Network)n;
            if (!nets.count(net)) {
                SetLimited(net);
            }
        }
    }

    CService addrProxy;
    bool fProxy = false;
    if (SysCfg().IsArgCount("-proxy")) {
        addrProxy = CService(SysCfg().GetArg("-proxy", ""), 9050);
        if (!addrProxy.IsValid()) {
            return InitError(strprintf(_("Invalid -proxy address: '%s'"), SysCfg().GetArg("-proxy", "")));
        }

        if (!IsLimited(NET_IPV4)) {
            SetProxy(NET_IPV4, addrProxy, nSocksVersion);
        }
        if (nSocksVersion > 4) {
            if (!IsLimited(NET_IPV6)) {
                SetProxy(NET_IPV6, addrProxy, nSocksVersion);
            }
            SetNameProxy(addrProxy, nSocksVersion);
        }
        fProxy = true;
    }

    // -onion can override normal proxy, -noonion disables tor entirely
    // -tor here is a temporary backwards compatibility measure
    if (SysCfg().IsArgCount("-tor")) {
        LogPrint(BCLog::INFO, "Notice: option -tor has been replaced with -onion and will be removed in a later version.\n");
    }
    if (!(SysCfg().GetArg("-onion", "") == "0") && !(SysCfg().GetArg("-tor", "") == "0") && (fProxy || SysCfg().IsArgCount("-onion") || SysCfg().IsArgCount("-tor"))) {
        CService addrOnion;
        if (!SysCfg().IsArgCount("-onion") && !SysCfg().IsArgCount("-tor")) {
            addrOnion = addrProxy;
        } else {
            addrOnion = SysCfg().IsArgCount("-onion") ? CService(SysCfg().GetArg("-onion", ""), 9050) : CService(SysCfg().GetArg("-tor", ""), 9050);
        }

        if (!addrOnion.IsValid()) {
            return InitError(strprintf(_("Invalid -onion address: '%s'"), SysCfg().IsArgCount("-onion") ? SysCfg().GetArg("-onion", "") : SysCfg().GetArg("-tor", "")));
        }
        SetProxy(NET_TOR, addrOnion, 5);
        SetReachable(NET_TOR);
    }

    fNoListen   = !SysCfg().GetBoolArg("-listen", true);
    fDiscover   = SysCfg().GetBoolArg("-discover", true);
    fNameLookup = SysCfg().GetBoolArg("-dns", true);

    bool fBound = false;
    if (!fNoListen) {
        if (SysCfg().IsArgCount("-bind")) {
            vector<string> tmp = SysCfg().GetMultiArgs("-bind");
            for (const auto &strBind : tmp) {
                CService addrBind;
                if (!Lookup(strBind.c_str(), addrBind, GetListenPort(), false)) {
                    return InitError(strprintf(_("Cannot resolve -bind address: '%s'"), strBind));
                }
                fBound |= Bind(addrBind, (BF_EXPLICIT | BF_REPORT_ERROR));
            }
        } else {
            struct in_addr inaddr_any;
            inaddr_any.s_addr = INADDR_ANY;
            fBound |= Bind(CService(in6addr_any, GetListenPort()), BF_NONE);
            fBound |= Bind(CService(inaddr_any, GetListenPort()), !fBound ? BF_REPORT_ERROR : BF_NONE);
        }
        if (!fBound) {
            return InitError(_("Failed to listen on any port. Use -listen=0 if you want this."));
        }
    }

    if (SysCfg().IsArgCount("-externalip")) {
        vector<string> tmp = SysCfg().GetMultiArgs("-externalip");
        for (const auto &strAddr : tmp) {
            CService addrLocal(strAddr, GetListenPort(), fNameLookup);
            if (!addrLocal.IsValid()) {
                return InitError(strprintf(_("Cannot resolve -externalip address: '%s'"), strAddr));
            }
            AddLocal(CService(strAddr, GetListenPort(), fNameLookup), LOCAL_MANUAL);
        }
    }

    {
        vector<string> tmp = SysCfg().GetMultiArgs("-seednode");
        for (auto strDest : tmp) {
            AddOneShot(strDest);
        }
    }

    SysCfg().SetReIndex(SysCfg().GetBoolArg("-reindex", false));

    SysCfg().SetLogFailures(SysCfg().GetBoolArg("-logfailures", false));

    SysCfg().SetGenReceipt(SysCfg().GetBoolArg("-genreceipt", false));

    filesystem::path blocksDir = GetDataDir() / "blocks";
    if (!filesystem::exists(blocksDir)) {
        filesystem::create_directories(blocksDir);
    }

    try {
        pWalletMain = CWallet::GetInstance();
        RegisterWallet(pWalletMain);
        pWalletMain->LoadWallet(false);
    } catch (std::exception &e) {
        std::cout << "load wallet failed: " << e.what() << std::endl;
    }

    int64_t nStart = GetTimeMillis();
    bool fLoaded   = false;
    while (!fLoaded) {
        bool fReset = SysCfg().IsReindex();
        string strLoadError;

        do {
            try {
                UnloadBlockIndex();
                delete pCdMan;

                bool fReIndex = SysCfg().IsReindex();
                pCdMan = new CCacheDBManager(fReIndex, false);
                if (fReIndex)
                    pCdMan->pBlockCache->WriteReindexing(true);

                mempool.SetMemPoolCache();

                if (!LoadBlockIndex()) {
                    strLoadError = _("Error loading block database");
                    break;
                }

                // If the loaded chain has a wrong genesis, bail out immediately
                // (we're likely using a testnet datadir, or the other way around).
                if (!mapBlockIndex.empty() && chainActive.Genesis() == nullptr)
                    return InitError(_("Incorrect or no genesis block found. Wrong datadir for network?"));

                // Initialize the block index (no-op if non-empty database was already loaded)
                if (!InitBlockIndex()) {
                    strLoadError = _("Error initializing block database");
                    break;
                }

                // Check for changed -txindex state
                if (SysCfg().IsTxIndex() != SysCfg().GetBoolArg("-txindex", true)) {
                    strLoadError = _("You need to rebuild the database using -reindex to change -txindex");
                    break;
                }

                if (!VerifyDB(SysCfg().GetArg("-checklevel", 3), SysCfg().GetArg("-checkblocks", 288))) {
                    strLoadError = _("Corrupted block database detected");
                    break;
                }

            } catch (std::exception &e) {
                LogPrint(BCLog::INFO, "%s\n", e.what());
                strLoadError = _("Error opening block database! ") + e.what();
                break;
            }

            fLoaded = true;
        } while (false);

        if (!fLoaded) {
            // Rebuild the block database first.
            if (!fReset) {
                LogPrint(BCLog::INFO, "Need to rebuild the block database first.\n");
                SysCfg().SetReIndex(true);
                fRequestShutdown = false;
            } else {
                return InitError(strLoadError);
            }
        }
    }

    // As LoadBlockIndex can take several minutes, it's possible the user
    // requested to kill the GUI during the last operation. If so, exit.
    // As the program has not fully started yet, Shutdown() is possibly overkill.
    if (fRequestShutdown) {
        LogPrint(BCLog::INFO, "Shutdown requested. Exiting.\n");
        return false;
    }

    LogPrint(BCLog::INFO, "Build %lu block indexes into memory (%lldms)\n", mapBlockIndex.size(), GetTimeMillis() - nStart);

    if (SysCfg().GetBoolArg("-printblockindex", false) || SysCfg().GetBoolArg("-printblocktree", false)) {
        PrintBlockTree();
        return false;
    }

    if (SysCfg().IsArgCount("-printblock")) {
        string strMatch = SysCfg().GetArg("-printblock", "");
        int32_t nFound  = 0;
        for (map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.begin(); mi != mapBlockIndex.end(); ++mi) {
            uint256 hash = (*mi).first;
            if (strncmp(hash.ToString().c_str(), strMatch.c_str(), strMatch.size()) == 0) {
                CBlockIndex *pIndex = (*mi).second;
                CBlock block;
                ReadBlockFromDisk(pIndex, block);
                block.BuildMerkleTree();

                block.Print();

                nFound++;
            }
        }
        if (nFound == 0)
            LogPrint(BCLog::INFO, "No blocks matching %s were found\n", strMatch);

        return false;
    }

    // scan for better chains in the block chain database, that are not yet connected in the active best chain
    CValidationState state;
    if (!ActivateBestChain(state))
        return InitError("Failed to connect best block");

    nStart                   = GetTimeMillis();
    CBlockIndex *pBlockIndex = chainActive.Tip();
    int32_t nCacheHeight     = SysCfg().GetTxCacheHeight();
    int32_t nCount           = 0;
    CBlock block;
    while (pBlockIndex && nCacheHeight-- > 0) {
        if (!ReadBlockFromDisk(pBlockIndex, block))
            return InitError("Failed to read block from disk");

        if (!pCdMan->pTxCache->AddBlockTx(block))
            return InitError("Failed to add block to transaction memory cache");

        pBlockIndex = pBlockIndex->pprev;
        ++nCount;
    }
    LogPrint(BCLog::INFO, "Added the latest %d blocks to transaction memory cache (%dms)\n", nCount, GetTimeMillis() - nStart);

    if (!pCdMan->pPpCache->ReleadBlocks(*pCdMan->pSysParamCache, chainActive.Tip())) {
        return InitError("Init prices of PriceFeedMemCache failed");
    }

    vector<boost::filesystem::path> vImportFiles;
    if (SysCfg().IsArgCount("-loadblock")) {
        vector<string> tmp = SysCfg().GetMultiArgs("-loadblock");
        for (auto strFile : tmp) {
            vImportFiles.push_back(strFile);
        }

    }
    threadGroup.create_thread(boost::bind(&ThreadImport, vImportFiles));


    nStart = GetTimeMillis();
    {
        CAddrDB adb;
        if (!adb.Read(addrman)) {
            LogPrint(BCLog::INFO, "Invalid or missing peers.dat; recreating\n");
        }
    }

    LogPrint(BCLog::INFO, "Loaded %i addresses from peers.dat (%dms)\n", addrman.size(), GetTimeMillis() - nStart);

    if (!CheckDiskSpace())
        return false;

    RandAddSeedPerfmon();

    StartNode(threadGroup);

    if (SysCfg().IsServer()) {
        if (!StartRPCServer()) {
            return InitError(_("Failed to start RPC server. "));
        }
    }

    // Generate coins in the background
    if (pWalletMain) {
        GenerateProduceBlockThread(SysCfg().GetBoolArg("-genblock", false), pWalletMain, SysCfg().GetArg("-genblocklimit", -1));

        if (!SysCfg().IsReindex()) {
            pWalletMain->ResendWalletTransactions();
        }
        threadGroup.create_thread(boost::bind(&ThreadFlushWalletDB, boost::ref(pWalletMain->strWalletFile)));

        //resend unconfirmed tx
        threadGroup.create_thread(boost::bind(&ThreadRelayTx, pWalletMain));
    }

    return !fRequestShutdown;
}

fs::path AbsPathForConfigVal(const fs::path& path, bool net_specific = true)
{
    if (path.is_absolute()) {
        return path;
    }
    return fs::absolute(path, GetDataDir(net_specific));
}

/**
 * Initialize global loggers.
 *
 * Note that this is called very early in the process lifetime, so you should be
 * careful about what global state you rely on here.
 */
bool InitLogging()
{
    LogInstance().m_print_to_file = SysCfg().GetBoolArg("-logprinttofile", false);
    LogInstance().m_file_path = AbsPathForConfigVal(SysCfg().GetArg("-debuglogfile", DEFAULT_DEBUGLOGFILE));
    LogInstance().m_print_to_console = SysCfg().GetBoolArg("-logprinttoconsole", true);
    LogInstance().m_print_file_line  = SysCfg().GetBoolArg("-logprintfileline", false);
    LogInstance().m_log_timestamps = SysCfg().GetBoolArg("-logtimestamps", DEFAULT_LOGTIMESTAMPS);
    LogInstance().m_log_time_micros = SysCfg().GetBoolArg("-logtimemicros", DEFAULT_LOGTIMEMICROS);
    LogInstance().m_log_threadnames = SysCfg().GetBoolArg("-logthreadnames", DEFAULT_LOGTHREADNAMES);
    LogInstance().m_totoal_written_size = LogInstance().GetCurrentLogSize();
    LogInstance().m_max_log_size = SysCfg().GetArg("-debuglogfilesize", 500 * 1024 * 1024);
    fLogIPs = SysCfg().GetBoolArg("-logips", DEFAULT_LOGIPS);

    // TODO: ...
    // nLogMaxSize = GetArg("-logmaxsize", 100) * 1024 * 1024;

    // m_mapMultiArgs["-debug"].push_back("error");  // Enable ERROR logger by default

    // ********************************************************* initialize logging
    if (SysCfg().IsArgCount("-debug")) {
        // Special-case: if -debug=0/-nodebug is set, turn off debugging messages
        const std::vector<std::string> categories = SysCfg().GetMultiArgs("-debug");

        if (std::none_of(categories.begin(), categories.end(),
            [](std::string cat) { return cat == "0" || cat == "none"; })) {
            for (const auto& cat : categories) {
                if (!LogInstance().EnableCategory(cat)) {
                    fprintf(stdout, "Unsupported logging category -debug=%s.\n", cat.c_str());
                }
            }
        }
    }


    if (!LogInstance().StartLogging()) {
        fprintf(stdout, "Start logging error! log_file=%s",
                        LogInstance().m_file_path.string().c_str());
        return false;
    }

    std::string version_string = FormatFullVersion();
#ifdef VER_DEBUG
    version_string += " (debug build)";
#else
    version_string += " (release build)";
#endif
    LogPrint(BCLog::INFO, PACKAGE_NAME " version %s\n", version_string);

    // LogInstance().DisableCategory("INFO");
    // LogInstance().EnableCategory("NET");
    // if (GetBoolArg("-shrinkdebugfile", !fDebug))
    //     ShrinkDebugFile();

     if (!LogInstance().m_log_timestamps)
        LogPrint(BCLog::INFO, "Startup time: %s\n", FormatISO8601DateTime(GetTime()));

    return true;
}