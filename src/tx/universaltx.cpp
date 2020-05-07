// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "universaltx.h"

#include "commons/util/util.h"
#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "config/version.h"

#include "wasm/wasm_context.hpp"
#include "wasm/types/name.hpp"
#include "wasm/abi_def.hpp"
#include "wasm/abi_serializer.hpp"
#include "wasm/wasm_variant_trace.hpp"
#include "wasm/exception/exceptions.hpp"
#include "wasm/modules/wasm_native_dispatch.hpp"
#include "wasm/modules/wasm_router.hpp"

#include <sstream>

///////////////////////////////////////////////////////////////////////////////
// class CUniversalTx

map <UnsignedCharArray, uint64_t> &get_signatures_cache() {
    //fixme:this map should be in maxsize to protect memory
    static map <UnsignedCharArray, uint64_t> signatures_cache;
    return signatures_cache;
}

inline void add_signature_to_cache(const UnsignedCharArray& signature, const uint64_t& account) {
    get_signatures_cache()[signature] = account;
}

inline bool get_signature_from_cache(const UnsignedCharArray& signature, uint64_t& account) {

    auto itr = get_signatures_cache().find(signature);
    if (itr != get_signatures_cache().end()) {
        account = itr->second;
        return true;
    }
    return false;

}

void CUniversalTx::pause_billing_timer() {

    if (billed_time > chrono::microseconds(0)) {
        return;// already paused
    }

    auto now    = system_clock::now();
    billed_time = std::chrono::duration_cast<std::chrono::microseconds>(now - pseudo_start);

}

void CUniversalTx::resume_billing_timer() {

    if (billed_time == chrono::microseconds(0)) {
        return;// already release pause
    }
    auto now     = system_clock::now();
    pseudo_start = now - billed_time;
    billed_time  = chrono::microseconds(0);

}

void CUniversalTx::validate_contracts(CTxExecuteContext& context) {

    auto &database = *context.pCw;

    for (auto inline_trx: inline_transactions) {

        auto contract = wasm::name(inline_trx.contract);
        if (is_native_contract(contract.value)) continue;

        CUniversalContractStore contract_store;
        CHAIN_ASSERT( database.contractCache.GetContract(CRegID(contract.value), contract_store),
                      wasm_chain::contract_exception,
                      "cannot get contract with regid '%s'",
                      contract.to_string() )

        CHAIN_ASSERT( contract_store.code.size() > 0 && contract_store.abi.size() > 0,
                      wasm_chain::contract_exception,
                      "contract '%s' abi or code  does not exist",
                      contract.to_string() )

    }

}

void CUniversalTx::validate_authorization(const std::vector<uint64_t>& authorization_accounts) {

    //authorization in each inlinetransaction must be a subset of signatures from transaction
    for (auto i: inline_transactions) {
        for (auto p: i.authorization) {
            auto itr = std::find(authorization_accounts.begin(), authorization_accounts.end(), p.account);
            CHAIN_ASSERT( itr != authorization_accounts.end(),
                          wasm_chain::missing_auth_exception,
                          "authorization %s does not have signature",
                          wasm::name(p.account).to_string())
            // if(p.account != account){
            //     WASM_ASSERT( false,
            //                  account_operation_exception,
            //                  "CUniversalTx.authorization_validation, authorization %s does not have signature",
            //                  wasm::name(p.account).to_string().c_str())
            // }
        }
    }

}

//bool CUniversalTx::validate_payer_signature(CTxExecuteContext &context)

void
CUniversalTx::get_accounts_from_signatures(CCacheWrapper& database, std::vector <uint64_t>& authorization_accounts) {

    TxID signature_hash = GetHash();

    map <UnsignedCharArray, uint64_t> signatures_duplicate_check;

    auto spPayer = sp_tx_account;

    for (auto s : signatures) {
        signatures_duplicate_check[s.signature] = s.account;

        uint64_t authorization_account;
        if (get_signature_from_cache(s.signature, authorization_account)) {
            authorization_accounts.push_back(authorization_account);
            continue;
        }

        CHAIN_ASSERT( spPayer->regid.GetIntValue() != s.account,
                      wasm_chain::tx_duplicate_sig,
                      "duplicate signatures from payer '%s'", spPayer->regid.ToString())

        auto spAccount = GetAccount(database, CRegID(s.account));
        CHAIN_ASSERT( spAccount,
                      wasm_chain::account_access_exception, "%s",
                      "can not get account from regid '%s'", wasm::name(s.account).to_string() )

        CHAIN_ASSERT( spAccount->owner_pubkey.Verify(signature_hash, s.signature),
                      wasm_chain::unsatisfied_authorization,
                      "can not verify signature '%s bye public key '%s' and hash '%s' ",
                      to_hex(s.signature), spAccount->owner_pubkey.ToString(), signature_hash.ToString() )

        authorization_account = wasm::name(s.account).value;
        add_signature_to_cache(s.signature, authorization_account);
        authorization_accounts.push_back(authorization_account);

    }

    CHAIN_ASSERT( signatures_duplicate_check.size() == authorization_accounts.size(),
                  wasm_chain::tx_duplicate_sig,
                  "duplicate signature included")

    //append payer
    authorization_accounts.push_back(spPayer->regid.GetIntValue());

}

