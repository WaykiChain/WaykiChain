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
#include "tx/contracttx.h"
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


static void load_contract(CContractDBCache*        db_contract,
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

//submitluacontractcalltx
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
    proposal.param_values.push_back(std::make_pair(type, CVarIntValue<uint64_t>(val)));

    std::shared_ptr<CProposalRequestTx> pBaseTx = std::make_shared<CProposalRequestTx>();
    pBaseTx->txUid        = sendUserId;
    pBaseTx->llFees       = fee.GetAmountInSawi();
    pBaseTx->fee_symbol   = fee.symbol;
    pBaseTx->valid_height = height;
    pBaseTx->proposal = CProposalStorageBean(std::make_shared<CGovSysParamProposal>(proposal));
    return pBaseTx;
}


std::shared_ptr<CBaseTx> genWasmContractCallTx(json_spirit::Value param_json) {
    try {
        const Value& str_from = JSON::GetObjectFieldValue(param_json, "sender");
        auto authorizer_name   = wasm::regid(str_from.get_str());
        const Value& str_fee = JSON::GetObjectFieldValue(param_json, "fee");
        const Value& str_height = JSON::GetObjectFieldValue(param_json, "height");
        ComboMoney fee = RPC_PARAM::GetComboMoney(str_fee,  SYMB::WICC);
        int32_t height = AmountToRawValue(str_height);

        auto db_contract = pCdMan->pContractCache;

        std::shared_ptr<CUniversalTx> pBaseTx = std::make_shared<CUniversalTx>();

        pBaseTx->nTxType      = UNIVERSAL_TX;
        pBaseTx->txUid        = CRegID(authorizer_name.value);
        pBaseTx->valid_height = height;
        pBaseTx->fee_symbol   = fee.symbol;
        pBaseTx->llFees       = fee.GetAmountInSawi();
        Array json_transactions = JSON::GetObjectFieldValue(param_json, "transactions").get_array();
        for (auto& json_transaction : json_transactions) {
            const Value& str_contract   = JSON::GetObjectFieldValue(json_transaction, "contract");
            const Value& str_action     = JSON::GetObjectFieldValue(json_transaction, "action");
            const Value& str_data       = JSON::GetObjectFieldValue(json_transaction, "data");

        // std::vector<char> abi;

        /*   wasm::regid       contract_regid = wasm::regid(params[1].get_str());
            if (!get_native_contract_abi(contract_regid.value, abi)) {
                CUniversalContractStore contract_store;
                load_contract(db_contract, contract_regid, contract_store);
                abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
            }
        */
            std::vector<char> abi;
            wasm::regid contract_regid = wasm::regid(str_contract.get_str());
            if (!wasm::get_native_contract_abi(contract_regid.value, abi)) {
                CUniversalContractStore contract_store;
                load_contract(db_contract, contract_regid, contract_store);
                abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
            }

            auto action = wasm::name(str_action.get_str());

            std::vector<char> action_data = wasm::abi_serializer::pack(
                    abi,
                    action.to_string(),
                    str_data,
                    wasm::max_serialization_time);

            auto auth = authorizer_name;
            Value str_auth = null_type;

            if (JSON::GetObjectFieldValue(json_transaction, "auth", str_auth)) {    
                auth = wasm::regid(str_auth.get_str());
            }

            pBaseTx->inline_transactions.push_back({
                    contract_regid.value,
                    action.value,
                    std::vector<wasm::permission>{{auth.value, wasm::wasmio_owner}},
                    action_data
                });

            if ( !str_auth.is_null()) {
                pBaseTx->signatures.push_back({auth.value, {}});
            } 
        }

        return pBaseTx;
    } JSON_RPC_CAPTURE_AND_RETHROW;
}

