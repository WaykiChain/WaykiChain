// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
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
#include "wasm_context.hpp"
#include "exceptions.hpp"
#include "types/name.hpp"
#include "types/asset.hpp"
#include "wasm_config.hpp"

using namespace std;
using namespace boost;
using namespace json_spirit;
using namespace boost::assign;
using std::chrono::microseconds;
// using namespace wasm;

// send code and abi
Value setcodewasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 4 || params.size() > 7) {
        throw runtime_error(
                "setcodewasmcontracttx \"sender\" \"contract\" \"wasm_file\" \"abi_file\" [\"memo\"] [symbol:fee:unit]\n"
                "\ncreate a transaction of registering a contract app\n"
                "\nArguments:\n"
                "1.\"sender\": (string required) contract owner address from this wallet\n"
                "2.\"contract\": (string required), contract name\n"
                "3.\"wasm_file\": (string required), the file path of the contract code\n"
                "4.\"abi_file\": (string required), the file path of the contract abi\n"
                "5.\"symbol:fee:unit\": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi\n"
                "6.\"memo\": (string optional) the memo of contract\n"
                "\nResult:\n"
                "\"txhash\": (string)\n"
                "\nExamples:\n"
                + HelpExampleCli("setcodewasmcontracttx",
                                 "\"10-3\" \"20-3\" \"/tmp/myapp.wasm\" \"/tmp/myapp.bai\"") +
                "\nAs json rpc call\n"
                + HelpExampleRpc("setcodewasmcontracttx",
                                 "\"10-3\" \"20-3\" \"/tmp/myapp.wasm\" \"/tmp/myapp.bai\""));

        // 1.sender
        // 2.contract(id)
        // 3.filepath for code
        // 4.filepath for abi
        // 5.memo
        // 6.fee
    }

    CKeyID sender;
    if (!GetKeyId(params[0].get_str(), sender)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sender address");
    }

    CRegID contractRegID(params[1].get_str());
    if (contractRegID.IsEmpty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
    }

    string codeFile = GetAbsolutePath(params[2].get_str()).string();
    string abiFile = GetAbsolutePath(params[3].get_str()).string();

    string code, abi;
    if (codeFile.empty() || abiFile.empty()) {
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Wasm code or abi file do not exist!");
    }

    if (!codeFile.empty()) {
        char byte;
        ifstream f(codeFile, ios::binary);
        while (f.get(byte)) code.push_back(byte);
        size_t size = code.size();
        if (size == 0 || size > MAX_CONTRACT_CODE_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("contract code is empty or larger than %d bytes", MAX_CONTRACT_CODE_SIZE));
        }

        if (size == 0 || size > MAX_CONTRACT_CODE_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("contract abi is empty or lager than %d bytes", MAX_CONTRACT_CODE_SIZE));
        }
    }

    //validate code
    try {
        vector <uint8_t> code_v;
        code_v.insert(code_v.begin(), code.begin(), code.end());
        CWasmInterface wasmInterface;
        wasmInterface.validate(code_v);
    } catch (wasm::exception &e) {
        throw JSONRPCError(e.code(), e.detail());
    }


    if (!abiFile.empty()) {
        char byte;
        ifstream f(abiFile, ios::binary);
        while (f.get(byte)) abi.push_back(byte);
        size_t size = abi.size();
        if (size == 0 || size > MAX_CONTRACT_CODE_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("contract abi is empty or lager than %d bytes", MAX_CONTRACT_CODE_SIZE));
        }
    }

    //validate abi
    try {
        json_spirit::Value abiJson;
        json_spirit::read_string(abi, abiJson);

        abi_def abi_d;
        from_variant(abiJson, abi_d);
        wasm::abi_serializer abis(abi_d, max_serialization_time);
        
        //abi in vector
        std::vector<char> abi_v = wasm::pack<wasm::abi_def>(abi_d);
        abi = string(abi_v.begin(), abi_v.end());

    } catch (wasm::exception &e) {
        throw JSONRPCError(e.code(), e.detail());
    }

    string memo;
    if (params.size() > 5) {
        string memo = params[5].get_str();
        if (memo.size() > MAX_CONTRACT_MEMO_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                               strprintf("The size of the memo of a contract must less than %d bytes",
                                         MAX_CONTRACT_MEMO_SIZE));
        }
    }

    const ComboMoney &fee = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_DEPLOY_TX);

    int height = chainActive.Tip()->height;

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

        RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetSawiAmount());

        if (!pWalletMain->HaveKey(sender)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        CRegID senderRegID;
        pCdMan->pAccountCache->GetRegId(sender, senderRegID);


        uint64_t contract = wasm::RegID2Name(contractRegID);

        tx.nTxType = WASM_CONTRACT_TX;
        tx.txUid = senderRegID;
        tx.fee_symbol = fee.symbol;
        tx.llFees = fee.GetSawiAmount();


        tx.inlinetransactions.push_back({wasmio, 
                                         wasm::N(setcode), 
                                         std::vector<permission>{{wasmio, wasmio_owner}},
                                         wasm::pack(std::tuple(contract, code, abi, memo))});


        if (0 == height) {
            height = chainActive.Tip()->height;
        }
        tx.valid_height = height;
        //tx.nRunStep = tx.data.size();

        if (!pWalletMain->Sign(sender, tx.ComputeSignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx * ) & tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;

    json_spirit::Value abi_v;
    json_spirit::read_string(std::get<1>(ret), abi_v);

    json_spirit::Config::add(obj, "result",  abi_v);

    return obj;
}

Value callwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
                "callwasmcontracttx \"sender addr\" \"contract\" \"action\" \"data\" \"amount\" \"fee\" \n"
                "1.\"sender \": (string, required) tx sender's base58 addr\n"
                "2.\"contract\":   (string, required) contract name\n"
                "3.\"action\":   (string, required) action name\n"
                "4.\"data\":   (json string, required) action data\n"
                // "5.\"amount\":      (numeric, required) amount of WICC to be sent to the contract account\n"
                "5.\"fee\":         (numeric, required) pay to miner\n"
                "\nResult:\n"
                "\"txid\":        (string)\n"
                "\nExamples:\n" +
                HelpExampleCli("callcontracttx",
                               "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 100") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("callcontracttx",
                               "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 100"));
        // 1.sender
        // 2.contract
        // 3.action
        // 4.data
        // 5.fee
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    // std::cout << "rpccall wasmcontracttx line321"
    //           << " sender:" << params[0].get_str()
    //           << " contract:" << params[1].get_str()
    //           << " action:" << params[2].get_str()
    //           //<< " data:"<< json_spirit::write(params[3].get_obj())
    //           << " data:" << params[3].get_str()
    //           //<< " amount:" << params[4].get_str()
    //           << " fee:" << params[4].get_str()
    //           << " \n";


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

    if (!pCdMan->pContractCache->HaveContract(contractRegID)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }


    uint64_t contract = wasm::RegID2Name(contractRegID);
    uint64_t action = wasm::NAME(params[2].get_str().c_str());

    string arguments = params[3].get_str();
    if (arguments.empty() || arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments empty or the size out of range");
    }


    CUniversalContract contractCode;
    pCdMan->pContractCache->GetContract(contractRegID, contractCode);

    std::vector<char> data;
    if (contractCode.abi.size() > 0)
        try {

            std::vector<char> abi(contractCode.abi.begin(), contractCode.abi.end());
            data = wasm::abi_serializer::pack(abi, wasm::name(action).to_string(), arguments,
                                              max_serialization_time);
        } catch (wasm::exception &e) {
            throw JSONRPCError(e.code(), e.detail());
        }
    else {
        data.insert(data.begin(), arguments.begin(), arguments.end());
    }


    ComboMoney fee = RPC_PARAM::GetFee(params, 4, TxType::UCONTRACT_INVOKE_TX);

    uint32_t height = chainActive.Height();

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sender, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sender), sendRegId) && sendRegId.IsMature(height + 1))
                 ? CUserID(sendRegId)
                 : CUserID(sendPubKey);

    // CRegID recvRegId;
    // if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
    //     throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    // }

    CWasmContractTx tx;
    tx.nTxType = WASM_CONTRACT_TX;
    tx.txUid = sendUserId;
    tx.valid_height = height;
    tx.fee_symbol = fee.symbol;
    tx.llFees = fee.GetSawiAmount();


    tx.inlinetransactions.push_back({contract, 
                                     action, 
                                     std::vector<permission>{{wasm::N(sender), wasmio_owner}}, 
                                     data});

    // tx.symbol = amount.symbol;
    // tx.amount = amount.GetSawiAmount();

    if (!pWalletMain->Sign(sender, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx * ) & tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;

    json_spirit::Value abi_v;
    json_spirit::read_string(std::get<1>(ret), abi_v);
    json_spirit::Config::add(obj, "result",  abi_v);

    return obj;
}

