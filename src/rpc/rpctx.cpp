// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "txdb.h"

#include "base58.h"
#include "rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "syncdatadb.h"
//#include "checkpoints.h"
#include "configuration.h"
#include "miner.h"
#include "main.h"
#include "vm/script.h"
#include "vm/vmrunenv.h"
#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"

#include "boost/tuple/tuple.hpp"
#define revert(height) ((height<<24) | (height << 8 & 0xff0000) |  (height>>8 & 0xff00) | (height >> 24))

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

extern CAccountViewDB *pAccountViewDB;

string RegIDToAddress(CUserID &userId) {
    CKeyID keyId;
    if (pAccountViewTip->GetKeyId(userId, keyId))
        return keyId.ToAddress();

    return "cannot get address from given RegId";
}

static bool GetKeyId(string const &addr, CKeyID &KeyId) {
    if (!CRegID::GetKeyID(addr, KeyId)) {
        KeyId = CKeyID(addr);
        if (KeyId.IsEmpty())
            return false;
    }
    return true;
}


Object GetTxDetailJSON(const uint256& txhash) {
    Object obj;
    std::shared_ptr<CBaseTx> pBaseTx;
    {
        LOCK(cs_main);
        CBlock genesisblock;
        CBlockIndex* pgenesisblockindex = mapBlockIndex[SysCfg().HashGenesisBlock()];
        ReadBlockFromDisk(genesisblock, pgenesisblockindex);
        assert(genesisblock.GetHashMerkleRoot() == genesisblock.BuildMerkleTree());
        for (unsigned int i = 0; i < genesisblock.vptx.size(); ++i) {
            if (txhash == genesisblock.GetTxHash(i)) {
                obj = genesisblock.vptx[i]->ToJson(*pAccountViewTip);
                obj.push_back(Pair("blockhash", SysCfg().HashGenesisBlock().GetHex()));
                obj.push_back(Pair("confirmedheight", (int) 0));
                obj.push_back(Pair("confirmedtime", (int) genesisblock.GetTime()));
                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << genesisblock.vptx[i];
                obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                return obj;
            }
        }

        if (SysCfg().IsTxIndex()) {
            CDiskTxPos postx;
            if (pScriptDBTip->ReadTxIndex(txhash, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                    obj = pBaseTx->ToJson(*pAccountViewTip);
                    obj.push_back(Pair("blockhash", header.GetHash().GetHex()));
                    obj.push_back(Pair("confirmedheight", (int) header.GetHeight()));
                    obj.push_back(Pair("confirmedtime", (int) header.GetTime()));

                    if (pBaseTx->nTxType == CONTRACT_TX) {
                        vector<CVmOperate> vOutput;
                        pScriptDBTip->ReadTxOutPut(pBaseTx->GetHash(), vOutput);
                        Array outputArray;
                        for (auto& item : vOutput) {
                            outputArray.push_back(item.ToJson());
                        }
                        obj.push_back(Pair("listOutput", outputArray));
                    }
                    CDataStream ds(SER_DISK, CLIENT_VERSION);
                    ds << pBaseTx;
                    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                } catch (std::exception &e) {
                    throw runtime_error(tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
                }
                return obj;
            }
        }
        {
            pBaseTx = mempool.Lookup(txhash);
            if (pBaseTx.get()) {
                obj = pBaseTx->ToJson(*pAccountViewTip);
                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << pBaseTx;
                obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
                return obj;
            }
        }
    }
    return obj;
}

Array GetTxAddressDetail(std::shared_ptr<CBaseTx> pBaseTx)
{
    Array arrayDetail;
    Object obj;
    std::set<CKeyID> vKeyIdSet;
    switch(pBaseTx->nTxType) {
    case REWARD_TX:
    {
        if (!pBaseTx->GetAddress(vKeyIdSet, *pAccountViewTip, *pScriptDBTip))
            return arrayDetail;

        obj.push_back(Pair("address", vKeyIdSet.begin()->ToAddress()));
        obj.push_back(Pair("category", "receive"));
        double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
        obj.push_back(Pair("amount", dAmount));
        obj.push_back(Pair("txtype", "REWARD_TX"));
        arrayDetail.push_back(obj);
        break;
    }
    case REG_ACCT_TX:
    {
        if (!pBaseTx->GetAddress(vKeyIdSet, *pAccountViewTip, *pScriptDBTip))
            return arrayDetail;

        obj.push_back(Pair("address", vKeyIdSet.begin()->ToAddress()));
        obj.push_back(Pair("category", "send"));
        double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
        obj.push_back(Pair("amount", -dAmount));
        obj.push_back(Pair("txtype", "REG_ACCT_TX"));
        arrayDetail.push_back(obj);
        break;
    }
    case COMMON_TX:
    {
        CCommonTx* ptx = (CCommonTx*)pBaseTx.get();
        CKeyID sendKeyID;
        CRegID sendRegID = boost:: get < CRegID > (ptx->srcRegId);
        sendKeyID = sendRegID.GetKeyID(*pAccountViewTip);
        CKeyID recvKeyID;
        if (ptx->desUserId.type() == typeid(CKeyID)) {
            recvKeyID = boost::get<CKeyID>(ptx->desUserId);
        } else if (ptx->desUserId.type() == typeid(CRegID)) {
            CRegID desRegID = boost::get<CRegID>(ptx->desUserId);
            recvKeyID = desRegID.GetKeyID(*pAccountViewTip);
        }

        obj.push_back(Pair("txtype", "COMMON_TX"));
        obj.push_back(Pair("memo", HexStr(ptx->memo)));
        obj.push_back(Pair("address", sendKeyID.ToAddress()));
        obj.push_back(Pair("category", "send"));
        double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
        obj.push_back(Pair("amount", -dAmount));
        arrayDetail.push_back(obj);
        Object objRec;
        objRec.push_back(Pair("txtype", "COMMON_TX"));
        objRec.push_back(Pair("memo", HexStr(ptx->memo)));
        objRec.push_back(Pair("address", recvKeyID.ToAddress()));
        objRec.push_back(Pair("category", "receive"));
        objRec.push_back(Pair("amount", dAmount));
        arrayDetail.push_back(objRec);

        break;
    }
    case CONTRACT_TX:
    {
        CContractTx* ptx = (CContractTx*)pBaseTx.get();
        CKeyID sendKeyID;
        CRegID sendRegID = boost:: get < CRegID > (ptx->srcRegId);
        sendKeyID = sendRegID.GetKeyID(*pAccountViewTip);
        CKeyID recvKeyID;
        if (ptx->desUserId.type() == typeid(CKeyID)) {
            recvKeyID = boost::get<CKeyID>(ptx->desUserId);
        } else if (ptx->desUserId.type() == typeid(CRegID)) {
            CRegID desRegID = boost::get<CRegID>(ptx->desUserId);
            recvKeyID = desRegID.GetKeyID(*pAccountViewTip);
        }
        obj.push_back(Pair("txtype", "CONTRACT_TX"));
        obj.push_back(Pair("arguments", HexStr(ptx->arguments)));
        obj.push_back(Pair("address", sendKeyID.ToAddress()));
        obj.push_back(Pair("category", "send"));
        double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
        obj.push_back(Pair("amount", -dAmount));
        arrayDetail.push_back(obj);
        Object objRec;
        objRec.push_back(Pair("txtype", "CONTRACT_TX"));
        objRec.push_back(Pair("arguments", HexStr(ptx->arguments)));
        objRec.push_back(Pair("address", recvKeyID.ToAddress()));
        objRec.push_back(Pair("category", "receive"));
        objRec.push_back(Pair("amount", dAmount));
        arrayDetail.push_back(objRec);

        vector<CVmOperate> vOutput;
        pScriptDBTip->ReadTxOutPut(pBaseTx->GetHash(), vOutput);
        Array outputArray;
        for (auto& item : vOutput) {
            Object objOutPut;
            string address;
            if (item.nacctype == regid) {
                vector<unsigned char> vRegId(item.accountid, item.accountid+6);
                CRegID regId(vRegId);
                CUserID userId(regId);
                address = RegIDToAddress(userId);
            } else if (item.nacctype == base58addr) {
                address.assign(item.accountid[0], sizeof(item.accountid));
            }

            objOutPut.push_back(Pair("address", address));

            uint64_t amount;
            memcpy(&amount, item.money, sizeof(item.money));
            double dAmount = amount / COIN;
            if (item.opType == ADD_FREE) {
                objOutPut.push_back(Pair("category", "receive"));
                objOutPut.push_back(Pair("amount", dAmount));
            } else if(item.opType == MINUS_FREE) {
                objOutPut.push_back(Pair("category", "send"));
                objOutPut.push_back(Pair("amount", -dAmount));
            }

            if (item.timeoutHeight > 0)
                objOutPut.push_back(Pair("freezeheight", (int) item.timeoutHeight));

            arrayDetail.push_back(objOutPut);
        }
        break;
    }
    case REG_CONT_TX:
    case DELEGATE_TX:
        if (!pBaseTx->GetAddress(vKeyIdSet, *pAccountViewTip, *pScriptDBTip))
            return arrayDetail;

        double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;

        obj.push_back(Pair("address", vKeyIdSet.begin()->ToAddress()));
        obj.push_back(Pair("category", "send"));
        obj.push_back(Pair("amount", -dAmount));
        if(pBaseTx->nTxType == REG_CONT_TX)
            obj.push_back(Pair("txtype", "REG_CONT_TX"));
        else if(pBaseTx->nTxType == DELEGATE_TX)
            obj.push_back(Pair("txtype", "DELEGATE_TX"));

        arrayDetail.push_back(obj);
        break;
    }
    return arrayDetail;
}

Value gettransaction(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettransaction \"txhash\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.txhash   (string,required) The hast of transaction.\n"
            "\nResult a object about the transaction detail\n"
            "\nResult:\n"
            "\n\"txhash\"\n"
            "\nExamples:\n"
            + HelpExampleCli("gettransaction","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gettransaction","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n"));

    uint256 txhash(uint256S(params[0].get_str()));
    std::shared_ptr<CBaseTx> pBaseTx;
    Object obj;
    LOCK(cs_main);
    CBlock genesisblock;
    CBlockIndex* pgenesisblockindex = mapBlockIndex[SysCfg().HashGenesisBlock()];
    ReadBlockFromDisk(genesisblock, pgenesisblockindex);
    assert(genesisblock.GetHashMerkleRoot() == genesisblock.BuildMerkleTree());
    for (unsigned int i=0; i<genesisblock.vptx.size(); ++i) {
        if (txhash == genesisblock.GetTxHash(i)) {
            double dAmount = static_cast<double>(genesisblock.vptx.at(i)->GetValue()) / COIN;
            obj.push_back(Pair("amount", dAmount));
            obj.push_back(Pair("confirmations",chainActive.Tip()->nHeight));
            obj.push_back(Pair("blockhash", genesisblock.GetHash().GetHex()));
            obj.push_back(Pair("blockindex", (int) i));
            obj.push_back(Pair("blocktime", (int) genesisblock.GetTime()));
            obj.push_back(Pair("txid",genesisblock.vptx.at(i)->GetHash().GetHex()));
            obj.push_back(Pair("details",GetTxAddressDetail(genesisblock.vptx.at(i))));
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            ds << genesisblock.vptx[i];
            obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
            return obj;
        }
    }
    bool findTx (false);
    if (SysCfg().IsTxIndex()) {
            CDiskTxPos postx;
            if (pScriptDBTip->ReadTxIndex(txhash, postx)) {
                findTx = true;
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                    double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
                    obj.push_back(Pair("amount", dAmount));
                    obj.push_back(Pair("confirmations",chainActive.Tip()->nHeight-(int) header.GetHeight()));
                    obj.push_back(Pair("blockhash", header.GetHash().GetHex()));
                    obj.push_back(Pair("blocktime", (int) header.GetTime()));
                    obj.push_back(Pair("txid",pBaseTx->GetHash().GetHex()));
                    obj.push_back(Pair("details",GetTxAddressDetail(pBaseTx)));
                    CDataStream ds(SER_DISK, CLIENT_VERSION);
                    ds << pBaseTx;
                    obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
                } catch (std::exception &e) {
                    throw runtime_error(tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
                }
                return obj;
            }
        }
    if(!findTx)
    {
        pBaseTx = mempool.Lookup(txhash);
        if(pBaseTx == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid txhash");
        }
        double dAmount = static_cast<double>(pBaseTx->GetValue()) / COIN;
        obj.push_back(Pair("amount", dAmount));
        obj.push_back(Pair("confirmations",0));
        obj.push_back(Pair("txid",pBaseTx->GetHash().GetHex()));
        obj.push_back(Pair("details",GetTxAddressDetail(pBaseTx)));
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << pBaseTx;
        obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
    }
    return obj;
}

