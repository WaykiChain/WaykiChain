// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wasmcontracttx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "config/version.h"
#include <sstream>

#include "wasm/wasm_context.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/types/name.hpp"
#include "wasm/abi_def.hpp"
#include "wasm/wasm_config.hpp"
#include "wasm/abi_serializer.hpp"
#include "wasm/wasm_native_contract_abi.hpp"

//static CCacheWrapper *g_pCW = nullptr;//= std::make_shared<CCacheWrapper>(mempool.cw.get());

static inline void to_variant( const wasm::permission &t, json_spirit::Value &v ) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(wasm::name(t.account), val);
    json_spirit::Config::add(obj, "account", val);

    to_variant(wasm::name(t.perm), val);
    json_spirit::Config::add(obj, "permission", val);

    v = obj;
}


static inline void to_variant( const wasm::inline_transaction &t, json_spirit::Value &v , CCacheWrapper &database) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(wasm::name(t.contract), val);
    json_spirit::Config::add(obj, "contract", val);

    to_variant(wasm::name(t.action), val);
    json_spirit::Config::add(obj, "action", val);

    json_spirit::Array arr;
    for (const auto &auth :t.authorization) {
        json_spirit::Value tmp;
        to_variant(auth, tmp);
        arr.push_back(tmp);
    }
    json_spirit::Config::add(obj, "authorization", json_spirit::Value(arr));

    std::vector<char> abi;
    if(t.contract == wasm::wasmio){
        wasm::abi_def wasmio_abi = wasmio_contract_abi();
        abi = wasm::pack<wasm::abi_def>(wasmio_abi);
    } else {
        //should be lock
        CUniversalContract contract;

        CAccount contract_account;
        if(database.accountCache.GetAccount(CNickID(wasm::name(t.contract).to_string()), contract_account)
                    && database.contractCache.GetContract(contract_account.regid, contract))
            abi.insert(abi.end(), contract.abi.begin(), contract.abi.end());
    }

    if (abi.size() > 0 && t.action != wasm::N(setcode)) {
        if(t.data.size() > 0){
            try{
                val = wasm::abi_serializer::unpack(abi, wasm::name(t.action).to_string(), t.data,
                                               max_serialization_time);
            } catch (...) {
                to_variant(ToHex(t.data,""), val);
            }
        }
    } else
        to_variant(ToHex(t.data,""), val);

    json_spirit::Config::add(obj, "data", val);

    v = obj;
}


static inline void to_variant( const wasm::inline_transaction_trace &t, json_spirit::Value &v, CCacheWrapper &database) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(t.trx_id.ToString(), val);
    json_spirit::Config::add(obj, "trx_id", val);

    // to_variant(t.elapsed.count(), val);
    // json_spirit::Config::add(obj, "elapsed", val);

    to_variant(wasm::name(t.receiver), val);
    json_spirit::Config::add(obj, "receiver", val);

    to_variant(t.trx, val, database);
    json_spirit::Config::add(obj, "trx", val);

    to_variant(t.console, val);
    json_spirit::Config::add(obj, "console", val);

    if (t.inline_traces.size() > 0) {
        json_spirit::Array arr;
        for (const auto &trace :t.inline_traces) {
            json_spirit::Value tmp;
            to_variant(trace, tmp, database);
            arr.push_back(tmp);
        }

        json_spirit::Config::add(obj, "inline_traces", json_spirit::Value(arr));

    }

    v = obj;

}


static inline void to_variant( const wasm::transaction_trace &t, json_spirit::Value &v, CCacheWrapper &database ) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(t.trx_id.ToString(), val);
    json_spirit::Config::add(obj, "trx_id", val);

    to_variant(t.elapsed.count(), val);
    json_spirit::Config::add(obj, "elapsed", val);

    if (t.traces.size() > 0) {
        json_spirit::Array arr;
        for (const auto &trace :t.traces) {
            json_spirit::Value tmp;
            to_variant(trace, tmp, database);
            arr.push_back(tmp);
        }

        json_spirit::Config::add(obj, "traces", json_spirit::Value(arr));
    }

    v = obj;
}

void CWasmContractTx::pause_billing_timer(){

  if(billed_time > chrono::microseconds(0)){
      return;// already paused
  }

  auto now = system_clock::now();
  billed_time = std::chrono::duration_cast<std::chrono::microseconds>(now - pseudo_start);

}

void CWasmContractTx::resume_billing_timer(){

  if(billed_time > chrono::microseconds(0)){
       return;// already paused
  }
  auto now = system_clock::now();
  pseudo_start = now - billed_time;

  billed_time = chrono::microseconds(0);

}

