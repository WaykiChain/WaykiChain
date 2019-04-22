// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "util.h"
#include "miner/miner.h"
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

int64_t nWalletUnlockTime;
static CCriticalSection cs_nWalletUnlockTime;

string HelpRequiringPassphrase() {
    return pwalletMain && pwalletMain->IsEncrypted()
               ? "\nRequires wallet passphrase to be set with walletpassphrase call."
               : "";
}

void EnsureWalletIsUnlocked() {
    if (pwalletMain->IsLocked())
        throw JSONRPCError(
            RPC_WALLET_UNLOCK_NEEDED,
            "Error: Please enter the wallet passphrase with walletpassphrase first.");
}

bool GetKeyId(string const& addr, CKeyID& keyId) {
    if (!CRegID::GetKeyId(addr, keyId)) {
        keyId = CKeyID(addr);
        return (!keyId.IsEmpty());
    }
    return true;
}

Value getnewaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewaddr  (\"IsMiner\")\n"
            "\nget a new address\n"
            "\nArguments:\n"
            "1. \"IsMiner\" (bool, optional)  If true, it creates two sets of key-pairs: one for "
            "mining and another for receiving miner fees.\n"
            "\nExamples:\n" +
            HelpExampleCli("getnewaddr", "") + "\nAs json rpc\n" +
            HelpExampleRpc("getnewaddr", ""));

    EnsureWalletIsUnlocked();

    bool IsForMiner = false;
    if (params.size() == 1) {
        RPCTypeCheck(params, list_of(bool_type));
        IsForMiner = params[0].get_bool();
    }

    CKey userkey;
    userkey.MakeNewKey();

    CKey minerKey;
    string minerPubKey = "null";

    if (IsForMiner) {
        minerKey.MakeNewKey();
        if (!pwalletMain->AddKey(userkey, minerKey)) {
            throw runtime_error("add miner key failed ");
        }
        minerPubKey = minerKey.GetPubKey().ToString();
    } else if (!pwalletMain->AddKey(userkey)) {
        throw runtime_error("add user key failed ");
    }

    CPubKey userPubKey = userkey.GetPubKey();
    CKeyID userKeyID   = userPubKey.GetKeyId();
    Object obj;
    obj.push_back(Pair("addr", userKeyID.ToAddress()));
    obj.push_back(Pair("minerpubkey", minerPubKey));  // "null" for non-miner address
    return obj;
}

Value addmultisigaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "addmultisigaddr nrequired [\"address\",...]\n"
            "\nget a new multisig address\n"
            "\nArguments:\n"
            "1. nrequired        (numeric, required) The number of required signatures out of the "
            "n keys or addresses.\n"
            "2. \"keysobject\"   (string, required) A json array of WICC addresses or "
            "hex-encoded public keys\n"
            "     [\n"
            "       \"address\"  (string) WICC address or hex-encoded public key\n"
            "       ...,\n"
            "     ]\n"
            "\nResult:\n"
            "\"addr\"  (string) A WICC address.\n"
            "\nExamples:\n"
            "\nAdd a 2-3 multisig address from 3 addresses\n" +
            HelpExampleCli("addmultisigaddr",
                           "2 \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\","
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("addmultisigaddr",
                           "2, \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\","
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\""));

    EnsureWalletIsUnlocked();

    int64_t nRequired = params[0].get_int64();
    const Array& keys = params[1].get_array();

    if (nRequired < 1) {
        throw runtime_error("a multisignature address must require at least one key to redeem");
    }

    if ((int64_t)keys.size() < nRequired) {
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)",
                      keys.size(), nRequired));
    }

    if ((int64_t)keys.size() > kSignatureNumberThreshold) {
        throw runtime_error(
            strprintf("too many keys supplied, no more than %d keys", kSignatureNumberThreshold));
    }

    CKeyID keyId;
    CPubKey pubKey;
    set<CPubKey> pubKeys;
    for (unsigned int i = 0; i < keys.size(); i++) {
        if (!GetKeyId(keys[i].get_str(), keyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get keyId.");
        }

        if (!pwalletMain->GetPubKey(keyId, pubKey)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get pubKey.");
        }

        pubKeys.insert(pubKey);
    }

    CScript script;
    script.SetMultisig(nRequired, pubKeys);
    CKeyID scriptId = script.GetID();
    pwalletMain->AddCScript(script);

    Object obj;
    obj.push_back(Pair("addr", scriptId.ToAddress()));
    return obj;
}

