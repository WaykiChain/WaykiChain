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

#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"


using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

extern CPBFTMan pbftMan ;

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
            "  \"mine_block\": xxxxx,           (numeric) whether to mine/generate blocks or not (1|0), 1: true, 0: "
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
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getinfo", "") + "\nAs json rpc\n" + HelpExampleRpc("getinfo", ""));

    ProxyType proxy;
    GetProxy(NET_IPV4, proxy);
    static const string fullVersion = strprintf("%s (%s)", FormatFullVersion().c_str(), CLIENT_DATE.c_str());

    Object obj;
    obj.push_back(Pair("version",               fullVersion));
    obj.push_back(Pair("protocol_version",      PROTOCOL_VERSION));
    obj.push_back(Pair("net_type",              NetTypeNames[SysCfg().NetworkID()]));
    obj.push_back(Pair("proxy",                 (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("public_ip",             publicIp));
    obj.push_back(Pair("conf_dir",              GetConfigFile().string().c_str()));
    obj.push_back(Pair("data_dir",              GetDataDir().string().c_str()));
    obj.push_back(Pair("block_interval",        (int32_t)::GetBlockInterval(chainActive.Height())));
    obj.push_back(Pair("genblock",              SysCfg().GetArg("-genblock", 0)));
    obj.push_back(Pair("time_offset",           GetTimeOffset()));

    if (pWalletMain) {
        obj.push_back(Pair("WICC_balance",      ValueFromAmount(pWalletMain->GetFreeCoins(SYMB::WICC))));
        obj.push_back(Pair("WUSD_balance",      ValueFromAmount(pWalletMain->GetFreeCoins(SYMB::WUSD))));
        obj.push_back(Pair("WGRT_balance",      ValueFromAmount(pWalletMain->GetFreeCoins(SYMB::WGRT))));
        if (pWalletMain->IsEncrypted())
            obj.push_back(Pair("wallet_unlock_time", nWalletUnlockTime));
    }

    obj.push_back(Pair("relay_fee_perkb",       ValueFromAmount(MIN_RELAY_TX_FEE)));

    obj.push_back(Pair("tipblock_fuel_rate",    (int32_t)chainActive.Tip()->nFuelRate));
    obj.push_back(Pair("tipblock_fuel",         chainActive.Tip()->nFuel));
    obj.push_back(Pair("tipblock_time",         (int32_t)chainActive.Tip()->nTime));
    obj.push_back(Pair("tipblock_hash",         chainActive.Tip()->GetBlockHash().ToString()));
    obj.push_back(Pair("tipblock_height",       chainActive.Height()));
    obj.push_back(Pair("synblock_height",       nSyncTipHeight));

    CBlockIndex* localFinIndex =pbftMan.GetLocalFinIndex() ;
    //CBlockIndex* globalFinIndex = chainActive.GetGlobalFinIndex() ;
    std::pair<int32_t ,uint256> globalfinblock = std::make_pair(0,uint256());
    pCdMan->pBlockCache->ReadGlobalFinBlock(globalfinblock);
    obj.push_back(Pair("finblock_height",       globalfinblock.first)) ;
    obj.push_back(Pair("finblock_hash",         globalfinblock.second.GetHex())) ;
    obj.push_back(Pair("local_finblock_height", localFinIndex->height)) ;
    obj.push_back(Pair("local_finblock_hash",   localFinIndex->GetBlockHash().GetHex())) ;

    obj.push_back(Pair("connections",           (int32_t)vNodes.size()));
    obj.push_back(Pair("errors",                GetWarnings("statusbar")));


    return obj;
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
    /*DEFINE( BLOCK_INDEX,          cw.blockCache.txDiskPosCache)         */ \
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
    DEFINE( DEX_OPERATOR_TRADE_PAIR, pDexCache, operator_trade_pair_cache) \
    /**** price feed */ \
    DEFINE( MEDIAN_PRICES,        pPriceFeedCache, median_price_cache) \
    DEFINE( PRICE_FEED_COIN_PAIRS,      pPriceFeedCache,price_feed_coin_pairs_cache) \
    /**** log db                                                                    */ \
    DEFINE( TX_EXECUTE_FAIL,      pLogCache,  executeFailCache ) \
    /**** tx receipt db                                                                    */ \
    DEFINE( TX_RECEIPT,           pReceiptCache,   txReceiptCache ) \
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
    str = DbCacheToString(pCdMan->db->cache) + "\n"; \
    fwrite(str.data(), 1, str.size(), f);

static void DumpDbOne(FILE *f, dbk::PrefixType prefixType, const string &prefixTypeStr) {
    string str = "";
    switch (prefixType) {
        DBK_PREFIX_CACHE_LIST(DUMP_DB_ONE);
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
