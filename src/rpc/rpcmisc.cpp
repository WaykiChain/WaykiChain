// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "netbase.h"
#include "miner/pbftmanager.h"
#include "rpc/core/rpccommons.h"
#include "rpc/core/rpcserver.h"
#include "commons/util/util.h"

#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockundo.h"

#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"


using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

extern CPBFTMan pbftMan;

string get_node_state() {
    if (SysCfg().IsImporting())
        return "Loading";
    else if (SysCfg().IsReindex())
        return "ReIndexing";
    else if (IsInitialBlockDownload())
        return "IBD";
    else
        return "InSync";
}

Value getcoinunitinfo(const Array& params, bool fHelp){
    if (fHelp || params.size() > 1) {
            string msg = "getcoinunitinfo\n"
                    "\nArguments:\n"
                     "\nExamples:\n"
                    + HelpExampleCli("getcoinunitinfo", "")
                    + "\nAs json rpc call\n"
                    + HelpExampleRpc("getcoinunitinfo", "");
            throw runtime_error(msg);
    }

    typedef std::function<bool(std::pair<std::string, uint64_t>, std::pair<std::string, uint64_t>)> Comparator;
	Comparator compFunctor =
			[](std::pair<std::string, uint64_t> elem1 ,std::pair<std::string, uint64_t> elem2)
			{
				return elem1.second < elem2.second;
			};

	// Declaring a set that will store the pairs using above comparision logic
	std::set<std::pair<std::string, uint64_t>, Comparator> setOfUnits(
			CoinUnitTypeMap.begin(), CoinUnitTypeMap.end(), compFunctor);

	Object obj;
    for (auto& it: setOfUnits) {
        obj.push_back(Pair(it.first, it.second));
    }
    return obj;
}