bool CUniversalTx::CheckTx(CTxExecuteContext& context) {
    auto &database           = *context.pCw;
    auto &check_tx_to_return = *context.pState;

    try {
        CHAIN_ASSERT( signatures.size() <= max_signatures_size, //only one signature from payer , the signatures must be 0
                      wasm_chain::sig_variable_size_limit_exception,
                      "signatures size must be <= %s", max_signatures_size)

        CHAIN_ASSERT( inline_transactions.size() > 0 && inline_transactions.size() <= max_inline_transactions_size,
                      wasm_chain::inline_transaction_size_exceeds_exception,
                      "inline_transactions size must be <= %s", max_inline_transactions_size)

        //IMPLEMENT_CHECK_TX_REGID(txUid.type());
        validate_contracts(context);

        std::vector <uint64_t> authorization_accounts;
        get_accounts_from_signatures(database, authorization_accounts);
        validate_authorization(authorization_accounts);

        //validate payer
        // CAccount payer;
        // CHAIN_ASSERT( database.accountCache.GetAccount(txUid, payer), wasm_chain::account_access_exception,
        //               "get payer failed, txUid '%s'", txUid.ToString())
        // CHAIN_ASSERT( payer.HasOwnerPubKey(), wasm_chain::account_access_exception,
        //               "payer '%s' unregistered", payer.regid.ToString())
        // CHAIN_ASSERT( find(authorization_accounts.begin(), authorization_accounts.end(),
        //                    wasm::name(payer.regid.ToString()).value) != authorization_accounts.end(),
        //               wasm_chain::missing_auth_exception,
        //               "can not find the signature by payer %s",
        //               payer.regid.ToString())

    } catch (wasm_chain::exception &e) {
        return check_tx_to_return.DoS(100, ERRORMSG(e.what()), e.code(), e.to_detail_string());
    }

    return true;
}

static uint64_t get_min_fee_in_wicc(CBaseTx& tx, CTxExecuteContext& context) {

    uint64_t min_fee;
    CHAIN_ASSERT(GetTxMinFee(*context.pCw, tx.nTxType, context.height, tx.fee_symbol, min_fee), wasm_chain::fee_exhausted_exception, "get minFee failed")

    return min_fee;
}

static uint64_t get_run_fee_in_wicc(const uint64_t& fuel, CBaseTx& tx, CTxExecuteContext& context) {

    uint64_t fuel_rate = context.fuel_rate;
    CHAIN_ASSERT(fuel_rate           >  0, wasm_chain::fee_exhausted_exception, "%s", "fuel_rate cannot be 0")
    CHAIN_ASSERT(MAX_BLOCK_FUEL  >= fuel, wasm_chain::fee_exhausted_exception, "fuel '%ld' > max block fuel '%ld'", fuel, MAX_BLOCK_FUEL)

    return fuel / 100 * fuel_rate;
}


// static void inline_trace_to_receipts(const wasm::inline_transaction_trace& trace,
//                                      vector<CReceipt>&                     receipts,
//                                      map<transfer_data_t,  uint64_t>&   receipts_duplicate_check) {

//     if (trace.trx.contract == wasmio_bank && trace.trx.action == wasm::N(transfer)) {

//         CReceipt receipt;
//         receipt.code = TRANSFER_ACTUAL_COINS;

//         transfer_data_t transfer_data = wasm::unpack < std::tuple < uint64_t, uint64_t, wasm::asset, string>> (trace.trx.data);
//         auto from                        = std::get<0>(transfer_data);
//         auto to                          = std::get<1>(transfer_data);
//         auto quantity                    = std::get<2>(transfer_data);
//         auto memo                        = std::get<3>(transfer_data);

