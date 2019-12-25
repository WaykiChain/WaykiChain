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
#include "wasm/wasm_native_contract.hpp"
#include "wasm/wasm_variant_trace.hpp"


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

void CWasmContractTx::pause_billing_timer() {

    if (billed_time > chrono::microseconds(0)) {
        return;// already paused
    }

    auto now    = system_clock::now();
    billed_time = std::chrono::duration_cast<std::chrono::microseconds>(now - pseudo_start);

}

void CWasmContractTx::resume_billing_timer() {

    if (billed_time == chrono::microseconds(0)) {
        return;// already release pause
    }
    auto now     = system_clock::now();
    pseudo_start = now - billed_time;
    billed_time  = chrono::microseconds(0);

}

void CWasmContractTx::validate_contracts(CTxExecuteContext& context) {

    auto &database = *context.pCw;

    for (auto i: inline_transactions) {

        wasm::name contract_name = wasm::name(i.contract);
        //wasm::name contract_action   = wasm::name(i.action);
        if (is_native_contract(contract_name.value)) continue;

        CAccount contract;
        WASM_ASSERT(database.accountCache.GetAccount(nick_name(contract_name.to_string()), contract),
                    account_operation_exception,
                    "CWasmContractTx.contract_validation, contract account does not exist, contract = %s",
                    contract_name.to_string().c_str())

        CUniversalContract contract_store;
        WASM_ASSERT(database.contractCache.GetContract(contract.regid, contract_store),
                    account_operation_exception,
                    "CWasmContractTx.contract_validation, cannot get contract with nick name = %s",
                    contract_name.to_string().c_str())
        WASM_ASSERT(contract_store.code.size() > 0 && contract_store.abi.size() > 0,
                    account_operation_exception,
                    "CWasmContractTx.contract_validation, %s contract abi or code  does not exist",
                    contract_name.to_string().c_str())

    }

}

void CWasmContractTx::validate_authorization(const std::vector<uint64_t>& authorization_accounts) {

    //authorization in each inlinetransaction must be a subset of signatures from transaction
    for (auto i: inline_transactions) {
        for (auto p: i.authorization) {
            auto itr = std::find(authorization_accounts.begin(), authorization_accounts.end(), p.account);
            WASM_ASSERT(itr != authorization_accounts.end(),
                        account_operation_exception,
                        "CWasmContractTx.authorization_validation, authorization %s does not have signature",
                        wasm::name(p.account).to_string().c_str())
            // if(p.account != account){
            //     WASM_ASSERT( false,
            //                  account_operation_exception,
            //                  "CWasmContractTx.authorization_validation, authorization %s does not have signature",
            //                  wasm::name(p.account).to_string().c_str())
            // }
        }
    }

}

//bool CWasmContractTx::validate_payer_signature(CTxExecuteContext &context)

void
CWasmContractTx::get_accounts_from_signatures(CCacheWrapper& database, std::vector <uint64_t>& authorization_accounts) {

    TxID signature_hash = GetHash();

    map <UnsignedCharArray, uint64_t> signatures_duplicate_check;

    for (auto s:signatures) {
        signatures_duplicate_check[s.signature] = s.account;

        uint64_t authorization_account;
        if (get_signature_from_cache(s.signature, authorization_account)) {
            authorization_accounts.push_back(authorization_account);
            continue;
        }

        CAccount account;
        WASM_ASSERT(database.accountCache.GetAccount(nick_name(wasm::name(s.account).to_string()), account),
                    account_operation_exception, "%s",
                    "CWasmContractTx.get_accounts_from_signature, can not get account from public key")        
        WASM_ASSERT(account.owner_pubkey.Verify(signature_hash, s.signature),
                    account_operation_exception,
                    "%s",
                    "CWasmContractTx::get_accounts_from_signature, can not get public key from signature")

        authorization_account = wasm::name(account.nickid.ToString()).value;
        add_signature_to_cache(s.signature, authorization_account);
        authorization_accounts.push_back(authorization_account);

    }

    WASM_ASSERT(signatures_duplicate_check.size() == authorization_accounts.size(),
                account_operation_exception,
                "%s",
                "CWasmContractTx::get_accounts_from_signature, duplicate signatures")

}