Value getinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "\nget various state information.\n"
            "\nArguments:\n"
            "Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": \"xxxxx\",          (string) the node program fullversion\n"
            "  \"protocol_version\": xxxxx,     (numeric) the protocol version\n"
            "  \"net_type\": \"xxxxx\",         (string) the blockchain network type (MAIN_NET|TEST_NET|REGTEST_NET)\n"
            "  \"proxy\": \"host:port\",        (string) the proxy server used by the node program\n"
            "  \"public_ip\": \"xxxxx\",        (string) the public IP of this node\n"
            "  \"conf_dir\": \"xxxxx\",         (string) the conf directory\n"
            "  \"data_dir\": \"xxxxx\",         (string) the data directory\n"
            "  \"block_interval\": xxxxx,       (numeric) the time interval (in seconds) to add a new block into the "
            "chain\n"
            "  \"genblock\": xxxxx,             (numeric) whether to mine/generate blocks or not (1|0), 1: true, 0: "
            "false\n"
            "  \"time_offset\": xxxxx,          (numeric) the time offset\n"

            "  \"wallet_balance\": xxxxx,       (numeric) the total coin balance of the wallet\n"
            "  \"wallet_unlock_time\": xxxxx,   (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 "
            "GMT) that the wallet is unlocked for transfers, or 0 if the wallet is being locked\n"

            "  \"perkb_miner_fee\": x.xxxx,     (numeric) the transaction fee set in wicc/kb\n"
            "  \"perkb_relay_fee\": x.xxxx,     (numeric) minimum relay fee for non-free transactions in wicc/kb\n"
            "  \"tipblock_fuel_rate\": xxxxx,   (numeric) the fuelrate of the tip block in chainActive\n"
            "  \"tipblock_fuel\": xxxxx,        (numeric) the fuel of the tip block in chainActive\n"
            "  \"tipblock_time\": xxxxx,        (numeric) the nTime of the tip block in chainActive\n"
            "  \"tipblock_hash\": \"xxxxx\",    (string) the tip block hash\n"
            "  \"tipblock_height\": xxxxx ,     (numeric) the number of blocks contained the most work in the network\n"
            "  \"synblock_height\": xxxxx ,     (numeric) the block height of the loggest chain found in the network\n"
            "  \"connections\": xxxxx,          (numeric) the number of connections\n"
            "  \"errors\": \"xxxxx\"            (string) any error messages\n"
            "  \"state\": \"xxxxx\"             (string) coind operation state\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getinfo", "") + "\nAs json rpc\n" + HelpExampleRpc("getinfo", ""));

    ProxyType proxy;
    GetProxy(NET_IPV4, proxy);
    static const string fullVersion = strprintf("%s (%s)", FormatFullVersion().c_str(), CLIENT_DATE.c_str());

    auto tipBlockIndex = chainActive.Tip();
    auto tipHeight = tipBlockIndex->height;
    const auto &tipBlockHash = tipBlockIndex->GetBlockHash();
    CDiskBlockIndex tipDiskBlockIndex;

    if (!pCdMan->pBlockIndexDb->GetBlockIndex(tipBlockHash, tipDiskBlockIndex)) {
        throw JSONRPCError(RPC_INVALID_PARAMS,
            strprintf("the index of block=%s not found in db", tipBlockIndex->GetIdString()));
    }

    Object obj;
    obj.push_back(Pair("version",               fullVersion));
    obj.push_back(Pair("protocol_version",      PROTOCOL_VERSION));
    obj.push_back(Pair("net_type",              NetTypeNames[SysCfg().NetworkID()]));
    obj.push_back(Pair("proxy",                 (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("public_ip",             publicIp));
    obj.push_back(Pair("conf_dir",              GetConfigFile().string().c_str()));
    obj.push_back(Pair("data_dir",              GetDataDir().string().c_str()));
    obj.push_back(Pair("block_interval",        (int32_t)::GetBlockInterval(tipHeight)));
    obj.push_back(Pair("genblock",              SysCfg().GetArg("-genblock", 0)));
    obj.push_back(Pair("time_offset",           GetTimeOffset()));

    if (pWalletMain) {
        obj.push_back(Pair("WICC_balance",      JsonValueFromAmount(pWalletMain->GetFreeCoins(SYMB::WICC))));
        obj.push_back(Pair("WUSD_balance",      JsonValueFromAmount(pWalletMain->GetFreeCoins(SYMB::WUSD))));
        obj.push_back(Pair("WGRT_balance",      JsonValueFromAmount(pWalletMain->GetFreeCoins(SYMB::WGRT))));
        if (pWalletMain->IsEncrypted())
            obj.push_back(Pair("wallet_unlock_time", nWalletUnlockTime));
    }

    obj.push_back(Pair("relay_fee_perkb",       JsonValueFromAmount(MIN_RELAY_TX_FEE)));

    obj.push_back(Pair("tipblock_tx_count",     (int64_t)tipDiskBlockIndex.nTx));
    obj.push_back(Pair("tipblock_fuel_rate",    (int64_t)tipDiskBlockIndex.nFuelRate));
    obj.push_back(Pair("tipblock_fuel",         tipDiskBlockIndex.nFuelFee));
    obj.push_back(Pair("tipblock_time",         (int64_t)tipDiskBlockIndex.nTime));
    obj.push_back(Pair("tipblock_hash",         tipBlockIndex->GetBlockHash().ToString()));
    obj.push_back(Pair("tipblock_height",       tipHeight));
    obj.push_back(Pair("synblock_height",       nSyncTipHeight));

    std::pair<int32_t ,uint256> globalfinblock = std::make_pair(0,uint256());
    pCdMan->pBlockCache->ReadGlobalFinBlock(globalfinblock);
    obj.push_back(Pair("finblock_height",       globalfinblock.first));
    obj.push_back(Pair("finblock_hash",         globalfinblock.second.GetHex()));

    if (SysCfg().GetArg("-showlocalfinblock", false)) {
        CBlockIndex* localFinIndex = pbftMan.GetLocalFinIndex();
        obj.push_back(Pair("local_finblock_height", localFinIndex->height));
        obj.push_back(Pair("local_finblock_hash",   localFinIndex->GetBlockHash().GetHex()));
    }

    obj.push_back(Pair("connections",           (int32_t)vNodes.size()));
    obj.push_back(Pair("errors",                GetWarnings("statusbar")));
    obj.push_back(Pair("state",                 get_node_state())); //IBD, Importing, ReIndexing, InSync

    return obj;
}

Value getfinblockcount (const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getfinblockcount\n"
            "\nReturn the height of the finality block.\n"
            "\nResult:\n"
          
            "\n  (numeric) The height of the finality block\n"

            "\nExamples:\n" +
            HelpExampleCli("getfinblockcount", "") + "\nAs json rpc\n" + HelpExampleRpc("getfinblockcount", ""));

    std::pair<int32_t ,uint256> globalfinblock = std::make_pair(0,uint256());
    pCdMan->pBlockCache->ReadGlobalFinBlock(globalfinblock);

    return globalfinblock.first;
}

