// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "rpc/core/rpccommons.h"
#include "rpc/core/rpcserver.h"
#include "init.h"
#include "main.h"
#include "sync.h"
#include "wallet/wallet.h"

#include <fstream>
#include <cstdint>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_writer_template.h"

#include "commons/json/json_spirit_reader_template.h"
#include "commons/json/json_spirit_reader.h"
#include "commons/json/json_spirit_stream_reader.h"


using namespace json_spirit;
using namespace std;

void EnsureWalletIsUnlocked();

static string EncodeDumpTime(int64_t nTime) { return DateTimeStrFormat("%Y-%m-%dT%H:%M:%SZ", nTime); }

Value dumpwallet(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpwallet \"filename\"\n"
            "\nDumps all wallet keys in a human-readable format.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename\n"
            "\nExamples:\n" +
            HelpExampleCli("dumpwallet", "target_dumpwallet_filepath") + "\nAs a json rpc call\n" +
            HelpExampleRpc("dumpwallet", "target_dumpwallet_filepath"));

    EnsureWalletIsUnlocked();

    string dumpFilePath = params[0].get_str().c_str();
    if (dumpFilePath.find(GetDataDir().string()) != std::string::npos)
        throw JSONRPCError(RPC_WALLET_FILEPATH_INVALID,
            "Wallet file shall not be saved into the Data dir to avoid likely file overwrite.");

    ofstream file;
	file.open(dumpFilePath);
	if (!file.is_open())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to open dump file.");

	Object reply;
	reply.push_back(Pair("created_version",     CLIENT_BUILD + CLIENT_DATE));
	reply.push_back(Pair("created_time",        EncodeDumpTime(GetTime())));
	reply.push_back(Pair("tip_block_height",    chainActive.Height()));
	reply.push_back(Pair("tip_block_hash",      chainActive.Tip()->GetBlockHash().ToString()));

	set<CKeyID> setKeyIds;
	pWalletMain->GetKeys(setKeyIds);
	Array arrKeys;
	for (auto & keyId : setKeyIds) {
		CKeyCombi keyCombi;
		pWalletMain->GetKeyCombi(keyId, keyCombi);
		Object obj = keyCombi.ToJsonObj();
		obj.push_back(Pair("keyid", keyId.ToString()));
		arrKeys.push_back(obj);
	}

	reply.push_back(Pair("key", arrKeys));
	file << write_string(Value(reply), true);
	file.close();

	Object reply2;
	reply2.push_back(Pair("info",   "successfully dumped wallet"));
	reply2.push_back(Pair("count",  (int32_t)setKeyIds.size()));
	return reply2;
}

Value importwallet(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "importwallet \"filename\"\n"
            "\nImports keys from a wallet dump file (see dumpwallet).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file to be imported\n"
            "\nExamples:\n"
            "\nDump the wallet first\n" +
            HelpExampleCli("dumpwallet", "\"target_dumpwallet_filepath\"") + "\nImport the wallet\n" +
            HelpExampleCli("importwallet", "\"target_dumpwallet_filepath\"") + "\nAs a json rpc call\n" +
            HelpExampleRpc("importwallet", "\"target_dumpwallet_filepath\""));

    EnsureWalletIsUnlocked();

    LOCK2(cs_main, pWalletMain->cs_wallet);

    ifstream file;
    file.open(params[0].get_str().c_str(), ios::in | ios::ate);
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fail to open dump file");

    file.seekg(0, file.beg);
    int importedKeySize = 0;
    if (file.good()) {
    	Value reply;
    	json_spirit::read(file, reply);
        const Value& keyObj   = find_value(reply.get_obj(), "key");
        const Array& keyArray = keyObj.get_array();
        for (auto const& keyItem : keyArray) {
            CKeyCombi keyCombi;
            const Value& obj = find_value(keyItem.get_obj(), "keyid");
            if (obj.type() == null_type)
                continue;

            string strKeyId = find_value(keyItem.get_obj(), "keyid").get_str();
            CKeyID keyId(uint160(ParseHex(strKeyId)));
            keyCombi.UnSerializeFromJson(keyItem.get_obj());
            if (!keyCombi.HaveMainKey() && !keyCombi.HaveMinerKey())
                continue;

            if (pWalletMain->AddKey(keyId, keyCombi))
                importedKeySize++;
        }
    }
    file.close();

    Object reply2;
    reply2.push_back(Pair("info",   "successfully imported wallet"));
    reply2.push_back(Pair("count",  importedKeySize));
    return reply2;
}

