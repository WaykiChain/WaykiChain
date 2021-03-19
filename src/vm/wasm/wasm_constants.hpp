#pragma once

#include <chrono>
#include "wasm/types/name.hpp"
#include "wasm/types/regid.hpp"

namespace wasm {

    using std::chrono::microseconds;


    const static uint16_t default_query_rows            = 100;

    const static auto max_serialization_time            = microseconds(25 * 1000);
    const static auto max_wasm_execute_time_mining      = 200;//in milliseconds
    const static auto max_wasm_execute_time_infinite    = 2000;
    //const static auto max_serialization_time = microseconds(60 * 1000 * 1000);
    const static uint16_t max_inline_transaction_depth  = 4;
    const static uint16_t max_recipients_size           = 16;
    const static uint16_t max_abi_array_size            = 8192;
    const static uint16_t max_inline_transaction_bytes  = 4096;
    const static uint32_t max_wasm_api_data_bytes       = 1024*1024;
    const static uint16_t max_inline_transactions_size  = 1024;
    const static uint16_t max_signatures_size           = 64;

    static const uint64_t wasmio                        = REGID(0, 100); //0-100
    static const uint64_t wasmio_bank                   = REGID(0, 800); //0-800
    static const uint64_t wasmio_voting                 = REGID(0, 900); //0-900

    const static uint64_t wasmio_code                   = NAME(wasmio_code);
    const static uint64_t wasmio_owner                  = NAME(wasmio_owner);

    const static uint64_t store_fuel_per_byte           = 10;
    const static uint64_t notice_fuel_per_recipient     = 10000;

    namespace wasm_constraints {
        constexpr unsigned maximum_linear_memory        = 33*1024*1024;//bytes
        constexpr unsigned maximum_mutable_globals      = 1024;        //bytes
        constexpr unsigned maximum_table_elements       = 1024;        //elements
        constexpr unsigned maximum_section_elements     = 1024;        //elements
        constexpr unsigned maximum_linear_memory_init   = 64*1024;     //bytes
        constexpr unsigned maximum_func_local_bytes     = 8192;        //bytes
        constexpr unsigned maximum_call_depth           = 250;         //nested calls
        constexpr unsigned maximum_code_size            = 20*1024*1024;

        static constexpr unsigned wasm_page_size        = 64*1024;

        static_assert(maximum_linear_memory%wasm_page_size      == 0, "maximum_linear_memory must be mulitple of wasm page size");
        static_assert(maximum_mutable_globals%4                 == 0, "maximum_mutable_globals must be mulitple of 4");
        static_assert(maximum_table_elements*8%4096             == 0, "maximum_table_elements*8 must be mulitple of 4096");
        static_assert(maximum_linear_memory_init%wasm_page_size == 0, "maximum_linear_memory_init must be mulitple of wasm page size");
        static_assert(maximum_func_local_bytes%8                == 0, "maximum_func_local_bytes must be mulitple of 8");
        static_assert(maximum_func_local_bytes                  > 32, "maximum_func_local_bytes must be greater than 32");
    } // namespace  wasm_constraints

}  // wasm