Value verifymessage(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage \"address\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"address\"         (string, required) The address to use for the signature.\n"
            "2. \"signature\"       (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"         (string, required) The message that was signed.\n"
            "\nResult:\n"
            "true|false             (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\n1) Unlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"my_passphrase\" 30") +
            "\n2) Create the signature\n"
            + HelpExampleCli("signmessage", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"my_message\"") +
            "\n3) Verify the signature\n"
            + HelpExampleCli("verifymessage", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"signature\" \"my_message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("verifymessage", "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"signature\", \"my_message\"")
        );


    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CKeyID keyId = RPC_PARAM::GetKeyId(params[0]);

    bool fInvalid = false;
    vector<unsigned char> vchSig = DecodeBase64(strSign.c_str(), &fInvalid);

    if (fInvalid)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Malformed base64 encoding");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    CPubKey pubkey;
    if (!pubkey.RecoverCompact(ss.GetHash(), vchSig))
        return false;

    return (pubkey.GetKeyId() == keyId);
}

//                 prefix type             db          cache
//               -------------------- -------------  ----------------------
#define DBK_PREFIX_CACHE_LIST(DEFINE) \
    /*** Asset Registry DB */ \
    DEFINE( ASSET,                pAssetCache, asset_cache )  \
    /**** block db                                                                          */ \
    /*DEFINE( BLOCK_INDEX,          pAssetCache, pBlockIndexDb) */ \
    DEFINE( BLOCKFILE_NUM_INFO,   pBlockCache, tx_diskpos_cache) \
    DEFINE( LAST_BLOCKFILE,       pBlockCache, last_block_file_cache) \
    DEFINE( REINDEX,              pBlockCache, reindex_cache) \
    DEFINE( FINALITY_BLOCK,       pBlockCache, finality_block_cache) \
    DEFINE( FLAG,                 pBlockCache, flag_cache) \
    DEFINE( BEST_BLOCKHASH,       pBlockCache, best_block_hash_cache) \
    DEFINE( TXID_DISKINDEX,       pBlockCache, tx_diskpos_cache) \
    /**** account db                                                                      */ \
    DEFINE( REGID_KEYID,          pAccountCache,  regId2KeyIdCache)\
    DEFINE( KEYID_ACCOUNT,        pAccountCache,  accountCache) \
    /**** contract db                                                                      */ \
    DEFINE( CONTRACT_DEF,         pContractCache,  contractCache ) \
    DEFINE( CONTRACT_DATA,        pContractCache,  contractDataCache) \
    DEFINE( CONTRACT_ACCOUNT,     pContractCache,  contractAccountCache) \
    DEFINE( CONTRACT_TRACES,      pContractCache,  contractTracesCache) \
    /**** delegate db                                                                      */ \
    DEFINE( VOTE,                 pDelegateCache,  voteRegIdCache) \
    DEFINE( LAST_VOTE_HEIGHT,     pDelegateCache,  last_vote_height_cache) \
    DEFINE( PENDING_DELEGATES,    pDelegateCache,  pending_delegates_cache) \
    DEFINE( ACTIVE_DELEGATES,     pDelegateCache,  active_delegates_cache) \
    DEFINE( REGID_VOTE,           pDelegateCache,  regId2VoteCache) \
    /**** cdp db                                                                     */ \
    DEFINE( CDP,                  pCdpCache,  cdp_cache) \
    DEFINE( CDP_BCOIN,            pCdpCache,  cdp_bcoin_cache ) \
    DEFINE( USER_CDP,             pCdpCache,  user_cdp_cache) \
    DEFINE( CDP_RATIO_INDEX,            pCdpCache,  cdp_ratio_index_cache) \
    DEFINE( CDP_HEIGHT_INDEX,            pCdpCache,  cdp_height_index_cache) \
    DEFINE( CDP_GLOBAL_DATA,      pCdpCache,  cdp_global_data_cache) \
    /**** cdp closed by redeem/forced or manned liquidate ***/  \
    DEFINE( CLOSED_CDP_TX,        pClosedCdpCache, closedCdpTxCache) \
    DEFINE( CLOSED_TX_CDP,        pClosedCdpCache, closedTxCdpCache) \
    /**** dex db                                                                    */ \
    DEFINE( DEX_ACTIVE_ORDER,     pDexCache, activeOrderCache) \
    DEFINE( DEX_BLOCK_ORDERS,     pDexCache, blockOrdersCache) \
    DEFINE( DEX_OPERATOR_LAST_ID, pDexCache, operator_last_id_cache) \
    DEFINE( DEX_OPERATOR_DETAIL,  pDexCache, operator_detail_cache) \
    DEFINE( DEX_OPERATOR_OWNER_MAP, pDexCache, operator_owner_map_cache) \
    /**** price feed */ \
    DEFINE( MEDIAN_PRICES,        pPriceFeedCache, median_price_cache) \
    DEFINE( PRICE_FEED_COIN_PAIRS,      pPriceFeedCache,price_feed_coin_pairs_cache) \
    /**** log db                                                                    */ \
    DEFINE( TX_EXECUTE_FAIL,      pLogCache,  executeFailCache ) \
    /**** tx receipt db                                                                    */ \
    DEFINE( TX_RECEIPT,           pReceiptCache,   tx_receipt_cache ) \
    /**** tx coinutxo db                                                                    */ \
    DEFINE( TX_UTXO,              pUtxoCache,   tx_utxo_cache) \
    /**** sys param db                                                              */\
    DEFINE(SYS_PARAM,             pSysParamCache, sys_param_chache) \
    DEFINE(MINER_FEE,             pSysParamCache, miner_fee_cache) \
    DEFINE(CDP_PARAM,             pSysParamCache, cdp_param_cache) \
    DEFINE(CDP_INTEREST_PARAMS,   pSysParamCache, cdp_interest_param_changes_cache) \
    DEFINE(TOTAL_BPS_SIZE,        pSysParamCache, current_total_bps_size_cache) \
    DEFINE(NEW_TOTAL_BPS_SIZE,    pSysParamCache, new_total_bps_size_cache)    \
    /**** sys govern db                                                */\
    DEFINE(SYS_GOVERN,            pSysGovernCache, governors_cache)      \
    DEFINE(GOVN_PROP,             pSysGovernCache, proposals_cache)      \
    DEFINE(GOVN_APPROVAL_LIST,    pSysGovernCache, approvals_cache)      \


