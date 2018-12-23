// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "init.h"
#include "main.h"
#include "net.h"
#include "netbase.h"
#include "rpcserver.h"
#include "util.h"

#include "../wallet/wallet.h"
#include "../wallet/walletdb.h"


#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

static  bool GetKeyId(string const &addr,CKeyID &KeyId) {
	if (!CRegID::GetKeyID(addr, KeyId)) {
		KeyId=CKeyID(addr);
		if (KeyId.IsEmpty())
		return false;
	}
	return true;
};

Value getbalance(const Array& params, bool fHelp)
{
	int size = params.size();
	if (fHelp || params.size() > 2) {
			string msg = "getbalance ( \"address\" minconf )\n"
					"\nIf account is not specified, returns the server's total available balance.\n"
					"If account is specified (DEPRECATED), returns the balance in the account.\n"
					"\nArguments:\n"
					 "1. \"address\"      (string, optional) DEPRECATED. The selected account or \"*\" for entire wallet.\n"
					 "2.  minconf         (numeric, optional, default=1) Only include transactions confirmed\n"
					 "\nExamples:\n"
					+ HelpExampleCli("getbalance", "de3nGsPR6i9qpQTNnpC9ASMpFKbKzzFLYF 0")
					+ "\nAs json rpc call\n"
					+ HelpExampleRpc("getbalance", "de3nGsPR6i9qpQTNnpC9ASMpFKbKzzFLYF 0");
			throw runtime_error(msg);
	}
	Object obj;
	if (size == 0) {
		obj.push_back(Pair("balance", ValueFromAmount(pwalletMain->GetRawBalance())));
		return std::move(obj);
	} else if (size == 1) {
		string addr = params[0].get_str();
		if (addr == "*") {
			obj.push_back(Pair("balance", ValueFromAmount(pwalletMain->GetRawBalance())));
			return std::move(obj);
		} else {
			CKeyID keyid;
			if (!GetKeyId(addr, keyid)) {
				throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid  address");
			}
			if (pwalletMain->HaveKey(keyid)) {
				CAccount account;
				CAccountViewCache accView(*pAccountViewTip, true);
				if (accView.GetAccount(CUserID(keyid), account)) {
					obj.push_back(Pair("balance", ValueFromAmount(account.GetRawBalance())));
					return std::move(obj);
				}
			} else {
				throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "address not inwallet");
			}
		}
	} else if (size == 2) {
		string addr = params[0].get_str();
		int nConf = params[1].get_int();
		int nMaxConf = SysCfg().GetArg("-maxconf", 30);
		if(nConf > nMaxConf) {
			throw JSONRPCError(RPC_INVALID_PARAMETER, "parameter minconf exceed maxconfed");
		}
		if (addr == "*") {
			if (0 != nConf) {
				CBlockIndex *pBlockIndex = chainActive.Tip();
				int64_t nValue(0);
				while (nConf) {
					if (pwalletMain->mapInBlockTx.count(pBlockIndex->GetBlockHash()) > 0) {
						map<uint256, std::shared_ptr<CBaseTransaction> > mapTx = pwalletMain->mapInBlockTx[pBlockIndex->GetBlockHash()].mapAccountTx;
						for (auto &item : mapTx) {
							if (COMMON_TX == item.second->nTxType) {
								CTransaction *pTx = (CTransaction *) item.second.get();
								CKeyID srcKeyId, desKeyId;
								pAccountViewTip->GetKeyId(pTx->srcRegId, srcKeyId);
								pAccountViewTip->GetKeyId(pTx->desUserId, desKeyId);
								if (!pwalletMain->HaveKey(srcKeyId) && pwalletMain->HaveKey(desKeyId)) {
									nValue = pTx->llValues;
								}
							}
						}
					}
					pBlockIndex = pBlockIndex->pprev;
					--nConf;
				}
				obj.push_back(Pair("balance", ValueFromAmount(pwalletMain->GetRawBalance() - nValue)));
				return std::move(obj);
			} else {
				obj.push_back(Pair("balance", ValueFromAmount(pwalletMain->GetRawBalance(false))));
				return std::move(obj);
			}
		} else {
			CKeyID keyid;
			if (!GetKeyId(addr, keyid)) {
				throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid  address");
			}
			if (pwalletMain->HaveKey(keyid)) {
				if (0 != nConf) {
					CBlockIndex *pBlockIndex = chainActive.Tip();
					int64_t nValue(0);
					while (nConf) {
						if (pwalletMain->mapInBlockTx.count(pBlockIndex->GetBlockHash()) > 0) {
							map<uint256, std::shared_ptr<CBaseTransaction> > mapTx = pwalletMain->mapInBlockTx[pBlockIndex->GetBlockHash()].mapAccountTx;
							for (auto &item : mapTx) {
								if (COMMON_TX == item.second->nTxType) {
									CTransaction *pTx = (CTransaction *) item.second.get();
									CKeyID srcKeyId, desKeyId;
									pAccountViewTip->GetKeyId(pTx->desUserId, desKeyId);
									if (keyid == desKeyId) {
										nValue = pTx->llValues;
									}
								}
							}
						}
						pBlockIndex = pBlockIndex->pprev;
						--nConf;
					}
					obj.push_back(Pair("balance", ValueFromAmount(pAccountViewTip->GetRawBalance(keyid) - nValue)));
					return std::move(obj);
				} else {
					obj.push_back(Pair("balance", ValueFromAmount(mempool.pAccountViewCache->GetRawBalance(keyid))));
					return std::move(obj);
				}
			} else {
				throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "address not inwallet");
			}
		}
	}

	throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid  address");

}

