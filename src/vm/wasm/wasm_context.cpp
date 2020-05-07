#include "wasm_context.hpp"
//#include "wasm/wasm_native_contract.hpp"
#include "wasm/types/name.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/wasm_log.hpp"
#include "entities/account.h"

#include "wasm/exception/exceptions.hpp"
#include "wasm/modules/wasm_native_dispatch.hpp"

using namespace std;
using namespace wasm;
// using std::chrono::microseconds;
// using std::chrono::system_clock;

namespace wasm {

    static inline void print_debug(uint64_t receiver, const inline_transaction_trace &trace) {
        if (!trace.console.empty()) {

            ostringstream prefix;

            auto contract_s = regid(trace.trx.contract).to_string();
            auto action_s   = name(trace.trx.action).to_string();
            auto receiver_s = regid(receiver).to_string();

            prefix << "[" << contract_s << "," << action_s << "]->" << receiver_s << "";
            std::cout << prefix.str()  << ": CONSOLE OUTPUT BEGIN =====================\n"
                      << trace.console << "\n"
                      << prefix.str()  << ": CONSOLE OUTPUT END   =====================\n\n";
        }
    }


    void wasm_context::reset_console() {
        _pending_console_output = std::ostringstream();
    }

    bool wasm_context::has_permission_from_inline_transaction(const permission& p){
        return std::find(trx.authorization.begin(), trx.authorization.end(), p) != trx.authorization.end();
    }

    void wasm_context::execute_inline(const inline_transaction& t) {

        //check authorization
        for (const auto p: t.authorization) {

            //inline wasmio.bank
            if (t.contract == wasmio_bank && (p.account != _receiver || p.perm != wasmio_code) ) {
                CHAIN_ASSERT( false,
                              wasm_chain::missing_auth_exception,
                              "Inline to wasmio.bank can be only authorized by contract-self '%s' in '%s' , but get '%s' in '%s'",
                              wasm::regid(_receiver).to_string(), wasm::name(wasmio_code).to_string(),
                              wasm::regid(p.account).to_string(), wasm::name(p.perm).to_string());
            }

            //call contract-self and authorized by contract
            if (t.contract == _receiver && p.account == _receiver ) continue;

            //call contract-self
            if (t.contract == _receiver && !has_permission_from_inline_transaction(p) ) {
                CHAIN_ASSERT( false,
                              wasm_chain::missing_auth_exception,
                              "Missing authorization by account '%s' in a new inline transaction",
                              wasm::regid(p.account).to_string());
            }

            //call another contract
            if (t.contract != _receiver && (p.account != _receiver || p.perm != wasmio_code)){
                CHAIN_ASSERT( false,
                              wasm_chain::missing_auth_exception,
                              "Inline to another contract can be only authorized by contract-self '%s' in wasmio.code, but get '%s' in ",
                              wasm::regid(_receiver).to_string(),
                              wasm::regid(p.account).to_string(), wasm::name(p.perm).to_string());
            }

        }

        inline_transactions.push_back(t);
    }

    bool wasm_context::get_code(const uint64_t& contract, std::vector <uint8_t> &code) {
        CUniversalContractStore contract_store;
        if (!database.contractCache.GetContract(CRegID(contract), contract_store))
            return false;

        code = vector <uint8_t>(contract_store.code.begin(), contract_store.code.end());
        return true;
    }

    uint64_t wasm_context::get_maintainer(const uint64_t& contract) {

        CRegID   maintainer;
        CUniversalContractStore contract_store;
        auto spContractAcct = control_trx.GetAccount(database, CRegID(contract));
        if (spContractAcct && database.contractCache.GetContract(spContractAcct->regid, contract_store)) {
            maintainer = contract_store.maintainer;
        }
        return maintainer.GetIntValue();
    }

    void wasm_context::initialize() {

        static bool wasm_interface_inited = false;
        if (!wasm_interface_inited) {
            wasm_interface_inited = true;
            wasmif.initialize(wasm::vm_type::eos_vm_jit);
        }
    }

    void wasm_context::execute(inline_transaction_trace &trace) {

        initialize();

        notified.push_back(_receiver);
        execute_one(trace);

        for (uint32_t i = 1; i < notified.size(); ++i) {
            _receiver = notified[i];

            trace.inline_traces.emplace_back();
            LogPrint(BCLog::WASM, "execute_one starts: %d...\n", i);
            execute_one(trace.inline_traces.back());
            LogPrint(BCLog::WASM, "execute_one finishes...\n");
        }

        CHAIN_ASSERT( recurse_depth < wasm::max_inline_transaction_depth,
                      wasm_chain::transaction_exception,
                      "max inline transaction depth per transaction reached");

        for (auto &inline_trx : inline_transactions) {
            trace.inline_traces.emplace_back();
            control_trx.execute_inline_transaction(trace.inline_traces.back(), inline_trx,
                                                   inline_trx.contract, database, receipts,
                                                   recurse_depth + 1);
        }

    }

