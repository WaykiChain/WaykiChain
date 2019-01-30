// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "util.h"
#include "miner.h"
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

string HelpRequiringPassphrase()
{
    return pwalletMain && pwalletMain->IsEncrypted()
        ? "\nRequires wallet passphrase to be set with walletpassphrase call."
        : "";
}

void EnsureWalletIsUnlocked()
{
    if (pwalletMain->IsLocked())
        throw JSONRPCError(RPC_WALLET_UNLOCK_NEEDED, 
            "Error: Please enter the wallet passphrase with walletpassphrase first.");
}

Value islocked(const Array& params,  bool fHelp)
{
    if(fHelp)
        return true;
    Object obj;
    if(!pwalletMain->IsEncrypted()) {       // decrypted
        obj.push_back(Pair("islock", 0));
    } else if (!pwalletMain->IsLocked()) {  // encryped but unlocked
        obj.push_back(Pair("islock", 1));
    } else {
        obj.push_back(Pair("islock", 2));   // encryped and locked
    }
    return obj;
}

bool GetKeyId(string const &addr,CKeyID &KeyId)
{
    if (!CRegID::GetKeyID(addr, KeyId)) {
        KeyId = CKeyID(addr);
        return (!KeyId.IsEmpty());
    }
    return true;
}

Value getnewaddress(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 1)
        throw runtime_error(
            "getnewaddress  (\"IsMiner\")\n"
            "\nget a new address\n"
            "\nArguments:\n"
            "1. \"IsMiner\" (bool, optional)  If true, it creates two sets of key-pairs, one of which is for miner.\n"
           "\nExamples:\n"
            + HelpExampleCli("getnewaddress", "")
            + HelpExampleCli("getnewaddress", "true")
        );
    EnsureWalletIsUnlocked();

    bool IsForMiner = false;
    if (params.size() == 1) {
        RPCTypeCheck(params, list_of(bool_type));
        IsForMiner = params[0].get_bool();
    }

    CKey userkey;
    userkey.MakeNewKey();

    Key minerKey;
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
    CKeyID userKeyID = userPubKey.GetKeyID();
    Object obj;
    obj.push_back( Pair("addr", userKeyID.ToAddress()) );
    obj.push_back( Pair("minerpubkey", minerPubKey) ); // "null" for non-miner address
    return obj;
}