Value gettxdetail(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettxdetail \"txhash\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.txhash   (string,required) The hash of transaction.\n"
            "\nResult an object of the transaction detail\n"
            "\nResult:\n"
            "\n\"txhash\"\n"
            "\nExamples:\n"
            + HelpExampleCli("gettxdetail","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gettxdetail","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n"));

    uint256 txhash(uint256S(params[0].get_str()));
    return GetTxDetailJSON(txhash);
}

//create a register account tx
Value registeraccounttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() == 0)
        throw runtime_error("registeraccounttx \"addr\" (\"fee\")\n"
            "\nregister local account public key to get its RegId\n"
            "\nArguments:\n"
            "1.addr: (string, required)\n"
            "2.fee: (numeric, optional) pay tx fees to miner\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 "));

    string addr = params[0].get_str();
    uint64_t fee = 0;
    uint64_t nDefaultFee = SysCfg().GetTxFee();
    if (params.size() > 1) {
        fee = params[1].get_uint64();
        if (fee < nDefaultFee) {
            char errorMsg[100] = {'\0'};
            sprintf(errorMsg, "input fee smaller than mintxfee: %ld sawi", nDefaultFee);
            throw JSONRPCError(RPC_INSUFFICIENT_FEE, errorMsg);
        }
    } else {
        fee = nDefaultFee;
    }

    //get keyid
    CKeyID keyid;
    if (!GetKeyId(addr, keyid))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "in registeraccounttx: Address invalid.");

    CRegisterAccountTx rtx;
    assert(pwalletMain != NULL);
    {
        //  LOCK2(cs_main, pwalletMain->cs_wallet);
        EnsureWalletIsUnlocked();

        //balance
        CAccountViewCache view(*pAccountViewTip, true);
        CAccount account;

        CUserID userId = keyid;
        if (!view.GetAccount(userId, account))
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: Account does not exist.");


        if (account.IsRegistered())
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: Account was already registered.");

        uint64_t balance = account.GetRawBalance();
        if (balance < fee) {
            LogPrint("ERROR", "balance=%d, vs fee=%d", balance, fee);
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: Account balance is insufficient");
        }

        CPubKey pubkey;
        if (!pwalletMain->GetPubKey(keyid, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: local wallet key not found.");

        CPubKey MinerPKey;
        if (pwalletMain->GetPubKey(keyid, MinerPKey, true)) {
            rtx.minerId = MinerPKey;
        } else {
            CNullID nullId;
            rtx.minerId = nullId;
        }
        rtx.userId = pubkey;
        rtx.llFees = fee;
        rtx.nValidHeight = chainActive.Tip()->nHeight;

        if (!pwalletMain->Sign(keyid, rtx.SignatureHash(), rtx.signature))
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: Sign failed.");
    }

    std::tuple<bool, string> ret;
    ret = pwalletMain->CommitTransaction((CBaseTx *) &rtx);
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: " + std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

//create a contract tx
Value callcontracttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
       throw runtime_error("callcontracttx \"sender_addr\" \"app_regid\" \"amount\" \"contract\" \"fee\" (\"height\")\n"
            "\ncreate contract transaction\n"
            "\nArguments:\n"
            "1.\"sender_addr\": (string, required)\n tx sender's base58 addr\n"
            "2.\"app_regid\":(string, required) contract RegId\n"
            "3.\"amount\":(numeric, required)\n amount of WICC to be sent to the contract account\n"
            "4.\"contract\": (string, required) contract method invoke content (Hex encode required)\n"
            "5.\"fee\": (numeric, required) pay to miner\n"
            "6.\"height\": (numeric, optional)create height,If not provide use the tip block hegiht in chainActive\n"
            "\nResult:\n"
            "\"contract tx str\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("callcontracttx",
                "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\""
                "411994-1"
                "01020304 "
                "1") + "\nAs json rpc call\n"
            + HelpExampleRpc("callcontracttx",
                "wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6 [\"411994-1\"] "
                "\"5yNhSL7746VV5qWHHDNLkSQ1RYeiheryk9uzQG6C5d\""
                "100000 "
                "\"01020304 \""
                "1") );
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(str_type)(int_type)(int_type));

    //argument-1: sender's base58 addr
    CRegID userId(params[0].get_str());
    CKeyID srckeyid;
    if (userId.IsEmpty()) {
        if (!GetKeyId(params[0].get_str(), srckeyid)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sender's Base58 Addr is invalid");
        }

        if (!pAccountViewTip->GetRegId(CUserID(srckeyid), userId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Sender has NO RegId");
        }
    }

    //argument-2: App RegId
    CRegID appId(params[1].get_str()); //App RegId
    if (appId.IsEmpty()) {
        throw runtime_error("in callcontracttx : addresss is error!\n");
    }

    //argument-3: amount to be sent to the app account
    uint64_t amount = params[2].get_uint64();

    //argument-4: contract (Hex input)
    vector<unsigned char> arguments = ParseHex(params[3].get_str());
    if (arguments.size() >= kContractArgumentMaxSize) {
        throw runtime_error("in callcontracttx : arguments's size is larger than kContractArgumentMaxSize\n");
    }

    //argument-5: fee
    uint64_t fee = params[4].get_uint64();
    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw runtime_error("in callcontracttx : fee is smaller than nMinTxFee\n");
    }

    //argument-6: height
    uint32_t height(0);
    if (params.size() > 5)
        height = params[5].get_int();

    EnsureWalletIsUnlocked();
    std::shared_ptr<CContractTx> tx = std::make_shared<CContractTx>();
    {
        //balance
        CAccountViewCache view(*pAccountViewTip, true);
        CAccount secureAcc;

        if (!pScriptDBTip->HaveScript(appId)) {
            throw runtime_error(tinyformat::format("in callcontracttx : regid %s not exist\n", appId.ToString()));
        }
        tx.get()->nTxType   = CONTRACT_TX;
        tx.get()->srcRegId  = userId;
        tx.get()->desUserId = appId;
        tx.get()->llValues  = amount;
        tx.get()->llFees    = fee;
        tx.get()->arguments = arguments;
        if (0 == height) {
            height = chainActive.Tip()->nHeight;
        }
        tx.get()->nValidHeight = height;

        //get keyid by accountid
        CKeyID keyid;
        if (!view.GetKeyId(userId, keyid)) {
            CID id(userId);
            LogPrint("INFO", "callcontracttx: %s no key id\r\n", HexStr(id.GetID()).c_str());
//          assert(0);
            throw JSONRPCError(RPC_WALLET_ERROR, "callcontracttx Error: no key id.");
        }

        vector<unsigned char> signature;
        if (!pwalletMain->Sign(keyid, tx.get()->SignatureHash(), tx.get()->signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "callcontracttx Error: Sign failed.");
        }
    }

    std::tuple<bool, string> ret;
    ret = pwalletMain->CommitTransaction((CBaseTx *) tx.get());
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error:" + std::get<1>(ret));
    }
    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

// register a contract app tx
Value registercontracttx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error("registercontracttx \"addr\" \"filepath\"\"fee\" (\"height\") (\"appdesc\")\n"
            "\ncreate a transaction of registering a contract app\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) contract owner address from this wallet\n"
            "2.\"filepath\": (string required), the file path of the app script\n"
            "3.\"fee\": (numeric required) pay to miner (the larger the size of script, the bigger fees are required)\n"
            "4.\"height\": (numeric optional) valid height, when not specified, the tip block hegiht in chainActive will be used\n"
            "5.\"appdesc\": (string optional) new app description\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("registercontracttx",
                "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"myapp.lua\" 1000000 (10000) (\"appdesc\")") +
                "\nAs json rpc call\n"
            + HelpExampleRpc("registercontracttx",
                "WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH \"myapp.lua\" 1000000 (10000) (\"appdesc\")"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(int_type)(str_type));

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    std::tuple<bool, string> result = CVmlua::CheckScriptSyntax(luaScriptFilePath.c_str());
    bool bOK = std::get<0>(result);
    if (!bOK)
        throw JSONRPCError(RPC_INVALID_PARAMS, std::get<1>(result));

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw runtime_error("registercontracttx open script file (" + luaScriptFilePath + ") error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    if (lSize <= 0 || lSize > kContractScriptMaxSize) { // contract script file size must be <= 64 KB)
        fclose(file);
        throw JSONRPCError(RPC_INVALID_PARAMS, (lSize == -1) ? "File size is unknown" : ((lSize == 0) ? "File is empty" : "File size exceeds 64 KB limit."));
    }

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == NULL) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }
    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);  //及时释放
        fclose(file);  //及时关闭
        throw runtime_error("read script file error");
    } else {
        fclose(file); //使用完关闭文件
    }

    CVmScript vmScript;
    vmScript.GetRom().insert(vmScript.GetRom().end(), buffer, buffer + lSize);

    if (buffer)
        free(buffer);

    if (params.size() > 4) {
        string scriptDesc = params[4].get_str();
        vmScript.GetMemo().insert(vmScript.GetMemo().end(), scriptDesc.begin(), scriptDesc.end());
    }

    vector<unsigned char> vscript;
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << vmScript;
    vscript.assign(ds.begin(), ds.end());

    uint64_t fee = params[2].get_uint64();
    int height(0);
    if (params.size() > 3)
        height = params[3].get_int();

    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw runtime_error("in registercontracttx :fee is smaller than nMinTxFee\n");
    }
    //get keyid
    CKeyID keyid;
    if (!GetKeyId(params[0].get_str(), keyid)) {
        throw runtime_error("in registercontracttx :send address err\n");
    }

    assert(pwalletMain != NULL);
    CRegisterContractTx tx;
    {
        EnsureWalletIsUnlocked();
        //balance
        CAccountViewCache view(*pAccountViewTip, true);
        CAccount account;

        uint64_t balance = 0;
        CUserID userId   = keyid;
        if (view.GetAccount(userId, account)) {
            balance = account.GetRawBalance();
        }

        if (!account.IsRegistered()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "in registercontracttx Error: Account is not registered.");
        }
        if (!pwalletMain->HasKey(keyid)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "in registercontracttx Error: WALLET file is not correct.");
        }
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "in registercontracttx Error: Account balance is insufficient.");
        }

        CRegID regId;
        view.GetRegId(keyid, regId);

        tx.regAcctId = regId;
        tx.script    = vscript;
        tx.llFees    = fee;
        tx.nRunStep  = vscript.size();
        if (0 == height) {
            height = chainActive.Tip()->nHeight;
        }
        tx.nValidHeight = height;

        if (!pwalletMain->Sign(keyid, tx.SignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "registercontracttx Error: Sign failed.");
        }
    }

    std::tuple<bool, string> ret;
    ret = pwalletMain->CommitTransaction((CBaseTx *) &tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "registercontracttx Error:" + std::get<1>(ret));
    }
    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

//vote a delegate transaction
Value votedelegatetx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
            "votedelegatetx \"sendaddr\" \"opervotes\" \"fee\" (\"height\") \n"
            "\ncreate a delegate vote transaction\n"
            "\nArguments:\n"
            "1.\"sendaddr\": (string required) The address from which votes are sent to other "
            "delegate addresses\n"
            "2. \"opervotes\"    (string, required) A json array of oper votes to corresponding "
            "delegates\n"
            " [\n"
            "   {\n"
            "      \"delegate\":\"address\", (string, required) The delegate address where votes "
            "are recevied\n"
            "      \"votes\": n (numeric, required) votes, increase votes when positive or reduce "
            "votes when negative\n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric required) pay fee to miner\n"
            "4.\"height\": (numeric optional) valid height. When not supplied, the tip block "
            "height in chainActive will be used.\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("votedelegatetx",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 10000 ") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("votedelegatetx",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 10000 "));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fee    = params[2].get_uint64();  // real type
    int nHeight     = 0;
    if (params.size() > 3) {
        nHeight = params[3].get_int();
    }
    Array operVoteArray = params[1].get_array();

    CKeyID keyid;
    if (!GetKeyId(sendAddr, keyid)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid Address: " + sendAddr);
    }
    CDelegateTx delegateTx;
    assert(pwalletMain != NULL);
    {
        EnsureWalletIsUnlocked();
        CAccountViewCache view(*pAccountViewTip, true);
        CAccount account;

        CUserID userId = keyid;
        if (!view.GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist.");
        }

        if (!account.IsRegistered()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is not registered.");
        }

        uint64_t balance = account.GetRawBalance();
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient.");
        }

        if (!pwalletMain->HasKey(keyid)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send tx address is not in wallet file.");
        }

        delegateTx.llFees = fee;
        if (0 != nHeight) {
            delegateTx.nValidHeight = nHeight;
        } else {
            delegateTx.nValidHeight = chainActive.Tip()->nHeight;
        }
        delegateTx.userId = account.regID;

        for (auto operVote : operVoteArray) {
            COperVoteFund operVoteFund;
            const Value& delegateAddress = find_value(operVote.get_obj(), "delegate");
            const Value& delegateVotes   = find_value(operVote.get_obj(), "votes");
            if (delegateAddress.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Vote fund address error or fund value error.");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddress.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address error.");
            }
            CAccount delegateAcct;
            if (!view.GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address is not registered.");
            }
            operVoteFund.fund.pubKey = delegateAcct.PublicKey;
            operVoteFund.fund.value  = (uint64_t)abs(delegateVotes.get_int64());
            if (delegateVotes.get_int64() > 0) {
                operVoteFund.operType = ADD_FUND;
            } else {
                operVoteFund.operType = MINUS_FUND;
            }
            delegateTx.operVoteFunds.push_back(operVoteFund);
        }

        if (!pwalletMain->Sign(keyid, delegateTx.SignatureHash(), delegateTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed.");
        }
    }

    std::tuple<bool, string> ret;
    ret = pwalletMain->CommitTransaction((CBaseTx*)&delegateTx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }
    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