Value gettablewasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"table\":   (string, required) table name\n"
                "3.\"numbers\":   (numberic, optional) numbers\n"
                "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
                "\nResult:\n"
                "\"rows\":        (string)\n"
                "\"more\":        (bool)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"stat\" 10") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"stat\", 10"));
        // 1.contract(id)
        // 2.table
        // 3.number
        // 4.begin_key
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));

    // std::cout << "rpccall gettablerowwasmcontracttx "
    //           << " contract:" << params[0].get_str()
    //           << " table:" << params[1].get_str()
    //           << " numbers:" << params[2].get_str()
    //           //<< " last_key:" << params[3].get_str()
    //           << std::endl;

    //EnsureWalletIsUnlocked();

    CRegID contractRegID(params[0].get_str());
    if (contractRegID.IsEmpty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
    }

    uint64_t contract = wasm::RegID2Name(contractRegID);

    CUniversalContract contractCode;
    if (!pCdMan->pContractCache->GetContract(contractRegID, contractCode))
        throw JSONRPCError(READ_SCRIPT_FAIL, "can not get contract code");

    //string abi = contractCode.abi;
    std::vector<char> abi(contractCode.abi.begin(), contractCode.abi.end());
    if (abi.size() == 0)
        throw JSONRPCError(READ_SCRIPT_FAIL, "this contract didn't set abi");


    uint64_t table = wasm::NAME(params[1].get_str().c_str());

    uint64_t numbers = default_query_rows;
    if (params.size() > 2)
        numbers = std::atoi(params[2].get_str().data());

    string keyPrefix;
    std::vector<char> k = wasm::pack(std::tuple(contract, table));
    keyPrefix.insert(keyPrefix.end(), k.begin(), k.end());

    string lastKey = ""; // TODO: get last key
    if (params.size() > 3) {
        lastKey = FromHex(params[3].get_str());
    }

    auto pGetter = pCdMan->pContractCache->CreateContractDatasGetter(contractRegID, keyPrefix, numbers, lastKey);
    if (!pGetter || !pGetter->Execute()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "get contract datas error! contract_regid=%s, ");
    }

    json_spirit::Object object;
    try {
        json_spirit::Array vars;
        string last_key;

        for (auto item : pGetter->data_list) {
            last_key = pGetter->GetKey(item);
            const string &value = pGetter->GetValue(item);

            std::vector<char> row(value.begin(), value.end());
            //row.insert(row.end(), value.begin(), value.end());
            json_spirit::Value v = wasm::abi_serializer::unpack(abi, table, row, max_serialization_time);

            json_spirit::Object &obj = v.get_obj();
            obj.push_back(Pair("key", ToHex(last_key, "")));
            obj.push_back(Pair("value", ToHex(last_key, "")));

            vars.push_back(v);
        }

        object.push_back(Pair("rows", vars));
        object.push_back(Pair("more", pGetter->has_more));

    } catch (wasm::exception &e) {
        throw JSONRPCError( e.code(), e.detail() );
    }

    return object;


}