Value signmessage(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "signmessage \"WICC address\" \"message\"\n"
            "\nSign a message with the private key of an address"
            + HelpRequiringPassphrase() + "\n"
            "\nArguments:\n"
            "1. \"WICC address\"  (string, required) The Coin address associated with the private key to sign.\n"
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

static std::tuple<bool, string> SendMoney(const CRegID &sendRegId, const CUserID &recvRegId, 
    int64_t nValue, int64_t nFee) {
    CTransaction tx;
    tx.srcRegId = sendRegId;
    tx.desUserId = recvRegId;
    tx.llValues = nValue;
    tx.llFees = (0 == nFee) ? SysCfg().GetTxFee() : nFee;
    tx.nValidHeight = chainActive.Tip()->nHeight;

    CKeyID keyID;
    if (!pAccountViewTip->GetKeyId(sendRegId, keyID)) {
        return std::make_tuple(false, "key or keID failed");
    }
    if (!pwalletMain->Sign(keyID, tx.SignatureHash(), tx.signature)) {
        return std::make_tuple(false, "SendMoney Sign failed");
    }

    std::tuple<bool, string> ret = pwalletMain->CommitTransaction((CBaseTransaction *)&tx);
    bool flag = std::get<0>(ret);
    string te = std::get<1>(ret);
    if (flag == true)
        te = tx.GetHash().ToString();
    return std::make_tuple(flag, te.c_str());
}

Value sendtoaddress(const Array& params, bool fHelp)
{
    int size = params.size();
    if (fHelp || (size != 2 && size != 3)) {
        throw runtime_error(
                "sendtoaddress (\"sendaddress\") \"recvaddress\" \"amount\"\n"
                "\nSend an amount to a given address. The amount is a real and is rounded to the nearest 0.00000001\n"
                + HelpRequiringPassphrase() + "\nArguments:\n"
                "1. \"sendaddress\" (string, optional) The address where coins are sent from.\n"
                "2. \"recvaddress\" (string, required) The address where coins are received.\n"
                "3.\"amount\" (string, required) \n"
                "\nResult:\n"
                "\"transactionid\" (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
                + HelpExampleCli("sendtoaddress",
                "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"donation\" \"seans outpost\"")
                + HelpExampleRpc("sendtoaddress",
                "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1, \"donation\", \"seans outpost\""
                + HelpExampleCli("sendtoaddress", "\"0-6\" 10 ")
                + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 10 ")
                + HelpExampleCli("sendtoaddress", "\"0-6\" \"0-5\" 10 ")
                + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" \"0-6\"10 ")));
    }

    EnsureWalletIsUnlocked();

    CRegID sendRegId, recvRegId;
    CKeyID sendKeyId, recvKeyId;
    int64_t nAmount = 0;
    int64_t nDefaultFee = SysCfg().GetTxFee();

    if (size == 3) {
        if (!GetKeyId(params[0].get_str(), sendKeyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");
        }
        if (!GetKeyId(params[1].get_str(), recvKeyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");
        }
        nAmount = AmountToRawValue(params[2]);
        if (pAccountViewTip->GetRawBalance(sendKeyId) < nAmount + nDefaultFee) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "sendaddress does not have enough coins");
        }
    } else {
        if (!GetKeyId(params[0].get_str(), recvKeyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");
        }
        nAmount = AmountToRawValue(params[1]);

        set<CKeyID> sKeyIds;
        sKeyIds.clear();
        pwalletMain->GetKeys(sKeyIds);
        if(sKeyIds.empty()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Wallet has no key");
        }
        bool sufficientFee = false;
        for (auto &keyId: sKeyIds) {
            if (pAccountViewTip->GetRawBalance(keyId) >= nAmount + nDefaultFee) {
                sendKeyId = keyId;
                sufficientFee = true;
                break;
            }
        }
        if (!sufficientFee) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Insufficient coins in wallet to send");
        }
    }

    if (!pAccountViewTip->GetRegId(CUserID(sendKeyId), sendRegId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "sendadress not registered or invalid");
    }

    std::tuple<bool, string> ret;
    if (pAccountViewTip->GetRegId(CUserID(recvKeyId), recvRegId)) {
        ret = SendMoney(sendRegId, recvRegId, nAmount, nDefaultFee);
    } else { //receiver key not registered yet
        ret = SendMoney(sendRegId, CUserID(recvKeyId), nAmount, nDefaultFee);
    }
    Object obj;
    obj.push_back(Pair(std::get<0>(ret) ? "hash" : "error code", std::get<1>(ret)));
    return obj;
}

