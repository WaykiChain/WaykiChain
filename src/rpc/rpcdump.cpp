// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "init.h"
#include "main.h"
#include "sync.h"
#include "../wallet/wallet.h"

#include <fstream>
#include <stdint.h>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include "json/json_spirit_value.h"
#include "json/json_spirit_writer_template.h"

#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_reader.h"
#include "json/json_spirit_stream_reader.h"


using namespace json_spirit;
using namespace std;

void EnsureWalletIsUnlocked();

string static EncodeDumpTime(int64_t nTime) {
    return DateTimeStrFormat("%Y-%m-%dT%H:%M:%SZ", nTime);
}

//int64_t static DecodeDumpTime(const string &str) {
//    static const boost::posix_time::ptime epoch = boost::posix_time::from_time_t(0);
//    static const locale loc(locale::classic(),
//        new boost::posix_time::time_input_facet("%Y-%m-%dT%H:%M:%SZ"));
//    istringstream iss(str);
//    iss.imbue(loc);
//    boost::posix_time::ptime ptime(boost::date_time::not_a_date_time);
//    iss >> ptime;
//    if (ptime.is_not_a_date_time())
//        return 0;
//    return (ptime - epoch).total_seconds();
//}

//string static EncodeDumpString(const string &str) {
//    stringstream ret;
//    for (auto c : str) {
//        if (c <= 32 || c >= 128 || c == '%') {
//            ret << '%' << HexStr(&c, &c + 1);
//        } else {
//            ret << c;
//        }
//    }
//    return ret.str();
//}

string DecodeDumpString(const string &str)
{
    stringstream ret;
    for (unsigned int pos = 0; pos < str.length(); pos++) {
        unsigned char c = str[pos];
        if (c == '%' && pos+2 < str.length()) {
            c = (((str[pos+1]>>6)*9+((str[pos+1]-'0')&15)) << 4) |
                ((str[pos+2]>>6)*9+((str[pos+2]-'0')&15));
            pos += 2;
        }
        ret << c;
    }
    return ret.str();
}

Value dropminerkeys(const Array& params, bool fHelp)
{
	if (fHelp || params.size() != 0) {
		throw runtime_error("dropminerkeys \n"
            "\ndrop all miner keys in a wallet for cool mining.\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("dropminerkeys", "")
            + HelpExampleRpc("dropminerkeys", "")
		);
    }

	EnsureWalletIsUnlocked();
	if (!pWalletMain->IsReadyForCoolMiner(*pCdMan->pAccountCache)) {
		throw runtime_error("there is no cool miner key or miner key which has registered");
	}

	pWalletMain->ClearAllMainKeysForCoolMiner();
	Object ret;
	ret.push_back( Pair("info", "wallet is ready for cool mining.") );
	return ret;
}

Value importprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 3)
        throw runtime_error(
            "importprivkey \"WICC privkey\" ( \"label\" rescan )\n"
            "\nAdds a private key (as returned by dumpprivkey) to your wallet.\n"
            "\nArguments:\n"
            "1. \"WICC privkey\"   (string, required) The private key (see dumpprivkey)\n"
            "2. \"label\"            (string, optional) an optional label\n"
            "3. rescan               (boolean, optional, default=true) Rescan the wallet for transactions\n"
            "\nExamples:\n"
            "\nDump a private key\n"
            + HelpExampleCli("dumpprivkey", "\"myaddress\"") +
            "\nImport the private key\n"
            + HelpExampleCli("importprivkey", "\"mykey\"") +
            "\nImport using a label\n"
            + HelpExampleCli("importprivkey", "\"mykey\" \"testing\" false") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("importprivkey", "\"mykey\", \"testing\", false")
        );

    EnsureWalletIsUnlocked();

    string strSecret = params[0].get_str();
//  string strLabel = "";
//  if (params.size() > 1)
//      strLabel = params[1].get_str();
//
//	bool fRescan(true);    // Whether to perform rescan after import
//	if (params.size() > 2) {
//		fRescan = params[2].get_bool();
//	}

    CCoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);

    if (!fGood)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

    CKey key = vchSecret.GetKey();
    if (!key.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key outside allowed range");

    CPubKey pubkey = key.GetPubKey();
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);

        if (!pWalletMain->AddKey(key))
            throw JSONRPCError(RPC_WALLET_ERROR, "Error adding key to wallet");
    }
    Object reply2;
    reply2.push_back(Pair("imported_key_address", pubkey.GetKeyId().ToAddress()));
    return reply2;
}

