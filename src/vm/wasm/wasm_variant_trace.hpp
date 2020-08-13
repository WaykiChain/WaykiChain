#pragma once
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include "wasm/wasm_context.hpp"
#include "wasm/types/name.hpp"
#include "wasm/abi_def.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/abi_serializer.hpp"
//#include "wasm/wasm_native_contract_abi.hpp"
//#include "wasm/wasm_native_contract.hpp"
#include "wasm_trace.hpp"
#include "tx/contracttx.h"
#include "wasm/modules/wasm_native_dispatch.hpp"

namespace wasm {



template<typename Api>
struct resolver_factory {
static auto make(Api& api) {
    return [&api](const uint64_t &account) -> std::vector<char> {

        std::vector<char> abi;
        if (!get_native_contract_abi(account, abi)) {
            CUniversalContractStore contract_store;
            if (api.contractCache.GetContract(CRegID(account), contract_store)){
                abi.insert(abi.end(), contract_store.abi.begin(), contract_store.abi.end());
            }
        }
        return abi;
    };
  }
};

template<typename Api>
auto make_resolver(Api& api) {
    return resolver_factory<Api>::make(api);
}

static inline void to_variant(const signature_pair &t, json_spirit::Value &v) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(wasm::regid(t.account), val);
    json_spirit::Config::add(obj, "account", val);

    to_variant(HexStr(t.signature), val);
    json_spirit::Config::add(obj, "signature", val);

    v = obj;
}

static inline void to_variant(const wasm::permission &t, json_spirit::Value &v) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(wasm::regid(t.account), val);
    json_spirit::Config::add(obj, "account", val);

    to_variant(wasm::name(t.perm), val);
    json_spirit::Config::add(obj, "permission", val);

    v = obj;
}

static inline void to_variant(const wasm::inline_transaction &t, json_spirit::Value &v) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(wasm::regid(t.contract), val);
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
    to_variant(to_hex(t.data, ""), val);
    json_spirit::Config::add(obj, "data", val);

    v = obj;
}


template<typename Resolver>
static inline void to_variant(const wasm::inline_transaction &t, json_spirit::Value &v, Resolver resolver) {

    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(wasm::regid(t.contract), val);
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

    std::vector<char> abi = resolver(t.contract);

    //fixme:setcode should be removed
    if (abi.size() > 0 && t.action != wasm::NAME(setcode)) {
        if (t.data.size() > 0) {
            try {
                val = wasm::abi_serializer::unpack(abi, wasm::name(t.action).to_string(), t.data,
                                                   max_serialization_time);
            } catch (...) {
                to_variant(to_hex(t.data, ""), val);
            }
        }
    } else
        to_variant(to_hex(t.data, ""), val);

    json_spirit::Config::add(obj, "data", val);

    v = obj;
}

template<typename Resolver>
static inline void to_variant(const wasm::inline_transaction_trace &t, json_spirit::Value &v, Resolver resolver) {
    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(t.trx_id.ToString(), val);
    json_spirit::Config::add(obj, "trx_id", val);

    // to_variant(t.elapsed.count(), val);
    // json_spirit::Config::add(obj, "elapsed", val);

    to_variant(wasm::regid(t.receiver), val);
    json_spirit::Config::add(obj, "receiver", val);

    to_variant(t.trx, val, resolver);
    json_spirit::Config::add(obj, "trx", val);

    if(t.console.size() > 0){
        to_variant(t.console, val);
        json_spirit::Config::add(obj, "console", val);
    }

    if (t.inline_traces.size() > 0) {
        json_spirit::Array arr;
        for (const auto &trace :t.inline_traces) {
            json_spirit::Value tmp;
            to_variant(trace, tmp, resolver);
            arr.push_back(tmp);
        }

        json_spirit::Config::add(obj, "inline_traces", json_spirit::Value(arr));

    }

    v = obj;

}


template<typename Variant, typename Resolver>
static inline void add_field_variant(json_spirit::Object &obj, const string& name, const Variant &var, Resolver resolver) {
    json_spirit::Value value;
    to_variant(var, value);
    json_spirit::Config::add(obj, name, value);
}

template<typename Resolver>
static inline void to_variant(const wasm::transaction_trace &t, json_spirit::Value &v, Resolver resolver) {

    json_spirit::Object obj;

    add_field_variant(obj, "trx_id", t.trx_id.ToString(), resolver);
    add_field_variant(obj, "elapsed", t.elapsed.count(), resolver);
    add_field_variant(obj, "burned_fuel", t.fuel, resolver);
    add_field_variant(obj, "fuel_rate", t.fuel_rate, resolver);
    add_field_variant(obj, "minimum_fee", t.minimum_tx_execute_fee, resolver);

    if (t.traces.size() > 0) {
        json_spirit::Array arr;
        for (const auto &trace :t.traces) {
            json_spirit::Value tmp;
            to_variant(trace, tmp, resolver);
            arr.push_back(tmp);
        }

        json_spirit::Config::add(obj, "traces", json_spirit::Value(arr));
    }

    v = obj;
}


template<typename Resolver>
static inline void to_variant(const wasm::transaction_log &t, json_spirit::Value &v, Resolver resolver) {
    json_spirit::Object obj;

    json_spirit::Value val;
    to_variant(t.trx_id.ToString(), val);
    json_spirit::Config::add(obj, "trx_id", val);

    to_variant(wasm::regid(t.receiver), val);
    json_spirit::Config::add(obj, "receiver", val);

    // to_variant(t.trx, val, resolver);
    // json_spirit::Config::add(obj, "trx", val);

    to_variant(t.topic, val);
    json_spirit::Config::add(obj, "topic", val);

    std::vector<char> abi = resolver(t.trx.contract);

    if (abi.size() > 0 ) {
        if (t.data.size() > 0) {
            try {
                val = wasm::abi_serializer::unpack_data(abi, t.topic, t.data,
                                                   max_serialization_time);
            } catch (...) {
                to_variant(to_hex(t.data, ""), val);
            }
        }
    } else
        to_variant(to_hex(t.data, ""), val);

    json_spirit::Config::add(obj, "data", val);


    v = obj;

}

template<typename Resolver>
static inline void to_variant(const std::vector<wasm::transaction_log> &ts, json_spirit::Value &v, Resolver resolver) {
    json_spirit::Object obj;

    if (ts.size() > 0) {
        json_spirit::Array arr;
        for (const auto &t :ts) {
            json_spirit::Value tmp;
            to_variant(t, tmp, resolver);
            arr.push_back(tmp);

            WASM_TRACE("%s", t.topic);
        }

        json_spirit::Config::add(obj, "logs", json_spirit::Value(arr));
    }


    v = obj;

}



} //wasm