string DbCacheToString(CBlockIndexDB &cache) {
    std::shared_ptr<leveldb::Iterator> pCursor(cache.NewIterator());
    const std::string &prefix = dbk::GetKeyPrefix(dbk::BLOCK_INDEX);
    string str;
    for (pCursor->Seek(prefix); pCursor->Valid(); pCursor->Next()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pCursor->key();
            if (slKey.starts_with(prefix)) {
                leveldb::Slice slValue = pCursor->value();
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                CDiskBlockIndex diskIndex;
                ssValue >> diskIndex;
                uint256 hash;
                ParseDbKey(slKey, dbk::BLOCK_INDEX, hash);
                str += strprintf("{%s}={%s},\n", hash.ToString(), diskIndex.ToString());

            } else {
                break;  // if shutdown requested or finished loading block index
            }
        } catch (const std::exception &e) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Deserialize or I/O error - %s", e.what()));
        }
    }

    return strprintf("-->%s, data={%s}\n", prefix, str);
}

template<int32_t PREFIX_TYPE, typename KeyType, typename ValueType>
string DbCacheToString(CCompositeKVCache<PREFIX_TYPE, KeyType, ValueType> &cache) {
    string str;
    CDbIterator< CCompositeKVCache<PREFIX_TYPE, KeyType, ValueType> > it(cache);
    for(it.First(); it.IsValid(); it.Next()) {
        str += strprintf("%s={%s},\n", db_util::ToString(it.GetKey()), db_util::ToString(it.GetValue()));
    }
    return strprintf("-->%s, data={%s}\n", GetKeyPrefix(cache.PREFIX_TYPE), str);
}

