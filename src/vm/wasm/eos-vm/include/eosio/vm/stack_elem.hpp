#pragma once

#include <eosio/vm/opcodes.hpp>
#include <eosio/vm/variant.hpp>

#include <cstdint>

namespace eosio { namespace vm {

   class operand_stack_elem : public variant<i32_const_t, i64_const_t, f32_const_t, f64_const_t> {
      public:
         using variant<i32_const_t, i64_const_t, f32_const_t, f64_const_t>::variant;
         inline int32_t&  to_i32() & { return get<i32_const_t>().data.i; }
         inline uint32_t& to_ui32() & { return get<i32_const_t>().data.ui; }
         inline float&    to_f32() & { return get<f32_const_t>().data.f; }
         inline uint32_t& to_fui32() & { return get<f32_const_t>().data.ui; }

         inline int64_t&  to_i64() & { return get<i64_const_t>().data.i; }
         inline uint64_t& to_ui64() & { return get<i64_const_t>().data.ui; }
         inline double&   to_f64() & { return get<f64_const_t>().data.f; }
         inline uint64_t& to_fui64() & { return get<f64_const_t>().data.ui; }

         inline int32_t  to_i32() const & { return get<i32_const_t>().data.i; }
         inline uint32_t to_ui32() const & { return get<i32_const_t>().data.ui; }
         inline float    to_f32() const & { return get<f32_const_t>().data.f; }
         inline uint32_t to_fui32() const & { return get<f32_const_t>().data.ui; }

         inline int64_t  to_i64() const & { return get<i64_const_t>().data.i; }
         inline uint64_t to_ui64() const & { return get<i64_const_t>().data.ui; }
         inline double   to_f64() const & { return get<f64_const_t>().data.f; }
         inline uint64_t to_fui64() const & { return get<f64_const_t>().data.ui; }

   };
}} // nameo::vm
