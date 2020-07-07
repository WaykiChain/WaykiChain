// Copyright (c) 2019 xiaoyu
// Copyright (c) 2017-2019 WaykiChain Developers
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
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_reader.h"
#include "commons/json/json_spirit_writer.h"

#include "wasm/datastream.hpp"
#include "wasm/abi_serializer.hpp"
#include "wasm/wasm_context.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/types/regid.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/wasm_context.hpp"
#include "wasm/wasm_rpc_message.hpp"
#include "wasm/wasm_variant_trace.hpp"
#include "wasm/exception/exceptions.hpp"
#include "wasm/wasm_control_rpc.hpp"

using namespace std;
using namespace boost;
using namespace json_spirit;
using namespace boost::assign;
using std::chrono::microseconds;

Array GetKeyPrefixArray(const Value &jsonValue) {
    auto valueType = jsonValue.type();

    if (valueType == json_spirit::array_type) {
        return jsonValue.get_array();
    } else if (valueType == json_spirit::str_type) {
        json_spirit::Value newObj;
        try {
            json_spirit::read_string_or_throw(jsonValue.get_str(), newObj);
        } catch (json_spirit::Error_position &e) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Parse json of key prefix array error! line[%d:%d] %s",
                    e.line_, e.column_, e.reason_));
        }
        auto newObjType = newObj.type();
        if (newObjType == json_spirit::obj_type || newObjType == json_spirit::array_type) {
            return newObj.get_array();
        }
    }
    throw JSONRPCError(RPC_INVALID_PARAMETER, "The contract action args type must be object or array,"
                                              " or string of object or array");
}

Array GetKeyPrefixArray(const Array &params, uint32_t index) {
    return (params.size() > index) ? GetKeyPrefixArray(params[index]) : Array();
}

void read_file_limit(const string& path, string& data, uint64_t max_size){

    CHAIN_ASSERT( path.size() > 0, wasm_chain::file_read_exception, "file name is missing")

    char byte;
    ifstream f(path, ios::binary);
    CHAIN_ASSERT( f.is_open() , wasm_chain::file_not_found_exception, "file '%s' not found, it must be file name with full path", path)

    streampos pos = f.tellg();
    f.seekg(0, ios::end);
    size_t size = f.tellg();

    CHAIN_ASSERT( size != 0,        wasm_chain::file_read_exception, "file is empty")
    CHAIN_ASSERT( size <= max_size, wasm_chain::file_read_exception,
                  "file is larger than max limited '%d' bytes", MAX_CONTRACT_CODE_SIZE)
    //if (size == 0 || size > max_size) return false;
    f.seekg(pos);
    while (f.get(byte)) data.push_back(byte);
}

//TODO: to be extended for LuaVM
void read_and_validate_code(const string& path, string& code, VMType &type) {
    read_file_limit(path, code, MAX_WASM_CONTRACT_CODE_BYTES);

    vector <uint8_t> c;
    c.insert(c.begin(), code.begin(), code.end());
    wasm_interface wasmif;
    wasmif.validate(c);

    type = VMType::WASM_VM;
}

void read_and_validate_abi(const string& abi_file, string& abi){

    read_file_limit(abi_file, abi, MAX_WASM_CONTRACT_ABI_BYTES);
    json_spirit::Value abi_json;
    json_spirit::read_string(abi, abi_json);

    abi_def abi_struct;
    from_variant(abi_json, abi_struct);
    wasm::abi_serializer abis(abi_struct, max_serialization_time);//validate in abi_serializer constructor

    std::vector<char> abi_bytes = wasm::pack<wasm::abi_def>(abi_struct);
    abi                         = string(abi_bytes.begin(), abi_bytes.end());

}

void load_contract(CContractDBCache*        db_contract,
                   const wasm::regid&       contract_regid,
                   CUniversalContractStore& contract_store ){

    CHAIN_ASSERT( db_contract->GetContract(CRegID(contract_regid.value), contract_store),
                  wasm_chain::contract_exception,
                  "cannot get contract '%s'",
                  contract_regid.to_string())

    CHAIN_ASSERT( contract_store.vm_type == VMType::WASM_VM,
                  wasm_chain::vm_type_mismatch, "vm type must be wasm VM")

    CHAIN_ASSERT( contract_store.abi.size() > 0,
                  wasm_chain::abi_not_found_exception, "contract abi not found")
    //JSON_RPC_ASSERT(ucontract.code.size() > 0,                                 RPC_WALLET_ERROR,  "contract lose code")
}