Value abijsontobinwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"action\" \"data\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"action\":   (string, required) action name\n"
                "3.\"data\":   (json string, required) action data\n"
                "\nResult:\n"
                "\"data\":        (string)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"transfer\" ") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"transfer\" "));
        // 1.contract(id)
        // 2.action
        // 3.data
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    CRegID contractRegID(params[0].get_str());
    if (contractRegID.IsEmpty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid contract address");
    }
   
    //uint64_t contract = wasm::RegID2Name(contractRegID);
    uint64_t action = wasm::NAME(params[1].get_str().c_str());

    string arguments = params[2].get_str();
    if (arguments.empty() || arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments empty or the size out of range");
    }

    CUniversalContract contractCode;
    pCdMan->pContractCache->GetContract(contractRegID, contractCode);

    std::vector<char> data;
    if (contractCode.abi.size() > 0){
        try {

            std::vector<char> abi(contractCode.abi.begin(), contractCode.abi.end());
            data = wasm::abi_serializer::pack(abi, wasm::name(action).to_string(), arguments,
                                              max_serialization_time);
        } catch (wasm::exception &e) {
            throw JSONRPCError(e.code(), e.detail());
        }
    } else {
        throw JSONRPCError(READ_SCRIPT_FAIL, "Can not find contract abi");
    }

    json_spirit::Object object;
    object.push_back(Pair("data", wasm::ToHex(data,"")));

    return object;    
}