bool CWasmContractTx::CheckTx(CTxExecuteContext& context) {

    try {
        auto &database = *context.pCw;
        auto &state    = *context.pState;

        WASM_ASSERT(signatures.size() > 0 && signatures.size() <= max_signatures_size, account_operation_exception, "%s",
                    "CWasmContractTx.CheckTx, Signatures size must be <= %s", max_signatures_size)

        WASM_ASSERT(inline_transactions.size() > 0 && inline_transactions.size() <= max_inline_transactions_size, account_operation_exception, 
                    "CWasmContractTx.CheckTx, Inline_transactions size must be <= %s", max_inline_transactions_size)

        IMPLEMENT_CHECK_TX_REGID(txUid.type());
        validate_contracts(context);

        std::vector <uint64_t> authorization_accounts;
        get_accounts_from_signatures(database, authorization_accounts);
        validate_authorization(authorization_accounts);

        //validate payer
        CAccount payer;
        WASM_ASSERT(database.accountCache.GetAccount(txUid, payer), account_operation_exception, "%s",
                    "CWasmContractTx.CheckTx, get payer failed")
        WASM_ASSERT(payer.HaveOwnerPubKey(), account_operation_exception, "%s",
                    "CWasmContractTx.CheckTx, payer unregistered")
        WASM_ASSERT(find(authorization_accounts.begin(), authorization_accounts.end(),
                         wasm::name(payer.nickid.ToString()).value) != authorization_accounts.end(),
                    account_operation_exception,
                    "CWasmContractTx.CheckTx, can not find the signature by payer %s",
                    payer.nickid.ToString().c_str())

    } catch (wasm::exception &e) {

        WASM_TRACE("%s", e.detail())
        return context.pState->DoS(100, ERRORMSG(e.detail()), e.code(), e.detail());
    }

    return true;
}

static uint64_t get_fuel_limit(CBaseTx& tx, CTxExecuteContext& context) {

    uint64_t fuel_rate    = context.fuel_rate;
    WASM_ASSERT(fuel_rate > 0, fuel_fee_exception, "%s", "get_fuel_limit, fuel_rate cannot be 0")

    uint64_t min_fee;
    WASM_ASSERT(GetTxMinFee(tx.nTxType, context.height, tx.fee_symbol, min_fee), fuel_fee_exception, "%s", "get_fuel_limit, get minFee failed")
    WASM_ASSERT(tx.llFees >= min_fee, fuel_fee_exception, "get_fuel_limit, fee must >= min fee '%ld', but get '%ld'", min_fee, tx.llFees)

    uint64_t fee_for_miner = min_fee * CONTRACT_CALL_RESERVED_FEES_RATIO / 100;
    uint64_t fee_for_gas   = tx.llFees - fee_for_miner;
    uint64_t fuel_limit    = std::min<uint64_t>(fee_for_gas / fuel_rate / 10 , MAX_BLOCK_RUN_STEP);//1.2 WICC
    WASM_ASSERT(fuel_limit > 0, fuel_fee_exception, "%s", "get_fuel_limit, fuel limit equal 0")

    if(context.transaction_status == wasm::transaction_status_type::validating){
    WASM_TRACE("fee_for_gas:%ld", fee_for_gas)
    WASM_TRACE("MAX_BLOCK_RUN_STEP:%ld", MAX_BLOCK_RUN_STEP)
    }

    return fuel_limit;
}

static void inline_trace_to_receipts(const wasm::inline_transaction_trace& trace, vector<CReceipt>& receipts) {

    if (trace.trx.contract == wasmio_bank && trace.trx.action == wasm::N(transfer)) {
        CReceipt receipt;
        receipt.code = TRANSFER_ACTUAL_COINS;

        std::tuple < uint64_t, uint64_t, wasm::asset, string > transfer_data 
                      = wasm::unpack < std::tuple < uint64_t, uint64_t, wasm::asset, string>> (trace.trx.data);
        auto from     = std::get<0>(transfer_data);
        auto to       = std::get<1>(transfer_data);
        auto quantity = std::get<2>(transfer_data);
        auto memo     = std::get<3>(transfer_data);

        receipt.from_uid    = CUserID(CNickID(wasm::name(from).to_string()));
        receipt.to_uid      = CUserID(CNickID(wasm::name(to).to_string()));
        receipt.coin_symbol = quantity.sym.code().to_string();
        receipt.coin_amount = quantity.amount;

        receipts.push_back(receipt);
    }

    for (auto t: trace.inline_traces) {
        inline_trace_to_receipts(t, receipts);
    }

}