//create a vote delegate raw transaction
Value genvotedelegateraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() <  3  || params.size() > 4) {
        throw runtime_error(
            "genvotedelegateraw \"addr\" \"opervotes\" \"fee\" \"height\"\n"
            "\nget a vote delegate transaction raw transaction\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) from address that votes delegate(s)\n"
            "2. \"opervotes\"    (string, required) A json array of json oper vote to delegates\n"
            " [\n"
            " {\n"
            "    \"delegate\":\"address\", (string, required) The transaction id\n"
            "    \"votes\":n  (numeric, required) votes\n"
            " }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric required) pay to miner\n"
            "4.\"height\": (numeric optional)valid height,If not provide, use the tip block hegiht "
            "in chainActive\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("genvotedelegateraw",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 1000") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("genvotedelegateraw",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 1000"));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fee    = params[2].get_uint64();  // real type
    int nHeight     = 0;
    if (params.size() > 3) {
        nHeight = params[3].get_int();
    }
    Array operVoteArray = params[1].get_array();

    // get keyid
    CKeyID keyid;
    if (!GetKeyId(sendAddr, keyid)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Send tx address error.");
    }
    CDelegateTx delegateTx;
    assert(pwalletMain != NULL);
    {
        EnsureWalletIsUnlocked();
        CAccountViewCache view(*pAccountViewTip, true);
        CAccount account;

        CUserID userId = keyid;
        if (!view.GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist.");
        }

        if (!account.IsRegistered()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is not registered.");
        }

        uint64_t balance = account.GetRawBalance();
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient.");
        }

        if (!pwalletMain->HasKey(keyid)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send tx address is not in wallet file.");
        }

        delegateTx.llFees = fee;
        if (0 != nHeight) {
            delegateTx.nValidHeight = nHeight;
        } else {
            delegateTx.nValidHeight = chainActive.Tip()->nHeight;
        }
        delegateTx.userId = account.regID;

        for (auto operVote : operVoteArray) {
            COperVoteFund operVoteFund;
            const Value& delegateAddress = find_value(operVote.get_obj(), "delegate");
            const Value& delegateVotes   = find_value(operVote.get_obj(), "votes");
            if (delegateAddress.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Voted delegator's address type "
                                   "error or vote value error.");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddress.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Voted delegator's address error.");
            }
            CAccount delegateAcct;
            if (!view.GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY,
                                   "Voted delegator's address is not registered.");
            }
            operVoteFund.fund.pubKey = delegateAcct.PublicKey;
            operVoteFund.fund.value  = (uint64_t)abs(delegateVotes.get_int64());
            if (delegateVotes.get_int64() > 0) {
                operVoteFund.operType = ADD_FUND;
            } else {
                operVoteFund.operType = MINUS_FUND;
            }
            delegateTx.operVoteFunds.push_back(operVoteFund);
        }

        if (!pwalletMain->Sign(keyid, delegateTx.SignatureHash(), delegateTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed.");
        }
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = delegateTx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value listaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "listaddr\n"
            "\nreturn Array containing address,balance,haveminerkey,regid information.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listaddr", "")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listaddr", ""));
    }

    Array retArry;
    assert(pwalletMain != NULL);
    {
        set<CKeyID> setKeyId;
        pwalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0) {
            return retArry;
        }
        CAccountViewCache accView(*pAccountViewTip, true);

        for (const auto &keyId : setKeyId) {
            CUserID userId(keyId);
            CAccount acctInfo;
            accView.GetAccount(userId, acctInfo);
            CKeyCombi keyCombi;
            pwalletMain->GetKeyCombi(keyId, keyCombi);

            Object obj;
            obj.push_back(Pair("addr", keyId.ToAddress()));
            obj.push_back(Pair("balance", (double)acctInfo.GetRawBalance()/ (double) COIN));
            obj.push_back(Pair("hasminerkey", keyCombi.HasMinerKey()));
            obj.push_back(Pair("regid",acctInfo.regID.ToString()));
            retArry.push_back(obj);
        }
    }

    return retArry;
}