bool CWasmContractTx::CheckTx(CTxExecuteContext &context) {
    // IMPLEMENT_CHECK_TX_FEE;
    // IMPLEMENT_CHECK_TX_REGID(txUid.type());

    // if (!contract.IsValid()) {
    //     return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, contract is invalid"),
    //                      REJECT_INVALID, "vmscript-invalid");
    // }

    // uint64_t llFuel = GetFuel(GetFuelRate(cw.contractCache));
    // if (llFees < llFuel) {
    //     return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, fee too litter to afford fuel "
    //                      "(actual:%lld vs need:%lld)", llFees, llFuel),
    //                      REJECT_INVALID, "fee-too-litter-to-afford-fuel");
    // }

    // // If valid height range changed little enough(i.e. 3 blocks), remove it.
    // if (GetFeatureForkVersion(height) == MAJOR_VER_R2) {
    //     unsigned int nTxSize = ::GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    //     double dFeePerKb     = double(llFees - llFuel) / (double(nTxSize) / 1000.0);
    //     if (dFeePerKb < CBaseTx::nMinRelayTxFee) {
    //         return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, fee too litter in fees/Kb "
    //                          "(actual:%.4f vs need:%lld)", dFeePerKb, CBaseTx::nMinRelayTxFee),
    //                          REJECT_INVALID, "fee-too-litter-in-fees/Kb");
    //     }
    // }

    // CAccount account;
    // if (!cw.accountCache.GetAccount(txUid, account)) {
    //     return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, get account failed"),
    //                      REJECT_INVALID, "bad-getaccount");
    // }
    // if (!account.HaveOwnerPubKey()) {
    //     return state.DoS(
    //         100, ERRORMSG("CWasmContractTx::CheckTx, account unregistered"),
    //         REJECT_INVALID, "bad-account-unregistered");
    // }

    // IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);

    return true;
}

bool CWasmContractTx::ExecuteTx(CTxExecuteContext &context) {

    try {

        auto &database         = *context.pCw;
        auto execute_tx_return = context.pState;

        pseudo_start = system_clock::now();

        wasm::transaction_trace trx_trace;
        trx_trace.trx_id = GetHash();
        //trx_trace.block_height = nHeight;
        //trx_trace.block_time =

        for (auto trx: inlinetransactions) {
            trx_trace.traces.emplace_back();
            DispatchInlineTransaction(trx_trace.traces.back(), trx, trx.contract, database, 0);
        }
        trx_trace.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - pseudo_start);        

        json_spirit::Value v;
        to_variant(trx_trace, v, database);

        execute_tx_return->SetReturn(json_spirit::write(v));

    } catch (wasm::exception &e) {
        return context.pState->DoS(100, ERRORMSG(e.detail()), e.code(), e.detail());
    }

    //cache.save(trace)
    return true;
}

void CWasmContractTx::DispatchInlineTransaction( wasm::inline_transaction_trace &trace,
                                                 wasm::inline_transaction &trx,
                                                 uint64_t receiver,
                                                 CCacheWrapper &database,
                                                 //CValidationState &state,
                                                 uint32_t recurse_depth ) {

    wasm_context ctx(*this, trx, database, recurse_depth);
    ctx._receiver = receiver;
    ctx.execute(trace);

}


bool CWasmContractTx::GetInvolvedKeyIds( CCacheWrapper &cw, set <CKeyID> &keyIds ) {
    CKeyID senderKeyId;
    if (!cw.accountCache.GetKeyId(txUid, senderKeyId))
        return false;

    keyIds.insert(senderKeyId);
    return true;
}

uint64_t CWasmContractTx::GetFuel( uint32_t nFuelRate ) {
    return std::max(uint64_t((nRunStep / 100.0f) * nFuelRate), 1 * COIN);
}


string CWasmContractTx::ToString( CAccountDBCache &accountCache ) {
    CKeyID senderKeyId;
    accountCache.GetKeyId(txUid, senderKeyId);

    // return strprintf("hash=%s, txType=%s, version=%d, contract=%s, senderuserid=%s, senderkeyid=%s, fee=%ld, valid_height=%d\n",
    //                  GetTxType(nTxType), GetHash().ToString(), nVersion, contract, txUid.ToString(), senderKeyId.GetHex(), llFees,
    //                  valid_height);

    return strprintf("hash=%s, txType=%s, version=%d, senderuserid=%s, senderkeyid=%s, fee=%ld, valid_height=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), senderKeyId.GetHex(), llFees,
                     valid_height);
}

Object CWasmContractTx::ToJson( const CAccountDBCache &accountCache ) const {
    Object object;

    CKeyID senderKeyId;
    accountCache.GetKeyId(txUid, senderKeyId);

    object.push_back(Pair("hash", GetHash().GetHex()));
    object.push_back(Pair("tx_type", GetTxType(nTxType)));
    object.push_back(Pair("version", nVersion));
    // object.push_back(Pair("contract", contract));
    // object.push_back(Pair("action", wasm::name(action).to_string()));
    // object.push_back(Pair("data", HexStr(data)));
    //object.push_back(Pair("regId", txUid.get<CRegID>().ToString()));
    //object.push_back(Pair("addr", senderKeyId.ToAddress()));
    //object.push_back(Pair("script", "script_content"));
    object.push_back(Pair("sender", senderKeyId.GetHex()));

    object.push_back(Pair("fees", llFees));
    object.push_back(Pair("valid_height", valid_height));
    return object;
}

