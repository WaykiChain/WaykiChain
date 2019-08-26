// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "commons/util.h"
#include "miner/miner.h"
#include "../wallet/wallet.h"
#include "../wallet/walletdb.h"
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "persistence/contractdb.h"
#include "vm/luavm/appaccount.h"

#include <stdint.h>
#include <boost/assign/list_of.hpp>


using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

int64_t nWalletUnlockTime;
static CCriticalSection cs_nWalletUnlockTime;


string HelpRequiringPassphrase() {
    return pWalletMain && pWalletMain->IsEncrypted()
               ? "\nRequires wallet passphrase to be set with walletpassphrase call."
               : "";
}

void EnsureWalletIsUnlocked() {
    if (pWalletMain->IsLocked())
        throw JSONRPCError(
            RPC_WALLET_UNLOCK_NEEDED,
            "Error: Please enter the wallet passphrase with walletpassphrase first.");
}

Value getnewaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewaddr  (\"IsMiner\")\n"
            "\nget a new address\n"
            "\nArguments:\n"
            "1. \"IsMiner\" (bool, optional) If true, it creates two sets of key-pairs: one for "
            "mining and another for receiving miner fees.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getnewaddr", "") + "\nAs json rpc\n" +
            HelpExampleRpc("getnewaddr", ""));

    EnsureWalletIsUnlocked();

    bool isForMiner = false;
    if (params.size() == 1) {
        RPCTypeCheck(params, list_of(bool_type));
        isForMiner = params[0].get_bool();
    }

    CKey userkey;
    userkey.MakeNewKey();

    CKey minerKey;
    string minerPubKey = "null";

    if (isForMiner) {
        minerKey.MakeNewKey();
        if (!pWalletMain->AddKey(userkey, minerKey)) {
            throw runtime_error("add miner key failed ");
        }
        minerPubKey = minerKey.GetPubKey().ToString();
    } else if (!pWalletMain->AddKey(userkey)) {
        throw runtime_error("add user key failed ");
    }

    CPubKey userPubKey = userkey.GetPubKey();
    CKeyID userKeyID   = userPubKey.GetKeyId();

    Object obj;
    obj.push_back(Pair("addr",          userKeyID.ToAddress()));
    obj.push_back(Pair("minerpubkey",   minerPubKey));  // "null" for non-miner address

    return obj;
}