Value abibintojsonwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"table\":   (string, required) table name\n"
                "3.\"numbers\":   (numberic, optional) numbers\n"
                "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
                "\nResult:\n"
                "\"rows\":        (string)\n"
                "\"more\":        (bool)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"stat\" 10") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"stat\", 10"));
        // 1.contract(id)
        // 2.table
        // 3.number
        // 4.begin_key
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));
    json_spirit::Object object;
    return object;  

}

Value getcodewasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"table\":   (string, required) table name\n"
                "3.\"numbers\":   (numberic, optional) numbers\n"
                "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
                "\nResult:\n"
                "\"rows\":        (string)\n"
                "\"more\":        (bool)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"stat\" 10") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"stat\", 10"));
        // 1.contract(id)
        // 2.table
        // 3.number
        // 4.begin_key
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));
    json_spirit::Object object;
    return object;  

}

Value getabiwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"table\":   (string, required) table name\n"
                "3.\"numbers\":   (numberic, optional) numbers\n"
                "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
                "\nResult:\n"
                "\"rows\":        (string)\n"
                "\"more\":        (bool)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"stat\" 10") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"stat\", 10"));
        // 1.contract(id)
        // 2.table
        // 3.number
        // 4.begin_key
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));

    json_spirit::Object object;
    return object;  

}

Value getrawcodewasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"table\":   (string, required) table name\n"
                "3.\"numbers\":   (numberic, optional) numbers\n"
                "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
                "\nResult:\n"
                "\"rows\":        (string)\n"
                "\"more\":        (bool)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"stat\" 10") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"stat\", 10"));
        // 1.contract(id)
        // 2.table
        // 3.number
        // 4.begin_key
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));
    json_spirit::Object object;
    return object;  

}

Value getrawabiwasmcontracttx( const Array &params, bool fHelp ) {
    if (fHelp || params.size() < 2 || params.size() > 4) {
        throw runtime_error(
                "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
                "1.\"contract\": (string, required) contract name\n"
                "2.\"table\":   (string, required) table name\n"
                "3.\"numbers\":   (numberic, optional) numbers\n"
                "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
                "\nResult:\n"
                "\"rows\":        (string)\n"
                "\"more\":        (bool)\n"
                "\nExamples:\n" +
                HelpExampleCli("gettablewasmcontracttx",
                               " \"411994-1\" \"stat\" 10") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("gettablewasmcontracttx",
                               "\"411994-1\", \"stat\", 10"));
        // 1.contract(id)
        // 2.table
        // 3.number
        // 4.begin_key
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));
    json_spirit::Object object;
    return object;  

}