std::shared_ptr<CBaseTx> genDelegateVotetx(json_spirit::Value param_json) {
    const Value& str_from = JSON::GetObjectFieldValue(param_json, "sender");
    const Value& str_fee = JSON::GetObjectFieldValue(param_json, "fee");
    const Value& str_height = JSON::GetObjectFieldValue(param_json, "height");

    CUserID sendUserId = RPC_PARAM::GetUserId(str_from, true);
    ComboMoney fee   = RPC_PARAM::GetComboMoney(str_fee,  SYMB::WICC);
    int32_t height = AmountToRawValue(str_height);
    CGovSysParamProposal proposal;

    std::shared_ptr<CDelegateVoteTx> delegateVoteTx = std::make_shared<CDelegateVoteTx>();
    delegateVoteTx->txUid        = sendUserId;
    delegateVoteTx->llFees       = fee.GetAmountInSawi();
    delegateVoteTx->valid_height = height;
    Array arr_votes = JSON::GetObjectFieldValue(param_json, "votelist").get_array();

    for (auto objVote : arr_votes) {
        const Value& delegateAddr  = JSON::GetObjectFieldValue(objVote, "delegate");
        const Value& delegateVotes = JSON::GetObjectFieldValue(objVote, "votes");
        if (delegateAddr.type() == null_type || delegateVotes == null_type) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Vote fund address error or fund value error");
        }
        auto delegateUid = RPC_PARAM::ParseUserIdByAddr(delegateAddr);
        CAccount delegateAcct;
        if (!pCdMan->pAccountCache->GetAccount(delegateUid, delegateAcct)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address does not exist");
        }
        if (!delegateAcct.HasOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Delegate address is unregistered");
        }

        VoteType voteType    = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN : VoteType::MINUS_BCOIN;
        CUserID candidateUid = CUserID(delegateAcct.regid);
        uint64_t bcoins      = (uint64_t)abs(delegateVotes.get_int64());

        CCandidateVote candidateVote(voteType, candidateUid, bcoins);
        delegateVoteTx->candidateVotes.push_back(candidateVote);
    }

    return delegateVoteTx;

}

const char *gen_rawtx_rpc_help_message = R"=====(
    genunsignedtxraw "func" "params"
    get the serialization json format  parameters
    Arguments:
    1."func":           (string required) the func need serilize
    2."params":         (string required), the json format param of func
    Result:
    "raw":            (string)
    Examples:
    > ./coind genunsignedtxraw func param
    As json rpc call
    > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"gettxserilize", "params":["]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
)=====";


unordered_map <string, std::shared_ptr<CBaseTx> (*)(json_spirit::Value)> nameToFuncMap = {
    { "submitsendtx",                &genSendTx                 },
    { "submitaccountpermscleartx",   &genAccountPermsClearTx    },
    { "submitucontractcalltx",       &genContractCalltx         },
    { "submitwasmcontractcalltx",    &genWasmContractCallTx     },
    { "submitdelegatevotetx",    &genDelegateVotetx     }
};

Value GetGenunsignedArgs(const Value &jsonValue) {
    auto newValue = jsonValue;
    if (newValue.type() == json_spirit::str_type){
        json_spirit::read_string(jsonValue.get_str(), newValue);
    }

    if (newValue.type() == json_spirit::obj_type){
        return newValue;
    }

    throw JSONRPCError(RPC_INVALID_PARAMETER, "The genunsignedtxraw args type must be object,"
                                              " or string of object");
}

/**
 * Example Usage:
 *
 * coind genunsignedtxraw submitsendtx '{"from":"0-1","to":"wcnDdWdWTSe8qv6wyoQknT8xCf4R5ahiK2", "amount":"WICC:10000:SAWI", "fee":"WICC:10000:SAWI", "height":100, "memo":"tesst"}'
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
Value genunsignedtxraw(const Array &params, bool fHelp) {

    if (fHelp || params.size() != 2) {
        throw runtime_error(gen_rawtx_rpc_help_message);
    }

    string func = params[0].get_str();

    auto argsIn = GetGenunsignedArgs(params[1]);
    std::shared_ptr<CBaseTx> pBaseTx = nameToFuncMap[func](argsIn);

    Object obj;
    obj.push_back(Pair("txid", pBaseTx->GetHash().GetHex()));
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << pBaseTx;
    obj.push_back(Pair("unsigned_txraw", HexStr(ds.begin(), ds.end())));
    return obj;

}