template<int32_t PREFIX_TYPE, typename ValueType>
string DbCacheToString(CSimpleKVCache<PREFIX_TYPE, ValueType> &cache) {
    auto pData = cache.GetDataPtr();
    if (pData) {
        return strprintf("-->%s, data={%s}\n", GetKeyPrefix(cache.PREFIX_TYPE), db_util::ToString(*pData));
    }
    return "";
}

#define DUMP_DB_ONE(prefixType, db, cache) \
    case dbk::prefixType: { str = DbCacheToString(pCdMan->db->cache); break;}
#define DUMP_DB_ALL(prefixType, db, cache) \
    str += DbCacheToString(pCdMan->db->cache) + "\n";

static void DumpDbOne(FILE *f, dbk::PrefixType prefixType, const string &prefixTypeStr) {
    string str = "";
    switch (prefixType) {
        DBK_PREFIX_CACHE_LIST(DUMP_DB_ONE);
        case dbk::BLOCK_INDEX:
            str = DbCacheToString(*pCdMan->pBlockIndexDb);
            break;
        default :
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("unsupported dump db data of key prefix type=%s",
                prefixTypeStr));
            break;
    }
    fwrite(str.data(), 1, str.size(), f);
}

static void DumpDbAll(FILE *f) {
    string str = "";
    DBK_PREFIX_CACHE_LIST(DUMP_DB_ALL);
    str += DbCacheToString(*pCdMan->pBlockIndexDb) + "\n";
    fwrite(str.data(), 1, str.size(), f);
}


Value dumpdb(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 2)
        throw runtime_error(
            "dumpdb \"[key_prefix_type]\" \"[file_path]\"\n"
            "\ndump db data to file\n"
            "\nArguments:\n"
            "1. \"key_prefix_type\"   (string, optional) the data key prefix type, * is all data, default is *\n"
            "2. \"file_path\"       (string, optional) the output file path, if empty output to stdout, default is empty.\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpdb", "") + "\nAs json rpc\n" + HelpExampleRpc("dumpdb", "")
        );

    string prefixTypeStr = "";
    if (params.size() > 0)
        prefixTypeStr = params[0].get_str();
    string filePath = "";
    if (params.size() > 1)
        filePath = params[1].get_str();

    struct FileCloser {
        FILE *file = nullptr;
        ~FileCloser() {
            if (file != nullptr) {
                fclose(file);
                file = nullptr;
            }
        }
    };

    FILE *file = nullptr;
    FileCloser fileCloser;
    if (!filePath.empty()) {
        file = fopen(filePath.c_str(), "w");
        if (file == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("opten file error! file=%s",
                filePath));
        }
        fileCloser.file = file;
    } else {
        file = stdout;
    }

    if (!prefixTypeStr.empty() && prefixTypeStr != "*") {
        dbk::PrefixType prefixType = dbk::ParseKeyPrefixType(prefixTypeStr);
        if (prefixType == dbk::EMPTY)
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("unsupported db data key prefix type=%s",
                prefixTypeStr));
        DumpDbOne(file, prefixType, prefixTypeStr);
    } else {
        DumpDbAll(file);
    }

    return Object();
}

template<int32_t PREFIX_TYPE, typename KeyType, typename ValueType>
Object UndoLogToJson(CCompositeKVCache<PREFIX_TYPE, KeyType, ValueType> &cache, const CDbOpLog &opLog) {
    Object obj;
    KeyType key;
    #ifdef DB_OP_LOG_NEW_VALUE
        std::pair<ValueType, ValueType> valuePair; // (old_value, new_value)
        opLog.Get(key, valuePair);
        obj.push_back(Pair("key",  db_util::ToString(key)));
        obj.push_back(Pair("value",  db_util::ToString(valuePair.first)));
        obj.push_back(Pair("new_value",  db_util::ToString(valuePair.second)));
    #else  // DB_OP_LOG_NEW_VALUE
        ValueType value;
        opLog.Get(key, value);
        obj.push_back(Pair("key",  db_util::ToString(key)));
        obj.push_back(Pair("value",  db_util::ToString(value)));
    #endif // else DB_OP_LOG_NEW_VALUE
    return obj;
}

