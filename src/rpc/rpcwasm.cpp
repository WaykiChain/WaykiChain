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
#include "wasmcontext.hpp"
#include "exceptions.hpp"
#include "types/name.hpp"
#include "types/asset.hpp"
#include "wasmconfig.hpp"

using namespace std;
using namespace boost;
using namespace json_spirit;
using namespace boost::assign;
using std::chrono::microseconds;
// using namespace wasm;

string ToHex(string str, string separator = " ")
{

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;

    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

    return ss.str();

}

string ToHex(std::vector<char> str, string separator = " ")
{

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;

    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

    return ss.str();

}

string ToHex(std::vector<uint8_t> str, string separator = " ")
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
        throw runtime_error("setcodewasmcontracttx \"addr\" \"contract_id\" \"code_file\" \"abi_file\" [\"memo\"] [symbol:fee:unit]\n"
            "\ncreate a transaction of registering a contract app\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) contract owner address from this wallet\n"
            "2.\"contract_id\": (string required), the script id, regid\n"
            "3.\"code_file\": (string required), the file path of the contract code\n"
            "4.\"abi_file\": (string required), the file path of the contract abi\n"
            "5.\"memo\": (string optional) the memo of contract\n"
            "6. \"symbol:fee:unit\": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi\n"
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
        // 5.fee
        // 6.memo
        // 7.height
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
    if(codeFile.empty() || abiFile.empty()){
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Wasm code or abi file do not exist!");
    }

    if (!codeFile.empty())
    {
        char byte;
        ifstream f(codeFile, ios::binary);
        while(f.get(byte))  code.push_back(byte);
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

    if (!abiFile.empty())
    //    throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "abi file does not exist!");
    {
        char byte;
        ifstream f(abiFile, ios::binary);
        while(f.get(byte))  abi.push_back(byte);
        size_t size = abi.size();
        if (size == 0 || size > MAX_CONTRACT_CODE_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                strprintf("contract abi is empty or lager than %d bytes", MAX_CONTRACT_CODE_SIZE));
        }
    }

    json_spirit::Value abiJson;
    json_spirit::read_string(abi, abiJson);

    std::cout << "wasmsetcodecontracttx line173"
             << " abi:" << json_spirit::write(abiJson)
             << " \n";

    string memo;
    if (params.size() > 4) {
        string memo = params[4].get_str();
        if (memo.size() > MAX_CONTRACT_MEMO_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER,
                strprintf("The size of the memo of a contract must less than %d bytes", MAX_CONTRACT_MEMO_SIZE));
        }
    }

    const ComboMoney &fee = RPC_PARAM::GetFee(params, 5, TxType::UCONTRACT_DEPLOY_TX);

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

        std::cout << "wasmsetcodecontracttx line250"
         << " contract:"<< contract
         << " \n";

        tx.nTxType        = WASM_CONTRACT_TX;
        tx.txUid          = senderRegID;
        tx.fee_symbol     = fee.symbol;
        tx.llFees         = fee.GetSawiAmount();


        tx.contract       = wasm::name("wasmio").value;
        tx.action         = wasm::name("setcode").value;
        tx.data           = wasm::pack(std::tuple(contract, code, abi, memo));

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

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type)(str_type));

    std::cout << "rpccall wasmcontracttx line321" 
         << " sender:" << params[0].get_str() 
         << " contract:"<< params[1].get_str()
         << " action:"<< params[2].get_str()
         //<< " data:"<< json_spirit::write(params[3].get_obj())
         << " data:"<< params[3].get_str()
         << " amount:"<< params[4].get_str()
         << " fee:"<< params[5].get_str()
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

    if (!pCdMan->pContractCache->HaveContract(contractRegID)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }


    uint64_t contract = wasm::RegID2Name(contractRegID);
    uint64_t action = wasm::name(params[2].get_str()).value;

    string arguments = params[3].get_str();
    //string arguments = json_spirit::write(params[3].get_obj());
    if (arguments.empty() || arguments.size() > MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments empty or the size out of range");
    }

    // std::cout << "callwasmcontracttx ------------------------------------------"
    //               << " contractRegId:" << contractRegID.ToString()
    //               << " \n";

    CUniversalContract contractCode;
    pCdMan->pContractCache->GetContract(contractRegID, contractCode);

    //std::cout << "rpccall wasmcontracttx contractAbi:" << contractCode.code << std::endl;
    std::vector<char> data;

    if( contractCode.abi.size() > 0 )
        try {
            data = wasm::abi_serializer::pack( contractCode.abi, wasm::name(action).to_string(), arguments, max_serialization_time );
            //std::cout << "rpccall wasmcontracttx action data:" << ToHex(data) << std::endl;
        } catch( wasm::CException& e ){

            throw JSONRPCError(e.errCode, e.errMsg);
        }

    else{
        //string params = json_spirit::write(data_v);
        data.insert(data.begin(), arguments.begin(), arguments.end() );
    }

    //std::cout << "rpccall wasmcontracttx action data:" << ToHex(data) << std::endl;

    // wasm::name issuer = wasm::name("walker");
    // wasm::asset maximum_supply = wasm::asset{1000000000, symbol("BTC", 4)};
    // std::vector<char> data = wasm::pack(std::tuple(issuer, maximum_supply));


    ComboMoney amount = RPC_PARAM::GetComboMoney(params[4], SYMB::WICC);
    ComboMoney fee    = RPC_PARAM::GetFee(params, 5, TxType::UCONTRACT_INVOKE_TX);


    uint32_t height     = chainActive.Height();

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
    tx.nTxType      = WASM_CONTRACT_TX;
    tx.txUid        = sendUserId;
    tx.valid_height = height;
    tx.fee_symbol   = fee.symbol;
    tx.llFees       = fee.GetSawiAmount();

    tx.contract     = contract;
    tx.action       = action;
    tx.data         = data;

    tx.symbol       = amount.symbol;
    tx.amount       = amount.GetSawiAmount();

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