// set code and abi to wasm/lua contract
Value submitsetcodetx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 3 || params.size() > 5, wasm::rpc::submit_setcode_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type));

    try{
        auto database = pCdMan->pAccountCache;
        auto wallet   = pWalletMain;
        CHAIN_ASSERT( wallet != NULL, wasm_chain::wallet_not_available_exception, "wallet error" )
        EnsureWalletIsUnlocked();

        CUniversalTx tx;
        {
            string code, abi;
            VMType vm;
            auto payer_regid        = RPC_PARAM::ParseRegId(params[0], "sender");
            read_and_validate_code(          params[1].get_str(), code, vm);
            read_and_validate_abi (          params[2].get_str(), abi);
            CRegID contract_regid = RPC_PARAM::ParseRegId(params, 3, "contract_regid",  CRegID()/*default*/);
            auto fee                = RPC_PARAM::GetFee(params, 4, TxType::UNIVERSAL_TX);

            CAccount payer_account;
            CHAIN_ASSERT( database->GetAccount(payer_regid, payer_account),
                          wasm_chain::account_access_exception,
                          "payer '%s' does not exist ",
                          payer_regid.ToString())

            RPC_PARAM::CheckAccountBalance(payer_account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());
            auto payer      = wasm::regid(payer_regid.GetIntValue());
            auto contract   = wasm::regid(contract_regid.GetIntValue());
            tx.nTxType      = UNIVERSAL_TX;
            tx.txUid        = CUserID(payer_regid);
            tx.fee_symbol   = fee.symbol;
            tx.llFees       = fee.GetAmountInSawi();
            tx.valid_height = chainActive.Tip()->height;
            string memo = ""; // empty memo
            std::vector<permission> permissions = std::vector<permission>{{payer.value, wasmio_owner}};
            auto data = wasm::pack(std::make_tuple(contract, payer, (uint8_t)vm, code, abi, memo));
            tx.inline_transactions.push_back({wasmio, wasm::N(setcode), permissions, data});

            CHAIN_ASSERT( wallet->Sign(payer_account.keyid, tx.GetHash(), tx.signature),
                          wasm_chain::wallet_sign_exception, "wallet sign error")
        }

        CValidationState state;
        bool fSuccess = wallet->CommitTx((CBaseTx * ) & tx, state);
        JSON_RPC_ASSERT(fSuccess, RPC_WALLET_ERROR, state.GetRejectReason());

        Object obj_return;
        // json_spirit::Config::add(obj_return, "txid", std::get<1>(ret) );
        // return obj_return;

        // Object obj_return;
        // Value  v_trx_id, value_json;
        // json_spirit::read(retMsg, value_json);

        // //if (value_json.type() == json_spirit::obj_type) {
        // auto o = value_json.get_obj();
        // for (json_spirit::Object::const_iterator iter = o.begin(); iter != o.end(); ++iter) {
        //     string name = Config_type::get_name(*iter);
        //     if (name == "trx_id") {
        //         v_trx_id = Config_type::get_value(*iter);
        //         break;
        //     }
        // }
        //}

        obj_return.push_back(Pair("txid", tx.GetHash().ToString()));
        // TODO: get trace
        return obj_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;
}