template<int32_t PREFIX_TYPE, typename ValueType>
Object UndoLogToJson(CSimpleKVCache<PREFIX_TYPE, ValueType> &cache, const CDbOpLog &opLog) {
    Object obj;

    #ifdef DB_OP_LOG_NEW_VALUE
        std::pair<ValueType, ValueType> valuePair; // (old_value, new_value)
        opLog.Get(valuePair);
        obj.push_back(Pair("value",  db_util::ToString(valuePair.first)));
        obj.push_back(Pair("new_value",  db_util::ToString(valuePair.second)));
    #else  // DB_OP_LOG_NEW_VALUE
        ValueType value;
        opLog.Get(value);
        obj.push_back(Pair("value",  db_util::ToString(value)));
    #endif // else DB_OP_LOG_NEW_VALUE
    return obj;
}

Object UndoLogToJson(const CNullObject&, const CDbOpLog &opLog) {
    Object obj;
    obj.push_back(Pair("key",  HexStr(opLog.GetKey())));
    obj.push_back(Pair("value", HexStr(opLog.GetValue())));
    return obj;
}

template<typename CacheType>
void UndoLogsToJson(CacheType &cache, const CDbOpLogs &opLogs, Object &categoryObj) {
    Array dbLogArray;
    for (const CDbOpLog &opLog : opLogs) {
        dbLogArray.push_back(UndoLogToJson(cache, opLog));
    }
    categoryObj.push_back(Pair("cache_type", typeid(CacheType).name()));
    categoryObj.push_back(Pair("log_count", (int64_t)dbLogArray.size()));
    categoryObj.push_back(Pair("db_logs", dbLogArray));
}

#define UNDO_LOGS_TO_JSON(prefixType, db, cache) \
    case dbk::prefixType: { UndoLogsToJson(pCdMan->db->cache, opLogs, categoryObj); break;}

static const CNullObject UNKNOWN_LOG_TYPE = {};

Object UndoLogsToJson(dbk::PrefixType prefixType, const CDbOpLogs &opLogs) {
    Object categoryObj;
    categoryObj.push_back(Pair("db_prefix", dbk::GetKeyPrefix(prefixType)));
    categoryObj.push_back(Pair("db_prefix_memo", dbk::GetKeyPrefixMemo(prefixType)));
    switch (prefixType) {
        DBK_PREFIX_CACHE_LIST(UNDO_LOGS_TO_JSON);
        default :
            UndoLogsToJson(UNKNOWN_LOG_TYPE, opLogs, categoryObj);
            break;
    }
    return categoryObj;
}

