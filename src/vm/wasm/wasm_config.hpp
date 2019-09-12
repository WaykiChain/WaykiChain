#pragma once

#include <chrono>
#include "wasm/types/name.hpp"

namespace wasm {

    using std::chrono::microseconds;


    const static uint16_t default_query_rows = 10;

    const static auto max_serialization_time = microseconds(15 * 1000);
    const static uint16_t max_inline_transaction_depth = 4;

    const static uint64_t wasmio            = N(wasmio);
    const static uint64_t wasmio_bank       = N(wasmio.bank);
    const static uint64_t wasmio_code       = N(wasmio.code);
    const static uint64_t wasmio_owner      = N(wasmio.owner);

}  // wasm