// set a new maintainer to a wasm/lua contract
Value submitsetcodertx( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 3 || params.size() > 4, wasm::rpc::submit_setcoder_tx_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type));

    const ComboMoney& fee         = RPC_PARAM::GetFee(params, 4, TxType::UNIVERSAL_TX);
     try{
        auto database = pCdMan->pAccountCache;
        auto wallet   = pWalletMain;
        CHAIN_ASSERT( wallet != NULL, wasm_chain::wallet_not_available_exception, "wallet error" )
        EnsureWalletIsUnlocked();

        CUniversalTx tx;
        {
            CAccount payer;
            auto payer_regid        = RPC_PARAM::ParseRegId(params[0], "sender");
            auto contract           = RPC_PARAM::ParseRegId(params[1], "contract");
            auto maintainer         = RPC_PARAM::ParseRegId(params[2], "maintainer");
            const ComboMoney& fee   = RPC_PARAM::GetFee(params, 3, TxType::UNIVERSAL_TX);

            CHAIN_ASSERT( database->GetAccount(payer_regid, payer),
                          wasm_chain::account_access_exception,
                          "payer '%s' does not exist ",
                          payer_regid.ToString())

            RPC_PARAM::CheckAccountBalance(payer, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

            tx.nTxType      = UNIVERSAL_TX;
            tx.txUid        = payer.regid;
            tx.fee_symbol   = fee.symbol;
            tx.llFees       = fee.GetAmountInSawi();
            tx.valid_height = chainActive.Tip()->height;
            std::vector<permission> permissions = {{payer_regid.GetIntValue(), wasmio_owner}};
            auto data = wasm::pack(std::tuple<wasm::regid, wasm::regid>(contract.GetIntValue(),
                                                                        maintainer.GetIntValue()));
            tx.inline_transactions.push_back({wasmio, wasm::N(setcoder), permissions, data});

            CHAIN_ASSERT( wallet->Sign(payer.keyid, tx.GetHash(), tx.signature),
                          wasm_chain::wallet_sign_exception, "wallet sign error")
        }

        CValidationState state;
        bool fSuccess = wallet->CommitTx((CBaseTx * ) & tx, state);
        JSON_RPC_ASSERT(fSuccess, RPC_WALLET_ERROR, state.GetRejectReason());

        Object obj_return;
        // json_spirit::Config::add(obj_return, "txid", std::get<1>(ret) );
        // return obj_return;

        // Object obj_return;
        // Value  v_trx_id, value_json;
        // json_spirit::read(retMsg, value_json);

        // //if (value_json.type() == json_spirit::obj_type) {
        // auto o = value_json.get_obj();
        // for (json_spirit::Object::const_iterator iter = o.begin(); iter != o.end(); ++iter) {
        //     string name = Config_type::get_name(*iter);
        //     if (name == "trx_id") {
        //         v_trx_id = Config_type::get_value(*iter);
        //         break;
        //     }
        // }
        // //}

        obj_return.push_back(Pair("txid", tx.GetHash().ToString()));
        return obj_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value submittx( const Array &params, bool fHelp ) {

    //WASM_TRACE(params[1].get_str())
    RESPONSE_RPC_HELP( fHelp || params.size() < 4 || params.size() > 5 , wasm::rpc::submit_tx_rpc_help_message)

    try {
        auto db_account  = pCdMan->pAccountCache;
        auto db_contract = pCdMan->pContractCache;
        auto wallet      = pWalletMain;

        CHAIN_ASSERT( wallet != NULL, wasm_chain::wallet_not_available_exception, "wallet error" )
        EnsureWalletIsUnlocked();

        //get abi
        std::vector<char> abi;
        auto contract_regid        = RPC_PARAM::ParseRegId(params[1], "contract");
        auto contract = wasm::regid(contract_regid.GetIntValue());
        if (!get_native_contract_abi(contract_regid.GetIntValue(), abi)) {
            CUniversalContractStore contract_store;
            load_contract(db_contract, contract, contract_store);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        CUniversalTx tx;
        {
            CAccount payer;
            auto payer_regid     = RPC_PARAM::ParseRegId(params[0], "sender");
            auto     action      = wasm::name(params[2].get_str());
            CHAIN_ASSERT(db_account->GetAccount(payer_regid, payer), wasm_chain::account_access_exception,
                        "payer '%s' does not exist",payer_regid.ToString())

            auto argsIn = RPC_PARAM::GetWasmContractArgs(params[3]);

            CHAIN_ASSERT( abi.size() > 0,
                          wasm_chain::inline_transaction_data_size_exceeds_exception, "did not get abi")

            std::vector<char> action_data = wasm::abi_serializer::pack(abi, action.to_string(), argsIn, max_serialization_time);
            CHAIN_ASSERT( action_data.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                          wasm_chain::inline_transaction_data_size_exceeds_exception,
                          "inline transaction args is out of size(%u vs %u)",
                          action_data.size(), MAX_CONTRACT_ARGUMENT_SIZE)

            ComboMoney fee  = RPC_PARAM::GetFee(params, 4, TxType::UNIVERSAL_TX);

            tx.nTxType      = UNIVERSAL_TX;
            tx.txUid        = payer.regid;
            tx.valid_height = chainActive.Height();
            tx.fee_symbol   = fee.symbol;
            tx.llFees       = fee.GetAmountInSawi();

            //for(int i = 0; i < 300; i++)
            std::vector<permission> permissions = {{payer_regid.GetIntValue(), wasmio_owner}};
            tx.inline_transactions.push_back({contract.value, action.value, permissions, action_data});

            //tx.signatures.push_back({payer_regid.value, vector<uint8_t>()});
            CHAIN_ASSERT( wallet->Sign(payer.keyid, tx.GetHash(), tx.signature),
                          wasm_chain::wallet_sign_exception, "wallet sign error")
            //tx.set_signature({payer_regid.value, tx.signature});
        }

        CValidationState state(SysCfg().IsTxTrace());
        bool fSuccess = wallet->CommitTx((CBaseTx * ) & tx, state);
        JSON_RPC_ASSERT(fSuccess, RPC_WALLET_ERROR, state.GetRejectReason()) //fixme: could get exception from committx

        Object obj_return;
        // Value  value_json;
        // json_spirit::read(retMsg, value_json);
        // json_spirit::Config::add(obj_return, "result", value_json );
        obj_return.push_back(Pair("txid", tx.GetHash().ToString()));
        obj_return.push_back(Pair("tx_trace", *state.GetTrace()));
        return obj_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value wasm_gettable( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::get_table_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type));

    try{
        auto db_contract    = pCdMan->pContractCache;
        auto contract_regid = RPC_PARAM::ParseRegId(params[0], "contract");
        auto contract_table = wasm::name(params[1].get_str());
        auto key_prefix_arr = GetKeyPrefixArray(params, 2);
        uint64_t numbers = (params.size() > 3) ? std::atoi(params[3].get_str().data()) : default_query_rows;
        string start_key = (params.size() > 4) ? from_hex(params[4].get_str()) : "";

        // JSON_RPC_ASSERT(!is_native_contract(contract_regid.value), RPC_INVALID_PARAMS,
        //                 "cannot get table from native contract '%s'", contract_regid.to_string())
        auto contract = wasm::regid(contract_regid.GetIntValue());
        CHAIN_ASSERT( !is_native_contract(contract.value), wasm_chain::native_contract_access_exception,
                    "cannot get table from native contract '%s'", contract.to_string() )

        CUniversalContractStore contract_store;
        load_contract(db_contract, contract, contract_store);
        std::vector<char> abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());

        std::vector<char> key_prefix = wasm::pack(std::tuple<uint64_t, uint64_t>(contract.value, contract_table.value));
        string search_key(key_prefix.data(),key_prefix.size());
        if (!key_prefix_arr.empty()) {
            auto keys = wasm::abi_serializer::pack_keys(abi, key_prefix_arr, max_serialization_time);
            search_key.append(keys.begin(), keys.end());
        }

        auto pContractDataIt = db_contract->CreateContractDataIterator(contract_regid, search_key);
        // JSON_RPC_ASSERT(pContractDataIt, RPC_INVALID_PARAMS,
        //                 "cannot get table from contract '%s'", contract_regid.to_string())
        CHAIN_ASSERT( pContractDataIt, wasm_chain::table_not_found,
                      "cannot get table '%s' from contract '%s'", contract_table.to_string(), contract_regid.ToString() )

        bool                hasMore = false;
        json_spirit::Object object_return;
        json_spirit::Array  row_json;
        if (start_key.empty()) {
            pContractDataIt->First();
        } else {
            pContractDataIt->SeekUpper(&start_key);
        }
        for (; pContractDataIt->IsValid(); pContractDataIt->Next()) {
            if (pContractDataIt->GotCount() > numbers) {
                hasMore = true;
                break;
            }
            const string& key   = pContractDataIt->GetContractKey();
            const string& value = pContractDataIt->GetValue();

            //unpack value in bytes to json
            std::vector<char> value_bytes(value.begin(), value.end());
            json_spirit::Value   value_json  = wasm::abi_serializer::unpack(abi, contract_table.value, value_bytes, max_serialization_time);
            json_spirit::Object& object_json = value_json.get_obj();

            //append key and value
            object_json.push_back(Pair("key",   to_hex(key, "")));
            //object_json.push_back(Pair("value", to_hex(value, "")));

            row_json.push_back(value_json);
        }

        object_return.push_back(Pair("rows", row_json));
        object_return.push_back(Pair("more", hasMore));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}


Value wasm_getrow( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 3 , wasm::rpc::get_row_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    try{
        auto db_contract    = pCdMan->pContractCache;
        auto contract_regid = RPC_PARAM::ParseRegId(params[0], "contract");
        auto contract_table = wasm::name(params[1].get_str());
        auto key = RPC_PARAM::GetBinStrFromHex(params[2], "key");

        auto contract = wasm::regid(contract_regid.GetIntValue());
        CHAIN_ASSERT( !is_native_contract(contract.value), wasm_chain::native_contract_access_exception,
                    "cannot get table from native contract '%s'", contract.to_string() )

        CUniversalContractStore contract_store;
        load_contract(db_contract, contract, contract_store);
        std::vector<char> abi (contract_store.abi.begin(), contract_store.abi.end());
        // std::vector<char> key_prefix = wasm::pack(std::tuple<uint64_t, uint64_t>(contract.value, contract_table.value));
        // string data_key = string(key_prefix.data(),key_prefix.size()) + key;
        string value;
        CHAIN_ASSERT(db_contract->GetContractData(contract_regid, key, value), wasm_chain::table_not_found,
                      "the key '%s' of table '%s' of contract '%s' does not exist",
                      HexStr(key), contract_table.to_string(), contract_regid.ToString())

        std::vector<char> value_bytes(value.begin(), value.end());
        json_spirit::Value   value_json  = wasm::abi_serializer::unpack(abi, contract_table.value, value_bytes, max_serialization_time);
        return value_json.get_obj();
    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value wasm_json2bin( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::json_to_bin_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    try{
        auto db_contract        = pCdMan->pContractCache;
        auto contract_regid = RPC_PARAM::ParseRegId(params[0], "contract");
        auto contract_action    = wasm::name(params[1].get_str());

        auto contract = wasm::regid(contract_regid.GetIntValue());
        std::vector<char>  abi;
        CUniversalContractStore contract_store;
        if(!get_native_contract_abi(contract.value, abi)){
            load_contract(db_contract, contract, contract_store);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        string arguments = params[2].get_str();
        CHAIN_ASSERT( !arguments.empty() && arguments.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                      wasm_chain::rpc_params_size_exceeds_exception,
                      "arguments is empty or out of size")
        std::vector<char> action_data(arguments.begin(), arguments.end() );
        if( abi.size() > 0 ) action_data = wasm::abi_serializer::pack(abi, contract_action.to_string(), arguments, max_serialization_time);

        json_spirit::Object object_return;
        object_return.push_back(Pair("data", wasm::to_hex(action_data,"")));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}


Value wasm_bin2json( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() < 2 || params.size() > 4 , wasm::rpc::bin_to_json_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    try{
        auto db_contract        = pCdMan->pContractCache;
        auto contract_regid = RPC_PARAM::ParseRegId(params[0], "contract");
        auto contract_action    = wasm::name(params[1].get_str());

        auto contract = wasm::regid(contract_regid.GetIntValue());
        std::vector<char>  abi;
        CUniversalContractStore contract_store;
        if(!get_native_contract_abi(contract.value, abi)){
            load_contract(db_contract, contract, contract_store);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        string arguments = from_hex(params[2].get_str());
        CHAIN_ASSERT( !arguments.empty() && arguments.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                      wasm_chain::rpc_params_size_exceeds_exception,
                      "arguments is empty or out of size")

        json_spirit::Object object_return;
        std::vector<char>   action_data(arguments.begin(), arguments.end() );
        json_spirit::Value  value = wasm::abi_serializer::unpack(abi, contract_action.to_string(), action_data, max_serialization_time);
        object_return.push_back(Pair("data", value));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value wasm_getcode( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 1 , wasm::rpc::get_code_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type));

    try{
        auto db_contract       = pCdMan->pContractCache;
        auto contract_regid = RPC_PARAM::ParseRegId(params[0], "contract");
        // JSON_RPC_ASSERT(!is_native_contract(contract_regid.value),
        //                 RPC_INVALID_PARAMS,
        //                 "cannot get code from native contract '%s'", contract_regid.to_string())
        auto contract = wasm::regid(contract_regid.GetIntValue());
        CHAIN_ASSERT( !is_native_contract(contract.value), wasm_chain::native_contract_access_exception,
                      "cannot get code from native contract '%s'", contract.to_string() )

        CUniversalContractStore contract_store;
        load_contract(db_contract, contract, contract_store);

        json_spirit::Object object_return;
        object_return.push_back(Pair("code", wasm::to_hex(contract_store.code, "")));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value wasm_getabi( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 1 , wasm::rpc::get_abi_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type));

    try{
        auto db_contract    = pCdMan->pContractCache;
        auto contract_regid = RPC_PARAM::ParseRegId(params[0], "contract");

        auto contract = wasm::regid(contract_regid.GetIntValue());
        vector<char>       abi;
        CUniversalContractStore contract_store;
        if(!get_native_contract_abi(contract.value, abi)){
            load_contract(db_contract, contract, contract_store);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        json_spirit::Object object_return;
        json_spirit::Value  abi_json;
        abi_def             abi_struct = wasm::unpack<wasm::abi_def>(abi);
        wasm::to_variant(abi_struct, abi_json);
        object_return.push_back(Pair("abi", abi_json));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value wasm_gettxtrace( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 1 , wasm::rpc::get_tx_trace_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type));

    try{
        auto database  = std::make_shared<CCacheWrapper>(pCdMan);
        auto resolver  = make_resolver(*database);
        auto trx_id    = uint256S(params[0].get_str());

        string  trace_string;
        CHAIN_ASSERT( database->contractCache.GetContractTraces(trx_id, trace_string),
                      wasm_chain::transaction_trace_access_exception,
                      "get tx '%s' trace failed",
                      trx_id.ToString())

        json_spirit::Object object_return;
        json_spirit::Value  value_json;
        std::vector<char>   trace_bytes = std::vector<char>(trace_string.begin(), trace_string.end());
        transaction_trace   trace       = wasm::unpack<transaction_trace>(trace_bytes);

        to_variant(trace, value_json, resolver);
        object_return.push_back(Pair("tx_trace", value_json));

        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value wasm_abidefjson2bin( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 1 , wasm::rpc::abi_def_json_to_bin_wasm_rpc_help_message)
    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type));

    try{
         string json = params[0].get_str();
        //string json;
        //read_file_limit (params[0].get_str(),  json, MAX_CONTRACT_CODE_SIZE);

        json_spirit::Value abi_json;
        json_spirit::read_string(json, abi_json);

        abi_def abi_struct;
        from_variant(abi_json, abi_struct);
        wasm::abi_serializer abis(abi_struct, max_serialization_time);//validate in abi_serializer constructor

        std::vector<char> abi_bytes = wasm::pack<wasm::abi_def>(abi_struct);

        json_spirit::Object object_return;
        object_return.push_back(Pair("data", wasm::to_hex(abi_bytes,"")));
        return object_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

Value wasm_getresult( const Array &params, bool fHelp ) {

    RESPONSE_RPC_HELP( fHelp || params.size() != 3 , wasm::rpc::get_result_wasm_rpc_help_message)

    try {
        //auto db_account  = pCdMan->pAccountCache;
        auto db = CCacheWrapper(pCdMan);

        //get abi
        std::vector<char> abi;
        auto contract_regid        = RPC_PARAM::ParseRegId(params[0], "contract");
        auto action                   = wasm::name(params[1].get_str());
        auto args                     = RPC_PARAM::GetWasmContractArgs(params[2]);

        auto contract              = wasm::regid(contract_regid.GetIntValue());
        if (!get_native_contract_abi(contract_regid.GetIntValue(), abi)) {
            CUniversalContractStore contract_store;
            load_contract(&db.contractCache, contract, contract_store);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }

        std::vector<char> action_data = wasm::abi_serializer::pack(abi, action.to_string(), args, max_serialization_time);
        CHAIN_ASSERT( action_data.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                      wasm_chain::inline_transaction_data_size_exceeds_exception,
                      "inline transaction args is out of size(%u vs %u)",
                      action_data.size(), MAX_CONTRACT_ARGUMENT_SIZE)

        auto tx = inline_transaction{contract.value, action.value, {{}}, action_data};

        wasm_control_rpc ctrl(db);
        ctrl.call_inline_transaction(tx);

        const auto &ret_value = ctrl.ret_value;
        CHAIN_ASSERT( !ret_value.type.empty() && !ret_value.value.empty(),
                      wasm_chain::rpc_no_ret_exception,
                      "contract action=%s does not emit result", action.to_string())

        auto value_json = wasm::abi_serializer::unpack_data(abi, ret_value.type, ret_value.value, max_serialization_time);
        Object result;
        result.push_back(Pair("name", ret_value.name));
        result.push_back(Pair("type", ret_value.type));
        result.push_back(Pair("value", value_json));

        auto resolver = make_resolver(db);
        json_spirit::Value trace_json;
        to_variant(ctrl.trx_trace, trace_json, resolver);

        Object obj_return;
        obj_return.push_back(Pair("block_height", chainActive.Height()));
        obj_return.push_back(Pair("result", result));
        obj_return.push_back(Pair("trace", trace_json));

        return obj_return;

    } JSON_RPC_CAPTURE_AND_RETHROW;

}