Value getblockundo(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
            "getblockundo \"hash or height\"\n"
            "\nIf verbose is false, returns a string that is serialized, hex-encoded data for block 'hash'.\n"
            "If verbose is true, returns an Object with information about block <hash>.\n"
            "\nArguments:\n"
            "1.\"hash or height\"   (string or numeric, required) string for the block hash, or numeric for the block "
                                                                  "height\n"

            "\nResult: block undo object\n"
            "\nExamples:\n" +
            HelpExampleCli("getblockundo", "\"d640d051704155b1fd3ec8d0331497448c259b0ab0499e109da7ae2bc7423bc2\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("getblockundo", "\"d640d051704155b1fd3ec8d0331497448c259b0ab0499e109da7ae2bc7423bc2\""));
    }

    // RPCTypeCheck(params, boost::assign::list_of(str_type)(bool_type)); disable this to allow either string or int argument

    std::string strHash;
    if (int_type == params[0].type()) {
        int height = params[0].get_int();
        if (height < 0 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height out of range.");

        CBlockIndex* pBlockIndex = chainActive[height];
        strHash                  = pBlockIndex->GetBlockHash().GetHex();
    } else {
        strHash = params[0].get_str();
    }
    uint256 hash(uint256S(strHash));

    auto mapIt = mapBlockIndex.find(hash);
    if (mapIt == mapBlockIndex.end())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Block not found");

    CBlockIndex* pBlockIndex = mapIt->second;

    CBlockUndo blockUndo;
    CDiskBlockPos pos = pBlockIndex->GetUndoPos();
    if (pos.IsNull())
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("no undo data available! block=%d:%s",
            pBlockIndex->height, pBlockIndex->GetBlockHash().ToString()));

    if (!blockUndo.ReadFromDisk(pos, pBlockIndex->pprev->GetBlockHash()))
        throw JSONRPCError(RPC_INTERNAL_ERROR, strprintf("read undo data failed! block=%d:%s",
            pBlockIndex->height, pBlockIndex->GetBlockHash().ToString()));

    Object obj;
    obj.push_back(Pair("block_height",  pBlockIndex->height));
    obj.push_back(Pair("block_hash",  pBlockIndex->pprev->GetBlockHash().ToString()));
    obj.push_back(Pair("count", (int64_t)blockUndo.vtxundo.size()));
    Array txArray;
    for (size_t i = 0; i < blockUndo.vtxundo.size(); i++) {
        CTxUndo txUndo = blockUndo.vtxundo[i];
        Object txObj;
        txObj.push_back(Pair("index", (int64_t)i));
        txObj.push_back(Pair("tx_hash",  txUndo.txid.ToString()));
        Array categoryArray;
        for (auto opLogPair : txUndo.dbOpLogMap.GetMap()) {
            auto prefixType = dbk::ParseKeyPrefixType(opLogPair.first);
            const CDbOpLogs &opLogs = opLogPair.second;
            categoryArray.push_back(UndoLogsToJson(prefixType, opLogs));
        }
        txObj.push_back(Pair("category", categoryArray));
        txArray.push_back(txObj);
    }
    obj.push_back(Pair("tx_undos", txArray));

    return obj;
}


static string SizeToString(uint64_t sz) {
    static const uint64_t SZ_KB = 1024;
    static const uint64_t SZ_MB = SZ_KB * 1024;
    static const uint64_t SZ_GB = SZ_MB * 1024;
    static const uint64_t SZ_TB = SZ_GB * 1024;
    if (sz >= SZ_TB) {
        return strprintf("%.5f TB", sz / SZ_TB);
    } else if (sz >= SZ_GB) {
        return strprintf("%.5f GB", sz / SZ_GB);
    } else if (sz >= SZ_MB) {
        return strprintf("%.5f MB", sz / SZ_MB);
    } else if (sz >= SZ_KB) {
        return strprintf("%.5f KB", sz / SZ_KB);
    } else {
        return strprintf("%llu B", sz);
    }
}

extern set<CBlockIndex *, CBlockIndexWorkComparator> setBlockIndexValid;

extern CSignatureCache signatureCache;