Value sendtoaddresswithfee(const Array& params, bool fHelp)
{
    int size = params.size();
    if (fHelp || (size != 3 && size != 4)) {
        throw runtime_error(
                "sendtoaddresswithfee (\"sendaddress\") \"recvaddress\" \"amount\" (fee)\n"
                "\nSend an amount to a given address with fee. The amount is a real and is rounded to the nearest 0.00000001\n"
                "\nArguments:\n"
                "1.\"sendaddress\"  (string, optional) The Coin address to send to.\n"
                "2.\"recvaddress\"  (string, required) The Coin address to receive.\n"
                "3.\"amount\"       (string,required) \n"
                "4.\"fee\"          (string,required) \n"
                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("sendtoaddresswithfee", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 10000000 1000")
                + HelpExampleCli("sendtoaddresswithfee",
                "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"donation\" \"seans outpost\"")
                + HelpExampleRpc("sendtoaddresswithfee",
                "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1, \"donation\", \"seans outpost\""
                + HelpExampleCli("sendtoaddresswithfee", "\"0-6\" 10 ")
                + HelpExampleCli("sendtoaddresswithfee", "\"00000000000000000005\" 10 ")
                + HelpExampleCli("sendtoaddresswithfee", "\"0-6\" \"0-5\" 10 ")
                + HelpExampleCli("sendtoaddresswithfee", "\"00000000000000000005\" \"0-6\"10 ")));
    }

    EnsureWalletIsUnlocked();

    CRegID sendRegId, recvRegId;
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
            char errorMsg[100] = {'\0'};
            sprintf(errorMsg, "Given fee(%ld) < Default fee (%ld)", nFee, nDefaultFee);
            throw JSONRPCError(RPC_INSUFFICIENT_FEE, string(errorMsg));
        }
        if (pAccountViewTip->GetRawBalance(sendKeyId) < nAmount + nActualFee) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "sendaddress does not have enough coins");
        }
    } else { //sender address omitted
        if (!GetKeyId(params[0].get_str(), recvKeyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid recvaddress");
        }
        nAmount = AmountToRawValue(params[1]);
        nFee = AmountToRawValue(params[2]);
        nActualFee = max(nDefaultFee, nFee);
        if (nFee < nDefaultFee) {
            char errorMsg[100] = {'\0'};
            sprintf(errorMsg, "Given fee(%ld) < Default fee (%ld)", nFee, nDefaultFee);
            throw JSONRPCError(RPC_INSUFFICIENT_FEE, string(errorMsg));
        }

        set<CKeyID> sKeyIds;
        sKeyIds.clear();
        pwalletMain->GetKeys(sKeyIds);
        if (sKeyIds.empty()) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Wallet has no key!");
        }
        bool sufficientFee = false;
        for (auto &keyId: sKeyIds) {
            if (pAccountViewTip->GetRawBalance(keyId) >= nAmount + nActualFee) {
                sendKeyId = keyId;
                sufficientFee = true;
                break;
            }
        }
        if (!sufficientFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE, "Insufficient coins in wallet to send");
        }
    }

    if (!pAccountViewTip->GetRegId(CUserID(sendKeyId), sendRegId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "sendaddress not registered or invalid");
    }

    std::tuple<bool,string> ret;
    if (pAccountViewTip->GetRegId(CUserID(recvKeyId), recvRegId)) {
        ret = SendMoney(sendRegId, recvRegId, nAmount, nFee);
    } else { //receiver key not registered yet
        ret = SendMoney(sendRegId, CUserID(recvKeyId), nAmount, nFee);
    }
    Object obj;
    obj.push_back(Pair(std::get<0>(ret) ? "hash" : "error code", std::get<1>(ret)));
    return obj;
}

