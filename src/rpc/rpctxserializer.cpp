// Copyright (c) 2019 Joss
// Copyright (c) 2017-2019 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include <stdint.h>
#include <chrono>
#include <boost/assign/list_of.hpp>

#include "commons/base58.h"
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_reader.h"
#include "commons/json/json_spirit_writer.h"
#include "rpc/rpcapi.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "tx/tx.h"
#include "tx/accountpermscleartx.h"
#include "tx/proposaltx.h"
#include "tx/wasmcontracttx.h"
#include "init.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "config/configuration.h"
#include "miner/miner.h"
#include "main.h"
#include "net.h"
#include "netbase.h"
#include "wasm/datastream.hpp"
#include "wasm/abi_serializer.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/modules/wasm_native_dispatch.hpp"
#include "wasm/wasm_context.hpp"
#include "wasm/types/inline_transaction.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/exception/exceptions.hpp"

using namespace std;
using namespace boost;
using namespace json_spirit;
using namespace boost::assign;
using std::chrono::microseconds;

std::shared_ptr<CBaseTx> genSendTx(json_spirit::Value param_json) {

    const Value& str_from = JSON::GetObjectFieldValue(param_json, "sender");
    const Value& str_to = JSON::GetObjectFieldValue(param_json, "to");
    const Value& str_amount = JSON::GetObjectFieldValue(param_json, "amount");
    const Value& str_fee = JSON::GetObjectFieldValue(param_json, "fee");
    const Value& memo = JSON::GetObjectFieldValue(param_json, "memo");
    const Value& str_height = JSON::GetObjectFieldValue(param_json, "height");

    CUserID sendUserId = RPC_PARAM::GetUserId(str_from, true);
    CUserID recvUserId = RPC_PARAM::GetUserId(str_to);
    ComboMoney cmCoin  = RPC_PARAM::GetComboMoney(str_amount, SYMB::WICC);
    ComboMoney cmFee   = RPC_PARAM::GetComboMoney(str_fee,  SYMB::WICC);
    int32_t height = AmountToRawValue(str_height);

    std::shared_ptr<CBaseTx> pBaseTx = std::make_shared<CCoinTransferTx>(sendUserId, recvUserId, height, cmCoin.symbol,
                                                    cmCoin.GetAmountInSawi(), cmFee.symbol, cmFee.GetAmountInSawi(), memo.get_str());
    return pBaseTx;
}

//submitcontractcalltx
/**
{
   "addr": "",
   "fee": "",
   “height”: ""
}*/
std::shared_ptr<CBaseTx> genAccountPermsClearTx(json_spirit::Value param_json) {

    const Value& str_from = JSON::GetObjectFieldValue(param_json, "sender");
    const Value& str_fee = JSON::GetObjectFieldValue(param_json, "fee");
    const Value& str_height = JSON::GetObjectFieldValue(param_json, "height");

    CUserID sendUserId = RPC_PARAM::GetUserId(str_from, true);
    ComboMoney fee = RPC_PARAM::GetComboMoney(str_fee, SYMB::WICC);
    int32_t height = AmountToRawValue(str_height);

    std::shared_ptr<CBaseTx> pBaseTx = std::make_shared<CAccountPermsClearTx>(sendUserId, height, fee.symbol, fee.GetAmountInSawi());
    return pBaseTx;
}

/****
 *
 *
 * *
 */