Value createmultisig(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "createmultisig nrequired [\"address\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "\nArguments:\n"
            "1. nrequired        (numeric, required) The number of required signatures out of the "
            "n keys or addresses.\n"
            "2. \"keysobject\"   (string, required) A json array of WICC addresses or "
            "hex-encoded public keys\n"
            "     [\n"
            "       \"address\"  (string) WICC address or hex-encoded public key\n"
            "       ...,\n"
            "     ]\n"
            "\nResult:\n"
            "\"addr\"  (string) A WICC address.\n"
            "\nExamples:\n"
            "\nCreate a 2-3 multisig address from 3 addresses\n" +
            HelpExampleCli("createmultisig",
                           "2 \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\","
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("createmultisig",
                           "2, \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\","
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\""));

    EnsureWalletIsUnlocked();

    int64_t nRequired = params[0].get_int64();
    const Array& keys = params[1].get_array();

    if (nRequired < 1) {
        throw runtime_error("a multisignature address must require at least one key to redeem");
    }

    if ((int64_t)keys.size() < nRequired) {
        throw runtime_error(
            strprintf("not enough keys supplied "
                      "(got %u keys, but need at least %d to redeem)",
                      keys.size(), nRequired));
    }

    if ((int64_t)keys.size() > kSignatureNumberThreshold) {
        throw runtime_error(
            strprintf("too many keys supplied, no more than %d keys", kSignatureNumberThreshold));
    }

    CKeyID keyId;
    CPubKey pubKey;
    set<CPubKey> pubKeys;
    for (unsigned int i = 0; i < keys.size(); i++) {
        if (!GetKeyId(keys[i].get_str(), keyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get keyId.");
        }

        if (!pwalletMain->GetPubKey(keyId, pubKey)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get pubKey.");
        }

        pubKeys.insert(pubKey);
    }

    CScript script;
    script.SetMultisig(nRequired, pubKeys);
    CKeyID scriptId = script.GetID();

    Object obj;
    obj.push_back(Pair("addr", scriptId.ToAddress()));
    obj.push_back(Pair("script", HexStr(script.ToString())));
    return obj;
}

Value signmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2) {
        throw runtime_error("signmessage \"WICC address\" \"message\"\n"
            "\nSign a message by the private key of the given address"
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
            "1. \"WICC address\"  (string, required) The coin address associated with the private key to sign.\n"
            "2. \"message\"         (string, required) The message to create a signature of.\n"
            "\nResult:\n"
            "\"signature\"          (string) The signature of the message encoded in base 64\n"
            "\nExamples:\n"
            "\nUnlock the wallet for 30 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"mypassphrase\" 30") +
            "\nCreate the signature\n"
            + HelpExampleCli("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"my message\"") +
            "\nVerify the signature\n"
            + HelpExampleCli("verifymessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\" \"signature\" \"my message\"") +
            "\nAs json rpc\n"
            + HelpExampleRpc("signmessage", "\"1D1ZrZNe3JUo7ZycKEYQQiQAWd9y54F4XZ\", \"my message\"")
        );
    }

    EnsureWalletIsUnlocked();

    string strAddress = params[0].get_str();
    string strMessage = params[1].get_str();

    CKeyID keyID(strAddress);
    if (keyID.IsEmpty())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKey key;
    if (!pwalletMain->GetKey(keyID, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

static std::tuple<bool, string> SendMoney(const CKeyID& sendKeyId, const CKeyID& recvKeyId,
                                          int64_t nValue, int64_t nFee) {
    /**
     * We need to choose the proper field as the sender/receiver's account according to
     * the two factor: whether the sender's account is registered or not, whether the
     * RegID is mature or not.
     *
     * |-------------------------------|-------------------|-------------------|
     * |                               |      SENDER       |      RECEIVER     |
     * |-------------------------------|-------------------|-------------------|
     * | NOT registered                |     Public Key    |      Key ID       |
     * |-------------------------------|-------------------|-------------------|
     * | registered BUT immature       |     Public Key    |      Key ID       |
     * |-------------------------------|-------------------|-------------------|
     * | registered AND mature         |     Reg ID        |      Reg ID       |
     * |-------------------------------|-------------------|-------------------|
     */
    CPubKey sendPubKey;
    if (!pwalletMain->GetPubKey(sendKeyId, sendPubKey))
        return std::make_tuple(false, "Key not found in the local wallet.");

    int nHeight = chainActive.Height();
    CUserID sendUserId, recvUserId;
    CRegID sendRegId, recvRegId;
    sendUserId = (pAccountViewTip->GetRegId(CUserID(sendKeyId), sendRegId) &&
                  nHeight - sendRegId.GetHeight() > kRegIdMaturePeriodByBlock)
                     ? CUserID(sendRegId)
                     : CUserID(sendPubKey);
    recvUserId = (pAccountViewTip->GetRegId(CUserID(recvKeyId), recvRegId) &&
                  nHeight - recvRegId.GetHeight() > kRegIdMaturePeriodByBlock)
                     ? CUserID(recvRegId)
                     : CUserID(recvKeyId);
    CCommonTx tx;
    tx.srcUserId    = sendUserId;
    tx.desUserId    = recvUserId;
    tx.bcoinBalance = nValue;
    tx.llFees       = (0 == nFee) ? SysCfg().GetTxFee() : nFee;
    tx.nValidHeight = nHeight;

    if (!pwalletMain->Sign(sendKeyId, tx.SignatureHash(), tx.signature))
        return std::make_tuple(false, "Sign failed");

    std::tuple<bool, string> ret = pwalletMain->CommitTransaction((CBaseTx *)&tx);
    bool flag = std::get<0>(ret);
    string te = std::get<1>(ret);
    if (flag)
        te = tx.GetHash().ToString();
    return std::make_tuple(flag, te.c_str());
}

Value sendtoaddress(const Array& params, bool fHelp) {
    int size = params.size();
    if (fHelp || (size != 2 && size != 3))
        throw runtime_error(
            "sendtoaddress (\"sendaddress\") \"recvaddress\" \"amount\"\n"
            "\nSend an amount to a given address.\n" +
            HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1.\"sendaddress\" (string, optional) The address where coins are sent from.\n"
            "2.\"recvaddress\" (string, required) The address where coins are received.\n"
            "3.\"amount\" (string, required)\n"
            "\nResult:\n"
            "\"transactionid\" (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("sendtoaddress", "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" 10000000") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("sendtoaddress", "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", 10000000"));

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    int64_t nAmount = 0;
    int64_t nDefaultFee = SysCfg().GetTxFee();

    if (size == 3) {
        if (!GetKeyId(params[0].get_str(), sendKeyId))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

        if (!GetKeyId(params[1].get_str(), recvKeyId))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");

        nAmount = AmountToRawValue(params[2]);
        if (pAccountViewTip->GetRawBalance(sendKeyId) < nAmount + nDefaultFee)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough coins");
    } else { // size == 2
        if (!GetKeyId(params[0].get_str(), recvKeyId))
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");

        nAmount = AmountToRawValue(params[1]);

        set<CKeyID> sKeyIds;
        sKeyIds.clear();
        pwalletMain->GetKeys(sKeyIds);
        if (sKeyIds.empty())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Wallet has no key");

        bool sufficientFee = false;
        for (auto& keyId : sKeyIds) {
            if (keyId != recvKeyId &&
                (pAccountViewTip->GetRawBalance(keyId) >= nAmount + nDefaultFee)) {
                sendKeyId     = keyId;
                sufficientFee = true;
                break;
            }
        }
        if (!sufficientFee) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                "Can't find any account with sufficient coins in wallet to send");
        }
    }

    std::tuple<bool, string> ret = SendMoney(sendKeyId, recvKeyId, nAmount, nDefaultFee);

    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

Value sendtoaddresswithfee(const Array& params, bool fHelp) {
    int size = params.size();
    if (fHelp || (size != 3 && size != 4)) {
        throw runtime_error(
            "sendtoaddresswithfee (\"sendaddress\") \"recvaddress\" \"amount\" (fee)\n"
            "\nSend an amount to a given address with fee.\n"
            "\nArguments:\n"
            "1.\"sendaddress\"  (string, optional) The Coin address to send to.\n"
            "2.\"recvaddress\"  (string, required) The Coin address to receive.\n"
            "3.\"amount\"       (string, required)\n"
            "4.\"fee\"          (string, required)\n"
            "\nResult:\n"
            "\"transactionid\"  (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("sendtoaddress",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" 10000000 10000") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("sendtoaddress",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", 10000000, 10000"));
    }

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    int64_t nAmount = 0;
    int64_t nFee = 0;
    int64_t nActualFee = 0;
    int64_t nDefaultFee = SysCfg().GetTxFee();

    if (size == 4) {
        if (!GetKeyId(params[0].get_str(), sendKeyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");
        }
        if (!GetKeyId(params[1].get_str(), recvKeyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");
        }
        nAmount = AmountToRawValue(params[2]);
        nFee = AmountToRawValue(params[3]);
        nActualFee = max(nDefaultFee, nFee);
        if (nFee < nDefaultFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                               strprintf("Given fee(%ld) < Default fee (%ld)", nFee, nDefaultFee));
        }

        if (pAccountViewTip->GetRawBalance(sendKeyId) < nAmount + nActualFee) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough coins");
        }
    } else {  // sender address omitted
        if (!GetKeyId(params[0].get_str(), recvKeyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");
        }
        nAmount = AmountToRawValue(params[1]);
        nFee = AmountToRawValue(params[2]);
        nActualFee = max(nDefaultFee, nFee);
        if (nFee < nDefaultFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                               strprintf("Given fee(%ld) < Default fee (%ld)", nFee, nDefaultFee));
        }

        set<CKeyID> sKeyIds;
        sKeyIds.clear();
        pwalletMain->GetKeys(sKeyIds);
        if (sKeyIds.empty()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Wallet has no key!");
        }
        bool sufficientFee = false;
        for (auto& keyId : sKeyIds) {
            if (keyId != recvKeyId &&
                (pAccountViewTip->GetRawBalance(keyId) >= nAmount + nDefaultFee)) {
                sendKeyId     = keyId;
                sufficientFee = true;
                break;
            }
        }
        if (!sufficientFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                "Can't find any account with sufficient coins in wallet to send");
        }
    }

    std::tuple<bool, string> ret = SendMoney(sendKeyId, recvKeyId, nAmount, nFee);

    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

Value gensendtoaddressraw(const Array& params, bool fHelp) {
    int size = params.size();
    if (fHelp || size < 4 || size > 5) {
        throw runtime_error(
            "gensendtoaddressraw \"sendaddress\" \"recvaddress\" \"amount\" \"fee\" \"height\"\n"
            "\n create common transaction by sendaddress, recvaddress, amount, fee, height\n" +
            HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1.\"sendaddress\"  (string, required) The Coin address to send to.\n"
            "2.\"recvaddress\"  (string, required) The Coin address to receive.\n"
            "3.\"amount\"  (numeric, required)\n"
            "4.\"fee\"     (numeric, required)\n"
            "5.\"height\"  (int, optional)\n"
            "\nResult:\n"
            "\"transactionid\"  (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("gensendtoaddressraw",
                           "\"WRJAnKvf8F8xdeuaceXJXz9AcNRdVvH5JG\" "
                           "\"Wef9QkwAwBhtZaT3ASmMJzC7dt1kzo1xob\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("gensendtoaddressraw",
                           "\"WRJAnKvf8F8xdeuaceXJXz9AcNRdVvH5JG\", "
                           "\"Wef9QkwAwBhtZaT3ASmMJzC7dt1kzo1xob\", 10000, 10000, 100"));
    }

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");
    }

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");
    }

    int64_t amount = AmountToRawValue(params[2]);
    int64_t fee    = AmountToRawValue(params[3]);
    if (amount == 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Send 0 amount disallowed!");
    }

    int height = chainActive.Tip()->nHeight;
    if (params.size() > 4) {
        height = params[4].get_int();
    }

    CPubKey sendPubKey;
    if (!pwalletMain->GetPubKey(sendKeyId, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId, recvUserId;
    CRegID sendRegId, recvRegId;
    sendUserId = (pAccountViewTip->GetRegId(CUserID(sendKeyId), sendRegId) &&
                  height - sendRegId.GetHeight() > kRegIdMaturePeriodByBlock)
                     ? CUserID(sendRegId)
                     : CUserID(sendPubKey);
    recvUserId = (pAccountViewTip->GetRegId(CUserID(recvKeyId), recvRegId) &&
                  height - recvRegId.GetHeight() > kRegIdMaturePeriodByBlock)
                     ? CUserID(recvRegId)
                     : CUserID(recvKeyId);

    CCommonTx tx;
    tx.srcUserId    = sendUserId;
    tx.desUserId    = recvUserId;
    tx.bcoinBalance     = amount;
    tx.llFees       = fee;
    tx.nValidHeight = height;

    if (!pwalletMain->Sign(sendKeyId, tx.SignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value getassets(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1) {
        throw runtime_error("getassets \"contract_regid\"\n"
            "\nThe collection of all assets\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required) Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getassets", "11-1")
            + HelpExampleRpc("getassets", "11-1"));
    }

    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() == true) {
        throw runtime_error("in getassets :scriptid size is error!\n");
    }

    if (!pScriptDBTip->HaveScript(regid)) {
        throw runtime_error("in getassets :scriptid  is not exist!\n");
    }

    Object retObj;

    set<CKeyID> sKeyid;
    pwalletMain->GetKeys(sKeyid);
    if (sKeyid.empty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No Key In wallet \n");
    }

    uint64_t totalassets = 0;
    Array arrayAssetIds;
    set<CKeyID>::iterator it;
    for (it = sKeyid.begin(); it != sKeyid.end(); it++) {
        CKeyID KeyId = *it;

        if (KeyId.IsNull()) {
            continue;
        }

        vector<unsigned char> veckey;
        string addr = KeyId.ToAddress();
        veckey.assign(addr.c_str(), addr.c_str() + addr.length());

        std::shared_ptr<CAppUserAccount> temp = std::make_shared<CAppUserAccount>();
        if (!pScriptDBTip->GetScriptAcc(regid, veckey, *temp.get())) {
            continue;
        }

        temp.get()->AutoMergeFreezeToFree(chainActive.Tip()->nHeight);
        uint64_t freeValues = temp.get()->GetbcoinBalance();
        uint64_t freezeValues = temp.get()->GetAllFreezedValues();
        totalassets += freeValues;
        totalassets += freezeValues;

        Object result;
        result.push_back(Pair("Address", addr));
        result.push_back(Pair("FreeValues", freeValues));
        result.push_back(Pair("FreezedFund", freezeValues));

        arrayAssetIds.push_back(result);
    }

    retObj.push_back(Pair("TotalAssets", totalassets));
    retObj.push_back(Pair("Lists", arrayAssetIds));

    return retObj;
}

// Value dispersebalance(const Array& params, bool fHelp)
// {
//     int size = params.size();
//     if (fHelp || (size != 2)) {
//         throw runtime_error(
//                 "dispersebalance \"send address\" \"amount\"\n"
//                 "\nSend an amount to a address list. \n"
//                 + HelpRequiringPassphrase() + "\nArguments:\n"
//                 "1. send address   (string, required) The Koala address to receive\n"
//                 "2. amount (required)\n"
//                 "3.\"description\"   (string, required) \n"
//                 "\nResult:\n"
//                 "\"transactionid\"  (string) The transaction id.\n"
//                 "\nExamples:\n"
//                 + HelpExampleCli("dispersebalance", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
//                 + HelpExampleRpc("dispersebalance", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1"));
//     }

//     EnsureWalletIsUnlocked();

//     CKeyID sendKeyId;

//     if (!GetKeyId(params[0].get_str(), sendKeyId)) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "send address Invalid  ");
//     }
//     if(!pwalletMain->HaveKey(sendKeyId)) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "send address Invalid  ");
//     }

//     CRegID sendRegId;
//     if (!pAccountViewTip->GetRegId(CUserID(sendKeyId), sendRegId)) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "send address not activated  ");
//     }

//     int64_t nAmount = 0;
//     nAmount = params[1].get_real() * COIN;
//     if(nAmount <= 0) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "nAmount <= 0  ");
//     }

//     set<CKeyID> sKeyid;
//     pwalletMain->GetKeys(sKeyid); //get addrs
//     if (sKeyid.empty()) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No Key In wallet \n");
//     }

//     Array arrayTxIds;
//     Object retObj;
//     set<CKeyID>::iterator it;
//     CKeyID recvKeyId;

//     for (it = sKeyid.begin(); it!=sKeyid.end(); it++) {
//         recvKeyId = *it;
//         if (recvKeyId.IsNull()) {
//             continue;
//         }

//         if(sendKeyId.ToString() == recvKeyId.ToString())
//             continue;

//         CRegID revreg;
//         CUserID rev;

//         if (pAccountViewTip->GetRegId(CUserID(recvKeyId), revreg)) {
//             rev = revreg;
//         } else {
//             rev = recvKeyId;
//         }

//         if(pAccountViewTip->GetRawBalance(sendreg) < nAmount + SysCfg().GetTxFee()) {
//             break;
//         }

//         CTransaction tx(sendreg, rev, SysCfg().GetTxFee(), nAmount , chainActive.Height());

//         if (!pwalletMain->Sign(sendKeyId, tx.SignatureHash(), tx.signature)) {
//             continue;
//         }

//         std::tuple<bool,string> ret = pwalletMain->CommitTransaction((CBaseTx *) &tx);
//         if(!std::get<0>(ret))
//              continue;
//         arrayTxIds.push_back(std::get<1>(ret));
//     }

//     retObj.push_back(Pair("Tx", arrayTxIds));
//     return retObj;
// }

Value backupwallet(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error("backupwallet \"dest_dir\"\n"
            "\nSafely copies wallet.dat to a target directory.\n"
            "\nArguments:\n"
            "1. \"dest_dir\"   (string, required) The destination directory\n"
            "\nExamples:\n"
            + HelpExampleCli("backupwallet", "\"~/backup_wallet/\"")
            + HelpExampleRpc("backupwallet", "\"~/backup_wallet/\"")
        );

    string strDest = params[0].get_str();
    string backupFilePath = strDest.c_str();
    if (backupFilePath.find(GetDataDir().string()) != std::string::npos)
        throw JSONRPCError(RPC_WALLET_FILEPATH_INVALID,
            "Wallet backup file shall not be saved into the Data dir to avoid likely file overwrite.");

    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}

static void LockWallet()
{
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = 0;
    pwalletMain->Lock();
}

Value walletpassphrase(const Array& params, bool fHelp)
{
    if (pwalletMain->IsEncrypted() && (fHelp || params.size() != 2))
        throw runtime_error("walletpassphrase \"passphrase\" timeout\n"
            "\nStores the wallet decryption key in memory for 'timeout' seconds.\n"
            "This is needed prior to performing transactions related to private keys such as sending WICC coins\n"
            "\nArguments:\n"
            "1. \"passphrase\"     (string, required) The wallet passphrase\n"
            "2. timeout            (numeric, required) The time to keep the decryption key in seconds.\n"
            "\nNote:\n"
            "Issuing the walletpassphrase command while the wallet is already unlocked will set a new unlock\n"
            "time that overrides the old one.\n"
            "\nExamples:\n"
            "\nunlock the wallet for 60 seconds\n"
            + HelpExampleCli("walletpassphrase", "\"my passphrase\" 60") +
            "\nLock the wallet again (before 60 seconds)\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("walletpassphrase", "\"my passphrase\", 60")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fHelp)
        return true;

    if (!pwalletMain->IsEncrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE,
            "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();
    //assert(0);
    if (strWalletPass.length() > 0) {
        if (!pwalletMain->Unlock(strWalletPass))
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    } else
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout>\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.");

    int64_t nSleepTime = params[1].get_int64();
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = GetTime() + nSleepTime;
    RPCRunLater("lockwallet", LockWallet, nSleepTime);

    Object retObj;
    retObj.push_back( Pair("wallet_unlocked", true) );
    return retObj;
}

Value walletpassphrasechange(const Array& params, bool fHelp)
{
    if (pwalletMain->IsEncrypted() && (fHelp || params.size() != 2))
        throw runtime_error("walletpassphrasechange \"oldpassphrase\" \"newpassphrase\"\n"
            "\nChanges the wallet passphrase from 'oldpassphrase' to 'newpassphrase'.\n"
            "\nArguments:\n"
            "1. \"oldpassphrase\"      (string, required) The current passphrase\n"
            "2. \"newpassphrase\"      (string, required) The new passphrase\n"
            "\nExamples:\n"
            + HelpExampleCli("walletpassphrasechange", "\"old one\" \"new one\"")
            + HelpExampleRpc("walletpassphrasechange", "\"old one\", \"new one\"")
        );

    if (fHelp)
        return true;

    if (!pwalletMain->IsEncrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE,
            "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

    // TODO: get rid of these .c_str() calls by implementing SecureString::operator=(string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strOldWalletPass;
    strOldWalletPass.reserve(100);
    strOldWalletPass = params[0].get_str().c_str();

    SecureString strNewWalletPass;
    strNewWalletPass.reserve(100);
    strNewWalletPass = params[1].get_str().c_str();

    if (strOldWalletPass.length() < 1 || strNewWalletPass.length() < 1)
        throw runtime_error(
            "walletpassphrasechange <oldpassphrase> <newpassphrase>\n"
            "Changes the wallet passphrase from <oldpassphrase> to <newpassphrase>.");

    if (!pwalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    Object retObj;
    retObj.push_back(Pair("chgpwd", true));
    return retObj;
}

Value encryptwallet(const Array& params, bool fHelp)
{
    if (fHelp || (!pwalletMain->IsEncrypted() && params.size() != 1)) {
        throw runtime_error(
            "encryptwallet \"passphrase\"\n"
            "\nEncrypts the wallet with 'passphrase'. This is for first time encryption.\n"
            "After this, any calls that interact with private keys such as sending or signing \n"
            "will require the passphrase to be set prior the making these calls.\n"
            "Use the walletpassphrase call for this, and then walletlock call.\n"
            "If the wallet is already encrypted, use the walletpassphrasechange call.\n"
            "Note that this will shutdown the server.\n"
            "\nArguments:\n"
            "1. \"passphrase\"    (string, required) The passphrase to encrypt the wallet with. It must be at least 1 character, but should be long.\n"
            "\nExamples:\n"
            "\nEncrypt you wallet\n"
            + HelpExampleCli("encryptwallet", "\"my passphrase\"") +
            "\nNow set the passphrase to use the wallet, such as for signing or sending Coin\n"
            + HelpExampleCli("walletpassphrase", "\"my passphrase\"") +
            "\nNow we can so something like sign\n"
            + HelpExampleCli("signmessage", "\"WICC address\" \"test message\"") +
            "\nNow lock the wallet again by removing the passphrase\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("encryptwallet", "\"my passphrase\"")
        );
    }
    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (pwalletMain->IsEncrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: Wallet was already encrypted and shall not be encrypted again.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() < 1) {
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");
    }

    if (!pwalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    //BDB seems to have a bad habit of writing old data into
    //slack space in .dat files; that is bad if the old data is
    //unencrypted private keys. So:
    StartShutdown();

//    string defaultFilename = SysCfg().GetArg("-wallet", "wallet.dat");
//    string strFileCopy = defaultFilename + ".rewrite";
//
//    boost::filesystem::remove(GetDataDir() / defaultFilename);
//    boost::filesystem::rename(GetDataDir() / strFileCopy, GetDataDir() / defaultFilename);

    Object retObj;
    retObj.push_back( Pair("wallet_encrypted", true) );
    return retObj;
}

Value walletlock(const Array& params, bool fHelp)
{
    if (fHelp || (pwalletMain->IsEncrypted() && params.size() != 0)) {
        throw runtime_error("walletlock\n"
            "\nRemoves the wallet encryption key from memory, hence locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked first.\n"
            "\nExamples:\n"
            "\nSet the passphrase for 2 minutes to perform a transaction\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 120") +
            "\nPerform a send (requires passphrase set)\n"
            + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 1.0") +
            "\nClear the passphrase since we are done before 2 minutes is up\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("walletlock", ""));
    }

    if (!pwalletMain->IsEncrypted()) {
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE,
            "Error: running with an unencrypted wallet, but walletlock was called.");
    }

    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }

    Object retObj;
    retObj.push_back( Pair("wallet_lock", true) );
    return retObj;
}

Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1) {
        throw runtime_error("settxfee \"amount\"\n"
            "\nSet the default transaction fee per kB.\n"
            "\nArguments:\n"
            "1. amount         (numeric, required) The transaction fee in WICC/kB rounded to the nearest 0.00000001\n"
            "\nResult\n"
            "true|false        (boolean) Returns true if successful\n"
            "\nExamples:\n"
            + HelpExampleCli("settxfee", "0.00001")
            + HelpExampleRpc("settxfee", "0.00001")
        );
    }

    // Amount
    int64_t nAmount = 0;
    if (params[0].get_real() != 0.0) {
       nAmount = AmountToRawValue(params[0]);        // rejects 0.0 amounts
       SysCfg().SetDefaultTxFee(nAmount);
    }
    return true;
}