Value dumpprivkey(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "dumpprivkey \"address\"\n"
            "\nReturns the private key corresponding to the given address.\n"
            "Then the importprivkey can be used with this output in another wallet for migration purposes.\n"
            "\nArguments:\n"
            "1. \"address\"   (string, required) address\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("dumpprivkey", "\"address\"") + "\nAs a json rpc call\n" +
            HelpExampleRpc("dumpprivkey", "\"address\""));

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    CCoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address.");

    CKeyID keyId;
    if (!address.GetKeyId(keyId))
        throw JSONRPCError(RPC_TYPE_ERROR, "The address is not associated with any private key.");

    CKey vchSecret;
    if (!pWalletMain->GetKey(keyId, vchSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known.");

    CKey minerkey;
	pWalletMain->GetKey(keyId, minerkey, true);
    Object reply;
    	reply.push_back(Pair("privkey", CCoinSecret(vchSecret).ToString()));

    if (minerkey.IsValid() && minerkey.ToString() != vchSecret.ToString())
    	reply.push_back(Pair("minerkey", CCoinSecret(minerkey).ToString()));
    else
    	reply.push_back(Pair("minerkey", "null"));

    return reply;
}

// TODO: enable rescan wallet.
Value importprivkey(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 1 && params.size() != 2))
        throw runtime_error(
            "importprivkey \"privkey\"\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet.\n"
            "\nArguments:\n"
            "1.\"privkey\"      (string, required) The private key, which can be the mining key when address is supplied (also refer to dumpprivkey)\n"
            "2.\"address\"      (string, optional) Set the miner's saving account address when importing the mining privkey in front\n"
            "\nExamples:\n"
            "\nDump privkey first\n" +
            HelpExampleCli("dumpprivkey", "\"address\"") + "\nImport privkey\n" +
            HelpExampleCli("importprivkey", "\"privkey\"") + "\nAs a json rpc call\n" +
            HelpExampleRpc("importprivkey", "\"privkey\""));

    EnsureWalletIsUnlocked();

    string strSecret = params[0].get_str();
    CCoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding.");

    CKey key = vchSecret.GetKey();
    if (!key.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range.");

    CPubKey pubkey = key.GetPubKey();
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);

        if (params.size() == 1) {
            if (!pWalletMain->AddKey(key))
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to add key into wallet.");
        } else {
            CKeyCombi keyCombi;
            CKey emptyMainKey;
            keyCombi.SetMainKey(emptyMainKey);
            keyCombi.SetMinerKey(key);

            CKeyID keyid;
            if (!GetKeyId(params[1].get_str(), keyid))
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

            if (!pWalletMain->AddKey(keyid, keyCombi))
                throw JSONRPCError(RPC_WALLET_ERROR, "Failed to add key into wallet.");
        }
    }

    Object ret;
    ret.push_back(Pair("imported_key_address", pubkey.GetKeyId().ToAddress()));
    return ret;
}

Value dropminerkeys(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "dropminerkeys\n"
            "\ndrop all miner keys in a wallet for cool mining.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("dropminerkeys", "") + "\nAs a json rpc call\n" + HelpExampleRpc("dropminerkeys", ""));
    }

    EnsureWalletIsUnlocked();

    if (!pWalletMain->IsReadyForCoolMiner(*pCdMan->pAccountCache)) {
        throw runtime_error("there is no cool miner key or miner key which has registered");
    }

    pWalletMain->ClearAllMainKeysForCoolMiner();

    Object ret;
    ret.push_back(Pair("info", "wallet is ready for cool mining."));

    return ret;
}

Value dropprivkey(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "dropprivkey \"address\"\n"
            "\ndrop keys for the given address.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("dropprivkey", "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\"") + "\nAs a json rpc call\n" +
            HelpExampleRpc("dropprivkey", "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\""));
    }

    EnsureWalletIsUnlocked();

    CKeyID keyid;
    if (!GetKeyId(params[0].get_str(), keyid))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");

    CKey key;
    if (!pWalletMain->GetKey(keyid, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to acquire CKey from address.");

    if (!pWalletMain->RemoveKey(key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Failed to drop privkey from wallet.");

    Object ret;
    ret.push_back(Pair("info", "privkey was dropped."));

    return ret;
}