std::shared_ptr<CBaseTx> genContractCalltx(json_spirit::Value param_json) {

    const Value& str_from = JSON::GetObjectFieldValue(param_json, "sender");
    const Value& str_contract_regid = JSON::GetObjectFieldValue(param_json, "contractRegid");
    const Value& str_args = JSON::GetObjectFieldValue(param_json, "args");
    const Value& str_amount = JSON::GetObjectFieldValue(param_json, "amount");
    const Value& str_fee = JSON::GetObjectFieldValue(param_json, "fee");
    const Value& str_height = JSON::GetObjectFieldValue(param_json, "height");

    CUserID sendUserId = RPC_PARAM::GetUserId(str_from, true);
    CUserID contractRegId = RPC_PARAM::GetUserId(str_contract_regid);
    ComboMoney amount = RPC_PARAM::GetComboMoney(str_amount,  SYMB::WICC);
    ComboMoney fee = RPC_PARAM::GetComboMoney(str_fee,  SYMB::WICC);
    int32_t height = AmountToRawValue(str_height);


    std::shared_ptr<CUniversalContractInvokeTx> pBaseTx = std::make_shared<CUniversalContractInvokeTx>();
    pBaseTx->nTxType      = UCONTRACT_INVOKE_TX;
    pBaseTx->txUid        = sendUserId;
    pBaseTx->app_uid      = contractRegId;
    pBaseTx->coin_symbol  = amount.symbol;
    pBaseTx->coin_amount  = amount.GetAmountInSawi();
    pBaseTx->fee_symbol   = fee.symbol;
    pBaseTx->llFees       = fee.GetAmountInSawi();
    pBaseTx->arguments    = str_args.get_str();
    pBaseTx->valid_height = height;
    return pBaseTx;
}

/**
 *
 *
 *
 * */
std::shared_ptr<CBaseTx> genParamGovernProposal(json_spirit::Value param_json) {

    const Value& str_from = JSON::GetObjectFieldValue(param_json, "sender");
    const Value& str_key = JSON::GetObjectFieldValue(param_json, "key");
    const Value& str_val = JSON::GetObjectFieldValue(param_json, "value");
    const Value& str_fee = JSON::GetObjectFieldValue(param_json, "fee");
    const Value& str_height = JSON::GetObjectFieldValue(param_json, "height");

    CUserID sendUserId = RPC_PARAM::GetUserId(str_from, true);
    ComboMoney fee   = RPC_PARAM::GetComboMoney(str_fee,  SYMB::WICC);
    int32_t height = AmountToRawValue(str_height);
    uint64_t val = str_val.get_uint64();
    CGovSysParamProposal proposal;

    SysParamType type = GetSysParamType(str_key.get_str());
    if(type == SysParamType::NULL_SYS_PARAM_TYPE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("system param type(%s) is not exist",str_key.get_str()));

    string errorInfo = CheckSysParamValue(type, val);
    if(errorInfo != EMPTY_STRING)
        throw JSONRPCError(RPC_INVALID_PARAMETER, errorInfo);
    proposal.param_values.push_back(std::make_pair(type, val));

    std::shared_ptr<CProposalRequestTx> pBaseTx = std::make_shared<CProposalRequestTx>();
    pBaseTx->txUid        = sendUserId;
    pBaseTx->llFees       = fee.GetAmountInSawi();
    pBaseTx->fee_symbol   = fee.symbol;
    pBaseTx->valid_height = height;
    pBaseTx->proposal = CProposalStorageBean(std::make_shared<CGovSysParamProposal>(proposal));
    return pBaseTx;
}


