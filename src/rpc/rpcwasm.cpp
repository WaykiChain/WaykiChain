// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "config/configuration.h"
#include "miner/miner.h"
#include "main.h"
//#include "vm/vmrunenv.h"
#include <stdint.h>
#include <chrono>

#include "entities/contract.h"

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"
#include "json/json_spirit_writer.h"

#include "datastream.hpp"
#include "abi_serializer.hpp"
#include "wasmcontext.hpp"
#include "exceptions.hpp"
#include "types/name.hpp"
#include "types/asset.hpp"

using namespace std;
using namespace boost;
using namespace json_spirit;
using namespace boost::assign;
using std::chrono::microseconds;
// using namespace wasm;

static bool GetKeyId(string const &addr, CKeyID &KeyId) {
    if (!CRegID::GetKeyId(addr, KeyId)) {
        KeyId = CKeyID(addr);
        if (KeyId.IsEmpty())
            return false;
    }
    return true;
}

string StringToHexString(string str, string separator = " ")
{

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;

    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

    return ss.str();

}

string VectorToHexString(std::vector<uint8_t> str, string separator = " ")
{

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;

    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

    return ss.str();

}

// send code and abi
Value setcodewasmcontracttx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 4 || params.size() > 7) {
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
        // 1.sender
        // 2.contract(id)
        // 3.filepath for code
        // 4.filepath for abi
        // 5.fee
        // 6.memo
        // 7.height
    }

   RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(int_type)(str_type)(int_type));

   // std::cout << "wasmsetcodecontracttx line103"
   //           << " sender:" << params[0].get_str()
   //           << " contract:"<< params[1].get_str()
   //           << " code:"<< params[2].get_str()
   //           << " abi:"<< params[3].get_str()
   //           << " fee:"<< params[4].get_uint64()
   //           << " memo:"<< params[5].get_str()
   //           << " \n";

    string code, abi;
    string codeFile = GetAbsolutePath(params[2].get_str()).string();
    string abiFile = GetAbsolutePath(params[3].get_str()).string();

    if(codeFile.empty() && abiFile.empty()){
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Wasm code and abi file both do not exist!");
    }

    if (!codeFile.empty())
    //    throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Wasm file does not exist!");
    //std::cout << "deploywasmcontracttx line69: codeFile: " << codeFile << " " << "\n";
    // if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
    //     throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");
    {
        char byte;
        ifstream f(codeFile, ios::binary);
        while(f.get(byte))  code.push_back(byte);
        size_t size = code.size();
        //std::cout << "deploywasmcontracttx line131: code:" << StringToHexString(code) <<" size:"<< size <<"\n";
        if (size <= 0 || size > MAX_CONTRACT_CODE_SIZE) {
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                (size == -1) ? "File size is unknown"
                              : ((size == 0) ? "File is empty" : "contract code must less than 64 Kbytes"));
        }

        if (size <= 0 || size > MAX_CONTRACT_CODE_SIZE) { // contract code size must be <= 64 KB)
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                (size == -1) ? "File size is unknown"
                              : ((size == 0) ? "File is empty" : "File size exceeds 64 KB limit"));
        }
    }

    if (!abiFile.empty())
    //    throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "abi file does not exist!");
    {
        char byte;
        ifstream f(abiFile, ios::binary);
        while(f.get(byte))  abi.push_back(byte);
        int size = abi.size();
        //std::cout << "deploywasmcontracttx line131: code:" << StringToHexString(code) <<" size:"<< size <<"\n";
        if (size <= 0 || size > MAX_CONTRACT_CODE_SIZE) {
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                (size == -1) ? "File size is unknown"
                              : ((size == 0) ? "File is empty" : "Contract abi must less than 8 Kbytes"));
        }

        if (size <= 0 || size > MAX_CONTRACT_CODE_SIZE) { // contract code size must be <= 64 KB)
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                (size == -1) ? "File size is unknown"
                              : ((size == 0) ? "File is empty" : "File size exceeds 8 KB limit"));
        }
    }

    json_spirit::Value abiJson;
    json_spirit::read_string(abi, abiJson);

    std::cout << "wasmsetcodecontracttx line173"
             << " abi:" << json_spirit::write(abiJson)
             << " \n";


    //CWasmCode wasmCode;
    //wasmCode.GetCode().insert(wasmCode.GetCode().end(),code.begin(), code.end());
    string memo;
    if (params.size() > 5) {
        string memo = params[5].get_str();
        if (memo.size() > MAX_CONTRACT_CODE_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "The size of the memo of a contract must less than 100 bytes");
        }
        //wasmCode.GetCode().insert(wasmCode.GetCode().end(),description.begin(), description.end());
    }

    // string contractCode;
    // CDataStream ds(SER_DISK, CLIENT_VERSION);
    // ds << wasmCode;
    // contractCode.assign(ds.begin(), ds.end());

    uint64_t fee = params[4].get_uint64();

    int height = chainActive.Tip()->height;
    if (params.size() > 6)
    {
        if (params[6].get_int() != 0)
            height = params[6].get_int();
    }

    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fee is less than minimum transaction fee");
    }


    CKeyID sender;
    if (!GetKeyId(params[0].get_str(), sender)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sender address");
    }

    CRegID contractRegID(params[1].get_str());
    if (contractRegID.IsEmpty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
    }

    assert(pWalletMain != NULL);
    CWasmContractTx tx;
    {
        EnsureWalletIsUnlocked();

        CAccount account;
        if (!pCdMan->pAccountCache->GetAccount(sender, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Invalid send address");
        }

        if (!account.HaveOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(sender)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        CRegID senderRegID;
        pCdMan->pAccountCache->GetRegId(sender, senderRegID);


        uint64_t contract = wasm::RegID2Name(contractRegID);

        std::cout << "wasmsetcodecontracttx line250"
         << " contract:"<< contract
         << " \n";

        tx.nTxType      = WASM_CONTRACT_TX;
        tx.txUid          = senderRegID;
        tx.llFees         = fee;


        tx.contract       = wasm::name("wasmio").value;
        tx.action         = wasm::name("setcode").value;
        tx.data           = wasm::pack(std::tuple(contract, code, abi, memo));

        tx.nRunStep       = tx.data.size();
        if (0 == height) {
            height = chainActive.Tip()->height;
        }
        tx.valid_height   = height;
        tx.nRunStep       = tx.data.size();


        //CAccountDBCache view;
        //std::cout << "deploywasmcontracttx line112: " << tx.ToString(view) << " " << "\n";
        if (!pWalletMain->Sign(sender, tx.ComputeSignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) &tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }
    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}

Value callwasmcontracttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "callwasmcontracttx \"sender addr\" \"contract\" \"action\" \"data\" \"amount\" \"fee\" (\"height\")\n"
            "1.\"sender addr\": (string, required) tx sender's base58 addr\n"
            "2.\"contract\":   (string, required) contract name\n"
            "3.\"action\":   (string, required) action name\n"
            "4.\"data\":   (json string, required) action data\n"
            "5.\"amount\":      (numeric, required) amount of WICC to be sent to the contract account\n"
            "6.\"fee\":         (numeric, required) pay to miner\n"
            "7.\"height\":      (numberic, optional) valid height\n"
            "\nResult:\n"
            "\"txid\":        (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 100"));
        // 1.sender
        // 2.contract(id)
        // 3.action
        // 4.data
        // 5.amount
        // 6.fee
        // 7.height
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(obj_type)(int_type)(int_type));

    std::cout << "rpccall wasmcontracttx line321"
         << " sender:" << params[0].get_str()
         << " contract:"<< params[1].get_str()
         << " action:"<< params[2].get_str()
         << " data:"<< json_spirit::write(params[3].get_obj())
         << " amount:"<< params[4].get_uint64()
         << " fee:"<< params[5].get_uint64()
         << " \n";

    EnsureWalletIsUnlocked();

    CKeyID sender;
    if (!GetKeyId(params[0].get_str(), sender))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    // if (!GetKeyId(params[1].get_str(), recvKeyId)) {
    //     throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    // }

    CRegID contractRegID(params[1].get_str());
    if (contractRegID.IsEmpty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
    }


    uint64_t contract = wasm::RegID2Name(contractRegID);
    uint64_t action = wasm::name(params[2].get_str()).value;


    //json_spirit::arguments  = params[3].get_obj();
    string arguments = json_spirit::write(params[3].get_obj());

    // string arguments = params[3].get_str();
    // if (arguments.size() >= kContractArgumentMaxSize) {
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size out of range");
    // }


    CUniversalContract contractCode;
    pCdMan->pContractCache->GetContract(contractRegID, contractCode);
    std::vector<char> data;

    if( contractCode.abi.size() > 0 )
        try {
            data = wasm::abi_serializer::pack( contractCode.abi, wasm::name(action).to_string(), arguments, max_serialization_time );
        } catch( wasm::CException& e ){

            throw JSONRPCError(e.errCode, e.errMsg);
        }

    else{
        //string params = json_spirit::write(data_v);
        data.insert(data.begin(), arguments.begin(), arguments.end() );
    }

    // wasm::name issuer = wasm::name("walker");
    // wasm::asset maximum_supply = wasm::asset{1000000000, symbol("BTC", 4)};
    // std::vector<char> data = wasm::pack(std::tuple(issuer, maximum_supply));


    int64_t amount = AmountToRawValue(params[4]);
    uint64_t fee    = AmountToRawValue(params[5]);
    int height     = (params.size() > 6) ? params[6].get_int() : chainActive.Height();
    if (fee == 0) {
        GetTxMinFee(TxType::UCONTRACT_DEPLOY_TX, height, SYMB::WICC, fee);
    }

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sender, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sender), sendRegId) && sendRegId.IsMature(chainActive.Height() + 1))
                     ? CUserID(sendRegId)
                     : CUserID(sendPubKey);

    // CRegID recvRegId;
    // if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    // }

    if (!pCdMan->pContractCache->HaveContract(contractRegID)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }

    CWasmContractTx tx;
    tx.nTxType      = WASM_CONTRACT_TX;
    tx.txUid        = sendUserId;
    tx.valid_height = height;
    tx.llFees       = fee;
    tx.fee_symbol   = SYMB::WICC;

    tx.contract     = contract;
    tx.action       = action;
    tx.data         = data;

    tx.amount       = amount;
    //tx.symbol       = symbol;

    if (!pWalletMain->Sign(sender, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx*)&tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}