Value gettablerowwasmcontracttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
                "gettablerowwasmcontracttx \"sender addr\" \"contract\" \"action\" \"data\" \"amount\" \"fee\" (\"height\")\n"
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
        // 3.table
        // 4.scope
        // 5.number
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    std::cout << "rpccall gettablerowwasmcontracttx "
              << " sender:" << params[0].get_str()
              << " contract:"<< params[1].get_str()
              << " table:"<< params[2].get_str()
              // << " scope:"<< params[3].get_str()
              << " number:"<< params[3].get_str()
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
    uint64_t table = wasm::name(params[2].get_str()).value;
    // uint64_t scope = 0;
    // if (params[3].get_str().size() > 0)
    //     scope = wasm::name(params[3].get_str()).value;

    uint64_t number = 10;//params[3].get_int();

    CUniversalContract contractCode;
    if(!pCdMan->pContractCache->GetContract(contractRegID, contractCode))
        throw JSONRPCError(READ_SCRIPT_FAIL, "can not get contract code");

    string abi = contractCode.abi;
    if (abi.size() == 0)
        throw JSONRPCError(READ_SCRIPT_FAIL, "this contract didn't set abi");

    std::vector<char> k = wasm::pack(std::tuple(contract, table));
    string keyPrefix;
    keyPrefix.insert(keyPrefix.end(), k.begin(), k.end());

    std::cout << "rpccall gettablerowwasmcontracttx "
          << " keyPrefix:" << ToHex(keyPrefix,"")
          << std::endl;

    string lastKey = ""; // TODO: get last key
    auto pGetter = pCdMan->pContractCache->CreateContractDatasGetter(contractRegID, keyPrefix, number, lastKey);
    if (!pGetter || !pGetter->Execute()) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "get contract datas error! contract_regid=%s, ");
    }   

    std::cout << "rpccall gettablerowwasmcontracttx "
      << " pGetter->data_list size:" << pGetter->data_list.size()
      << std::endl; 

    json_spirit::Object object;
    try {

        //std::vector<json_spirit::Value> 
        json_spirit::Array vars;
        for (auto item : pGetter->data_list ){
            const string& value = pGetter->GetValue(item);
                          
            std::vector<char> row;
            row.insert(row.end(), value.begin(), value.end());
            json_spirit::Value v = wasm::abi_serializer::unpack(abi, table, row, max_serialization_time);

            vars.push_back(v);
        }

        object.push_back(Pair("rows", vars));

    } catch (CException&e ){
        throw JSONRPCError(ABI_PARSE_FAIL, e.errMsg );
    }

    return object;


}