Value addmulsigaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "addmulsigaddr nrequired [\"address\",...]\n"
            "\nget a new multisig address\n"
            "\nArguments:\n"
            "1. nrequired        (numeric, required) The number of required signatures out of the "
            "n keys or addresses.\n"
            "2. \"keysobject\"   (string, required) A json array of WICC addresses or "
            "hex-encoded public keys\n"
            "[\n"
            "  \"address\"  (string) WICC address or hex-encoded public key\n"
            "  ...,\n"
            "]\n"
            "\nResult:\n"
            "\"addr\"  (string) A WICC address.\n"
            "\nExamples:\n"
            "\nAdd a 2-3 multisig address from 3 addresses\n" +
            HelpExampleCli("addmulsigaddr",
                           "2 \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("addmulsigaddr",
                           "2, \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
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

    if ((int64_t)keys.size() > MAX_MULSIG_NUMBER) {
        throw runtime_error(
            strprintf("too many keys supplied, no more than %d keys", MAX_MULSIG_NUMBER));
    }

    CKeyID keyId;
    CPubKey pubKey;
    set<CPubKey> pubKeys;
    for (unsigned int i = 0; i < keys.size(); i++) {
        if (!GetKeyId(keys[i].get_str(), keyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get keyId.");
        }

        if (!pWalletMain->GetPubKey(keyId, pubKey)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get pubKey.");
        }

        pubKeys.insert(pubKey);
    }

    CMulsigScript script;
    script.SetMultisig(nRequired, pubKeys);
    CKeyID scriptId = script.GetID();
    pWalletMain->AddCScript(script);

    Object obj;
    obj.push_back(Pair("addr", scriptId.ToAddress()));
    return obj;
}

Value createmulsig(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2)
        throw runtime_error(
            "createmulsig nrequired [\"address\",...]\n"
            "\nCreates a multi-signature address with n signature of m keys required.\n"
            "\nArguments:\n"
            "1. nrequired        (numeric, required) The number of required signatures out of the "
            "n keys or addresses.\n"
            "2. \"keysobject\"   (string, required) A json array of WICC addresses or "
            "hex-encoded public keys\n"
            "[\n"
            "\"address\"  (string) WICC address or hex-encoded public key\n"
            "  ...,\n"
            "]\n"
            "\nResult:\n"
            "\"addr\"  (string) A WICC address.\n"
            "\nExamples:\n"
            "\nCreate a 2-3 multisig address from 3 addresses\n" +
            HelpExampleCli("createmulsig",
                           "2 \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\"") +
            "\nAs json rpc\n" +
            HelpExampleRpc("createmulsig",
                           "2, \"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
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

    if ((int64_t)keys.size() > MAX_MULSIG_NUMBER) {
        throw runtime_error(
            strprintf("too many keys supplied, no more than %d keys", MAX_MULSIG_NUMBER));
    }

    CKeyID keyId;
    CPubKey pubKey;
    set<CPubKey> pubKeys;
    for (unsigned int i = 0; i < keys.size(); i++) {
        if (!GetKeyId(keys[i].get_str(), keyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get keyId.");
        }

        if (!pWalletMain->GetPubKey(keyId, pubKey)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get pubKey.");
        }

        pubKeys.insert(pubKey);
    }

    CMulsigScript script;
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

    CKeyID keyId(strAddress);
    if (keyId.IsEmpty())
        throw JSONRPCError(RPC_TYPE_ERROR, "Invalid address");

    CKey key;
    if (!pWalletMain->GetKey(keyId, key))
        throw JSONRPCError(RPC_WALLET_ERROR, "Private key not available");

    CHashWriter ss(SER_GETHASH, 0);
    ss << strMessageMagic;
    ss << strMessage;

    vector<unsigned char> vchSig;
    if (!key.SignCompact(ss.GetHash(), vchSig))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sign failed");

    return EncodeBase64(&vchSig[0], vchSig.size());
}

Value send(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 4 && params.size() != 5))
        throw runtime_error(
            "send \"from\" \"to\" \"symbol:coin:unit\" \"symbol:fee:unit\" (\"memo\")\n"
            "\nSend coins to a given address.\n" +
            HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1.\"from\"                 (string, required) The address where coins are sent from.\n"
            "2.\"to\"                   (string, required) The address where coins are received.\n"
            "3.\"symbol:coin:unit\":    (symbol:amount:unit, required) transferred coins\n"
            "4.\"symbol:fee:unit\":     (symbol:amount:unit, required) fee paid to miner, default is WICC:10000:sawi\n"
            "5.\"memo\":                (string, optional)\n"
            "\nResult:\n"
            "\"txid\"                   (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("send",
                           "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\" \"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\" "
                           "\"WICC:1000000:sawi\" \"Hello, WaykiChain!\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("send",
                           "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\", \"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"WICC:1000000:sawi\", \"Hello, WaykiChain!\""));

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    if (!GetKeyId(params[1].get_str(), recvKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");

    CAccount account;
    if (!pCdMan->pAccountCache->GetAccount(sendKeyId, account)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sender account not exist");
    }

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Sender account not found in wallet");

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
    CUserID sendUserId, recvUserId;
    CRegID sendRegId, recvRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) && sendRegId.IsMature(chainActive.Height()))
                     ? CUserID(sendRegId)
                     : CUserID(sendPubKey);
    recvUserId = (pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId) && recvRegId.IsMature(chainActive.Height()))
                     ? CUserID(recvRegId)
                     : CUserID(recvKeyId);

    ComboMoney cmCoin;
    if (!ParseRpcInputMoney(params[2].get_str(), cmCoin, SYMB::WICC))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Coin ComboMoney format error");

    if (cmCoin.amount == 0)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Coins is zero!");

    ComboMoney cmFee = RPC_PARAM::GetFee(params, 3, UCOIN_TRANSFER_TX);

    TokenSymbol coinSymbol = cmCoin.symbol;
    uint64_t coinAmount    = cmCoin.amount * CoinUnitTypeTable.at(cmCoin.unit);
    TokenSymbol feeSymbol  = cmFee.symbol;
    uint64_t fee           = cmFee.amount * CoinUnitTypeTable.at(cmFee.unit);
    uint64_t totalAmount = coinAmount;
    if (coinSymbol == feeSymbol) {
        totalAmount += fee;
    }

    if (coinSymbol == SYMB::WICC) {
        if (account.GetToken(SYMB::WICC).free_amount < totalAmount)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough bcoins");
    } else if (coinSymbol == SYMB::WUSD) {
        if (account.GetToken(SYMB::WUSD).free_amount < totalAmount)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough coins");
    } else if (coinSymbol == SYMB::WGRT) {
        if (account.GetToken(SYMB::WGRT).free_amount < totalAmount)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough coins");
    } else {
        throw JSONRPCError(RPC_PARSE_ERROR, "This currency is not currently supported.");
    }

    if (feeSymbol == SYMB::WICC) {
        if (account.GetToken(SYMB::WICC).free_amount < fee)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough bcoins");
    } else if (feeSymbol == SYMB::WUSD) {
        if (account.GetToken(SYMB::WUSD).free_amount < fee)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough scoins");
    } else if (feeSymbol == SYMB::WGRT) {
        if (account.GetToken(SYMB::WGRT).free_amount < fee)
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sendaddress does not have enough fcoins");
    } else {
        throw JSONRPCError(RPC_PARSE_ERROR, "This currency is not currently supported.");
    }

    string memo = params.size() == 5 ? params[4].get_str() : "";

    CCoinTransferTx tx(sendUserId, recvUserId, chainActive.Height(), coinSymbol, coinAmount, feeSymbol, fee, memo);

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature))
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx *)&tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));

    return obj;
}

