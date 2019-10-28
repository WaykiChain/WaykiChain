// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "netbase.h"
#include "rpc/core/rpccommons.h"
#include "rpc/core/rpcserver.h"
#include "commons/util.h"

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
			CoinUnitTypeTable.begin(), CoinUnitTypeTable.end(), compFunctor);

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
            "  \"ext_ip\": \"xxxxx\",           (string) the external ip of the node\n"
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
    obj.push_back(Pair("ext_ip",                externalIp));
    obj.push_back(Pair("conf_dir",              GetConfigFile().string().c_str()));
    obj.push_back(Pair("data_dir",              GetDataDir().string().c_str()));
    obj.push_back(Pair("block_interval",        (int32_t)::GetBlockInterval(chainActive.Height())));
    obj.push_back(Pair("genblock",              SysCfg().GetArg("-genblock", 0)));
    obj.push_back(Pair("time_offset",           GetTimeOffset()));

    if (pWalletMain) {
        obj.push_back(Pair("wallet_balance",    ValueFromAmount(pWalletMain->GetFreeBcoins())));
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

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CKeyID keyId;
    if (!GetKeyId(strAddress,keyId))
        throw JSONRPCError(RPC_TYPE_ERROR, "Address does not refer to key");

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