std::shared_ptr<CBaseTx> genWasmContractCalltx(json_spirit::Value param_json) {
    const Value& str_from = JSON::GetObjectFieldValue(param_json, "sender");
    auto authorizer_name   = wasm::name(str_from.get_str());
    const Value& str_fee = JSON::GetObjectFieldValue(param_json, "fee");
    const Value& str_height = JSON::GetObjectFieldValue(param_json, "height");
    ComboMoney fee = RPC_PARAM::GetComboMoney(str_fee,  SYMB::WICC);
    int32_t height = AmountToRawValue(str_height);
    
    auto database_contract = pCdMan->pContractCache;

    std::shared_ptr<CWasmContractTx> pBaseTx = std::make_shared<CWasmContractTx>();

    pBaseTx->nTxType      = WASM_CONTRACT_TX;
    pBaseTx->txUid        = CRegID(authorizer_name.value);
    pBaseTx->valid_height = height;
    pBaseTx->fee_symbol   = fee.symbol;
    pBaseTx->llFees       = fee.GetAmountInSawi();

    Array json_transactions = JSON::GetObjectFieldValue(param_json, "transactions").get_array();
    for (auto json_transaction: json_transactions) {
        const Value& str_contract = JSON::GetObjectFieldValue(json_transaction, "contract");
        const Value& str_action = JSON::GetObjectFieldValue(json_transaction, "action");
        const Value& str_data= JSON::GetObjectFieldValue(json_transaction, "data");

        std::vector<char> abi;
        wasm::name contract_name = wasm::name(str_contract.get_str());
        if(!wasm::get_native_contract_abi(contract_name.value, abi)){
            CAccount           contract;
            CUniversalContract contract_store;
            database_contract->GetContract(contract.regid, contract_store);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }
        auto action = wasm::name(str_action.get_str());

        std::vector<char> action_data = wasm::abi_serializer::pack(
                abi,
                action.to_string(),
                str_data,
                wasm::max_serialization_time);

        pBaseTx->inline_transactions.push_back({
                contract_name.value,
                action.value,
                std::vector<wasm::permission>{{authorizer_name.value, wasm::wasmio_owner}},
                action_data
            });
    }

    Value json_signs;
    if(JSON::GetObjectFieldValue(param_json, "signs", json_signs)) {
        Array sign_arr = json_signs.get_array();
        for (auto sign: sign_arr) {
            const Value& str_auth = JSON::GetObjectFieldValue(sign, "auth");
            auto auth = wasm::name(str_auth.get_str());
            const Value& str_sign = JSON::GetObjectFieldValue(sign, "sign");
            std::vector<uint8_t> signature(str_sign.get_str().begin(), str_sign.get_str().end());
            pBaseTx->signatures.push_back({auth.value, signature});
        }
    }
    return pBaseTx;
}

const char *gen_rawtx_rpc_help_message = R"=====(
    gettxserilize "func" "params"
    get the serialization json format  parameters
    Arguments:
    1."func":           (string required) the func need serilize
    2."params":         (string required), the json format param of func
    Result:
    "raw":            (string)
    Examples:
    > ./coind gettxserilize func param
    As json rpc call
    > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"gettxserilize", "params":["]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
)=====";


unordered_map <string, std::shared_ptr<CBaseTx> (*)(json_spirit::Value)> nameToFuncMap = {
    { "submitsendtx",                &genSendTx                 },
    { "submitaccountpermscleartx",   &genAccountPermsClearTx    },
    { "submitucontractcalltx",       &genContractCalltx         },
    { "submitwasmcontractcalltx",    &genWasmContractCalltx     }
};

/**
 * Example Usage:
 *
 * coind genrawtx submitsendtx '{"from":"0-1","to":"wcnDdWdWTSe8qv6wyoQknT8xCf4R5ahiK2", "amount":"WICC:10000:SAWI", "fee":"WICC:10000:SAWI", "height":100, "memo":"tesst"}'
 *
 * {
   "send": "",
   "to": "",
   "amount": "" ,
   "fee": "",
   "memo": "",
   “height”: ""
 * }
 *
*/
Value genrawtx(const Array &params, bool fHelp) {

    if (fHelp || params.size() != 2) {
        throw runtime_error(gen_rawtx_rpc_help_message);
    }

    RPCTypeCheck(params, list_of(str_type)(str_type));

    string func = params[0].get_str();
    string param = params[1].get_str();
    json_spirit::Value param_json;
    json_spirit::read_string(param, param_json);
    std::shared_ptr<CBaseTx> pBaseTx = nameToFuncMap[func](param_json);

    Object obj;
    obj.push_back(Pair("txid", pBaseTx->GetHash().GetHex()));
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << pBaseTx;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;

}
