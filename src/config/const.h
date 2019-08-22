// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CONFIG_CONST_H
#define CONFIG_CONST_H

#include <string>
#include <unordered_map>

using namespace std;

namespace SYMB {
    static const string WICC                = "WICC";
    static const string WGRT                = "WGRT";
    static const string WUSD                = "WUSD";
    static const string WCNY                = "WCNY";

    static const string WBTC                = "WBTC";
    static const string WETH                = "WETH";
    static const string WEOS                = "WEOS";

    static const string USD                 = "USD";
    static const string CNY                 = "CNY";
    static const string EUR                 = "EUR";
    static const string BTC                 = "BTC";
    static const string USDT                = "USDT";
    static const string GOLD                = "GOLD";
    static const string KWH                 = "KWH";
}

struct CoinUnitTypeHash {
    size_t operator()(const string &unit) const noexcept {
        return std::hash<string>{}(unit);
    }
};

namespace COIN_UNIT {
    static const string SAWI = "sawi";
    static const string QUWI = "quwi";
    static const string MUWI = "muwi";
    static const string HUWI = "huwi";
    static const string SIWI = "siwi";
    static const string MIWI = "miwi";
    static const string LEWI = "lewi";
    static const string FEWI = "fewi";
    static const string WI   = "wi";
    static const string KWI  = "kwi";
    static const string MWI  = "mwi";
}

static const unordered_map<string, uint64_t, CoinUnitTypeHash> CoinUnitTypeTable {
    {"sawi", 1                  },  // 0.00000001
    {"quwi", 10                 },  // 0.0000001
    {"muwi", 100                },  // 0.000001
    {"huwi", 1000               },  // 0.00001
    {"siwi", 10000              },  // 0.0001
    {"miwi", 100000             },  // 0.001
    {"lewi", 1000000            },  // 0.01
    {"fewi", 10000000           },  // 0.1
    {"wi",   100000000          },  // 1
    {"kwi",  100000000000       },  // 1000
    {"mwi",  100000000000000    },  // 1000,000
};

static const uint64_t COIN = 100000000;  //10^8 = 1 WICC
static const uint64_t CENT = 1000000;    //10^6 = 0.01 WICC

/** the max token symbol len */
static const uint32_t MAX_TOKEN_SYMBOL_LEN = 12;
/** the max asset name len */
static const uint32_t MAX_ASSET_NAME_LEN = 12;
static const uint32_t MIN_ASSET_SYMBOL_LEN = 7;
static const uint64_t MAX_ASSET_TOTAL_SUPPLY = 9000000000 * COIN; // 90 billion

/** the total blocks of burn fee need */
static const uint32_t DEFAULT_BURN_BLOCK_SIZE = 50;
static const uint64_t MAX_BLOCK_RUN_STEP      = 12000000;
static const int64_t INIT_FUEL_RATES          = 100;  // 100 unit / 100 step
static const int64_t MIN_FUEL_RATES           = 1;    // 1 unit / 100 step

/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const uint32_t MAX_BLOCK_SIZE = 4000000;
/** Default for -blockmaxsize and -blockminsize, which control the range of sizes the mining code will create **/
static const uint32_t DEFAULT_BLOCK_MAX_SIZE = 3750000;
static const uint32_t DEFAULT_BLOCK_MIN_SIZE = 1024 * 10;
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const uint32_t DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
/** The maximum size for transactions we're willing to relay/mine */
static const uint32_t MAX_STANDARD_TX_SIZE = 100000;

/** The maximum number of orphan blocks kept in memory */
static const uint32_t MAX_ORPHAN_BLOCKS = 750;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int32_t MAX_BLOCKS_IN_TRANSIT_PER_PEER = 128;
/** Timeout in seconds before considering a block download peer unresponsive. */
static const uint32_t BLOCK_DOWNLOAD_TIMEOUT  = 60;

/** Minimum disk space required */
static const uint64_t MIN_DISK_SPACE = 52428800;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const uint32_t MAX_BLOCKFILE_SIZE = 0x8000000;  // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const uint32_t BLOCKFILE_CHUNK_SIZE = 0x1000000;  // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const uint32_t UNDOFILE_CHUNK_SIZE = 0x100000;  // 1 MiB
/** -dbcache default (MiB) */
static const int64_t DEFAULT_DB_CACHE = 100;
/** max. -dbcache in (MiB) */
static const int64_t MAX_DB_CACHE = sizeof(void *) > 4 ? 4096 : 1024;
/** min. -dbcache in (MiB) */
static const int64_t MIN_DB_CACHE = 4;

/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int32_t BLOCK_REWARD_MATURITY = 100;
/** RegId's mature period measured by blocks */
static const int32_t REG_ID_MATURITY = 100;

static const uint16_t MAX_MINED_BLOCK_COUNT      = 100;        // maximun cache size for mined blocks
static const int32_t MAX_RECENT_BLOCK_COUNT      = 1000;       // most recent block number limit
static const uint32_t MAX_RPC_SIG_STR_LEN        = 65 * 1024;  // 65K max length of raw string to be signed via rpc call
static const uint32_t MAX_SIGNATURE_SIZE         = 100;        // 100 bytes max size of tx or block signature
static const uint32_t MAX_CONTRACT_CODE_SIZE     = 65536;      // 64 KB max for contract script size
static const uint32_t MAX_CONTRACT_ARGUMENT_SIZE = 4096;       // 4 KB max for contract argument size
static const uint32_t MAX_COMMON_TX_MEMO_SIZE    = 100;        // 100 bytes max for memo size
static const uint32_t MAX_CONTRACT_MEMO_SIZE     = 100;        // 100 bytes max for memo size
static const int32_t MAX_MULSIG_NUMBER           = 15;         // m-n multisig, refer to n
static const int32_t MAX_MULSIG_SCRIPT_SIZE      = 1000;       // multisig script max size

static const string LUA_CONTRACT_LOCATION_PREFIX = "/tmp/lua/";  // prefix of lua contract file location
static const string LUA_CONTRACT_HEADLINE        = "mylib = require";

static const uint64_t INITIAL_BASE_COIN_AMOUNT               = 210000000;  // 210 million
static const uint32_t BLOCK_INTERVAL_PRE_STABLE_COIN_RELEASE = 10;         // 10 seconds
static const uint32_t BLOCK_INTERVAL_STABLE_COIN_RELEASE     = 3;          // 3 seconds

static const uint64_t INITIAL_SUBSIDY_RATE = 5;  // Initial subsidy rate upon vote casting
static const uint64_t FIXED_SUBSIDY_RATE   = 1;  // Eventual/lasting subsidy rate for vote casting

static const string EMPTY_STRING = "";

#endif //CONFIG_CONST_H