Value getinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getinfo\n"
            "\nget various state information.\n"
			"\nArguments:\n"
            "Returns an object containing various state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"version\": xxxxx,           (numeric) the server version\n"
        	"  \"fullversion\": xxxxx,       (string) the server fullversion\n"
            "  \"protocolversion\": xxxxx,   (numeric) the protocol version\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total Coin balance of the wallet\n"
            "  \"blocks\": xxxxxx,           (numeric) the current number of blocks processed in the server\n"
            "  \"timeoffset\": xxxxx,        (numeric) the time offset\n"
            "  \"connections\": xxxxx,       (numeric) the number of connections\n"
            "  \"proxy\": \"host:port\",     (string, optional) the proxy used by the server\n"
            "  \"difficulty\": xxxxxx,       (numeric) the current difficulty\n"
        	"  \"nettype\": xxxxx,           (string) the net type\n"
            "  \"chainwork\": xxxxxx,        (string) the  chainwork of the tip block in chainActive\n"
            "  \"tipblocktime\": xxxx,       (numeric) the  nTime of the tip block in chainActive\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"paytxfee\": x.xxxx,         (numeric) the transaction fee set in btc/kb\n"
            "  \"relayfee\": x.xxxx,         (numeric) minimum relay fee for non-free transactions in btc/kb\n"
        	"  \"fuelrate\": xxxxx,          (numeric) the  fuelrate of the tip block in chainActive\n"
        	"  \"fuel\": xxxxx,              (numeric) the  fuel of the tip block in chainActive\n"
        	"  \"data directory\": xxxxx,    (string) the data directory\n"
			"  \"tip block hash\": xxxxx,    (string) the tip block hash\n"
            "  \"errors\": \"...\"           (string) any error messages\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

    proxyType proxy;
    GetProxy(NET_IPV4, proxy);

    Object obj;
    obj.push_back(Pair("version",       (int)CLIENT_VERSION));
    string fullersion =strprintf("%s (%s)", FormatFullVersion().c_str(), CLIENT_DATE.c_str());
    obj.push_back(Pair("fullversion",fullersion));
    obj.push_back(Pair("protocolversion",(int)PROTOCOL_VERSION));

    if (pwalletMain) {
        obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
        obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetRawBalance())));
    }
    static const string name[] = {"MAIN_NET", "TEST_NET", "REGTEST_NET"};
    
    obj.push_back(Pair("timeoffset",    GetTimeOffset()));
   
    obj.push_back(Pair("proxy",         (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("nettype",       name[SysCfg().NetworkID()]));
    obj.push_back(Pair("chainwork", 	chainActive.Tip()->nChainWork.GetHex()));
    obj.push_back(Pair("tipblocktime", 	(int)chainActive.Tip()->nTime));
    if (pwalletMain && pwalletMain->IsCrypted())
	 obj.push_back(Pair("unlocked_until", nWalletUnlockTime));
	 obj.push_back(Pair("paytxfee",      ValueFromAmount(SysCfg().GetTxFee())));

    obj.push_back(Pair("relayfee",      ValueFromAmount(CTransaction::nMinRelayTxFee)));
    obj.push_back(Pair("fuelrate",     chainActive.Tip()->nFuelRate));
    obj.push_back(Pair("fuel", chainActive.Tip()->nFuel));
    obj.push_back(Pair("data directory",GetDataDir().string().c_str()));
//    obj.push_back(Pair("block high",    chainActive.Tip()->nHeight));
    obj.push_back(Pair("connections",   (int)vNodes.size()));
    obj.push_back(Pair("received blocks", (int) chainActive.Height()));
    obj.push_back(Pair("sync tip blocks", g_nSyncTipHeight));
    obj.push_back(Pair("tip block hash", chainActive.Tip()->GetBlockHash().ToString()));
    obj.push_back(Pair("errors",        GetWarnings("statusbar")));
    return obj;
}

class DescribeAddressVisitor : public boost::static_visitor<Object>
{
public:
    Object operator()(const CNoDestination &dest) const { return Object(); }

    Object operator()(const CKeyID &keyID) const {
        Object obj;
        CPubKey vchPubKey;
        pwalletMain->GetPubKey(keyID, vchPubKey);
        obj.push_back(Pair("isscript", false));
        obj.push_back(Pair("pubkey", HexStr(vchPubKey)));
        obj.push_back(Pair("iscompressed", vchPubKey.IsCompressed()));
        return obj;
    }


};

Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage \"WICC address\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"Coinaddress\"  (string, required) The Coin address to use for the signature.\n"
            "2. \"signature\"       (string, required) The signature provided by the signer in base 64 encoding (see signmessage).\n"
            "3. \"message\"         (string, required) The message that was signed.\n"
            "\nResult:\n"
            "true|false   (boolean) If the signature is verified or not.\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", \"signature\", \"my message\"")
        );

    string strAddress  = params[0].get_str();
    string strSign     = params[1].get_str();
    string strMessage  = params[2].get_str();

    CKeyID keyID;
    if (!GetKeyId(strAddress,keyID))
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

    return (pubkey.GetKeyID() == keyID);
}