Value getsendtoaddresstxraw(const Array& params, bool fHelp)
{
    int size = params.size();
    if (fHelp || size < 4 || size > 5 ) {
        throw runtime_error(
                "getsendtoaddresstxraw \"fee\" \"amount\" \"sendaddress\" \"recvaddress\" \"height\"\n"
                "\n create common transaction by height: fee, amount, sendaddress, recvaddress\n"
                + HelpRequiringPassphrase() + "\nArguments:\n"
                "1. \"fee\"     (numeric, required)  \n"
                "2. \"amount\"  (numeric, required)  \n"
                "3. \"sendaddress\"  (string, required) The Coin address to send to.\n"
                "4. \"recvaddress\"  (string, required) The Coin address to receive.\n"
                "5. \"height\"  (int, optional) \n"
                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("getsendtoaddresstxraw", "100 1000 \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
                + HelpExampleCli("getsendtoaddresstxraw",
                "100 1000 \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1 \"donation\" \"seans outpost\"")
                + HelpExampleRpc("getsendtoaddresstxraw",
                "100 1000 \"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1, \"donation\", \"seans outpost\""
                + HelpExampleCli("getsendtoaddresstxraw", "\"0-6\" 10 ")
                + HelpExampleCli("getsendtoaddresstxraw", "100 1000 \"00000000000000000005\" 10 ")
                + HelpExampleCli("getsendtoaddresstxraw", "100 1000 \"0-6\" \"0-5\" 10 ")
                + HelpExampleCli("getsendtoaddresstxraw", "100 1000 \"00000000000000000005\" \"0-6\"10 ")));
    }

    CKeyID sendKeyId, recvKeyId;

    auto GetUserID = [](string const &addr, CUserID &userId) {
        CRegID regId(addr);
        if(!regId.IsEmpty()) {
            userId = regId;
            return true;
        }

        CKeyID keyId(addr);
        if(!keyId.IsEmpty()) {
            userId = keyId;
            return true;
        }

        return false;
    };

    int64_t Fee = AmountToRawValue(params[0]);
    int64_t nAmount = AmountToRawValue(params[1]);
    if(nAmount == 0){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "send 0 amount disallowed!");
    }

    CUserID  sendId, recvId;
    if (!GetUserID(params[2].get_str(), sendId)){
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "sendaddress invalid");
    }
    if (!pAccountViewTip->GetKeyId(sendId, sendKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Get CKeyID failed from CUserID");
    }
    if (sendId.type() == typeid(CKeyID)) {
        CRegID regId;
        if(!pAccountViewTip->GetRegId(sendId, regId)){
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "sendaddress not registed");
        }
        sendId = regId;
    }

    if (!GetUserID(params[3].get_str(), recvId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "recvaddress invalid");
    }

    if (recvId.type() == typeid(CKeyID)) {
        CRegID regId;
        if (pAccountViewTip->GetRegId(recvId, regId)) {
            recvId = regId;
        }
    }

    int height = chainActive.Tip()->nHeight;
    if (params.size() > 4) {
        height = params[4].get_int();
    }

    std::shared_ptr<CTransaction> tx = std::make_shared<CTransaction>(sendId, recvId, Fee, nAmount, height);
    if (!pwalletMain->Sign(sendKeyId, tx->SignatureHash(), tx->signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,  "Sign failed");
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTransaction> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

#pragma pack(1)
typedef struct {
    unsigned char systype;
    unsigned char type;
    unsigned char address[34]; // coin transfer address

    IMPLEMENT_SERIALIZE
    (
        READWRITE(systype);
        READWRITE(type);
        for (unsigned int i = 0; i < 34; ++i)
            READWRITE(address[i]);
    )
}TRAN_USER;
#pragma pack()

Value notionalpoolingasset(const Array& params, bool fHelp)
{
    if(fHelp || params.size() < 2) {
        throw runtime_error(
                 "notionalpoolingasset \"scriptid\" \"recvaddress\"\n"
                 "\nThe collection of all assets\n"
                 "\nArguments:\n"
                 "1.\"scriptid\": (string, required)\n"
                 "2.\"recvaddress\"  (string, required) The Popcoin address to receive.\n"
                 "3.\"amount\" (number optional)\n"
                 "\nResult:\n"
                 "\nExamples:\n"
                 + HelpExampleCli("notionalpoolingasset", "11-1 pPKAiv9v4EaKjZGg7yWqnFJbhdZLVLyX8N 10")
                 + HelpExampleRpc("notionalpoolingasset", "11-1 pPKAiv9v4EaKjZGg7yWqnFJbhdZLVLyX8N 10"));
    }

    EnsureWalletIsUnlocked();
    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() == true) {
        throw runtime_error("in notionalpoolingasset :scriptid size is error!\n");
    }

    if (!pScriptDBTip->HaveScript(regid)) {
        throw runtime_error("in notionalpoolingasset :scriptid  is not exist!\n");
    }

    int64_t nAmount = 10 * COIN;
    if(3 == params.size())
        nAmount = params[2].get_real() * COIN;

    CKeyID recvKeyId;
    Object retObj;
    string strRevAddr = params[1].get_str();
    if (!GetKeyId(strRevAddr, recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "to address Invalid  ");
    }

    if(!pwalletMain->HaveKey(recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "to address Invalid  ");
    }

    set<CKeyID> sKeyid;
    pwalletMain->GetKeys(sKeyid);
    if(sKeyid.empty())
    {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No Key In wallet \n");
    }
    set<CKeyID> sResultKeyid;
    for (auto &te : sKeyid) {
        if(te.ToString() == recvKeyId.ToString())
            continue;
        if (pAccountViewTip->GetRawBalance(te) >= nAmount)
            sResultKeyid.insert(te);
    }

    set<CKeyID>::iterator it;
    CKeyID sendKeyId;
    Array arrayTxIds;

    CUserID rev;
    CRegID revreg;

    for (it = sResultKeyid.begin(); it != sResultKeyid.end(); it++) {
        sendKeyId = *it;

        if (sendKeyId.IsNull()) {
            continue;
        }

        CRegID sendreg;
        if (!pAccountViewTip->GetRegId(CUserID(sendKeyId), sendreg)) {
            continue;
        }

        TRAN_USER tu;
        memset(&tu, 0, sizeof(TRAN_USER));
        tu.systype = 0xff;
        tu.type = 0x03;
        memcpy(&tu.address, strRevAddr.c_str(), 34);

        CDataStream scriptData(SER_DISK, CLIENT_VERSION);
        scriptData << tu;
        string sendcontract = HexStr(scriptData);
        LogPrint("vm", "sendcontract=%s\n",sendcontract.c_str());

        vector_unsigned_char pContract;
        pContract = ParseHex(sendcontract);

        int nFuelRate = GetElementForBurn(chainActive.Tip());
        const int STEP = 645;
        int64_t nFee = (STEP / 100 + 1) * nFuelRate + SysCfg().GetTxFee();
        LogPrint("vm", "nFuelRate=%d, nFee=%lld\n",nFuelRate, nFee);
        CTransaction tx(sendreg, regid, nFee, 0 , chainActive.Height(), pContract);
        if (!pwalletMain->Sign(sendKeyId, tx.SignatureHash(), tx.signature)) {
            continue;
        }

        std::tuple<bool,string> ret = pwalletMain->CommitTransaction((CBaseTransaction *) &tx);
        if(!std::get<0>(ret))
             continue;
        arrayTxIds.push_back(std::get<1>(ret));
    }

    retObj.push_back( Pair("Tx", arrayTxIds) );
    return retObj;
}