Value importwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error("importwallet \"filename\"\n"
            "\nImports keys from a wallet dump file (see dumpwallet).\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The wallet file to be imported\n"
            "\nExamples:\n"
            "\nDump the wallet first\n"
            + HelpExampleCli("dumpwallet", "\"target_dumpwallet_filepath\"") +
            "\nImport the wallet\n"
            + HelpExampleCli("importwallet", "\"target_dumpwallet_filepath\"") +
            "\nImport using the json rpc call\n"
            + HelpExampleRpc("importwallet", "\"target_dumpwallet_filepath\"")
        );

    LOCK2(cs_main, pWalletMain->cs_wallet);

    EnsureWalletIsUnlocked();

    ifstream file;
    file.open(params[0].get_str().c_str(), ios::in | ios::ate);
    if (!file.is_open())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

    file.seekg(0, file.beg);
    int importedKeySize = 0;
    if (file.good()) {
    	Value reply;
    	json_spirit::read(file, reply);
    	const Value & keyObj = find_value(reply.get_obj(),"key");
    	const Array & keyArray = keyObj.get_array();
    	for (auto const &keyItem :keyArray) {
    		CKeyCombi keyCombi;
    		const Value &obj = find_value(keyItem.get_obj(), "keyid");
    		if(obj.type() == null_type)
    			continue;

    		string strKeyId = find_value(keyItem.get_obj(), "keyid").get_str();
    		CKeyID keyId(uint160(ParseHex(strKeyId)));
    		keyCombi.UnSerializeFromJson(keyItem.get_obj());
    		if(!keyCombi.HaveMainKey() && !keyCombi.HaveMinerKey())
    			continue;

                if (pWalletMain->AddKey(keyId, keyCombi))
                    importedKeySize++;
        }
    }
    file.close();

    Object reply2;
    reply2.push_back(Pair("imported_key_size", importedKeySize));
    return reply2;
}

Value dumpprivkey(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error("dumpprivkey \"address\"\n"
            "\nReturns the private key corresponding to the given WICC address.\n"
            "Then the importprivkey can be used with this output in another wallet for migration purposes.\n"
            "\nArguments:\n"
            "1. \"address\"   (string, required) WICC address\n"
            "\nResult:\n"
            "\"key\"                (string) The associated private key\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpprivkey", "\"$myaddress\"")
            + HelpExampleCli("importprivkey", "\"$myprivkey\"")
            + HelpExampleRpc("dumpprivkey", "\"$myaddress\"")
        );

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    CCoinAddress address;
    if (!address.SetString(strAddress))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Coin address");

    CKeyID keyId;
    if (!address.GetKeyId(keyId))
        throw JSONRPCError(RPC_TYPE_ERROR, "The address is not associated with any private key");

    CKey vchSecret;
    if (!pWalletMain->GetKey(keyId, vchSecret))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key for address " + strAddress + " is not known");

    CKey minerkey;
	pWalletMain->GetKey(keyId, minerkey, true);
    Object reply;
    	reply.push_back(Pair("privkey", CCoinSecret(vchSecret).ToString()));

    if (minerkey.IsValid() && minerkey.ToString() != vchSecret.ToString())
    	reply.push_back(Pair("minerkey", CCoinSecret(minerkey).ToString()));
    else
    	reply.push_back(Pair("minerkey", " "));

    return reply;
}

Value dumpwallet(const Array& params, bool fHelp) {
	if (fHelp || params.size() != 1)
		throw runtime_error("dumpwallet \"filename\"\n"
            "\nDumps all wallet keys in a human-readable format.\n"
            "\nArguments:\n"
            "1. \"filename\"    (string, required) The filename\n"
            "\nExamples:\n"
            + HelpExampleCli("dumpwallet", "$mywalletfilepath")
            + HelpExampleRpc("dumpwallet", "$mywalletfilepath"));

	EnsureWalletIsUnlocked();

    string dumpFilePath = params[0].get_str().c_str();
    if (dumpFilePath.find(GetDataDir().string()) != std::string::npos)
        throw JSONRPCError(RPC_WALLET_FILEPATH_INVALID,
            "Wallet file shall not be saved into the Data dir to avoid likely file overwrite.");

    ofstream file;
	file.open(dumpFilePath);
	if (!file.is_open())
		throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open wallet dump file");

	Object reply;
	reply.push_back(Pair("created by Coin", CLIENT_BUILD + CLIENT_DATE));
	reply.push_back(Pair("Created Time ", EncodeDumpTime(GetTime())));
	reply.push_back(Pair("Best block index hight ", chainActive.Height()));
	reply.push_back(Pair("Best block hash ", chainActive.Tip()->GetBlockHash().ToString()));

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
	file <<  write_string(Value(reply), true);
	file.close();
	Object reply2;
	reply2.push_back(Pair("info", "dump ok"));
	reply2.push_back(Pair("key size", (int)setKeyIds.size()));
	return reply2;
}