//         auto itr = receipts_duplicate_check.find(std::tuple(from, to ,quantity, memo));
//         if (itr == receipts_duplicate_check.end()){
//             receipts_duplicate_check[std::tuple(from, to ,quantity, memo)] = wasmio_bank;

//             receipt.from_uid    = CUserID(CRegID(from));
//             receipt.to_uid      = CUserID(CRegID(to));
//             receipt.coin_symbol = quantity.symbol.code().to_string();
//             receipt.coin_amount = quantity.amount;

//             receipts.push_back(receipt);
//         }
//     }

//     for (auto t: trace.inline_traces) {
//         inline_trace_to_receipts(t, receipts, receipts_duplicate_check);
//     }

// }

// static void trace_to_receipts(const wasm::transaction_trace& trace, vector<CReceipt>& receipts) {
//     map<transfer_data_t, uint64_t > receipts_duplicate_check;
//     for (auto t: trace.traces) {
//         inline_trace_to_receipts(t, receipts, receipts_duplicate_check);
//     }
// }

bool CUniversalTx::ExecuteTx(CTxExecuteContext &context) {

    auto& database             = *context.pCw;
    auto& execute_tx_to_return = *context.pState;
    context_type               = context.context_type;
    pending_block_time         = context.block_time;

    wasm::inline_transaction* trx_current_for_exception = nullptr;

    try {
        if (context_type == TxExecuteContextType::PRODUCE_BLOCK ||
            context_type == TxExecuteContextType::VALIDATE_MEMPOOL) {
            max_transaction_duration = std::chrono::milliseconds(wasm::max_wasm_execute_time_mining);
        }

        recipients_size        = 0;
        pseudo_start           = system_clock::now();//pseudo start for reduce code loading duration
        run_cost               = GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_fee_per_byte;

        wasm::transaction_trace trx_trace;
        trx_trace.trx_id = GetHash();

        for (auto& inline_trx : inline_transactions) {
            trx_current_for_exception = &inline_trx;

            trx_trace.traces.emplace_back();
            execute_inline_transaction(trx_trace.traces.back(), inline_trx, inline_trx.contract, database, receipts, 0);

            trx_current_for_exception = nullptr;
        }
        trx_trace.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - pseudo_start);

        CHAIN_ASSERT( trx_trace.elapsed.count() < max_transaction_duration.count() * 1000,
                      wasm_chain::tx_cpu_usage_exceeded,
                      "Tx execution time must be in '%d' microseconds, but get '%d' microseconds",
                      max_transaction_duration * 1000, trx_trace.elapsed.count())

        //bytes add margin
        run_cost      = run_cost + recipients_size * notice_fuel_fee_per_recipient;

        auto min_fee  = get_min_fee_in_wicc(*this, context) ;
        auto run_fee  = get_run_fee_in_wicc(run_cost, *this, context);

        auto minimum_tx_execute_fee = std::max<uint64_t>( min_fee, run_fee);

        CHAIN_ASSERT( minimum_tx_execute_fee <= llFees, wasm_chain::fee_exhausted_exception,
                      "tx.llFees '%ld' is not enough to charge minimum tx execute fee '%ld' , fuel_rate:%ld",
                      llFees, minimum_tx_execute_fee, context.fuel_rate);

        trx_trace.fuel_rate               = context.fuel_rate;
        trx_trace.minimum_tx_execute_fee  = minimum_tx_execute_fee;

        //save trx trace
        std::vector<char> trace_bytes = wasm::pack<transaction_trace>(trx_trace);
        CHAIN_ASSERT( database.contractCache.SetContractTraces(GetHash(),
                                                             std::string(trace_bytes.begin(), trace_bytes.end())),
                      wasm_chain::account_access_exception,
                      "set tx '%s' trace failed",
                      GetHash().ToString())

        //set fuel for block fuel sum
        fuel = run_cost;

        auto resolver = make_resolver(database);

        json_spirit::Value value_json;
        to_variant(trx_trace, value_json, resolver);
        string string_return = json_spirit::write(value_json);

        //execute_tx_to_return.SetReturn(GetHash().ToString());
        execute_tx_to_return.SetReturn(string_return);
    } catch (wasm_chain::exception &e) {

        string trx_current_str("inline_tx:");
        if( trx_current_for_exception != nullptr ){
            //fixme:should check the action data can be unserialize
            Value trx;
            to_variant(*trx_current_for_exception, trx);
            trx_current_str = json_spirit::write(trx);
        }
        CHAIN_EXCEPTION_APPEND_LOG( e, log_level::warn, "%s", trx_current_str)
        return execute_tx_to_return.DoS(100, ERRORMSG(e.what()), e.code(), e.to_detail_string());
    }

    return true;
}

