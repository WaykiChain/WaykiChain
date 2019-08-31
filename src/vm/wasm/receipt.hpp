#pragma once
#include "commons/uint256.h"

namespace wasm {

  struct base_trace {
    uint64_t receiver;
    CInlineTransaction trx;

    uint256 trx_id; 
    uint32_t block_height;
    uint32_t block_time; 
  }

  struct inline_transaction_trace : public base_action_trace {
    vector<base_trace> inline_traces; 
  }


  struct transaction_trace {
     uint256 trx_id; 
     uint32_t block_height;
     uint32_t block_time;   

     inline_transaction_trace traces;
  }

  
} //wasm