Value listtransactions(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 3)
            throw runtime_error(
                "listtransactions ( \"account\" count from includeWatchonly)\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"address\"    (string, optional) DEPRECATED. The account name. Should be \"*\"\n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listtransactions", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listtransactions", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listtransactions", "\"*\", 20, 100")
              );
    assert(pwalletMain != NULL);
    string strAddress = "*";
    if (params.size() > 0)
        strAddress = params[0].get_str();
    if ("" == strAddress) {
        strAddress = "*";
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if(params.size() > 1) {
        nCount =  params[1].get_int();
    }
    if(params.size() > 2) {
        nFrom = params[2].get_int();
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pwalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != NULL)
            blockInfoMap.insert(make_pair(pIndex->nHeight, wtx.first));
    }

    int txnCount(0);
    int nIndex(0);
    for (auto const &wtx : blockInfoMap) {
        CAccountTx accountTx = pwalletMain->mapInBlockTx[wtx.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (item.second->nTxType == COMMON_TX) {
                CCommonTx* ptx = (CCommonTx*)item.second.get();
                CKeyID sendKeyID;
                CRegID sendRegID = boost::get<CRegID>(ptx->srcRegId);
                sendKeyID        = sendRegID.GetKeyID(*pAccountViewTip);
                CKeyID recvKeyID;
                if (ptx->desUserId.type() == typeid(CKeyID)) {
                    recvKeyID = boost::get<CKeyID>(ptx->desUserId);
                } else if (ptx->desUserId.type() == typeid(CRegID)) {
                    CRegID desRegID = boost::get<CRegID>(ptx->desUserId);
                    recvKeyID       = desRegID.GetKeyID(*pAccountViewTip);
                }

                bool bSend = true;
                if ("*" != strAddress && sendKeyID.ToAddress() != strAddress) {
                    bSend = false;
                }

                bool bRecv = true;
                if ("*" != strAddress && recvKeyID.ToAddress() != strAddress) {
                    bRecv = false;
                }

                if(nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }

                if(!(bSend || bRecv)) {
                    continue;
                }

                if(nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                if (bSend) {
                    if (pwalletMain->HasKey(sendKeyID)) {
                        Object obj;
                        obj.push_back(Pair("address", recvKeyID.ToAddress()));
                        obj.push_back(Pair("category", "send"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", -dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("txtype", "COMMON_TX"));
                        obj.push_back(Pair("memo", HexStr(ptx->memo)));
                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }

                if (bRecv) {
                    if (pwalletMain->HasKey(recvKeyID)) {
                        Object obj;
                        obj.push_back(Pair("srcaddr", sendKeyID.ToAddress()));
                        obj.push_back(Pair("address", recvKeyID.ToAddress()));
                        obj.push_back(Pair("category", "receive"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("txtype", "COMMON_TX"));
                        obj.push_back(Pair("memo", HexStr(ptx->memo)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }
            } else if (item.second->nTxType == CONTRACT_TX) {
                CContractTx* ptx = (CContractTx*)item.second.get();
                CKeyID sendKeyID;
                CRegID sendRegID = boost::get<CRegID>(ptx->srcRegId);
                sendKeyID        = sendRegID.GetKeyID(*pAccountViewTip);
                CKeyID recvKeyID;
                if (ptx->desUserId.type() == typeid(CKeyID)) {
                    recvKeyID = boost::get<CKeyID>(ptx->desUserId);
                } else if (ptx->desUserId.type() == typeid(CRegID)) {
                    CRegID desRegID = boost::get<CRegID>(ptx->desUserId);
                    recvKeyID       = desRegID.GetKeyID(*pAccountViewTip);
                }

                bool bSend = true;
                if ("*" != strAddress && sendKeyID.ToAddress() != strAddress) {
                    bSend = false;
                }

                bool bRecv = true;
                if ("*" != strAddress && recvKeyID.ToAddress() != strAddress) {
                    bRecv = false;
                }

                if(nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }

                if(!(bSend || bRecv)) {
                    continue;
                }

                if(nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                if (bSend) {
                    if (pwalletMain->HasKey(sendKeyID)) {
                        Object obj;
                        obj.push_back(Pair("address", recvKeyID.ToAddress()));
                        obj.push_back(Pair("category", "send"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", -dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("txtype", "CONTRACT_TX"));
                        obj.push_back(Pair("arguments", HexStr(ptx->arguments)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }

                if (bRecv) {
                    if (pwalletMain->HasKey(recvKeyID)) {
                        Object obj;
                        obj.push_back(Pair("srcaddr", sendKeyID.ToAddress()));
                        obj.push_back(Pair("address", recvKeyID.ToAddress()));
                        obj.push_back(Pair("category", "receive"));
                        double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                        obj.push_back(Pair("amount", dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("txtype", "CONTRACT_TX"));
                        obj.push_back(Pair("arguments", HexStr(ptx->arguments)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }
            }
        }
    }
    return arrayData;
}

Value listtransactionsv2(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 3)
            throw runtime_error(
                "listtransactionsv2 ( \"account\" count from includeWatchonly)\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"address\"    (string, optional) DEPRECATED. The account name. Should be \"*\".\n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listtransactionsv2", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listtransactionsv2", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listtransactionsv2", "\"*\", 20, 100")
              );

    assert(pwalletMain != NULL);
    string strAddress = "*";
    if (params.size() > 0)
        strAddress = params[0].get_str();
    if("" == strAddress) {
        strAddress = "*";
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if(params.size() > 1) {
        nCount =  params[1].get_int();
    }
    if(params.size() > 2) {
        nFrom = params[2].get_int();
    }

    LOCK2(cs_main, pwalletMain->cs_wallet);

    int txnCount(0);
    int nIndex(0);
    CAccountViewCache accView(*pAccountViewTip, true);
    for (auto const &wtx : pwalletMain->mapInBlockTx) {
        for (auto const & item : wtx.second.mapAccountTx) {
            Object obj;
            CKeyID keyid;
            if (item.second.get() && item.second->nTxType == COMMON_TX) {
                CCommonTx* ptx = (CCommonTx*)item.second.get();

                if (!accView.GetKeyId(ptx->srcRegId, keyid)) {
                    continue;
                }
                string srcAddr = keyid.ToAddress();
                if (!accView.GetKeyId(ptx->desUserId, keyid)) {
                    continue;
                }
                string desAddr = keyid.ToAddress();
                if ("*" != strAddress && desAddr != strAddress) {
                    continue;
                }
                if (nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }
                if (nCount > 0 && txnCount++ > nCount) {
                    return arrayData;
                }
                obj.push_back(Pair("srcaddr", srcAddr));
                obj.push_back(Pair("desaddr", desAddr));
                double dAmount = static_cast<double>(item.second->GetValue()) / COIN;
                obj.push_back(Pair("amount", dAmount));
                obj.push_back(Pair("confirmations", chainActive.Tip()->nHeight - wtx.second.blockHeight));
                obj.push_back(Pair("blockhash", (chainActive[wtx.second.blockHeight]->GetBlockHash().GetHex())));
                obj.push_back(Pair("blocktime", (int64_t)(chainActive[wtx.second.blockHeight]->nTime)));
                obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                arrayData.push_back(obj);
            }
        }
    }
    return arrayData;

}

Value listcontracttx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3 || params.size() < 1)
            throw runtime_error("listcontracttx ( \"account\" count from )\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"account\"    (string). The contract RegId. \n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listcontracttx", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listcontracttx", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listcontracttx", "\"*\", 20, 100")
              );
    assert(pwalletMain != NULL);

    string strRegId = params[0].get_str();
    CRegID regid(strRegId);
    if (regid.IsEmpty() == true) {
        throw runtime_error("in listcontracttx: scriptid size error!\n");
    }

    if (!pScriptDBTip->HaveScript(regid)) {
        throw runtime_error("in listcontracttx: scriptid does not exist!\n");
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if (params.size() > 1) {
        nCount = params[1].get_int();
    }
    if (params.size() > 2) {
        nFrom = params[2].get_int();
    }

    auto getregidstring = [&](CUserID const &userId) {
        if(userId.type() == typeid(CRegID))
            return boost::get<CRegID>(userId).ToString();
        return string(" ");
    };

    LOCK2(cs_main, pwalletMain->cs_wallet);

    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pwalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != NULL)
            blockInfoMap.insert(make_pair(pIndex->nHeight, wtx.first));
    }

    int txnCount(0);
    int nIndex(0);
    for (auto const &wtx : blockInfoMap) {
        CAccountTx accountTx = pwalletMain->mapInBlockTx[wtx.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (item.second.get() && item.second->nTxType == CONTRACT_TX) {
                if (nFrom > 0 && nIndex++ < nFrom) {
                    continue;
                }
                if (nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                CContractTx* ptx = (CContractTx*) item.second.get();
                if (strRegId != getregidstring(ptx->desUserId)) {
                    continue;
                }

                CKeyID keyid;
                Object obj;

                CAccountViewCache accView(*pAccountViewTip, true);
                obj.push_back(Pair("hash", ptx->GetHash().GetHex()));
                obj.push_back(Pair("regid",  getregidstring(ptx->srcRegId)));
                accView.GetKeyId(ptx->srcRegId, keyid);
                obj.push_back(Pair("addr",  keyid.ToAddress()));
                obj.push_back(Pair("dest_regid", getregidstring(ptx->desUserId)));
                accView.GetKeyId(ptx->desUserId, keyid);
                obj.push_back(Pair("dest_addr", keyid.ToAddress()));
                obj.push_back(Pair("money", ptx->llValues));
                obj.push_back(Pair("fees", ptx->llFees));
                obj.push_back(Pair("height", ptx->nValidHeight));
                obj.push_back(Pair("arguments", HexStr(ptx->arguments)));
                arrayData.push_back(obj);

                txnCount++;
            }
        }
    }
    return arrayData;
}

Value listtx(const Array& params, bool fHelp) {
if (fHelp || params.size() > 2) {
        throw runtime_error("listtx\n"
                "\nget all confirmed transactions and all unconfirmed transactions from wallet.\n"
                "\nArguments:\n"
                "1. count          (numeric, optional, default=10) The number of transactions to return\n"
                "2. from           (numeric, optional, default=0) The number of transactions to skip\n"
                "\nExamples:\n"
                "\nResult:\n"
                "\nExamples:\n"
                "\nList the most recent 10 transactions in the system\n"
                + HelpExampleCli("listtx", "") +
                "\nList transactions 100 to 120\n"
                + HelpExampleCli("listtx", "20 100")
            );
    }

    Object retObj;
    int nDefCount = 10;
    int nFrom = 0;
    if(params.size() > 0) {
        nDefCount = params[0].get_int();
    }
    if(params.size() > 1) {
        nFrom = params[1].get_int();
    }
    assert(pwalletMain != NULL);

    //Object Inblockobj;
    Array ConfirmTxArry;
    int nCount = 0;
    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pwalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != NULL)
            blockInfoMap.insert(make_pair(pIndex->nHeight, wtx.first));
    }
    bool bUpLimited = false;
    for (auto const &blockInfo : blockInfoMap) {
        CAccountTx accountTx = pwalletMain->mapInBlockTx[blockInfo.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (nFrom-- > 0)
                continue;
            if (++nCount > nDefCount) {
                bUpLimited = true;
                break;
            }
            //Inblockobj.push_back(Pair("tx", item.first.GetHex()));
            ConfirmTxArry.push_back(item.first.GetHex());
        }
        if(bUpLimited) {
            break;
        }
    }
    retObj.push_back(Pair("ConfirmTx", ConfirmTxArry));
    //CAccountViewCache view(*pAccountViewTip, true);
    Array UnConfirmTxArry;
    for (auto const &wtx : pwalletMain->UnConfirmTx) {
        UnConfirmTxArry.push_back(wtx.first.GetHex());
    }
    retObj.push_back(Pair("UnConfirmTx", UnConfirmTxArry));
    return retObj;
}

Value getaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
          throw runtime_error(
                "getaccountinfo \"addr\"\n"
                "\nget account information\n"
                "\nArguments:\n"
                "1.\"addr\": (string, required) account base58 address"
                "Returns account details.\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("getaccountinfo", "WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\"")
           );
    }
    RPCTypeCheck(params, list_of(str_type));
    CKeyID keyid;
    CUserID userId;
    string addr = params[0].get_str();
    if (!GetKeyId(addr, keyid)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    userId = keyid;
    Object obj;
    {
        CAccount account;
        CAccountViewCache accView(*pAccountViewTip, true);
        if (accView.GetAccount(userId, account)) {
            if (!account.PublicKey.IsValid()) {
                CPubKey pk;
                CPubKey minerpk;
                if (pwalletMain->GetPubKey(keyid, pk)) {
                    pwalletMain->GetPubKey(keyid, minerpk, true);
                    account.PublicKey = pk;
                    account.keyID = pk.GetKeyID();
                    if (pk != minerpk && !account.MinerPKey.IsValid()) {
                        account.MinerPKey = minerpk;
                    }
                }
            }
            obj = account.ToJsonObj(true);
            obj.push_back(Pair("position", "inblock"));
        } else { //unregistered keyid
            CPubKey pk;
            CPubKey minerpk;
            if (pwalletMain->GetPubKey(keyid, pk)) {
                pwalletMain->GetPubKey(keyid, minerpk, true);
                account.PublicKey = pk;
                account.keyID = pk.GetKeyID();
                if (minerpk != pk) {
                    account.MinerPKey = minerpk;
                }
                obj = account.ToJsonObj(true);
                obj.push_back(Pair("position", "inwallet"));
            }
        }
    }
    return obj;
}

//list unconfirmed transaction of mine
Value listunconfirmedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
         throw runtime_error("listunconfirmedtx \n"
                "\nget the list  of unconfirmedtx.\n"
                "\nArguments:\n"
                "\nResult a object about the unconfirm transaction\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listunconfirmedtx", "")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("listunconfirmedtx", ""));
    }

    Object retObj;
    CAccountViewCache view(*pAccountViewTip, true);
    Array UnConfirmTxArry;
    for (auto const &wtx : pwalletMain->UnConfirmTx) {
        UnConfirmTxArry.push_back(wtx.second.get()->ToString(view));
    }
    retObj.push_back(Pair("UnConfirmTx", UnConfirmTxArry));
    return retObj;
}

static Value AccountLogToJson(const CAccountLog &accoutLog) {
    Object obj;
    obj.push_back(Pair("keyid", accoutLog.keyID.ToString()));
    obj.push_back(Pair("llValues", accoutLog.llValues));
    obj.push_back(Pair("nHeight", accoutLog.nVoteHeight));
    // Array array;
    // for (auto const& te : accoutLog.vRewardFund) {
    //     Object obj2;
    //     obj2.push_back(Pair("value", te.value));
    //     obj2.push_back(Pair("nHeight", te.nHeight));
    //     array.push_back(obj2);
    // }
    // obj.push_back(Pair("vRewardFund", array));
    return obj;
}

Value gettxoperationlog(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("gettxoperationlog \"txhash\"\n"
                    "\nget transaction operation log\n"
                    "\nArguments:\n"
                    "1.\"txhash\": (string required) \n"
                    "\nResult:\n"
                    "\"vOperFund\": (string)\n"
                    "\"authorLog\": (string)\n"
                    "\nExamples:\n"
                    + HelpExampleCli("gettxoperationlog",
                            "\"0001a87352387b5b4d6d01299c0dc178ff044f42e016970b0dc7ea9c72c08e2e494a01020304100000\"")
                    + "\nAs json rpc call\n"
                    + HelpExampleRpc("gettxoperationlog",
                            "\"0001a87352387b5b4d6d01299c0dc178ff044f42e016970b0dc7ea9c72c08e2e494a01020304100000\""));
    }
    RPCTypeCheck(params, list_of(str_type));
    uint256 txHash(uint256S(params[0].get_str()));
    vector<CAccountLog> vLog;
    Object retobj;
    retobj.push_back(Pair("hash", txHash.GetHex()));
    if (!GetTxOperLog(txHash, vLog))
        throw JSONRPCError(RPC_INVALID_PARAMS, "error hash");
    {
        Array arrayvLog;
        for (auto const &te : vLog) {
            Object obj;
            obj.push_back(Pair("addr", te.keyID.ToAddress()));
            Array array;
            array.push_back(AccountLogToJson(te));
            arrayvLog.push_back(obj);
        }
        retobj.push_back(Pair("AccountOperLog", arrayvLog));

    }
    return retobj;

}

static Value TestDisconnectBlock(int number) {
    CBlock block;
    Object obj;

    CValidationState state;
    if ((chainActive.Tip()->nHeight - number) < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "restclient Error: number");
    }
    if (number > 0) {
        do {
            CBlockIndex * pTipIndex = chainActive.Tip();
            LogPrint("DEBUG", "current height:%d\n", pTipIndex->nHeight);
            if (!DisconnectBlockFromTip(state))
                return false;
            chainMostWork.SetTip(pTipIndex->pprev);
            if (!EraseBlockIndexFromSet(pTipIndex))
                return false;
            if (!pblocktree->EraseBlockIndex(pTipIndex->GetBlockHash()))
                return false;
            mapBlockIndex.erase(pTipIndex->GetBlockHash());

//          if (!ReadBlockFromDisk(block, pindex))
//              throw ERRORMSG("VerifyDB() : *** ReadBlockFromDisk failed at %d, hash=%s", pindex->nHeight,
//                      pindex->GetBlockHash().ToString());
//          bool fClean = true;
//          CTransactionDBCache txCacheTemp(*pTxCacheTip, true);
//          CScriptDBViewCache contractScriptTemp(*pScriptDBTip, true);
//          if (!DisconnectBlock(block, state, view, pindex, txCacheTemp, contractScriptTemp, &fClean))
//              throw ERRORMSG("VerifyDB() : *** irrecoverable inconsistency in block data at %d, hash=%s", pindex->nHeight,
//                      pindex->GetBlockHash().ToString());
//          CBlockIndex *pindexDelete = pindex;
//          pindex = pindex->pprev;
//          chainActive.SetTip(pindex);
//
//          assert(view.Flush() &&txCacheTemp.Flush()&& contractScriptTemp.Flush() );
//          txCacheTemp.Clear();
        } while (--number);
    }
//      pTxCacheTip->Flush();

    obj.push_back(Pair("tip", strprintf("hash:%s hight:%s",chainActive.Tip()->GetBlockHash().ToString(),chainActive.Tip()->nHeight)));
    return obj;
}

Value disconnectblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("disconnectblock \"numbers\"\n"
                "\ndisconnect block\n"
                "\nArguments:\n"
                "1. \"numbers \"  (numeric, required) the block numbers.\n"
                "\nResult:\n"
                "\"disconnect result\"  (bool) \n"
                "\nExamples:\n"
                + HelpExampleCli("disconnectblock", "\"1\"")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("disconnectblock", "\"1\""));
    }
    int number = params[0].get_int();

    Value te = TestDisconnectBlock(number);

    return te;
}

Value resetclient(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("resetclient\n"
            "\nreset the client such that its blocks and wallet data is purged to none and needs to sync from network again.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("resetclient", "")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("resetclient", ""));
    }

    Value ret = TestDisconnectBlock(chainActive.Tip()->nHeight);
    if (chainActive.Tip()->nHeight == 0) {
        pwalletMain->CleanAll();
        CBlockIndex* te = chainActive.Tip();
        uint256 hash = te->GetBlockHash();
//      auto ret = remove_if( mapBlockIndex.begin(), mapBlockIndex.end(),[&](std::map<uint256, CBlockIndex*>::reference a) {
//          return (a.first == hash);
//      });
//      mapBlockIndex.erase(ret,mapBlockIndex.end());
        for (auto it = mapBlockIndex.begin(), ite = mapBlockIndex.end(); it != ite;) {
            if (it->first != hash)
                it = mapBlockIndex.erase(it);
            else
                ++it;
        }
        pAccountViewTip->Flush();
        pScriptDBTip->Flush();
        pTxCacheTip->Flush();

        assert(pAccountViewDB->GetDbCount() == 43);
        assert(pScriptDB->GetDbCount() == 0 || pScriptDB->GetDbCount() == 1);
        assert(pTxCacheTip->GetSize() == 0);

        CBlock firs = SysCfg().GenesisBlock();
        pwalletMain->SyncTransaction(uint256(), NULL, &firs);
        mempool.Clear();
    } else {
        throw JSONRPCError(RPC_WALLET_ERROR, "restclient Error: Reset failed.");
    }
    return ret;
}

Value listcontracts(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("listcontracts \"showDetail\"\n"
                "\nget the list of all registered contracts\n"
                "\nArguments:\n"
                "1. showDetail  (boolean, required) true to show scriptContent, otherwise to not show it.\n"
                "\nReturn an object contain many script data\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listcontracts", "true")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("listcontracts", "true"));
    }
    bool showDetail = false;
    showDetail = params[0].get_bool();
    Object obj;
    Array arrayScript;

    if (pScriptDBTip != NULL) {
        int nCount(0);
        if (!pScriptDBTip->GetScriptCount(nCount))
            throw JSONRPCError(RPC_DATABASE_ERROR, "get contract error: cannot get registered contract number.");
        CRegID regId;
        vector<unsigned char> vScript;
        Object script;
        if (!pScriptDBTip->GetScript(0, regId, vScript))
            throw JSONRPCError(RPC_DATABASE_ERROR, "get contract error: cannot get registered contract.");
        script.push_back(Pair("contractregid", regId.ToString()));
        CDataStream ds(vScript, SER_DISK, CLIENT_VERSION);
        CVmScript vmScript;
        ds >> vmScript;
        // string strDes(vmScript.GetMemo().begin(), vmScript.GetMemo()->end());
        script.push_back(Pair("memo", HexStr(vmScript.GetMemo())));

        if (showDetail)
            script.push_back(Pair("contract", HexStr(vmScript.GetRom().begin(), vmScript.GetRom().end())));

        arrayScript.push_back(script);
        while (pScriptDBTip->GetScript(1, regId, vScript)) {
            Object obj;
            obj.push_back(Pair("contractregid", regId.ToString()));
            CDataStream ds(vScript, SER_DISK, CLIENT_VERSION);
            CVmScript vmScript;
            ds >> vmScript;
            // string strDes(vmScript.GetMemo().begin(), vmScript.GetMemo().end());
            obj.push_back(Pair("memo", HexStr(vmScript.GetMemo())));
            if (showDetail)
                obj.push_back(Pair("contract", HexStr(vmScript.GetRom().begin(), vmScript.GetRom().end())));

            arrayScript.push_back(obj);
        }
    }

    obj.push_back(Pair("contracts", arrayScript));
    return obj;
}