Value gensendtoaddressraw(const Array& params, bool fHelp) {
    int size = params.size();
    if (fHelp || size < 4 || size > 5) {
        throw runtime_error(
            "gensendtoaddressraw \"sendaddress\" \"recvaddress\" \"amount\" \"fee\" \"height\"\n"
            "\ncreate common transaction by sendaddress, recvaddress, amount, fee, height\n" +
            HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1.\"sendaddress\"  (string, required) The Coin address to send to.\n"
            "2.\"recvaddress\"  (string, required) The Coin address to receive.\n"
            "3.\"amount\"  (numeric, required)\n"
            "4.\"fee\"     (numeric, required)\n"
            "5.\"height\"  (int, optional)\n"
            "\nResult:\n"
            "\"rawtx\"  (string) The raw transaction\n"
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
    if (amount <= 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Send amount <= 0 error!");
    }

    int height = chainActive.Height();
    if (params.size() > 4) {
        height = params[4].get_int();
        if (height <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
        }
    }

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");

    CUserID sendUserId, recvUserId;
    CRegID sendRegId, recvRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) &&
                sendRegId.IsMature(chainActive.Height())) ? CUserID(sendRegId) : CUserID(sendPubKey);

    recvUserId = (pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId) &&
                recvRegId.IsMature(chainActive.Height())) ? CUserID(recvRegId) : CUserID(recvKeyId);

    CAccount fromAccount;
    if (!pCdMan->pAccountCache->GetAccount(sendUserId, fromAccount))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sender User Account not found.");

    if (fromAccount.GetToken(SYMB::WICC).free_amount < (uint64_t)amount)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Sender User Account insufficient amount to transfer");

    CBaseCoinTransferTx tx;
    tx.txUid        = sendUserId;
    tx.toUid        = recvUserId;
    tx.coin_amount  = amount;
    tx.llFees       = fee;
    tx.valid_height = height;

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value genmulsigtx(const Array& params, bool fHelp) {
    int size = params.size();
    if (fHelp || size < 4 || size > 5) {
        throw runtime_error(
            "genmulsigtx \"multisigscript\" \"recvaddress\" \"amount\" \"fee\" \"height\"\n"
            "\n create multisig transaction by multisigscript, recvaddress, amount, fee, height\n" +
            HelpRequiringPassphrase() +
            "\nArguments:\n"
            "1.\"multisigscript\"  (string, required) The Coin address to send to.\n"
            "2.\"recvaddress\"  (string, required) The Coin address to receive.\n"
            "3.\"amount\"  (numeric, required)\n"
            "4.\"fee\"     (numeric, required)\n"
            "5.\"height\"  (int, optional)\n"
            "\nResult:\n"
            "\"txid\"  (string) The transaction id.\n"
            "\nExamples:\n" +
            HelpExampleCli("genmulsigtx",
                           "\"0203210233e68ec1402f875af47201efca7c9f210c93f10016ad73d6cd789212d5571"
                           "e9521031f3d66a05bf20e83e046b74d9073d925f5dce29970623595bc4d66ed81781dd5"
                           "21034819476f12ac0e53bd82bc3205c91c40e9c569b08af8db04503afdebceb7134c\" "
                           "\"Wef9QkwAwBhtZaT3ASmMJzC7dt1kzo1xob\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc(
                "genmulsigtx",
                "\"0203210233e68ec1402f875af47201efca7c9f210c93f10016ad73d6cd789212d5571e9521031f3d"
                "66a05bf20e83e046b74d9073d925f5dce29970623595bc4d66ed81781dd521034819476f12ac0e53bd"
                "82bc3205c91c40e9c569b08af8db04503afdebceb7134c\", "
                "\"Wef9QkwAwBhtZaT3ASmMJzC7dt1kzo1xob\", 10000, 10000, 100"));
    }

    EnsureWalletIsUnlocked();

    vector<unsigned char> multiScript = ParseHex(params[0].get_str());
    if (multiScript.empty() || multiScript.size() > MAX_MULSIG_SCRIPT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script size");
    }

    CKeyID recvKeyId;
    CUserID recvUserId;
    CRegID recvRegId;
    int height = chainActive.Height();

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");
    }

    recvUserId = (pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId) && recvRegId.IsMature(chainActive.Height()))
                     ? CUserID(recvRegId)
                     : CUserID(recvKeyId);

    int64_t amount = AmountToRawValue(params[2]);
    int64_t fee    = AmountToRawValue(params[3]);
    if (amount == 0) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Send 0 amount disallowed!");
    }

    if (params.size() > 4) {
        height = params[4].get_int();
    }

    CDataStream scriptDataStream(multiScript, SER_DISK, CLIENT_VERSION);
    CMulsigScript script;
    try {
        scriptDataStream >> script;
    } catch (std::exception& e) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script content");
    }

    int8_t required = (int8_t)script.GetRequired();
    std::set<CPubKey> pubKeys = script.GetPubKeys();
    vector<CSignaturePair> signaturePairs;
    CRegID regId;
    for (const auto& pubKey : pubKeys) {
        if (pCdMan->pAccountCache->GetRegId(CUserID(pubKey), regId) && regId.IsMature(chainActive.Height())) {
            signaturePairs.push_back(CSignaturePair(regId, UnsignedCharArray()));
        } else {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Immature regid or invalid key");
        }
    }

    CMulsigTx tx;
    tx.signaturePairs = signaturePairs;
    tx.desUserId      = recvUserId;
    tx.bcoins         = amount;
    tx.llFees         = fee;
    tx.required       = required;
    tx.valid_height   = height;

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value getcontractassets(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 1) {
        throw runtime_error("getcontractassets \"contract_regid\"\n"
            "\nThe collection of all assets\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required) Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractassets", "11-1")
            + HelpExampleRpc("getcontractassets", "11-1"));
    }

    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() == true) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid contract regid");
    }

    if (!pCdMan->pContractCache->HaveContract(regid)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to find contract");
    }

    Object retObj;

    set<CKeyID> keyIds;
    pWalletMain->GetKeys(keyIds);
    if (keyIds.empty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No keys in wallet");
    }

    uint64_t totalassets = 0;
    Array arrayAssetIds;
    set<CKeyID>::iterator it;
    for (it = keyIds.begin(); it != keyIds.end(); it++) {
        CKeyID keyId = *it;

        if (keyId.IsNull()) {
            continue;
        }

        string addr = keyId.ToAddress();
        std::shared_ptr<CAppUserAccount> temp = std::make_shared<CAppUserAccount>();
        if (!pCdMan->pContractCache->GetContractAccount(regid, addr, *temp.get())) {
            continue;
        }

        temp.get()->AutoMergeFreezeToFree(chainActive.Height());
        uint64_t freeValues = temp.get()->GetBcoins();
        uint64_t freezeValues = temp.get()->GetAllFreezedValues();
        totalassets += freeValues;
        totalassets += freezeValues;

        Object result;
        result.push_back(Pair("address", addr));
        result.push_back(Pair("free_value", freeValues));
        result.push_back(Pair("freezed_value", freezeValues));

        arrayAssetIds.push_back(result);
    }

    retObj.push_back(Pair("total_assets", totalassets));
    retObj.push_back(Pair("lists", arrayAssetIds));

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
//                 "\"txid\"  (string) The transaction id.\n"
//                 "\nExamples:\n"
//                 + HelpExampleCli("dispersebalance", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
//                 + HelpExampleRpc("dispersebalance", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1"));
//     }