Value getmemstat(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "getmemstat \n"
            "\nget memory stat.\n"
            "\nArguments:\n"

            "\nResult: memory stat\n"
            "\nExamples:\n" +
            HelpExampleCli("getmemstat", "") +
            "\nAs json rpc\n" +
            HelpExampleRpc("getmemstat", ""));
    }

    Object obj;
    // mapBlockIndex
    {
        Object statObj;
        statObj.push_back(Pair("count", (int64_t)mapBlockIndex.size()));
        uint64_t totalSz = 0;
        if (mapBlockIndex.size() > 0) {
            const auto &idx = *mapBlockIndex.rbegin()->second;
            // need to calc size for each idx?
            totalSz = sizeof(mapBlockIndex) + mapBlockIndex.size() * (sizeof(idx));
        }
        statObj.push_back(Pair("size", SizeToString(totalSz)));
        statObj.push_back(Pair("size_bytes", totalSz));

        obj.push_back(Pair("block_index_map", statObj));
    }

    {
        Object statObj;
        uint64_t totalSz = 0;
        {
            uint64_t cnt = chainActive.Height() + 1;
            statObj.push_back(Pair("count", cnt));
            if (cnt > 0) {
                // need to calc size for each idx?
                totalSz = sizeof(chainActive) + cnt * (sizeof(void*));
            }
        }
        statObj.push_back(Pair("size", SizeToString(totalSz)));
        statObj.push_back(Pair("size_bytes", totalSz));

        obj.push_back(Pair("active_chain", statObj));

    }

    // setBlockIndexValid
    {
        Object statObj;
        uint64_t totalSz = 0;
        {
            uint64_t cnt = setBlockIndexValid.size();
            statObj.push_back(Pair("count", cnt));
            if (cnt > 0) {
                // need to calc size for each idx?
                totalSz = sizeof(setBlockIndexValid) + cnt * (sizeof(void*));
            }
        }
        statObj.push_back(Pair("size", SizeToString(totalSz)));
        statObj.push_back(Pair("size_bytes", totalSz));

        obj.push_back(Pair("block_index_valid", statObj));

    }

    // signatureCache
    {
        Object statObj;
        uint64_t totalSz = 0;
        {
            uint64_t cnt = signatureCache.setValid.size();
            statObj.push_back(Pair("count", cnt));
            if (cnt > 0) {
                const auto &item = signatureCache.setValid.begin();
                // need to calc size for each idx?
                totalSz = sizeof(signatureCache) + cnt * (sizeof(item));
            }
        }
        statObj.push_back(Pair("size", SizeToString(totalSz)));
        statObj.push_back(Pair("size_bytes", totalSz));

        obj.push_back(Pair("signature_cache", statObj));

    }

    // mempool;
    {
        Object statObj;
        uint64_t totalSz = 0;
        {
            LOCK(mempool.cs);
            statObj.push_back(Pair("count", (int64_t)mempool.memPoolTxs.size()));
            if (mempool.memPoolTxs.size() > 0) {
                const auto &item = *mempool.memPoolTxs.rbegin();
                // need to calc size for each idx?
                totalSz = sizeof(mempool.memPoolTxs) + mempool.memPoolTxs.size() * (sizeof(item));
            }
        }
        statObj.push_back(Pair("size", SizeToString(totalSz)));
        statObj.push_back(Pair("size_bytes", totalSz));

        obj.push_back(Pair("tx_mem_pool", statObj));

    }

    return obj;
}

#ifdef ENABLE_GPERFTOOLS

#include <gperftools/heap-profiler.h>

Value startheapprofiler(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 0 && params.size() != 1)) {
        throw runtime_error(
            "startheapprofiler"
            "\nStart profiling and arrange to write profile data to file names of the form: \"prefix.0000\", \"prefix.0001\".\n"
            "\nArguments:\n"
            "1.\"prefix\"   (string, optional) prefix of output file names, default is \"coind\"\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("startheapprofiler", "") +
            "\nAs json rpc\n" +
            HelpExampleRpc("startheapprofiler", ""));
    }
    string prefix="coind";
    if (params.size() > 0)
        prefix = params[0].get_str();

    HeapProfilerStart(prefix.c_str());
    return Value();
}

Value stopheapprofiler(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "stopheapprofiler"
            "\nStop profiling.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("stopheapprofiler", "") +
            "\nAs json rpc\n" +
            HelpExampleRpc("stopheapprofiler", ""));
    }
    HeapProfilerStop();
    return Value();
}


Value getheapprofiler(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "getheapprofiler"
            "\nGet heap profiler.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getheapprofiler", "") +
            "\nAs json rpc\n" +
            HelpExampleRpc("getheapprofiler", ""));
    }
    Object obj;
    obj.push_back(Pair("runing", IsHeapProfilerRunning()));
    return obj;
}

Value dumpheapprofiler(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 0 && params.size() != 1)) {
        throw runtime_error(
            "dumpheapprofiler"
            "\nDump a profile now - can be used for dumping at a hopefully quiescent state in your program, "
            " in order to more easily track down memory leaks. Will include the reason in the logged message.\n"
            "\nArguments:\n"
            "1.\"reason\"   (string, optional) reason, default is \"\"\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("dumpheapprofiler", "") +
            "\nAs json rpc\n" +
            HelpExampleRpc("dumpheapprofiler", ""));
    }
    string reason = "";
    if (params.size() > 0)
        reason = params[0].get_str();

    HeapProfilerDump(reason.c_str());
    return Value();
}

#endif//DENABLE_GPERFTOOLS