static void trace_to_receipts(const wasm::transaction_trace& trace, vector<CReceipt>& receipts) {
    for (auto t: trace.traces) {
        inline_trace_to_receipts(t, receipts);
    }
}

bool CWasmContractTx::ExecuteTx(CTxExecuteContext &context) {

    try {
        auto &database         = *context.pCw;
        auto execute_tx_return = context.pState;
        transaction_status     = context.transaction_status;

        if(transaction_status == wasm::transaction_status_type::syncing ||
           transaction_status == wasm::transaction_status_type::validating ){
            max_transaction_duration = std::chrono::milliseconds(wasm::max_wasm_execute_time_mining);
        }

        //charger fee
        CAccount payer;
        WASM_ASSERT(database.accountCache.GetAccount(txUid, payer),
                    account_operation_exception,
                    "wasmnativecontract.Setcode, payer does not exist, payer uid = '%s'",
                    txUid.ToString().c_str())
        sub_balance(payer, wasm::asset(llFees, wasm::symbol(SYMB::WICC, 8)), database.accountCache);

        //pseudo start for reduce code compile duration
        pseudo_start    = system_clock::now();
        fuel            = GetSerializeSize(SER_DISK, CLIENT_VERSION) * store_fuel_fee_per_byte;
        recipients_size = 0;

        if(transaction_status == wasm::transaction_status_type::validating)
        {
           WASM_TRACE("bytes:%d", GetSerializeSize(SER_DISK, CLIENT_VERSION))
           WASM_TRACE("fuel:%ld", fuel)
        }

        std::vector<CReceipt>   receipts;
        wasm::transaction_trace trx_trace;
        trx_trace.trx_id = GetHash();

        for (auto trx: inline_transactions) {
            trx_trace.traces.emplace_back();
            execute_inline_transaction(trx_trace.traces.back(), trx, trx.contract, database, receipts, 0);
        }
        trx_trace.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - pseudo_start);

        WASM_ASSERT(trx_trace.elapsed.count() < max_transaction_duration.count() * 1000,
                    wasm_exception,
                    "CWasmContractTx::ExecuteTx, Tx execution time must be in '%d' microseconds, but get '%d' microseconds",
                    max_transaction_duration * 1000, trx_trace.elapsed.count())                   

        //check storage usage with the limited fuel
        uint64_t fee    = get_fuel_limit(*this, context);
        fuel            = fuel + recipients_size * notice_fuel_fee_per_recipient;
        if(transaction_status == wasm::transaction_status_type::validating){
        WASM_TRACE("fuel:%ld", fuel)
        WASM_TRACE("fee:%ld", fee)
        WASM_TRACE("recipients_size:%ld", recipients_size)
        }

        WASM_ASSERT(fee > fuel, fuel_fee_exception, "%s",
                    "CWasmContractTx.ExecuteTx, fee is not enough to afford fuel");

        //save trx trace
        std::vector<char> trace_bytes = wasm::pack<transaction_trace>(trx_trace);
        WASM_ASSERT(database.contractCache.SetContractTraces(GetHash(),
                                                             std::string(trace_bytes.begin(), trace_bytes.end())),
                    wasm_exception,
                    "CWasmContractTx::ExecuteTx, set tx trace failed! txid=%s",
                    GetHash().ToString().c_str())

        //save trx receipts
        trace_to_receipts(trx_trace, receipts);
        WASM_ASSERT(database.txReceiptCache.SetTxReceipts(GetHash(), receipts),
                    wasm_exception,
                    "CWasmContractTx::ExecuteTx, set tx receipts failed! txid=%s",
                    GetHash().ToString().c_str())

        execute_tx_return->SetReturn(GetHash().ToString());

        nRunStep = fuel;

    } catch (wasm::exception &e) {
        return context.pState->DoS(100, ERRORMSG(e.detail()), e.code(), e.detail());
    }

    return true;
}

void CWasmContractTx::execute_inline_transaction(wasm::inline_transaction_trace& trace,
                                                 wasm::inline_transaction&       trx,
                                                 uint64_t                        receiver,
                                                 CCacheWrapper&                  database,
                                                 vector <CReceipt>&              receipts,
                                                 uint32_t                        recurse_depth) {

    wasm_context wasm_execute_context(*this, trx, database, receipts, mining, recurse_depth);

    //check timeout
    WASM_ASSERT(std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - pseudo_start) <
                get_max_transaction_duration() * 1000,
                wasm_timeout_exception, "%s", "timeout");

    wasm_execute_context._receiver = receiver;
    wasm_execute_context.execute(trace);

}