void CUniversalTx::execute_inline_transaction(wasm::inline_transaction_trace& trace,
                                                 wasm::inline_transaction&       trx,
                                                 uint64_t                        receiver,
                                                 CCacheWrapper&                  database,
                                                 vector <CReceipt>&              receipts,
                                                 uint32_t                        recurse_depth) {

    wasm_context wasm_execute_context(*this, trx, database, receipts, mining, recurse_depth);

    //check timeout
    CHAIN_ASSERT( std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - pseudo_start) <
                  get_max_transaction_duration() * 1000,
                  wasm_chain::wasm_timeout_exception, "%s", "timeout");

    wasm_execute_context._receiver = receiver;
    wasm_execute_context.execute(trace);

}


bool CUniversalTx::GetInvolvedKeyIds(CCacheWrapper &cw, set <CKeyID> &keyIds) {

    CKeyID senderKeyId;
    if (!cw.accountCache.GetKeyId(txUid, senderKeyId))
        return false;

    keyIds.insert(senderKeyId);
    return true;
}

uint64_t CUniversalTx::GetFuelFee(CCacheWrapper &cw, int32_t height, uint32_t nFuelRate) {

    uint64_t minFee = 0;
    if (!GetTxMinFee(cw, nTxType, height, fee_symbol, minFee)) {
        LogPrint(BCLog::ERROR, "CUniversalTx::GetFuelFee(), get min_fee failed! fee_symbol=%s\n", fee_symbol);
        throw runtime_error("CUniversalTx::GetFuelFee(), get min_fee failed");
    }

    return std::max<uint64_t>(((fuel / 100.0f) * nFuelRate), minFee);
}

string CUniversalTx::ToString(CAccountDBCache &accountCache) {

    if (inline_transactions.size() == 0) return string("");
    inline_transaction trx = inline_transactions[0];

    CAccount payer;
    if (!accountCache.GetAccount(txUid, payer)) {
        return string("");
    }

    return strprintf(
            "txType=%s, hash=%s, ver=%d, payer=%s, llFees=%llu, contract=%s, action=%s, arguments=%s, "
            "valid_height=%d",
            GetTxType(nTxType), GetHash().ToString(), nVersion, payer.regid.ToString(), llFees,
            wasm::name(trx.contract).to_string(), wasm::name(trx.action).to_string(),
            HexStr(trx.data), valid_height);
}

Object CUniversalTx::ToJson(const CAccountDBCache &accountCache) const {

    if (inline_transactions.size() == 0) return Object{};

    CAccount payer;
    accountCache.GetAccount(txUid, payer);

    Object result;
    result.push_back(Pair("txid",             GetHash().GetHex()));
    result.push_back(Pair("tx_type",          GetTxType(nTxType)));
    result.push_back(Pair("ver",              nVersion));
    result.push_back(Pair("payer",            payer.regid.ToString()));
    result.push_back(Pair("payer_addr",       payer.keyid.ToAddress()));
    result.push_back(Pair("fee_symbol",       fee_symbol));
    result.push_back(Pair("fees",             llFees));
    result.push_back(Pair("valid_height",     valid_height));

    Value inline_transactions_arr;
    to_variant(inline_transactions, inline_transactions_arr);
    result.push_back(Pair("inline_transactions", inline_transactions_arr));

    Value signature_payer;
    to_variant(signature_pair{payer.regid.GetIntValue(), signature}, signature_payer);
    result.push_back(Pair("signature_payer", signature_payer));

    if (signatures.size() > 0) {
        Value signatures_arr;
        to_variant(signatures, signatures_arr);
        result.push_back(Pair("signature_pairs", signatures_arr));
    }

    return result;
}

void CUniversalTx::set_signature(const uint64_t& account, const vector<uint8_t>& signature) {
    for( auto& s:signatures ){
        if( s.account == account ){
            s.signature = signature;
            return;
        }
    }
    CHAIN_ASSERT(false, wasm_chain::missing_auth_exception, "cannot find account %s in signature list", wasm::name(account).to_string());
}

void CUniversalTx::set_signature(const wasm::signature_pair& signature) {
    set_signature(signature.account, signature.signature);
}