Value getcontractinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
            throw runtime_error(
                "getcontractinfo ( \"scriptid\" )\n"
                "\nget app information.\n"
                "\nArguments:\n"
                "1. \"scriptid\"    (string, required) the script ID. \n"
                "\nget app information in the systems\n"
                "\nExamples:\n"
                + HelpExampleCli("getcontractinfo", "123-1")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("getcontractinfo", "123-1"));

    string strRegId = params[0].get_str();
    CRegID regid(strRegId);
    if (regid.IsEmpty() == true) {
        throw runtime_error("in getcontractinfo: contract regid size invalid!\n");
    }

    if (!pScriptDBTip->HaveScript(regid)) {
        throw runtime_error("in getcontractinfo: contract regid not exist!\n");
    }

    vector<unsigned char> vScript;
    if (!pScriptDBTip->GetScript(regid, vScript)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "get script error: cannot get registered script.");
    }

    Object obj;
    obj.push_back(Pair("contract_regid", regid.ToString()));
    CDataStream ds(vScript, SER_DISK, CLIENT_VERSION);
    CVmScript vmScript;
    ds >> vmScript;
    obj.push_back(Pair("contract_memo", HexStr(vmScript.GetMemo())));
    obj.push_back(Pair("contract_content", HexStr(vmScript.GetRom().begin(), vmScript.GetRom().end())));
    return obj;
}

Value getaddrbalance(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        string msg = "getaddrbalance nrequired [\"key\",...] ( \"account\" )\n"
                "\nAdd a nrequired-to-sign multisignature address to the wallet.\n"
                "Each key is a  address or hex-encoded public key.\n" + HelpExampleCli("getaddrbalance", "")
                + "\nAs json rpc call\n" + HelpExampleRpc("getaddrbalance", "");
        throw runtime_error(msg);
    }

    assert(pwalletMain != NULL);

    CKeyID keyid;
    if (!GetKeyId(params[0].get_str(), keyid))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid  address");

    double dbalance = 0.0;
    {
        LOCK(cs_main);
        CAccountViewCache accView(*pAccountViewTip, true);
        CAccount secureAcc;
        CUserID userId = keyid;
        if (accView.GetAccount(userId, secureAcc)) {
            dbalance = (double) secureAcc.GetRawBalance() / (double) COIN;
        }
    }
    return dbalance;
}

Value generateblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("generateblock \"addr\"\n"
            "\ncreate a block with the appointed address\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required)\n"
            "\nResult:\n"
            "\nblockhash\n"
            "\nExamples:\n" +
            HelpExampleCli("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\""));
    }
    //get keyid
    CKeyID keyid;

    if (!GetKeyId(params[0].get_str(), keyid))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "in generateblock :address err");

//  uint256 hash = CreateBlockWithAppointedAddr(keyid);
//  if (hash.IsNull()) {
//      throw runtime_error("in generateblock :cannot generate block\n");
//  }
    Object obj;
//  obj.push_back(Pair("blockhash", hash.GetHex()));
    return obj;
}

Value listtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("listtxcache\n"
                "\nget all transactions in cahce\n"
                "\nArguments:\n"
                "\nResult:\n"
                "\"txcache\"  (string) \n"
                "\nExamples:\n" + HelpExampleCli("listtxcache", "")+ HelpExampleRpc("listtxcache", ""));
    }
    const map<uint256, set<uint256> > &mapTxHashByBlockHash = pTxCacheTip->GetTxHashCache();

    Array retTxHashArray;
    for (auto &item : mapTxHashByBlockHash) {
        Object blockObj;
        Array txHashArray;
        blockObj.push_back(Pair("blockhash", item.first.GetHex()));
        for (auto &txHash : item.second)
            txHashArray.push_back(txHash.GetHex());
        blockObj.push_back(Pair("txcache", txHashArray));
        retTxHashArray.push_back(blockObj);
    }

    return retTxHashArray;
}

Value reloadtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("reloadtxcache \n"
            "\nreload transactions catch\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("reloadtxcache", "")
            + HelpExampleRpc("reloadtxcache", ""));
    }
    pTxCacheTip->Clear();
    CBlockIndex *pIndex = chainActive.Tip();
    if ((chainActive.Tip()->nHeight - SysCfg().GetTxCacheHeight()) >= 0) {
        pIndex = chainActive[(chainActive.Tip()->nHeight - SysCfg().GetTxCacheHeight())];
    } else {
        pIndex = chainActive.Genesis();
    }
    CBlock block;
    do {
        if (!ReadBlockFromDisk(block, pIndex))
            return ERRORMSG("reloadtxcache() : *** ReadBlockFromDisk failed at %d, hash=%s",
                pIndex->nHeight, pIndex->GetBlockHash().ToString());

        pTxCacheTip->AddBlockToCache(block);
        pIndex = chainActive.Next(pIndex);
    } while (NULL != pIndex);

    Object obj;
    obj.push_back(Pair("info", "reload tx cache succeed"));
    return obj;
}

static int GetDataFromAppDb(CScriptDBViewCache &cache, const CRegID &regid, int pagesize, int index,
        vector<std::tuple<vector<unsigned char>, vector<unsigned char> > >&ret) {
    int dbsize;
    int height = chainActive.Height();
    cache.GetContractItemCount(regid, dbsize);
    if (0 == dbsize)
        throw runtime_error("GetDataFromAppDb :the app has NO data!\n");

    vector<unsigned char> value;
    vector<unsigned char> vScriptKey;

    if (!cache.GetContractData(height, regid, 0, vScriptKey, value))
        throw runtime_error("GetContractData :the app data retrieval failed!\n");

    if (index == 1)
        ret.push_back(std::make_tuple(vScriptKey, value));

    int readCount(1);
    while (--dbsize) {
        if (cache.GetContractData(height, regid, 1, vScriptKey, value)) {
            ++readCount;
            if (readCount > pagesize * (index - 1)) {
                ret.push_back(std::make_tuple(vScriptKey, value));
            }
        }
        if (readCount >= pagesize * index) {
            return ret.size();
        }
    }
    return ret.size();
}

Value getcontractdataraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error("getcontractdataraw \"contract_regid\" \"[pagesize or key]\" (\"index\")\n"
            "\nget the contract data (hexadecimal format) by a given app RegID\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required) App RegId\n"
            "2.[pagesize or key]: (pagesize int, required),if only two params,it is key, otherwise it is pagesize\n"
            "3.\"index\": (int optional)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractdataraw", "\"1304166-1\" \"key\"")
            + HelpExampleRpc("getcontractdataraw", "\"1304166-1\" \"key\""));
    }

    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() == true)
        throw runtime_error("getcontractdataraw : app regid not supplied!");

    if (!pScriptDBTip->HaveScript(regid))
        throw runtime_error("getcontractdataraw : app regid does NOT exist!");

    Object script;
    int height = chainActive.Height();
    CScriptDBViewCache contractScriptTemp(*pScriptDBTip, true);
    if (params.size() == 2) {
        vector<unsigned char> key = ParseHex(params[1].get_str());
        vector<unsigned char> value;
        if (!contractScriptTemp.GetContractData(height, regid, key, value)) {
            throw runtime_error("getcontractdataraw :the key does NOT exist!");
        }
        script.push_back(Pair("regid", params[0].get_str()));
        script.push_back(Pair("key", HexStr(key)));
        script.push_back(Pair("value", HexStr(value)));
        return script;

    } else {
        int dbsize;
        contractScriptTemp.GetContractItemCount(regid, dbsize);
        if (0 == dbsize) {
            throw runtime_error("getcontractdataraw :the contract has NO data!");
        }
        int pagesize = params[1].get_int();
        int index = params[2].get_int();

        vector<std::tuple<vector<unsigned char>, vector<unsigned char> > > ret;
        GetDataFromAppDb(contractScriptTemp, regid, pagesize, index, ret);
        Array retArray;
        for (auto te : ret) {
            vector<unsigned char> key = std::get<0>(te);
            vector<unsigned char> value = std::get<1>(te);
            Object firt;
            firt.push_back(Pair("key", HexStr(key)));
            firt.push_back(Pair("value", HexStr(value)));
            retArray.push_back(firt);
        }
        return retArray;
    }
    return script;
}

Value getcontractdata(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 2 || params.size() > 3) {
        throw runtime_error("getcontractdata \"contract_regid\" \"[pagesize or key]\" (\"index\")\n"
            "\nget the contract data (original input format) by a given contract RegID\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required) Contract RegId\n"
            "2.[pagesize or key]: (pagesize int, required),if only two params,it is key, otherwise it is pagesize\n"
            "3.\"index\": (int optional)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractdata", "\"1304166-1\" \"key\"")
            + HelpExampleRpc("getcontractdata", "\"1304166-1\" \"key\""));
    }
    int height = chainActive.Height();
    // RPCTypeCheck(params, list_of(str_type)(int_type)(int_type));
    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() == true) {
        throw runtime_error("getcontractdata : contract regid NOT supplied!");
    }

    if (!pScriptDBTip->HaveScript(regid)) {
        throw runtime_error("getcontractdata : contract regid NOT exist!");
    }
    Object script;

    CScriptDBViewCache contractScriptTemp(*pScriptDBTip, true);
    if (params.size() == 2) {
        string strKey = params[1].get_str();
        vector<unsigned char> key (strKey.length());
        std::copy(strKey.begin(), strKey.end(), key.begin());

        vector<unsigned char> value;
        if (!contractScriptTemp.GetContractData(height, regid, key, value)) {
            throw runtime_error("GetContractData :the key does NOT exist!");
        }
        string strValue (value.begin(), value.end());
        script.push_back( Pair("regid", params[0].get_str()) );
        script.push_back( Pair("key", strKey) );
        script.push_back( Pair("value", strValue) );
        return script;

    } else {
        int dbsize;
        contractScriptTemp.GetContractItemCount(regid, dbsize);
        if (0 == dbsize) {
            throw runtime_error("GetContractItemCount :the contract has NO data!");
        }
        int pagesize = params[1].get_int();
        int index = params[2].get_int();

        vector<std::tuple<vector<unsigned char>, vector<unsigned char> > > ret;
        GetDataFromAppDb(contractScriptTemp, regid, pagesize, index, ret);
        Array retArray;
        for (auto te : ret) {
            vector<unsigned char> key = std::get<0>(te);
            vector<unsigned char> value = std::get<1>(te);
            string sKey (key.begin(), key.end());
            string sValue (value.begin(), value.end());

            Object retObj;
            retObj.push_back( Pair("key", sKey) );
            retObj.push_back( Pair("value", sValue) );

            retArray.push_back(retObj);
        }
        return retArray;
    }
    return script;
}

