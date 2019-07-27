// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_CONST_H
#define CONFIG_CONST_H

#include <string>

using namespace std;

namespace SYMB {

static const string WICC = "WICC";
static const string WGRT = "WGRT";
static const string WUSD = "WUSD";

static const string USD  = "USD";
static const string CNY  = "CNY";
static const string EUR  = "EUR";
static const string BTC  = "BTC";
static const string USDT = "USDT";
static const string GOLD = "GOLD";
static const string KWH  = "KWH";

}

static const int64_t COIN = 100000000;  //10^8 = 1 WICC
static const int64_t CENT = 1000000;    //10^6 = 0.01 WICC

/** the total blocks of burn fee need */
static const unsigned int DEFAULT_BURN_BLOCK_SIZE = 50;
/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE = 4000000;
/** Default for -blockmaxsize and -blockminsize, which control the range of sizes the mining code will create **/
static const unsigned int DEFAULT_BLOCK_MAX_SIZE = 3750000;
static const unsigned int DEFAULT_BLOCK_MIN_SIZE = 1024 * 10;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
/** The maximum size for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_SIZE = 100000;
/** The maximum number of orphan blocks kept in memory */
static const unsigned int MAX_ORPHAN_BLOCKS = 750;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000;  // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000;  // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000;  // 1 MiB
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;
/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int LOCKTIME_THRESHOLD = 500000000;  // Tue Nov  5 00:53:20 1985 UTC
/** Maximum number of script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 128;
/** Timeout in seconds before considering a block download peer unresponsive. */
static const unsigned int BLOCK_DOWNLOAD_TIMEOUT = 60;
static const unsigned long MAX_BLOCK_RUN_STEP    = 12000000;
static const int64_t POS_REWARD                  = 10 * COIN;

/** max size of signature of tx or block */
static const int MAX_BLOCK_SIGNATURE_SIZE = 100;
// -dbcache default (MiB)
static const int64_t nDefaultDbCache = 100;
// max. -dbcache in (MiB)
static const int64_t nMaxDbCache = sizeof(void *) > 4 ? 4096 : 1024;
// min. -dbcache in (MiB)
static const int64_t nMinDbCache = 4;

#ifdef USE_UPNP
static const int fHaveUPnP = true;
#else
static const int fHaveUPnP = false;
#endif

static const uint64_t kTotalBaseCoinCount       = 210000000;    // 210 million
static const uint64_t kYearBlockCount           = 3153600;      // one year = 365 * 24 * 60 * 60 / 10
static const uint64_t kMinDiskSpace             = 52428800;     // Minimum disk space required
static const int kContractScriptMaxSize         = 65536;        // 64 KB max for contract script size
static const int kContractArgumentMaxSize       = 4096;         // 4 KB max for contract argument size
static const int kCommonTxMemoMaxSize           = 100;          // 100 bytes max for memo size
static const int kContractMemoMaxSize           = 100;          // 100 bytes max for memo size
static const int kMostRecentBlockNumberLimit    = 1000;         // most recent block number limit

static const int kMultisigNumberLimit           = 15;           // m-n multisig, refer to n
static const int KMultisigScriptMaxSize         = 1000;         // multisig script max size
static const int kRegIdMaturePeriodByBlock      = 100;          // RegId's mature period measured by blocks

const uint16_t kMaxMinedBlocks                  = 100;          // maximun cache size for mined blocks

static const string kContractScriptPathPrefix   = "/tmp/lua/";


#endif //CONFIG_CONST_H