Value getassets(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1) {
        throw runtime_error(
                 "getassets \"scriptid\"\n"
                 "\nThe collection of all assets\n"
                 "\nArguments:\n"
                 "1.\"scriptid\": (string, required)\n"
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

        std::shared_ptr<CAppUserAccout> temp = std::make_shared<CAppUserAccout>();
        if (!pScriptDBTip->GetScriptAcc(regid, veckey, *temp.get())) {
            continue;
        }

        temp.get()->AutoMergeFreezeToFree(chainActive.Tip()->nHeight);
        uint64_t freeValues = temp.get()->getllValues();
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

Value notionalpoolingbalance(const Array& params, bool fHelp)
{
    int size = params.size();
    if (fHelp || (size < 1)) {
        throw runtime_error(
                "notionalpoolingbalance  \"receive address\" \"amount\"\n"
                "\nSend an amount to a given address. The amount is a real and is rounded to the nearest 0.00000001\n"
                + HelpRequiringPassphrase() + "\nArguments:\n"
                "1. receive address   (string, required) The Koala address to receive\n"
                "2. amount (number optional)\n"
                "\nResult:\n"
                "\"transactionid\"  (string) The transaction id.\n"
                "\nExamples:\n"
                + HelpExampleCli("notionalpoolingbalance", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 0.1")
                + HelpExampleRpc("notionalpoolingbalance",
                "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\", 0.1, \"donation\", \"seans outpost\""));
    }

    EnsureWalletIsUnlocked();
    CKeyID sendKeyId;
    CKeyID recvKeyId;

    // Amount
    Object retObj;
    CRegID sendreg;
    //// from address to address

    if (!GetKeyId(params[0].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "to address Invalid  ");
    }

    if(!pwalletMain->HaveKey(recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "to address Invalid  ");
    }
/*
    nAmount = params[1].get_real() * COIN;
    if(nAmount <= SysCfg().GetTxFee())
        nAmount = 0.01 * COIN;
*/
    int64_t nAmount = 10 * COIN;
    if (2 == params.size())
        nAmount = params[1].get_real() * COIN;

    set<CKeyID> sKeyid;
    sKeyid.clear();
    pwalletMain->GetKeys(sKeyid); //get addrs
    if(sKeyid.empty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No Key In wallet \n");
    }
    set<CKeyID> sResultKeyid;
    for (auto &te : sKeyid) {
        if(te.ToString() == recvKeyId.ToString())
            continue;
        if (pAccountViewTip->GetRawBalance(te) > (nAmount + SysCfg().GetTxFee())) {
            if (pAccountViewTip->GetRegId(CUserID(te), sendreg)) {
                sResultKeyid.insert(te);
            }
        }
    }

    Array arrayTxIds;
    set<CKeyID>::iterator it;

    for (it = sResultKeyid.begin(); it != sResultKeyid.end(); it++) {
        sendKeyId = *it;
        if (sendKeyId.IsNull()) {
            continue;
        }

        CRegID recvRegId;
        CUserID recvUserId;

        if (!pAccountViewTip->GetRegId(CUserID(sendKeyId), sendreg)) {
            continue;
        }
        recvUserId = pAccountViewTip->GetRegId(CUserID(recvKeyId), recvRegId) ? recvRegId : recvKeyId;
        CTransaction tx(sendreg, recvUserId, SysCfg().GetTxFee(), 
            pAccountViewTip->GetRawBalance(sendreg) - SysCfg().GetTxFee() - nAmount,
            chainActive.Height());

        if (!pwalletMain->Sign(sendKeyId, tx.SignatureHash(), tx.signature)) {
            continue;
        }
        std::tuple<bool,string> ret = pwalletMain->CommitTransaction((CBaseTransaction *) &tx);
        if(!std::get<0>(ret))
             continue;

        arrayTxIds.push_back(std::get<1>(ret));
    }

    retObj.push_back(Pair("Tx", arrayTxIds));
    return retObj;
}

// Value (const Array& params, bool fHelp)
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

//         std::tuple<bool,string> ret = pwalletMain->CommitTransaction((CBaseTransaction *) &tx);
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
        throw runtime_error(
            "backupwallet \"destination\"\n"
            "\nSafely copies wallet.dat to destination, which can be a directory or a path with filename.\n"
            "\nArguments:\n"
            "1. \"destination\"   (string, required) The destination directory or file\n"
            "\nExamples:\n"
            + HelpExampleCli("backupwallet", "\"backup.dat\"")
            + HelpExampleRpc("backupwallet", "\"backup.dat\"")
        );

    string strDest = params[0].get_str();
    if (!BackupWallet(*pwalletMain, strDest))
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Wallet backup failed!");

    return Value::null;
}

static void LockWallet(CWallet* pWallet)
{
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = 0;
    pWallet->Lock();
}

Value walletpassphrase(const Array& params, bool fHelp)
{
    if (pwalletMain->IsEncrypted() && (fHelp || params.size() != 2))
        throw runtime_error(
            "walletpassphrase \"passphrase\" timeout\n"
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
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 60") +
            "\nLock the wallet again (before 60 seconds)\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("walletpassphrase", "\"my pass phrase\", 60")
        );

    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fHelp)
        return true;
    if (!pwalletMain->IsEncrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrase was called.");

    // Note that the walletpassphrase is stored in params[0] which is not mlock()ed
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    // TODO: get rid of this .c_str() by implementing SecureString::operator=(string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    strWalletPass = params[0].get_str().c_str();
    //assert(0);
    if (strWalletPass.length() > 0)
    {
        if (!pwalletMain->Unlock(strWalletPass))
            throw JSONRPCError(RPC_WALLET_PASSPHRASE_INCORRECT, "Error: The wallet passphrase entered was incorrect.");
    }
    else
        throw runtime_error(
            "walletpassphrase <passphrase> <timeout>\n"
            "Stores the wallet decryption key in memory for <timeout> seconds.");

    int64_t nSleepTime = params[1].get_int64();
    LOCK(cs_nWalletUnlockTime);
    nWalletUnlockTime = GetTime() + nSleepTime;
    RPCRunLater("lockwallet", boost::bind(LockWallet, pwalletMain), nSleepTime);
    Object retObj;
    retObj.push_back(Pair("passphrase", true));
    return retObj;
}

Value walletpassphrasechange(const Array& params, bool fHelp)
{
    if (pwalletMain->IsEncrypted() && (fHelp || params.size() != 2))
        throw runtime_error(
            "walletpassphrasechange \"oldpassphrase\" \"newpassphrase\"\n"
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
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletpassphrasechange was called.");

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

Value walletlock(const Array& params, bool fHelp)
{
    if (pwalletMain->IsEncrypted() && (fHelp || params.size() != 0))
        throw runtime_error(
            "walletlock\n"
            "\nRemoves the wallet encryption key from memory, locking the wallet.\n"
            "After calling this method, you will need to call walletpassphrase again\n"
            "before being able to call any methods which require the wallet to be unlocked.\n"
            "\nExamples:\n"
            "\nSet the passphrase for 2 minutes to perform a transaction\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\" 120") +
            "\nPerform a send (requires passphrase set)\n"
            + HelpExampleCli("sendtoaddress", "\"1M72Sfpbz1BPpXFHz9m3CdqATR44Jvaydd\" 1.0") +
            "\nClear the passphrase since we are done before 2 minutes is up\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs json rpc call\n"
            + HelpExampleRpc("walletlock", "")
        );

    if (fHelp)
        return true;
    if (!pwalletMain->IsEncrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: running with an unencrypted wallet, but walletlock was called.");
    {
        LOCK(cs_nWalletUnlockTime);
        pwalletMain->Lock();
        nWalletUnlockTime = 0;
    }
    Object retObj;
    retObj.push_back(Pair("walletlock", true));
    return retObj;
}

Value encryptwallet(const Array& params, bool fHelp)
{
    if (!pwalletMain->IsEncrypted() && (fHelp || params.size() != 1))
        throw runtime_error(
            "encryptwallet \"passphrase\"\n"
            "\nEncrypts the wallet with 'passphrase'. This is for first time encryption.\n"
            "After this, any calls that interact with private keys such as sending or signing \n"
            "will require the passphrase to be set prior the making these calls.\n"
            "Use the walletpassphrase call for this, and then walletlock call.\n"
            "If the wallet is already encrypted, use the walletpassphrasechange call.\n"
            "Note that this will shutdown the server.\n"
            "\nArguments:\n"
            "1. \"passphrase\"    (string, required) The pass phrase to encrypt the wallet with. It must be at least 1 character, but should be long.\n"
            "\nExamples:\n"
            "\nEncrypt you wallet\n"
            + HelpExampleCli("encryptwallet", "\"my pass phrase\"") +
            "\nNow set the passphrase to use the wallet, such as for signing or sending Coin\n"
            + HelpExampleCli("walletpassphrase", "\"my pass phrase\"") +
            "\nNow we can so something like sign\n"
            + HelpExampleCli("signmessage", "\"WICC address\" \"test message\"") +
            "\nNow lock the wallet again by removing the passphrase\n"
            + HelpExampleCli("walletlock", "") +
            "\nAs a json rpc call\n"
            + HelpExampleRpc("encryptwallet", "\"my pass phrase\"")
        );
    LOCK2(cs_main, pwalletMain->cs_wallet);

    if (fHelp)
        return true;
    if (pwalletMain->IsEncrypted())
        throw JSONRPCError(RPC_WALLET_WRONG_ENC_STATE, "Error: Wallet was already encrypted and shall not be encrypted again.");

    // TODO: get rid of this .c_str() by implementing SecureString::operator=(string)
    // Alternately, find a way to make params[0] mlock()'d to begin with.
    SecureString strWalletPass;
    strWalletPass.reserve(100);
    strWalletPass = params[0].get_str().c_str();

    if (strWalletPass.length() < 1)
        throw runtime_error(
            "encryptwallet <passphrase>\n"
            "Encrypts the wallet with <passphrase>.");

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
    retObj.push_back(Pair("encrypt", true));
    return retObj;
    //return "wallet encrypted; Coin server stopping, restart to run with encrypted wallet. The keypool has been flushed, you need to make a new backup.";
}

Value settxfee(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 1 || params.size() > 1)
        throw runtime_error(
            "settxfee \"amount\"\n"
            "\nSet the default transaction fee per kB.\n"
            "\nArguments:\n"
            "1. amount         (numeric, required) The transaction fee in WICC/kB rounded to the nearest 0.00000001\n"
            "\nResult\n"
            "true|false        (boolean) Returns true if successful\n"
            "\nExamples:\n"
            + HelpExampleCli("settxfee", "0.00001")
            + HelpExampleRpc("settxfee", "0.00001")
        );

    // Amount
    int64_t nAmount = 0;
    if (params[0].get_real() != 0.0)
    {
       nAmount = AmountToRawValue(params[0]);        // rejects 0.0 amounts
       SysCfg().SetDefaultTxFee(nAmount);
    }

    return true;
}

Value getwalletinfo(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 0)
        throw runtime_error(
            "getwalletinfo\n"
            "Returns an object containing various wallet state info.\n"
            "\nResult:\n"
            "{\n"
            "  \"walletversion\": xxxxx,     (numeric) the wallet version\n"
            "  \"balance\": xxxxxxx,         (numeric) the total Coin balance of the wallet\n"
            "  \"Inblocktx\": xxxxxxx,       (numeric) the size of transactions in the wallet\n"
            "  \"uncomfirmedtx\": xxxxxx,    (numeric) the size of unconfirmtx transactions in the wallet\n"
            "  \"unlocked_until\": ttt,      (numeric) the timestamp in seconds since epoch (midnight Jan 1 1970 GMT) that the wallet is unlocked for transfers, or 0 if the wallet is locked\n"
            "}\n"
            "\nExamples:\n"
            + HelpExampleCli("getwalletinfo", "")
            + HelpExampleRpc("getwalletinfo", "")
        );

    Object obj;
    obj.push_back(Pair("walletversion", pwalletMain->GetVersion()));
    obj.push_back(Pair("balance",       ValueFromAmount(pwalletMain->GetRawBalance())));
    obj.push_back(Pair("Inblocktx",       (int)pwalletMain->mapInBlockTx.size()));
    obj.push_back(Pair("unconfirmtx", (int)pwalletMain->UnConfirmTx.size()));
    if (pwalletMain->IsEncrypted())
        obj.push_back(Pair("unlocked_until", nWalletUnlockTime));
    return obj;
}

Value getsignature(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2)
           throw runtime_error(
               "getsignature\n"
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
        if (!fGood) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

        CKey key = vchSecret.GetKey();
        if (!key.IsValid()) throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key invalid");
        vector<unsigned char> signature;
        uint256 hash;
        hash.SetHex(params[1].get_str());
        if(key.Sign(hash, signature))
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Private key sign hash failed");

       Object obj;
       obj.push_back(Pair("signature", HexStr(signature.begin(), signature.end())));
       return obj;
}