bool CWasmContractTx::GetInvolvedKeyIds(CCacheWrapper &cw, set <CKeyID> &keyIds) {

    CKeyID senderKeyId;
    if (!cw.accountCache.GetKeyId(txUid, senderKeyId))
        return false;

    keyIds.insert(senderKeyId);
    return true;
}

uint64_t CWasmContractTx::GetFuel(int32_t height, uint32_t nFuelRate) {

    uint64_t minFee = 0;
    if (!GetTxMinFee(nTxType, height, fee_symbol, minFee)) {
        LogPrint(BCLog::ERROR, "CWasmContractTx::GetFuel(), get min_fee failed! fee_symbol=%s\n", fee_symbol);
        throw runtime_error("CWasmContractTx::GetFuel(), get min_fee failed");
    }

    return std::max<uint64_t>(((nRunStep / 100.0f) * nFuelRate), minFee);
}

string CWasmContractTx::ToString(CAccountDBCache &accountCache) {

    if (inline_transactions.size() == 0) return string("");
    inline_transaction trx = inline_transactions[0];

    CAccount sender;
    if (!accountCache.GetAccount(txUid, sender)) {
        return string("");
    }

    return strprintf(
            "txType=%s, hash=%s, ver=%d, sender=%s, llFees=%llu, contract=%s, action=%s, arguments=%s, "
            "valid_height=%d",
            GetTxType(nTxType), GetHash().ToString(), nVersion, sender.nickid.ToString(), llFees,
            wasm::name(trx.contract).to_string(), wasm::name(trx.action).to_string(),
            HexStr(trx.data), valid_height);
}

Object CWasmContractTx::ToJson(const CAccountDBCache &accountCache) const {

    if (inline_transactions.size() == 0) return Object{};
    
    CAccount payer;
    accountCache.GetAccount(txUid, payer);

    Object result;
    result.push_back(Pair("txid",             GetHash().GetHex()));
    result.push_back(Pair("tx_type",          GetTxType(nTxType)));
    result.push_back(Pair("ver",              nVersion));
    result.push_back(Pair("tx_payer",         payer.nickid.ToString()));
    result.push_back(Pair("addr_payer",       payer.keyid.ToAddress()));
    result.push_back(Pair("fee_symbol",       fee_symbol));
    result.push_back(Pair("fees",             llFees));
    result.push_back(Pair("valid_height",     valid_height));

    if(inline_transactions.size() == 1){
        Value tmp;
        to_variant(inline_transactions[0], tmp);
        result.push_back(Pair("inline_transaction", tmp));
    } else if(inline_transactions.size() > 1) {
        Value inline_transactions_arr;
        to_variant(inline_transactions, inline_transactions_arr);
        result.push_back(Pair("inline_transactions", inline_transactions_arr));
    }

    if(signatures.size() == 1){
        Value tmp;
        to_variant(signatures[0], tmp);
        result.push_back(Pair("signature_pair", tmp));
    } else if(signatures.size() > 1) {
        Value signatures_arr;
        to_variant(signatures, signatures_arr);
        result.push_back(Pair("signature_pairs", signatures_arr));
    }     

    return result;
}

//void CWasmContractTx::set_signature(uint64_t account, const vector<uint8_t>& signature) {
void CWasmContractTx::set_signature(uint64_t account, const vector<uint8_t>& signature) {
    for( auto& s:signatures ){
        if( s.account == account ){
            s.signature = signature;
            return;
        }
    }
    WASM_ASSERT(false, wasm_exception, "cannot find account %s in signature list", wasm::name(account).to_string().c_str());
}

void CWasmContractTx::set_signature(const wasm::signature_pair& signature) {

    set_signature(signature.account, signature.signature);
    // for( auto& s:signatures ){
    //     if( s.account == signature.account ){
    //         //s.signature = signature.signature;
    //         s = signature;
    //         return;
    //     }
    // }
    // WASM_ASSERT(false, wasm_exception, "cannot find account %s in signature list", wasm::name(signature.account).to_string().c_str());
}

