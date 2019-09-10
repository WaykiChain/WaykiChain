#pragma once

#include "commons/uint256.h"
#include "wasm/types/inlinetransaction.hpp"

namespace wasm {

    struct base_trace {
        uint256 trx_id;
        uint64_t receiver;
        inline_transaction trx;
        //uint32_t block_height;
        //uint32_t block_time;

        string console;
    };

    struct inline_transaction_trace : public base_trace {
        vector <inline_transaction_trace> inline_traces;
    };


    struct transaction_trace {
        uint256 trx_id;
        //uint32_t block_height;
        //uint32_t block_time;

        vector<inline_transaction_trace> traces;
    };


} //wasm