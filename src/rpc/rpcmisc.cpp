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
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

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

Value getbalance(const Array& params, bool fHelp) {
    int size = params.size();
    if (fHelp || params.size() > 2) {
            string msg = "getbalance ( \"address\" minconf )\n"
                    "\nIf account is not specified, returns the server's total available balance.\n"
                    "If account is specified (DEPRECATED), returns the balance in the account.\n"
                    "\nArguments:\n"
                     "1. \"address\"        (string, optional) DEPRECATED. The selected account or \"*\" for entire wallet.\n"
                     "2. \"minconf\"        (numeric, optional, default=1) Only include transactions confirmed.\n"
                     "\nExamples:\n"
                    + HelpExampleCli("getbalance", "de3nGsPR6i9qpQTNnpC9ASMpFKbKzzFLYF 0")
                    + "\nAs json rpc call\n"
                    + HelpExampleRpc("getbalance", "de3nGsPR6i9qpQTNnpC9ASMpFKbKzzFLYF 0");
            throw runtime_error(msg);
    }
    Object obj;
    if (size == 0) {
        obj.push_back(Pair("balance", ValueFromAmount(pWalletMain->GetFreeBcoins())));
        return obj;
    } else if (size == 1) {
        string addr = params[0].get_str();
        if (addr == "*") {
            obj.push_back(Pair("balance", ValueFromAmount(pWalletMain->GetFreeBcoins())));
            return obj;
        } else {
            CKeyID keyid;
            if (!GetKeyId(addr, keyid)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unregistered address");
            }
            if (pWalletMain->HaveKey(keyid)) {
                CAccount account;
                if (pCdMan->pAccountCache->GetAccount(CUserID(keyid), account)) {
                    obj.push_back(Pair("balance", ValueFromAmount(account.GetToken(SYMB::WICC).free_amount)));
                    return obj;
                }
            } else {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address not in wallet");
            }
        }
    } else if (size == 2) {
        string addr = params[0].get_str();
        int nConf = params[1].get_int();
        int nMaxConf = SysCfg().GetArg("-maxconf", 30);
        if(nConf > nMaxConf) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Parameter minconf exceed maxconf");
        }
        if (addr == "*") {
            if (0 != nConf) {
                CBlockIndex *pBlockIndex = chainActive.Tip();
                int64_t nValue(0);
                while (nConf) {
                    if (pWalletMain->mapInBlockTx.count(pBlockIndex->GetBlockHash()) > 0) {
                        map<uint256, std::shared_ptr<CBaseTx> > mapTx = pWalletMain->mapInBlockTx[pBlockIndex->GetBlockHash()].mapAccountTx;
                        for (auto &item : mapTx) {
                            if (BCOIN_TRANSFER_TX == item.second->nTxType) {
                                CBaseCoinTransferTx *pTx = (CBaseCoinTransferTx *)item.second.get();
                                CKeyID srcKeyId, desKeyId;
                                pCdMan->pAccountCache->GetKeyId(pTx->txUid, srcKeyId);
                                pCdMan->pAccountCache->GetKeyId(pTx->toUid, desKeyId);
                                if (!pWalletMain->HaveKey(srcKeyId) && pWalletMain->HaveKey(desKeyId)) {
                                    nValue = pTx->coin_amount;
                                }
                            }
                            // TODO: BCOIN_TRANSFER_MTX
                        }
                    }
                    pBlockIndex = pBlockIndex->pprev;
                    --nConf;
                }
                obj.push_back(Pair("balance", ValueFromAmount(pWalletMain->GetFreeBcoins() - nValue)));
                return obj;
            } else {
                obj.push_back(Pair("balance", ValueFromAmount(pWalletMain->GetFreeBcoins(false))));
                return obj;
            }
        } else {
            CKeyID keyid;
            if (!GetKeyId(addr, keyid)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Unregistered address");
            }
            if (pWalletMain->HaveKey(keyid)) {
                if (0 != nConf) {
                    CBlockIndex *pBlockIndex = chainActive.Tip();
                    int64_t nValue(0);
                    while (nConf) {
                        if (pWalletMain->mapInBlockTx.count(pBlockIndex->GetBlockHash()) > 0) {
                            map<uint256, std::shared_ptr<CBaseTx> > mapTx = pWalletMain->mapInBlockTx[pBlockIndex->GetBlockHash()].mapAccountTx;
                            for (auto &item : mapTx) {
                                if (BCOIN_TRANSFER_TX == item.second->nTxType) {
                                    CBaseCoinTransferTx *pTx = (CBaseCoinTransferTx *)item.second.get();
                                    CKeyID srcKeyId, desKeyId;
                                    pCdMan->pAccountCache->GetKeyId(pTx->toUid, desKeyId);
                                    if (keyid == desKeyId) {
                                        nValue = pTx->coin_amount;
                                    }
                                }
                                // TODO: BCOIN_TRANSFER_MTX
                            }
                        }
                        pBlockIndex = pBlockIndex->pprev;
                        --nConf;
                    }
                    obj.push_back(Pair("balance", ValueFromAmount(pCdMan->pAccountCache->GetAccountFreeAmount(keyid, SYMB::WICC) - nValue)));
                    return obj;
                } else {
                    obj.push_back(Pair("balance", ValueFromAmount(mempool.cw->accountCache.GetAccountFreeAmount(keyid, SYMB::WICC))));
                    return obj;
                }
            } else {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address not in wallet");
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
            "  \"version\": xxxxx,              (numeric) the node program version\n"
            "  \"full_version\": \"xxxxx\",     (string) the node program fullversion\n"
            "  \"protocol_version\": xxxxx,     (numeric) the protocol version\n"
            "  \"net_type\": \"xxxxx\",         (string) the blockchain network type (MAIN_NET|TEST_NET|REGTEST_NET)\n"
            "  \"proxy\": \"host:port\",        (string) the proxy server used by the node program\n"
            "  \"connections\": xxxxx,          (numeric) the number of connections\n"
            "  \"conf_dir\": \"xxxxx\",         (string) the conf directory\n"
            "  \"data_dir\": \"xxxxx\",         (string) the data directory\n"
            "  \"block_interval\": xxxxx,       (numeric) the time interval (in seconds) to add a new block into the chain\n"
            "  \"mine_block\": xxxxx,           (numeric) whether to mine/generate blocks or not (1|0), 1: true, 0: false\n"
            "  \"time_offset\": xxxxx,          (numeric) the time offset\n"

            "  \"wallet_version\": xxxxx,       (numeric) the wallet version\n"
            "  \"wallet_balance\": xxxxx,       (numeric) the total coin balance of the wallet\n"
            "  \"wallet_unlock_time\": xxxxx,   (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is being locked\n"

            "  \"perkb_miner_fee\": x.xxxx,     (numeric) the transaction fee set in wicc/kb\n"
            "  \"perkb_relay_fee\": x.xxxx,     (numeric) minimum relay fee for non-free transactions in wicc/kb\n"
            "  \"tipblock_fuel_rate\": xxxxx,   (numeric) the fuelrate of the tip block in chainActive\n"
            "  \"tipblock_fuel\": xxxxx,        (numeric) the fuel of the tip block in chainActive\n"
            "  \"tipblock_time\": xxxxx,        (numeric) the nTime of the tip block in chainActive\n"
            "  \"tipblock_hash\": \"xxxxx\",    (string) the tip block hash\n"
            "  \"tipblock_height\": xxxxx ,     (numeric) the number of blocks contained the most work in the network\n"
            "  \"syncblock_height\": xxxxx ,    (numeric) the block height of the loggest chain found in the network\n"
            "  \"errors\": \"xxxxx\"            (string) any error messages\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getinfo", "")
            + HelpExampleRpc("getinfo", "")
        );

    ProxyType proxy;
    GetProxy(NET_IPV4, proxy);
    static const string fullVersion = strprintf("%s (%s)", FormatFullVersion().c_str(), CLIENT_DATE.c_str());
    static const string netType[] = {"MAIN_NET", "TEST_NET", "REGTEST_NET"};

    Object obj;
    obj.push_back(Pair("version",               CLIENT_VERSION));
    obj.push_back(Pair("full_version",          fullVersion));
    obj.push_back(Pair("protocol_version",      PROTOCOL_VERSION));
    obj.push_back(Pair("net_type",              netType[SysCfg().NetworkID()]));
    obj.push_back(Pair("proxy",                 (proxy.first.IsValid() ? proxy.first.ToStringIPPort() : string())));
    obj.push_back(Pair("conf_dir",              GetConfigFile().string().c_str()));
    obj.push_back(Pair("data_dir",              GetDataDir().string().c_str()));
    obj.push_back(Pair("block_interval",        (int32_t)::GetBlockInterval(chainActive.Height())));
    obj.push_back(Pair("genblock",              SysCfg().GetArg("-genblock", 0)));
    obj.push_back(Pair("time_offset",           GetTimeOffset()));
    obj.push_back(Pair("connections",           (int32_t)vNodes.size()));
    obj.push_back(Pair("errors",                GetWarnings("statusbar")));

    if (pWalletMain) {
        obj.push_back(Pair("wallet_version",    pWalletMain->GetVersion()));
        obj.push_back(Pair("wallet_balance",    ValueFromAmount(pWalletMain->GetFreeBcoins())));
        if (pWalletMain->IsEncrypted())
            obj.push_back(Pair("wallet_unlock_time", nWalletUnlockTime));
    }

    obj.push_back(Pair("miner_fee_perkb",       ValueFromAmount(SysCfg().GetTxFee())));
    obj.push_back(Pair("relay_fee_perkb",       ValueFromAmount(CBaseTx::nMinRelayTxFee)));

    obj.push_back(Pair("tipblock_fuel_rate",    (int32_t)chainActive.Tip()->nFuelRate));
    obj.push_back(Pair("tipblock_fuel",         chainActive.Tip()->nFuel));
    obj.push_back(Pair("tipblock_time",         (int32_t)chainActive.Tip()->nTime));
    obj.push_back(Pair("tipblock_hash",         chainActive.Tip()->GetBlockHash().ToString()));
    obj.push_back(Pair("tipblock_height",       chainActive.Height()));
    obj.push_back(Pair("syncblock_height",      nSyncTipHeight));

    return obj;
}

Value verifymessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 3)
        throw runtime_error(
            "verifymessage \"WICC address\" \"signature\" \"message\"\n"
            "\nVerify a signed message\n"
            "\nArguments:\n"
            "1. \"wiccaddress\"  (string, required) The Coin address to use for the signature.\n"
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