Value getwalletinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0) {
        throw runtime_error("getwalletinfo\n"
            "Returns an object containing various wallet state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"wallet_version\": xxxxx,        (numeric) the wallet version\n"
            "  \"wallet_balance\": xxxxxxx,      (numeric) the total Coin balance of the wallet\n"
            "  \"wallet_encrypted\": true|false, (boolean) whether the wallet is encrypted or not\n"
            "  \"wallet_locked\":  true|false,   (boolean) whether the wallet is locked or not\n"
            "  \"unlocked_until\": xxxxx,        (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "  \"coinfirmed_tx_num\": xxxxxxx,   (numeric) the number of confirmed tx in the wallet\n"
            "  \"uncomfirmed_tx_num\": xxxxxx,   (numeric) the number of unconfirmed tx in the wallet\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getwalletinfo", "")
            + HelpExampleRpc("getwalletinfo", "")
        );
    }

    Object obj;
    obj.push_back(Pair("wallet_version",    pwalletMain->GetVersion()));
    obj.push_back(Pair("wallet_balance",    ValueFromAmount(pwalletMain->GetRawBalance())));
    obj.push_back(Pair("wallet_encrypted",  pwalletMain->IsEncrypted()));
    obj.push_back(Pair("wallet_locked",     pwalletMain->IsLocked()));
    obj.push_back(Pair("unlocked_until",    nWalletUnlockTime));
    obj.push_back(Pair("coinfirmed_tx_num", (int)pwalletMain->mapInBlockTx.size()));
    obj.push_back(Pair("unconfirmed_tx_num",(int)pwalletMain->UnConfirmTx.size()));
    return obj;
}

Value getsignature(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2)
           throw runtime_error("getsignature\n"
               "Returns an object containing a signature signed by the private key.\n"
               "\nArguments:\n"
                "1. \"private key\"   (string, required) The private key base58 encode string, used to sign hash.\n"
                "2. \"hash\"          (string, required) hash needed sign by private key.\n"
               "\nResult:\n"
               "{\n"
               "  \"signature\": xxxxx,     (string) private key signature\n"
               "}\n"
               "\nExamples:\n"
               + HelpExampleCli("getsignature", "")
               + HelpExampleRpc("getsignature", "")
           );

        string strSecret = params[0].get_str();
        CCoinSecret vchSecret;
        bool fGood = vchSecret.SetString(strSecret);
        if (!fGood)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

        CKey key = vchSecret.GetKey();
        if (!key.IsValid())
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key invalid");

        vector<unsigned char> signature;
        uint256 hash;
        hash.SetHex(params[1].get_str());
        if(key.Sign(hash, signature))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Private key sign hash failed");

       Object obj;
       obj.push_back(Pair("signature", HexStr(signature.begin(), signature.end())));
       return obj;
}
