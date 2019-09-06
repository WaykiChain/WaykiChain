#pragma once

#include <eosio/wasm_backend/exceptions.hpp>
#include <eosio/wasm_backend/utils.hpp>

#include <array>
#include <iostream>

namespace eosio { namespace vm {
   class validator {
      public:
         // validate limits 3.2.1
         inline bool validate(const resizable_limits& rl) {
            return rl.flags ? rl.maximum >= rl.initial : true;
         }

         // validate a function type 3.2.2
         inline bool validate(const func_type& ft) {
            return (ft.return_count == 0 || ft.return_count == 1);
         }

         // validate a table type 3.2.3
         inline bool validate(const table_type& tt) {
            static constexpr uint64_t max_limit = (uint64_t)2 << 32;
            bool max_not_exceeded = tt.flags ? tt.limits.maximum <= max_limit : true;
            return tt.limits.initial <= max_limit && max_not_exceeded && validate(tt.limits);
         }

         // validate a memory type 3.2.4
         inline bool validate(const memory_type& mt) {
            static constexpr uint64_t max_limit = (uint64_t)2 << 16;
            bool max_not_exceeded = mt.flags ? mt.limits.maximum <= max_limit : true;
            return mt.limits.initial <= max_limit && max_not_exceeded && validate(mt.limits);
         }

         // validate a global type 3.2.5
         inline bool validate(const global_type& gt) {
            return true;
         }

   };
}} // ns eosio::vm