Value getcontractconfirmdata(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 3 && params.size() !=4)) {
        throw runtime_error("getcontractconfirmdata \"regid\" \"pagesize\" \"index\"\n"
            "\nget script valid data\n"
            "\nArguments:\n"
            "1.\"regid\": (string, required) app RegId\n"
            "2.\"pagesize\": (int, required)\n"
            "3.\"index\": (int, required )\n"
            "4.\"minconf\":  (numeric, optional, default=1) Only include contract transactions confirmed \n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractconfirmdata", "\"1304166-1\" \"1\"  \"1\"")
            + HelpExampleRpc("getcontractconfirmdata", "\"1304166-1\" \"1\"  \"1\""));
    }
    std::shared_ptr<CScriptDBViewCache> pAccountViewCache;
    if(4 == params.size() && 0==params[3].get_int()) {
        pAccountViewCache.reset(new CScriptDBViewCache(*mempool.pScriptDBViewCache, true));
    }else {
        pAccountViewCache.reset(new CScriptDBViewCache(*pScriptDBTip, true));
    }
    int height = chainActive.Height();
    RPCTypeCheck(params, list_of(str_type)(int_type)(int_type));
    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() == true)
        throw runtime_error("getcontractdata :appregid NOT found!");

    if (!pAccountViewCache->HaveScript(regid))
        throw runtime_error("getcontractdata :appregid does NOT exist!");

    Object obj;
    int pagesize = params[1].get_int();
    int nIndex = params[2].get_int();

    int nKey = revert(height);
    CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
    ds << nKey;
    std::vector<unsigned char> vScriptKey(ds.begin(), ds.end());
    std::vector<unsigned char> vValue;
    Array retArray;
    int nReadCount = 0;
    while (pAccountViewCache->GetContractData(height, regid, 1, vScriptKey, vValue)) {
        Object item;
        ++nReadCount;
        if (nReadCount > pagesize * (nIndex - 1)) {
            item.push_back(Pair("key", HexStr(vScriptKey)));
            item.push_back(Pair("value", HexStr(vValue)));
            retArray.push_back(item);
        }
        if(nReadCount >= pagesize * nIndex) {
            break;
        }
    }
    return retArray;
}

Value saveblocktofile(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error("saveblocktofile \"blockhash\" \"filepath\"\n"
                "\n save the given block info to the given file\n"
                "\nArguments:\n"
                "1.\"blockhash\": (string, required)\n"
                "2.\"filepath\": (string, required)\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("saveblocktofile", "\"12345678901211111\" \"block.log\"")
                + HelpExampleRpc("saveblocktofile", "\"12345678901211111\" \"block.log\""));
    }
    string strblockhash = params[0].get_str();
    uint256 blockHash(uint256S(params[0].get_str()));
    if(0 == mapBlockIndex.count(blockHash)) {
        throw JSONRPCError(RPC_MISC_ERROR, "block hash is not exist!");
    }
    CBlockIndex *pIndex = mapBlockIndex[blockHash];
    CBlock blockInfo;
    if (!pIndex || !ReadBlockFromDisk(blockInfo, pIndex))
        throw runtime_error(_("Failed to read block"));
    assert(strblockhash == blockInfo.GetHash().ToString());
//  CDataStream ds(SER_NETWORK, PROTOCOL_VERSION);
//  ds << blockInfo.GetBlockHeader();
//  cout << "block header:" << HexStr(ds) << endl;
    string file = params[1].get_str();
    try {
        FILE* fp = fopen(file.c_str(), "wb+");
        CAutoFile fileout = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
        if (!fileout)
            throw JSONRPCError(RPC_MISC_ERROR, "open file:" + strblockhash + "failed!");
        if(chainActive.Contains(pIndex))
            fileout << pIndex->nHeight;
        fileout << blockInfo;
        fflush(fileout);
        //fileout object auto free fp point, don't need double free fp point.
//      fclose(fp);
    } catch (std::exception &e) {
        throw JSONRPCError(RPC_MISC_ERROR, "save block to file error");
    }
    return "save succeed";
}

Value getcontractitemcount(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("getcontractitemcount \"regid\"\n"
            "\nget the total number of contract db K-V items\n"
            "\nArguments:\n"
            "1.\"regid\": (string, required) Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractitemcount", "\"258988-1\"")
            + HelpExampleRpc("getcontractitemcount","\"258988-1\"")
        );
    }

    CRegID regId(params[0].get_str());
    if (regId.IsEmpty()) {
        throw runtime_error("contract RegId invalid!");
    }
    if (!pScriptDBTip->HaveScript(regId)) {
        throw runtime_error("contract with the given RegId does NOT exist!");
    }

    int nItemCount = 0;
    if (!pScriptDBTip->GetContractItemCount(regId, nItemCount)) {
        throw runtime_error("GetContractItemCount error!");
    }
    return nItemCount;
}

Value genregisteraccountraw(const Array& params, bool fHelp) {
    if (fHelp || (params.size() < 3  || params.size() > 4)) {
        throw runtime_error("genregisteraccountraw \"fee\" \"height\" \"publickey\" (\"minerpublickey\") \n"
                "\ncreate a register account transaction\n"
                "\nArguments:\n"
                "1.fee: (numeric, required) pay to miner\n"
                "2.height: (numeric, required)\n"
                "3.publickey: (string, required)\n"
                "4.minerpublickey: (string,optional)\n"
                "\nResult:\n"
                "\"txhash\": (string)\n"
                "\nExamples:\n"
                + HelpExampleCli("genregisteraccountraw",  "10000  3300 \"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\" \"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\"")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("genregisteraccountraw", " 10000 3300 \"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\" \"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\""));
    }
    CUserID ukey;
    CUserID uminerkey = CNullID();

    int64_t fee = AmountToRawValue(params[0]);
    int64_t nDefaultFee = SysCfg().GetTxFee();

    if (fee < nDefaultFee) {
        char errorMsg[100] = {'\0'};
        sprintf(errorMsg, "input fee smaller than mintxfee: %ld sawi", nDefaultFee);
        throw JSONRPCError(RPC_INSUFFICIENT_FEE, errorMsg);
    }

    int hight = params[1].get_int();

    CKeyID dummy;
    CPubKey pubk = CPubKey(ParseHex(params[2].get_str()));
    if (!pubk.IsCompressed() || !pubk.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "publickey invalid");
    }
    ukey = pubk;
    dummy = pubk.GetKeyID();

    if (params.size() > 3) {
        CPubKey minerpubk = CPubKey(ParseHex(params[3].get_str()));
        if (!minerpubk.IsCompressed() || !minerpubk.IsFullyValid()) {
            throw JSONRPCError(RPC_INVALID_PARAMS, "publickey invalid");
        }
        uminerkey = minerpubk;
    }

    EnsureWalletIsUnlocked();
    std::shared_ptr<CRegisterAccountTx> tx = std::make_shared<CRegisterAccountTx>(ukey, uminerkey, fee, hight);
    if (!pwalletMain->Sign(pubk.GetKeyID(), tx->SignatureHash(), tx->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,  "Sign failed");
    }
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value sendrawtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "sendrawtx \"transaction\" \n"
            "\nsend raw transaction\n"
            "\nArguments:\n"
            "1.\"transaction\": (string, required)\n"
            "\nExamples:\n"
            + HelpExampleCli("sendrawtx", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("sendrawtx", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\""));
    }
    //EnsureWalletIsUnlocked();
    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);

    std::shared_ptr<CBaseTx> tx;
    stream >> tx;
    std::tuple<bool, string> ret;
    ret = pwalletMain->CommitTransaction((CBaseTx *) tx.get());
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, "sendrawtx error: " + std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("hash", std::get<1>(ret)));
    return obj;
}

// cold-wallet feature
Value gencallcontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error("gencallcontractraw \"height\" \"fee\" \"amount\" \"user_regid\" \"contract_regid\" \"contract\" \n"
                "\ngenerate contract invocation rawtx\n"
                "\nArguments:\n"
                "1.\"fee\": (numeric, required) fee paid to miner\n"
                "2.\"amount\": (numeric, required) amount of coins transfered to the contract (could be 0)\n"
                "3.\"user_regid\": (string, required) contract callee regid\n"
                "4.\"contract_regid\": (string required) contract regid\n"
                "5.\"contract\": (string, required) contract arguments encoded as one hex string\n"
                "6.\"height\": (int, optional) a valid block height (current tip height when omitted)\n"
                "\nResult:\n"
                "\"contract invocation rawtx str\": (string)\n"
                "\nExamples:\n"
                + HelpExampleCli("gencallcontractraw",
                        "1000 01020304 000000000100 [\"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"] "
                                "[\"5yNhSL7746VV5qWHHDNLkSQ1RYeiheryk9uzQG6C5d\","
                                "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"] 10") + "\nAs json rpc call\n"
                + HelpExampleRpc("gencallcontractraw",
                        "1000 01020304 000000000100 000000000100 [\"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"] "
                                "[\"5yNhSL7746VV5qWHHDNLkSQ1RYeiheryk9uzQG6C5d\","
                                "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"] 10"));
    }
    RPCTypeCheck(params, list_of(real_type)(real_type)(str_type)(str_type)(str_type)(int_type));

    uint64_t fee = AmountToRawValue(params[0]);
    uint64_t amount = AmountToRawValue(params[1]);
    string sUserRegId = params[2].get_str();
    CRegID userRegId(sUserRegId);
    if (userRegId.IsEmpty()) {
        throw runtime_error("invalid user_regid: " + sUserRegId);
    }
    string sConRegId = params[3].get_str();
    CRegID conRegId(sConRegId);
    if (conRegId.IsEmpty()) {
        throw runtime_error("invalid contract_regid: %s" + sUserRegId);
    }
    vector<unsigned char> arguments = ParseHex(params[4].get_str());
    if (arguments.size() >= kContractArgumentMaxSize) {
        throw runtime_error("input arguments'size larger than kContractArgumentMaxSize");
    }
    int height = (params.size() == 6) ? params[5].get_int() : chainActive.Tip()->nHeight;

    if (fee > 0 && fee < CBaseTx::nMinTxFee)
        throw runtime_error("input fee smaller than nMinTxFee");
    if (conRegId.IsEmpty())
        throw runtime_error("contract regid invalid!\n");
    if (!pScriptDBTip->HaveScript(conRegId))
        throw runtime_error(tinyformat::format("regid %s not exist\n", conRegId.ToString()));
    if (height < chainActive.Tip()->nHeight - 250 || height > chainActive.Tip()->nHeight + 250)
        throw runtime_error("height is out of a valid range to the tip block height!\n");

    CAccountViewCache view(*pAccountViewTip, true);
    CKeyID keyId;
    if (!view.GetKeyId(userRegId, keyId)) {
        CID id(userRegId);
        string hexId = HexStr(id.GetID()).c_str();
        LogPrint("ERROR", "from address :%s has no keyid\r\n", hexId);
        throw runtime_error(tinyformat::format("from address :%s has no keyId\r\n", hexId));
    }

    std::shared_ptr<CContractTx> tx = std::make_shared<CContractTx>(
        userRegId, conRegId, fee, amount, height, arguments);
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;
    Object obj;
    string rawtx = HexStr(ds.begin(), ds.end());
    obj.push_back( Pair("rawtx", rawtx) );
    return obj;
}