    void wasm_context::execute_one(inline_transaction_trace &trace) {

        //auto start = system_clock::now();
        control_trx.recipients_size ++;

        trace.trx      = trx;
        trace.receiver = _receiver;

        auto* native   = get_wasm_act_route().route(_receiver);

        //reset_console();
        try {
            if (native) {
                (*native)(*this, trx.action);
            } else {
                vector <uint8_t> code;
                if (get_code(_receiver, code) && code.size() > 0) {
                    wasmif.execute(code, this);
                }
            }
        }  catch (wasm_chain::exception &e) {
            string console_output = (_pending_console_output.str().size() == 0) ?
                                        string("") :
                                        string(", console: ") + _pending_console_output.str();

            CHAIN_RETHROW_EXECPTION( e, log_level::warn,
                                     "[%s, %s]->%s%s",
                                     regid(contract()).to_string(),
                                     name(action()).to_string(),
                                     regid(receiver()).to_string(),
                                     console_output );
        } catch (...) {
            string console_output = (_pending_console_output.str().size() == 0) ?
                                        string("") :
                                        string(", console: ") + _pending_console_output.str();

            CHAIN_THROW( wasm_chain::chain_exception,
                         "[%s, %s]->%s%s",
                         regid(contract()).to_string(),
                         name(action()).to_string(),
                         regid(receiver()).to_string(),
                         console_output );
        }

        trace.trx_id  = control_trx.GetHash();
        trace.console = _pending_console_output.str();
        //trace.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - start);

        reset_console();

        if (contracts_console()) {
            print_debug(_receiver, trace);
        }

    }

    bool wasm_context::has_recipient(const uint64_t& account) const {
        for (auto a : notified)
            if (a == account)
                return true;
        return false;
    }

    void wasm_context::notify_recipient(const uint64_t& recipient)  {

        if (!has_recipient(recipient)) {
            notified.push_back(recipient);
        }

        CHAIN_ASSERT( notified.size() <= max_recipients_size,
                      wasm_chain::recipients_size_exceeds_exception,
                      "recipients size must be <= '%ld', but get '%ld'", max_recipients_size, notified.size() );

    }

    void wasm_context::require_auth( const uint64_t& account ) const {
        for (auto p: trx.authorization) {
            if (p.account == account) {
                return;
            }
        }
        CHAIN_ASSERT(false, wasm_chain::missing_auth_exception, "missing authority of %s", wasm::regid(account).to_string());
    }

    bool wasm_context::has_authorization( const uint64_t& account ) const {
        for (auto p: trx.authorization) {
            if (p.account == account) {
                return true;
            }
        }

        return false;
    }

    bool wasm_context::is_account( const uint64_t& account ) const {

        return database.accountCache.HasAccount(CRegID(account));
    }

    std::vector<uint64_t> wasm_context::get_active_producers(){

        auto &db_account  = database.accountCache;
        auto &db_delegate = database.delegateCache;

        std::vector<uint64_t> active_producers;
        VoteDelegateVector    producers;
        CHAIN_ASSERT( db_delegate.GetActiveDelegates(producers),
                      wasm_chain::account_access_exception,
                      "fail to get top delegates for active producer");

        for ( auto p: producers){
            CAccount producer;
            CHAIN_ASSERT( db_account.GetAccount(p.regid, producer),
                          wasm_chain::account_access_exception,
                          "producer account get account error, regid = %s",
                          p.regid.ToString())

            CHAIN_ASSERT( producer.regid.GetIntValue() != 0,
                          wasm_chain::account_access_exception,
                          "producer account does not have reg_id, regid = %s",
                          p.regid.ToString())

            active_producers.push_back(producer.regid.GetIntValue());
        }
        return active_producers;
    }

    bool wasm_context::get_system_asset_price(uint64_t base, uint64_t quote, std::vector<char>& price){

        wasm::symbol base_symbol  = wasm::symbol(base);
        wasm::symbol quote_symbol = wasm::symbol(quote);

        if (quote_symbol.precision() != 8) return false;//the precision of system asset must be 8
        if (base_symbol.precision()  != 8) return false;//the precision of system asset must be 8

        PriceCoinPair price_pair(base_symbol.code().to_string(), quote_symbol.code().to_string());

        //if(CheckPricePair(pricePair) != nullptr) return 0;
        auto &db_pricefeed  = database.priceFeedCache;
        uint64_t price_amount = db_pricefeed.GetMedianPrice(price_pair);
        if (price_amount == 0) return false;

        asset asset_price(price_amount, base_symbol);
        price = wasm::pack<asset>(asset_price);
        return true;
    }

    void wasm_context::update_storage_usage(const uint64_t& account, const int64_t& size_in_bytes){

        int64_t disk_usage    = size_in_bytes * store_fuel_fee_per_byte;
        control_trx.run_cost += (disk_usage < 0) ? 0 : disk_usage;
    }

}