//     EnsureWalletIsUnlocked();

//     CKeyID sendKeyId;

//     if (!GetKeyId(params[0].get_str(), sendKeyId)) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "send address Invalid  ");
//     }
//     if(!pWalletMain->HaveKey(sendKeyId)) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "send address Invalid  ");
//     }

//     CRegID sendRegId;
//     if (!pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId)) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "send address not activated  ");
//     }

//     int64_t nAmount = 0;
//     nAmount = params[1].get_real() * COIN;
//     if(nAmount <= 0) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "nAmount <= 0  ");
//     }

//     set<CKeyID> keyIds;
//     pWalletMain->GetKeys(keyIds); //get addrs
//     if (keyIds.empty()) {
//         throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No Key In wallet \n");
//     }

//     Array arrayTxIds;
//     Object retObj;
//     set<CKeyID>::iterator it;
//     CKeyID recvKeyId;

//     for (it = keyIds.begin(); it!=keyIds.end(); it++) {
//         recvKeyId = *it;
//         if (recvKeyId.IsNull()) {
//             continue;
//         }

//         if(sendKeyId.ToString() == recvKeyId.ToString())
//             continue;

//         CRegID revreg;
//         CUserID rev;

//         if (pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), revreg)) {
//             rev = revreg;
//         } else {
//             rev = recvKeyId;
//         }