Value genregistercontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 4) {
        throw runtime_error(
            "genregistercontractraw \"height\" \"fee\" \"addr\" \"flag\" \"script or scriptid\" (\"script description\")\n"
            "\nregister script\n"
            "\nArguments:\n"
            "1.\"fee\": (numeric required) pay to miner\n"
            "2.\"addr\": (string required)\n from address that registers the contract"
            "3.\"flag\": (bool, required) 0-1\n"
            "4.\"script or scriptid\": (string required), if flag=0 is script's file path, else if flag=1 scriptid\n"
            "5.\"height\": (int required) valid height\n"
            "6.\"script description\":(string optional) new script description\n"
            "\nResult:\n"
            "\"txhash\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("genregistercontractraw",
                    "\"10\" \"10000\" \"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\" \"1\" \"010203040506\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("genregistercontractraw",
                    "\"10\" \"10000\" \"5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\" \"1\" \"010203040506\" "));
    }

    RPCTypeCheck(params, list_of(real_type)(str_type)(int_type)(str_type)(int_type)(str_type));

    uint64_t fee = AmountToRawValue(params[0]);

    CVmScript vmScript;
    vector<unsigned char> vscript;
    int flag = params[2].get_bool();
    if (0 == flag) {
        string luaScriptFilePath = GetAbsolutePath(params[3].get_str()).string();
        if (luaScriptFilePath.empty())
            throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

        if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
            throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

        FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
        if (!file)
            throw runtime_error("genregistercontractraw open App Lua Script file" + luaScriptFilePath + "error");

        long lSize;
        fseek(file, 0, SEEK_END);
        lSize = ftell(file);
        rewind(file);

        // allocate memory to contain the whole file:
        char *buffer = (char*) malloc(sizeof(char) * lSize);
        if (buffer == NULL) {
            fclose(file);//及时关闭
            throw runtime_error("allocate memory failed");
        }

        if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
            free(buffer);
            fclose(file);//及时关闭
            throw runtime_error("read contract script file error");
        } else {
            fclose(file);
        }
        vmScript.GetRom().insert(vmScript.GetRom().end(), buffer, buffer + lSize);
        if (buffer)
            free(buffer);

        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << vmScript;

        vscript.assign(ds.begin(), ds.end());
    } else if (1 == flag) {
        vscript = ParseHex(params[3].get_str());
    }

    if (params.size() > 5) {
        string memo = params[5].get_str();
        vmScript.GetMemo().insert(vmScript.GetMemo().end(), memo.begin(), memo.end());
    }

    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw runtime_error("Error: fee smaller than nMinTxFee\n");
    }
    //get keyid
    CKeyID keyid;
    if (!GetKeyId(params[2].get_str(), keyid)) {
        throw runtime_error("Error: from address invalid\n");
    }

    //balance
    CAccountViewCache view(*pAccountViewTip, true);
    CAccount account;

    CUserID userId = keyid;
    if (!view.GetAccount(userId, account)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Account does not exist.");
    }
    if (!account.IsRegistered()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Error: Account not registered.");
    }

    if (flag) {
        vector<unsigned char> vscriptcontent;
        CRegID regid(params[2].get_str());
        if (!pScriptDBTip->GetScript(CRegID(vscript), vscriptcontent)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Error: contract RegId not exist");
        }
    }

    std::shared_ptr<CRegisterContractTx> tx = std::make_shared<CRegisterContractTx>();
    CRegID regId;
    view.GetRegId(keyid, regId);

    tx.get()->regAcctId = regId;
    tx.get()->script = vscript;
    tx.get()->llFees = fee;

    uint32_t height = chainActive.Tip()->nHeight;
    if (params.size() > 4) {
        height =  params[4].get_int();
    }
    tx.get()->nValidHeight = height;

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value sigstr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error("sigstr \"str\" \"addr\"\n"
                "\nsignature transaction\n"
                "\nArguments:\n"
                "1.\"str\": (string, required) sig str\n"
                "2.\"addr\": (string, required)\n"
                "\nExamples:\n"
                + HelpExampleCli("sigstr", "\"123-1\" \"W5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\" ")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("sigstr", "\"123-1\" \"W5zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG 010203040506\" "));
    }

    vector<unsigned char> vch(ParseHex(params[0].get_str()));

    string addr = params[1].get_str();
    CKeyID keyid;
    if (!GetKeyId(params[1].get_str(), keyid))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address invalid");

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get())
        return Value::null;

    Object obj;
    switch (pBaseTx.get()->nTxType) {
        case COMMON_TX: {
            std::shared_ptr<CCommonTx> tx =
                std::make_shared<CCommonTx>(pBaseTx.get());
            if (!pwalletMain->Sign(keyid, tx.get()->SignatureHash(), tx.get()->signature))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }
        case REG_ACCT_TX: {
            std::shared_ptr<CRegisterAccountTx> tx =
                std::make_shared<CRegisterAccountTx>(pBaseTx.get());
            if (!pwalletMain->Sign(keyid, tx.get()->SignatureHash(), tx.get()->signature))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }
        case CONTRACT_TX: {
            std::shared_ptr<CContractTx> tx =
                std::make_shared<CContractTx>(pBaseTx.get());
            if (!pwalletMain->Sign(keyid, tx.get()->SignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }
        case REWARD_TX:
            break;
        case REG_CONT_TX: {
            std::shared_ptr<CRegisterContractTx> tx =
                std::make_shared<CRegisterContractTx>(pBaseTx.get());
            if (!pwalletMain->Sign(keyid, tx.get()->SignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }
        case DELEGATE_TX: {
            std::shared_ptr<CDelegateTx> tx =
                std::make_shared<CDelegateTx>(pBaseTx.get());
            if (!pwalletMain->Sign(keyid, tx.get()->SignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }
        default:
            //      assert(0);
            break;
    }
    return obj;
}

Value decoderawtx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1) {
        throw runtime_error("decoderawtx \"hexstring\"\n"
            "\ndecode transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) hexstring\n"
            "\nExamples:\n"
            + HelpExampleCli("decoderawtx", "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f22af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea467d02d2d2c3db62a1addaa8009ccd\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("decoderawtx", "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f22af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea467d02d2d2c3db62a1addaa8009ccd\""));
    }
    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    LogPrint("DEBUG", "data size:%d", vch.size());
    Object obj;
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get())
        return obj;

    CAccountViewCache view(*pAccountViewTip, true);
    switch (pBaseTx.get()->nTxType) {
    case COMMON_TX: {
        std::shared_ptr<CCommonTx> tx = std::make_shared<CCommonTx>(pBaseTx.get());
        if (tx.get()) {
            obj = tx->ToJson(view);
        }
    }
        break;
    case REG_ACCT_TX: {
        std::shared_ptr<CRegisterAccountTx> tx = std::make_shared<CRegisterAccountTx>(pBaseTx.get());
        if (tx.get()) {
            obj = tx->ToJson(view);
        }
    }
        break;
    case CONTRACT_TX: {
        std::shared_ptr<CContractTx> tx = std::make_shared<CContractTx>(pBaseTx.get());
        if (tx.get()) {
            obj = tx->ToJson(view);
        }
    }
        break;
    case REWARD_TX:
        break;
    case REG_CONT_TX: {
        std::shared_ptr<CRegisterContractTx> tx = std::make_shared<CRegisterContractTx>(pBaseTx.get());
        if (tx.get()) {
            obj = tx->ToJson(view);
        }
    }
        break;
    case DELEGATE_TX: {
        std::shared_ptr<CDelegateTx> tx = std::make_shared<CDelegateTx>(pBaseTx.get());
        if (tx.get()) {
            obj = tx->ToJson(view);
        }
    }
        break;
    default:
        break;
    }
    return obj;
}

Value getalltxinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 0 && params.size() != 1)) {
        throw runtime_error("getalltxinfo \n"
            "\nget all transaction info\n"
            "\nArguments:\n"
            "1.\"nlimitCount\": (numeric, optional, default=0) 0 return all tx, else return number of nlimitCount txs \n"
            "\nResult:\n"
            "\nExamples:\n" + HelpExampleCli("getalltxinfo", "") + "\nAs json rpc call\n"
            + HelpExampleRpc("getalltxinfo", ""));
    }

    Object retObj;
    int nLimitCount(0);
    if(params.size() == 1)
        nLimitCount = params[0].get_int();
    assert(pwalletMain != NULL);
    if(nLimitCount <=0 ) {
        Array ComfirmTx;
        for (auto const &wtx : pwalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                ComfirmTx.push_back(objtx);
            }
        }
        retObj.push_back(Pair("Confirmed", ComfirmTx));

        Array UnComfirmTx;
        CAccountViewCache view(*pAccountViewTip, true);
        for (auto const &wtx : pwalletMain->UnConfirmTx) {
            Object objtx = GetTxDetailJSON(wtx.first);
            UnComfirmTx.push_back(objtx);
        }
        retObj.push_back(Pair("UnConfirmed", UnComfirmTx));
    } else {
        Array ComfirmTx;
        multimap<int, Object, std::greater<int> > mapTx;
        for (auto const &wtx : pwalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                int nConfHeight = find_value(objtx, "confirmedheight").get_int();
                mapTx.insert(pair<int, Object>(nConfHeight, objtx));
            }
        }
        int nSize(0);
        for(auto & txItem : mapTx) {
            if(++nSize > nLimitCount)
                break;
            ComfirmTx.push_back(txItem.second);
        }
        retObj.push_back(Pair("Confirmed", ComfirmTx));
    }

    return retObj;
}

Value printblockdbinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "printblockdbinfo \n"
            "\nprint block log\n"
            "\nArguments:\n"
            "\nExamples:\n" + HelpExampleCli("printblockdbinfo", "")
            + HelpExampleRpc("printblockdbinfo", ""));
    }

    if (!pAccountViewTip->Flush())
        throw runtime_error("Failed to write to account database\n");
    if (!pScriptDBTip->Flush())
        throw runtime_error("Failed to write to account database\n");
    WriteBlockLog(false, "");
    return Value::null;
}

Value getcontractaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error("getcontractaccountinfo \"contract_regid\" \"account_address | account_regid\""
            "\nget contract account info\n"
            "\nArguments:\n"
            "1.\"contract_regid\":(string, required) App RegId\n"
            "2.\"account_address or regid\": (string, required) contract account address or its regid\n"
            "3.\"minconf\"  (numeric, optional, default=1) Only include contract transactions confirmed \n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractaccountinfo", "\"452974-3\" \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getcontractaccountinfo", "\"452974-3\" \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\""));
    }

    string strAppRegId = params[0].get_str();
    if (!CRegID::IsSimpleRegIdStr(strAppRegId))
        throw runtime_error("getcontractaccountinfo: invalid contract regid: " + strAppRegId);

    CRegID appRegId(strAppRegId);

    vector<unsigned char> acctKey;
    if (CRegID::IsSimpleRegIdStr(params[1].get_str())) {
        CRegID acctRegId(params[1].get_str());
        CUserID acctUserId(acctRegId);
        string address = RegIDToAddress(acctUserId);
        acctKey.assign(address.c_str(), address.c_str() + address.length());
    } else { //in wicc address format
        string acctAddr = params[1].get_str();
        acctKey.assign(acctAddr.c_str(), acctAddr.c_str() + acctAddr.length());
    }

    std::shared_ptr<CAppUserAccount> appUserAccount = std::make_shared<CAppUserAccount>();
    if (params.size() == 3 && params[2].get_int() == 0) {
        CScriptDBViewCache viewCache(*mempool.pScriptDBViewCache, true);
        if (!viewCache.GetScriptAcc(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    } else {
        CScriptDBViewCache viewCache(*pScriptDBTip, true);
        if (!viewCache.GetScriptAcc(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    }
    appUserAccount.get()->AutoMergeFreezeToFree(chainActive.Tip()->nHeight);

    return Value(appUserAccount.get()->ToJson());
}

Value listcontractassets(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("listcontractassets regid\n"
            "\nreturn Array containing address, asset information.\n"
            "\nArguments: regid: Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listcontractassets", "1-1")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listcontractassets", "1-1"));
    }

    if (!CRegID::IsSimpleRegIdStr(params[0].get_str()))
        throw runtime_error("in listcontractassets :regid is invalid!\n");

    CRegID script(params[0].get_str());

    Array retArry;
    assert(pwalletMain != NULL);
    {
        set<CKeyID> setKeyId;
        pwalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0)
            return retArry;

        CScriptDBViewCache contractScriptTemp(*pScriptDBTip, true);

        for (const auto &keyId : setKeyId) {

            string address = keyId.ToAddress();
            vector<unsigned char> key;
            key.assign(address.c_str(), address.c_str() + address.length());

            std::shared_ptr<CAppUserAccount> tem = std::make_shared<CAppUserAccount>();
            if (!contractScriptTemp.GetScriptAcc(script, key, *tem.get())) {
                tem = std::make_shared<CAppUserAccount>(key);
            }
            tem.get()->AutoMergeFreezeToFree(chainActive.Tip()->nHeight);

            Object obj;
            obj.push_back(Pair("addr", address));
            obj.push_back(Pair("asset", (double) tem.get()->GetLlValues() / (double) COIN));
            retArry.push_back(obj);
        }
    }

    return retArry;
}


Value gethash(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("gethash  \"str\"\n"
            "\nget the hash of given str\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) \n"
            "\nresult an object \n"
            "\nExamples:\n"
            + HelpExampleCli("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\""));
    }

    string str = params[0].get_str();
    vector<unsigned char> vTemp;
    vTemp.assign(str.c_str(), str.c_str() + str.length());
    uint256 strhash = Hash(vTemp.begin(), vTemp.end());
    Object obj;
    obj.push_back(Pair("hash", strhash.ToString()));
    return obj;

}

Value getcontractkeyvalue(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error("getcontractkeyvalue  \"regid\" \"array\""
            "\nget contract key value\n"
            "\nArguments:\n"
            "1.\"regid\": (string, required) \n"
            "2.\"array\": (string, required) \n"
            "\nExamples:\n"
            + HelpExampleCli("getcontractkeyvalue", "\"1651064-1\" \"Wgim6agki6CmntK4LVs3QnCKbeQ2fsgqWP\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("getcontractkeyvalue", "\"1651064-1\" \"Wgim6agki6CmntK4LVs3QnCKbeQ2fsgqWP\""));
    }

    CRegID scriptid(params[0].get_str());
    Array array = params[1].get_array();

    int height = chainActive.Height();

    if (scriptid.IsEmpty() == true)
        throw runtime_error("in getcontractkeyvalue: contract regid size is error!\n");

    if (!pScriptDBTip->HaveScript(scriptid))
        throw runtime_error("in getcontractkeyvalue: contract regid not exist!\n");

    Array retArry;
    CScriptDBViewCache contractScriptTemp(*pScriptDBTip, true);

    for (size_t i = 0; i < array.size(); i++) {
        uint256 txhash(uint256S(array[i].get_str()));
        vector<unsigned char> key;  // = ParseHex(array[i].get_str());
        key.insert(key.begin(), txhash.begin(), txhash.end());
        vector<unsigned char> value;
        Object obj;
        if (!contractScriptTemp.GetContractData(height, scriptid, key, value)) {
            obj.push_back(Pair("key", array[i].get_str()));
            obj.push_back(Pair("value", HexStr(value)));
        } else {
            obj.push_back(Pair("key", array[i].get_str()));
            obj.push_back(Pair("value", HexStr(value)));
        }

        std::shared_ptr<CBaseTx> pBaseTx;
        int time = 0;
        int height = 0;
        if (SysCfg().IsTxIndex()) {
            CDiskTxPos postx;
            if (pScriptDBTip->ReadTxIndex(txhash, postx)) {
                CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
                CBlockHeader header;
                try {
                    file >> header;
                    fseek(file, postx.nTxOffset, SEEK_CUR);
                    file >> pBaseTx;
                    height = header.GetHeight();
                    time = header.GetTime();
                } catch (std::exception &e) {
                    throw runtime_error(tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
                }
            }
        }

        obj.push_back(Pair("confirmedheight", (int) height));
        obj.push_back(Pair("confirmedtime", (int) time));
        retArry.push_back(obj);
    }

    return retArry;
}

Value gencheckpoint(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 2) {
        throw runtime_error("gencheckpoint \"privatekey\" \"filepath\"\n"
            "\ngenerate checkpoint by Private key signature block.\n"
            "\nArguments:\n"
            "1. \"privatekey\"  (string, required) the private key\n"
            "2. \"filepath\"  (string, required) check point block path\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gencheckpoint", "\"privatekey\" \"filepath\"")
            + HelpExampleRpc("gencheckpoint", "\"privatekey\" \"filepath\""));
    }

    std::string strSecret = params[0].get_str();
    CCoinSecret vchSecret;
    bool fGood = vchSecret.SetString(strSecret);
    if (!fGood)
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid private key encoding");

    CKey key = vchSecret.GetKey();
    if (!key.IsValid())
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Private key invalid");

    string file = params[1].get_str();
    int nHeight(0);
    CBlock block;
    FILE* fp = NULL;
    try {
        fp = fopen(file.c_str(), "rb+");
        CAutoFile fileout = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
        if (!fileout)
            throw JSONRPCError(RPC_MISC_ERROR, "open file:" + file + "failed!");
        fileout >> nHeight;
        fileout >> block;
    } catch (std::exception &e) {
        fclose(fp);
        throw JSONRPCError(RPC_MISC_ERROR, strprintf("read block to file error:%s", e.what()).c_str());
    }

    SyncData::CSyncData data;
    SyncData::CSyncCheckPoint point;
    CDataStream sstream(SER_NETWORK, PROTOCOL_VERSION);
    point.m_height = nHeight;
    point.m_hashCheckpoint = block.GetHash();//chainActive[intTemp]->GetBlockHash();
    LogPrint("CHECKPOINT", "send hash = %s\n",block.GetHash().ToString());
    sstream << point;
    Object obj;
    if (data.Sign(key, std::vector<unsigned char>(sstream.begin(), sstream.end()))
        && data.CheckSignature(SysCfg().GetCheckPointPKey())) {
        obj.push_back(Pair("chenkpoint", data.ToJsonObj()));
        return obj;
    }
    return obj;
}

Value setcheckpoint(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "setcheckpoint \"filepath\"\n"
            "\nadd new checkpoint and send it out.\n"
            "\nArguments:\n"
            "1. \"filepath\"  (string, required) check point block path\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("setcheckpoint", "\"filepath\"")
            + HelpExampleRpc("setcheckpoint", "\"filepath\""));
    }
    SyncData::CSyncData data;
    ifstream file;
    file.open(params[0].get_str().c_str(), ios::in | ios::ate);
    if (!file.is_open())
          throw JSONRPCError(RPC_INVALID_PARAMETER, "Cannot open check point dump file");

    file.seekg(0, file.beg);
    if (file.good()){
        Value reply;
        json_spirit::read(file,reply);
        const Value & checkpoint = find_value(reply.get_obj(),"chenkpoint");
        if(checkpoint.type() ==  json_spirit::null_type)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "read check point failed");

        const Value & msg = find_value(checkpoint.get_obj(), "msg");
        const Value & sig = find_value(checkpoint.get_obj(), "sig");
        if(msg.type() == json_spirit::null_type || sig.type() == json_spirit::null_type)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "read msg or sig failed");

        data.m_vchMsg = ParseHex(msg.get_str());
        data.m_vchSig = ParseHex(sig.get_str());
    }
    file.close();
    if(!data.CheckSignature(SysCfg().GetCheckPointPKey()))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "check signature failed");

    SyncData::CSyncDataDb db;
    std::vector<SyncData::CSyncData> vdata;
    SyncData::CSyncCheckPoint point;
    CDataStream sstream(data.m_vchMsg, SER_NETWORK, PROTOCOL_VERSION);
    sstream >> point;
    db.WriteCheckpoint(point.m_height, data);
    Checkpoints::AddCheckpoint(point.m_height, point.m_hashCheckpoint);
    CheckActiveChain(point.m_height, point.m_hashCheckpoint);
    vdata.push_back(data);
    LOCK(cs_vNodes);
    BOOST_FOREACH(CNode* pnode, vNodes)
    {
        if (pnode->setcheckPointKnown.count(point.m_height) == 0) {
            pnode->setcheckPointKnown.insert(point.m_height);
            pnode->PushMessage("checkpoint", vdata);
        }
    }
    return tfm::format("sendcheckpoint :%d\n", point.m_height);
}

Value validateaddr(const Array& params, bool fHelp)
{
    if (fHelp || params.size() != 1)
        throw runtime_error("validateaddr \"wicc_address\"\n"
            "\ncheck whether address is valid or not\n"
            "\nArguments:\n"
            "1. \"wicc_address\"  (string, required) wicc coin address\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("validateaddr", "\"De5nZAbhMikMPGHzxvSGqHTgEuf3eNUiZ7\"")
            + HelpExampleRpc("validateaddr", "\"De5nZAbhMikMPGHzxvSGqHTgEuf3eNUiZ7\""));


    Object obj;
    CKeyID keyid;
    string addr = params[0].get_str();
    if (!GetKeyId(addr, keyid)) {
        obj.push_back(Pair("ret" , false));
    } else {
        obj.push_back(Pair("ret" , true));
    }
    return obj;
}

Value gettotalcoins(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0) {
        throw runtime_error(
            "gettotalcoins \n"
            "\nget the total number of circulating coins excluding those locked for votes\n"
            "\nand the toal number of registered addresses\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalcoins", "")
            + HelpExampleRpc("gettotalcoins", ""));
    }

    Object obj;
    {
        CAccountViewCache view(*pAccountViewDB, true);
        uint64_t totalCoins(0);
        uint64_t totalRegIds(0);
        std::tie(totalCoins, totalRegIds) = view.TraverseAccount();
        // auto [totalCoins, totalRegIds] = view.TraverseAccount(); //C++17
        obj.push_back( Pair("total_coins", ValueFromAmount(totalCoins)) );
        obj.push_back( Pair("total_regids", totalRegIds) );
    }
    return obj;
}

Value gettotalassets(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 1) {
        throw runtime_error("gettotalassets \n"
            "\nget all assets belonging to a contract\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalassets", "11-1")
            + HelpExampleRpc("gettotalassets", "11-1"));
    }
    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() == true)
        throw runtime_error("contract regid invalid!\n");

    if (!pScriptDBTip->HaveScript(regid))
        throw runtime_error("contract regid not exist!\n");

    CScriptDBViewCache contractScriptTemp(*pScriptDBTip, true);
    Object obj;
    {
        map<vector<unsigned char>, vector<unsigned char> > mapAcc;
        bool bRet = contractScriptTemp.GetAllScriptAcc(regid, mapAcc);
        if (bRet) {
            uint64_t totalassets = 0;
            map<vector<unsigned char>, vector<unsigned char>>::iterator it;
            for (it = mapAcc.begin(); it != mapAcc.end();++it) {
                CAppUserAccount appAccOut;
                vector<unsigned char> vKey = it->first;
                vector<unsigned char> vValue = it->second;

                CDataStream ds(vValue, SER_DISK, CLIENT_VERSION);
                ds >> appAccOut;

                totalassets += appAccOut.GetLlValues();
                totalassets += appAccOut.GetAllFreezedValues();
            }

            obj.push_back(Pair("total_assets", ValueFromAmount(totalassets)));
        } else
            throw runtime_error("failed to find contract account!\n");
    }
    return obj;
}

Value listtxbyaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "listtxbyaddr \n"
            "\nlist all transactions by their sender/receiver addresss\n"
            "\nArguments:\n"
            "1.\"address\": (string, required)\n"
            "2.\"height\": (numeric, required)\n"
            "\nResult: address related tx hash as array\n"
            "\nExamples:\n" +
            HelpExampleCli("listtxbyaddr",
                           "\"wcoA7yUW4fc4m6a2HSk36t4VVxzKUnvq4S\" \"10000\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("listtxbyaddr",
                           "\"wcoA7yUW4fc4m6a2HSk36t4VVxzKUnvq4S\", \"10000\""));
    }

    string address = params[0].get_str();
    int height     = params[1].get_int();
    if (height < 0 || height > chainActive.Height())
        throw runtime_error("Height out of range.");

    CKeyID keyId;
    if (!GetKeyId(address, keyId))
        throw runtime_error("Address invalid.");

    CScriptDBViewCache scriptDbView(*pScriptDBTip, true);
    map<vector<unsigned char>, vector<unsigned char>> mapTxHash;
    if (!scriptDbView.GetTxHashByAddress(keyId, height, mapTxHash))
        throw runtime_error("Failed to fetch data.");

    Object obj;
    Array arrayObj;
    for (auto item : mapTxHash) {
        arrayObj.push_back(string(item.second.begin(), item.second.end()));
    }
    obj.push_back(Pair("address", address));
    obj.push_back(Pair("height", height));
    obj.push_back(Pair("txarray", arrayObj));

    return obj;
}

Value listdelegates(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1) {
        throw runtime_error(
                "listdelegates \n"
                "\nreturns the specified number delegates by reversed order voting number.\n"
                "\nArguments:\n"
                "1. number           (number, optional) the number of the delegates, default to all delegates.\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listdelegates", "11")
                + HelpExampleRpc("listdelegates", "11"));
    }

    int nDelegateNum = (params.size() == 1) ? params[0].get_int() : IniCfg().GetDelegatesNum();
    if (nDelegateNum < 1 || nDelegateNum > 11) {
        char errorMsg[100] = {'\0'};
        sprintf(errorMsg, "input delegate number not between 1 and %ld", IniCfg().GetDelegatesNum());
        throw JSONRPCError(RPC_INVALID_PARAMS, errorMsg);
    }

    int nIndex = 0;
    vector<unsigned char> vScriptData;
    CScriptDBViewCache contractScriptTemp(*pScriptDBTip, true);
    CAccountViewCache view(*pAccountViewTip, true);
    Object obj;
    Array delegateArray;
    vector<unsigned char> vDelegateKey = {'d','e','l','e','g','a','t','e','_'};
    vector<unsigned char> vDelegatePrefix = vDelegateKey;
    while (--nDelegateNum >= 0) {
        CRegID regId(0,0);
        if (contractScriptTemp.GetContractData(0, regId, nIndex, vDelegateKey, vScriptData)) {
            nIndex = 1;
            vector<unsigned char>::iterator iterVotes = find_first_of(vDelegateKey.begin(), vDelegateKey.end(),
                vDelegatePrefix.begin(), vDelegatePrefix.end());
            string strVotes(iterVotes+9, iterVotes+25);
            uint64_t llVotes = 0;
            stringstream strValue;
            strValue.flags(ios::hex);
            strValue << strVotes;
            strValue >> llVotes;

            vector<unsigned char> vAcctRegId(iterVotes+26, vDelegateKey.end());
            CRegID acctRegId(vAcctRegId);
            CAccount account;
            if (!view.GetAccount(acctRegId, account))
                assert(0);

            uint64_t maxNum = 0xFFFFFFFFFFFFFFFF;
            if ((maxNum - llVotes) != account.llVotes) {
                LogPrint("INFO", "llVotes:%lld, account:%s", maxNum - llVotes, account.ToString());
                assert(0);
            }
            delegateArray.push_back(account.ToJsonObj());
      } else {
          assert(0);
      }
    }
    obj.push_back(Pair("delegates", delegateArray));
    return obj;
}