//         if (pCdMan->pAccountCache->GetAccountFreeAmount(sendreg, SYMB::WICC) < nAmount + SysCfg().GetTxFee()) {
//             break;
//         }

//         CTransaction tx(sendreg, rev, SysCfg().GetTxFee(), nAmount , chainActive.Height());

//         if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
//             continue;
//         }

//         std::tuple<bool,string> ret = pWalletMain->CommitTx((CBaseTx *) &tx);
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

    if (!BackupWallet(*pWalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}

static void LockWallet()
{
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = 0;
    pWalletMain->Lock();
}

Value walletpassphrase(const Array& params, bool fHelp)
{
    if (pWalletMain->IsEncrypted() && (fHelp || params.size() != 2))
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

    LOCK2(cs_main, pWalletMain->cs_wallet);

    if (fHelp)
        return true;

    if (!pWalletMain->IsEncrypted())
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
        if (!pWalletMain->Unlock(strWalletPass))
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
    if (pWalletMain->IsEncrypted() && (fHelp || params.size() != 2))
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

    if (!pWalletMain->IsEncrypted())
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

    if (!pWalletMain->ChangeWalletPassphrase(strOldWalletPass, strNewWalletPass))
        throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    Object retObj;
    retObj.push_back(Pair("chgpwd", true));
    return retObj;
}

Value encryptwallet(const Array& params, bool fHelp)
{
    if (fHelp || (!pWalletMain->IsEncrypted() && params.size() != 1)) {
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
    LOCK2(cs_main, pWalletMain->cs_wallet);

    if (pWalletMain->IsEncrypted())
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

    if (!pWalletMain->EncryptWallet(strWalletPass))
        throw JSONRPCError(RPC_WALLET_ENCRYPTION_FAILED, "Error: Failed to encrypt the wallet.");

    //BDB seems to have a bad habit of writing old data into
    //slack space in .dat files; that is bad if the old data is
    //unencrypted private keys. So:
    StartShutdown();

//    string defaultFileName = SysCfg().GetArg("-wallet", "wallet.dat");
//    string strFileCopy = defaultFileName + ".rewrite";
//
//    boost::filesystem::remove(GetDataDir() / defaultFileName);
//    boost::filesystem::rename(GetDataDir() / strFileCopy, GetDataDir() / defaultFileName);

    Object retObj;
    retObj.push_back( Pair("wallet_encrypted", true) );
    return retObj;
}

Value walletlock(const Array& params, bool fHelp)
{
    if (fHelp || (pWalletMain->IsEncrypted() && params.size() != 0)) {
        throw runtime_error("walletlock\n"
            "\nRemoves the wallet encryption key from memory, hence locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked first.\n"
            "\nExamples:\n"
            "\nSet the passphrase for 2 minutes to perform a transaction\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 120") +
            "\nPerform a send (requires passphrase set)\n"
            + HelpExampleCli("send", "\"0-1\" \"0-2\" 10000 10000") +
            "\nClear the passphrase since we are done before 2 minutes is up\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("walletlock", ""));
    }

    if (!pWalletMain->IsEncrypted()) {
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE,
            "Error: running with an unencrypted wallet, but walletlock was called.");
    }

    {
        LOCK(cs_nWalletUnlockTime);
        pWalletMain->Lock();
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
            "  \"unconfirmed_tx_num\": xxxxxx,   (numeric) the number of unconfirmed tx in the wallet\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getwalletinfo", "")
            + HelpExampleRpc("getwalletinfo", "")
        );
    }

    Object obj;
    obj.push_back(Pair("wallet_version",    pWalletMain->GetVersion()));
    obj.push_back(Pair("wallet_balance",    ValueFromAmount(pWalletMain->GetFreeBcoins())));
    obj.push_back(Pair("wallet_encrypted",  pWalletMain->IsEncrypted()));
    obj.push_back(Pair("wallet_locked",     pWalletMain->IsLocked()));
    obj.push_back(Pair("unlocked_until",    nWalletUnlockTime));
    obj.push_back(Pair("coinfirmed_tx_num", (int)pWalletMain->mapInBlockTx.size()));
    obj.push_back(Pair("unconfirmed_tx_num",(int)pWalletMain->unconfirmedTx.size()));
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
