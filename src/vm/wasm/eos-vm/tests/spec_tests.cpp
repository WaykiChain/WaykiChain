#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <iostream>
#include <math.h>

#include <catch2/catch.hpp>

#include "utils.hpp"
#include <wasm_config.hpp>

#include <eosio/vm/backend.hpp>

using namespace eosio;
using namespace eosio::vm;

extern wasm_allocator wa;
using backend_t = backend<std::nullptr_t>;

TEST_CASE("Testing wasm addressing", "[address_tests]") {
   // i32 bits
   {
      auto code = backend_t::read_wasm( address_i32_wasm );
      backend_t bkend( code );
      bkend.set_wasm_allocator( &wa );

      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good1", (uint32_t)0)) == (uint32_t)97);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good2", (uint32_t)0)) == (uint32_t)97);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good3", (uint32_t)0)) == (uint32_t)98);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good4", (uint32_t)0)) == (uint32_t)99);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good5", (uint32_t)0)) == (uint32_t)122);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good1", (uint32_t)0)) == (uint32_t)97);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good2", (uint32_t)0)) == (uint32_t)97);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good3", (uint32_t)0)) == (uint32_t)98);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good4", (uint32_t)0)) == (uint32_t)99);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good5", (uint32_t)0)) == (uint32_t)122);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good1", (uint32_t)0)) == (uint32_t)25185);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good2", (uint32_t)0)) == (uint32_t)25185);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good3", (uint32_t)0)) == (uint32_t)25442);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good4", (uint32_t)0)) == (uint32_t)25699);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good5", (uint32_t)0)) == (uint32_t)122);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good1", (uint32_t)0)) == (uint32_t)25185);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good2", (uint32_t)0)) == (uint32_t)25185);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good3", (uint32_t)0)) == (uint32_t)25442);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good4", (uint32_t)0)) == (uint32_t)25699);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good5", (uint32_t)0)) == (uint32_t)122);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good1", (uint32_t)0)) == (uint32_t)1684234849);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good2", (uint32_t)0)) == (uint32_t)1684234849);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good3", (uint32_t)0)) == (uint32_t)1701077858);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good4", (uint32_t)0)) == (uint32_t)1717920867);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good5", (uint32_t)0)) == (uint32_t)122);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good1", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good2", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good3", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good4", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good5", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good1", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good2", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good3", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good4", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good5", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good1", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good2", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good3", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good4", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good5", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good1", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good2", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good3", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good4", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good5", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good1", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good2", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good3", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good4", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good5", (uint32_t)65507)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good1", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good2", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good3", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good4", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8u_good5", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good1", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good2", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good3", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good4", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "8s_good5", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good1", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good2", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good3", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good4", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16u_good5", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good1", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good2", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good3", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good4", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "16s_good5", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good1", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good2", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good3", (uint32_t)65508)) == (uint32_t)0);
      CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "32_good4", (uint32_t)65508)) == (uint32_t)0);
   }

// i64 bits
   {
      auto code = backend_t::read_wasm( address_i64_wasm );
      backend_t bkend( code );
      bkend.set_wasm_allocator( &wa );

      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good1", (uint32_t)0)) == (uint64_t)97);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good2", (uint32_t)0)) == (uint64_t)97);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good3", (uint32_t)0)) == (uint64_t)98);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good4", (uint32_t)0)) == (uint64_t)99);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good5", (uint32_t)0)) == (uint64_t)122);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good1", (uint32_t)0)) == (uint64_t)97);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good2", (uint32_t)0)) == (uint64_t)97);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good3", (uint32_t)0)) == (uint64_t)98);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good4", (uint32_t)0)) == (uint64_t)99);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good5", (uint32_t)0)) == (uint64_t)122);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good1", (uint32_t)0)) == (uint64_t)25185);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good2", (uint32_t)0)) == (uint64_t)25185);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good3", (uint32_t)0)) == (uint64_t)25442);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good4", (uint32_t)0)) == (uint64_t)25699);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good5", (uint32_t)0)) == (uint64_t)122);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good1", (uint32_t)0)) == (uint64_t)25185);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good2", (uint32_t)0)) == (uint64_t)25185);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good3", (uint32_t)0)) == (uint64_t)25442);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good4", (uint32_t)0)) == (uint64_t)25699);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good5", (uint32_t)0)) == (uint64_t)122);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good1", (uint32_t)0)) == (uint64_t)1684234849);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good2", (uint32_t)0)) == (uint64_t)1684234849);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good3", (uint32_t)0)) == (uint64_t)1701077858);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good4", (uint32_t)0)) == (uint64_t)1717920867);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good5", (uint32_t)0)) == (uint64_t)122);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good1", (uint32_t)0)) == (uint64_t)1684234849);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good2", (uint32_t)0)) == (uint64_t)1684234849);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good3", (uint32_t)0)) == (uint64_t)1701077858);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good4", (uint32_t)0)) == (uint64_t)1717920867);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good5", (uint32_t)0)) == (uint64_t)122);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good1", (uint32_t)0)) == (uint64_t)0x6867666564636261);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good2", (uint32_t)0)) == (uint64_t)0x6867666564636261);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good3", (uint32_t)0)) == (uint64_t)0x6968676665646362);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good4", (uint32_t)0)) == (uint64_t)0x6a69686766656463);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good5", (uint32_t)0)) == (uint64_t)122);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good1", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good2", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good3", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good4", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good5", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good1", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good2", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good3", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good4", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good5", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good1", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good2", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good3", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good4", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good5", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good1", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good2", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good3", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good4", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good5", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good1", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good2", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good3", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good4", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good5", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good1", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good2", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good3", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good4", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good5", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good1", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good2", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good3", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good4", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good5", (uint32_t)65503)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good1", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good2", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good3", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good4", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8u_good5", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good1", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good2", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good3", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good4", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "8s_good5", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good1", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good2", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good3", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good4", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16u_good5", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good1", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good2", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good3", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good4", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "16s_good5", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good1", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good2", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good3", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good4", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32u_good5", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good1", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good2", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good3", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good4", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "32s_good5", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good1", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good2", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good3", (uint32_t)65504)) == (uint64_t)0);
      CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "64_good4", (uint32_t)65504)) == (uint64_t)0);
   }

// f32 bits
   {
      auto code = backend_t::read_wasm( address_f32_wasm );
      backend_t bkend( code );
      bkend.set_wasm_allocator( &wa );

      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good1", (uint32_t)0)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good2", (uint32_t)0)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good3", (uint32_t)0)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good4", (uint32_t)0)) == (float)0.0);
      CHECK(std::isnan(to_f32(*bkend.call_with_return(nullptr, "env", "32_good5", (uint32_t)0))));
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good1", (uint32_t)65524)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good2", (uint32_t)65524)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good3", (uint32_t)65524)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good4", (uint32_t)65524)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good5", (uint32_t)65524)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good1", (uint32_t)65525)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good2", (uint32_t)65525)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good3", (uint32_t)65525)) == (float)0.0);
      CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "32_good4", (uint32_t)65525)) == (float)0.0);
   }
// f64 bits
   {
      auto code = backend_t::read_wasm( address_f64_wasm );
      backend_t bkend( code );
      bkend.set_wasm_allocator( &wa );

      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good1", (uint32_t)0)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good2", (uint32_t)0)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good3", (uint32_t)0)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good4", (uint32_t)0)) == (double)0.0);
      CHECK(std::isnan(to_f64(*bkend.call_with_return(nullptr, "env", "64_good5", (uint32_t)0))));
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good1", (uint32_t)65510)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good2", (uint32_t)65510)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good3", (uint32_t)65510)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good4", (uint32_t)65510)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good5", (uint32_t)65510)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good1", (uint32_t)65511)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good2", (uint32_t)65511)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good3", (uint32_t)65511)) == (double)0.0);
      CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "64_good4", (uint32_t)65511)) == (double)0.0);
   }
}

#if 0
// tests to ensure that we are only accepting proper wasm binaries
BOOST_AUTO_TEST_CASE(binary_tests) {
try {
   {
      memory_manager::set_memory_limits( 32*1024*1024 );
      module mod;
      create_execution_context<interpret_visitor>("wasms/binary/b0.wasm", mod);
   }
   {
      memory_manager::set_memory_limits( 32*1024*1024 );
      module mod;
      create_execution_context<interpret_visitor>("wasms/binary/b1.wasm", mod);
   }
   {
      memory_manager::set_memory_limits( 32*1024*1024 );
      module mod;
      create_execution_context<interpret_visitor>("wasms/binary/b2.wasm", mod);
   }
   {
      memory_manager::set_memory_limits( 32*1024*1024 );
      module mod;
      create_execution_context<interpret_visitor>("wasms/binary/b3.wasm", mod);
   }
   {
      memory_manager::set_memory_limits( 32*1024*1024 );
      module mod;
      CHECK_THROWS_AS(create_execution_context<interpret_visitor>("wasms/binary/b4.wasm", mod), wasm_memory_exception);
   }
   // static constexpr const char* _wasm = "\x6d\x73\61\x00msa\x00\x01\x00\x00\x00msa\x00\x00\x00\x00\x01asm\x01\x00\x00\x00\x00wasm\x01\x00\x00\x00\x7fasm\x01\x00\x00\x00\x80asm\x01\x00\x00\x00\x82asm\x01\x00\x00\x00\xffasm\x01\x00\x00\x00\x00\x00\x00\x01msa\x00a\x00ms\x00\x01\x00\x00sm\x00a\x00\x00\x01\x00\x00ASM\x01\x00\x00\x00\x00\x81\xa2\x94\x01\x00\x00\x00\xef\xbb\xbf\x00asm\x01\x00\x00\x00\x00asm\x00asm\x01\x00asm\x01\x00\x00\x00asm\x00\x00\x00\x00\x00asm\x0d\x00\x00\x00\x00asm\x0e\x00\x00\x00\x00asm\x00\x01\x00\x00\x00asm\x00\x00\x01\x00\x00asm\x00\x00\x00\x01\x00asm\x01\x00\x00\x00\x05\x04\x01\x00\x82\x00\x00asm\x01\x00\x00\x00\x05\x07\x01\x00\x82\x80\x80\x80\x00\x00asm\x01\x00\x00\x00\x06\x07\x01\x7f\x00\x41\x80\x00\x0\x00asm\x01\x00\x00\x00\x06\x07\x01\x7f\x00\x41\xff\x7f\x0b\x00asm\x01\x00\x00\x00\x06\x0a\x01\x7f\x00\x41\x80\x80\x80\x80\x00\x0b\x00asm\x01\x00\x00\x00\x06\x0a\x01\x7f\x00\x41\xff\xff\xff\xff\x7f\x0b\x00asm\x01\x00\x00\x00\x06\x07\x01\x7e\x00\x42\x80\x00\x0b\x00asm\x01\x00\x00\x00\x06\x07\x01\x7e\x00\x42\xff\x7f\x0b\x00asm\x01\x00\x00\x00\x06\x0f\x01\x7e\x00\x42\x80\x80\x80\x80\x80\x80\x80\x80\x80\x00\x0b\x00asm\x01\x00\x00\x00\x06\x0f\x01\x7e\x00\x42\xff\xff\xff\xff\xff\xff\xff\xff\xff\x7f\x0b\x00asm\x01\x00\x00\x00\x05\x03\x01\x00\x00\x0b\x07\x01\x80\x00\x41\x00\x0b\x0\x00asm\x01\x00\x00\x00\x04\x04\x01\x70\x00\x00\x09\x07\x01\x80\x00\x41\x00\x0b\x0\x00asm\x01\x00\x00\x00\x05\x08\x01\x00\x82\x80\x80\x80\x80\x00\x00asm\x01\x00\x00\x00\x06\x0b\x01\x7f\x00\x41\x80\x80\x80\x80\x80\x00\x0b\x00asm\x01\x00\x00\x00\x06\x0b\x01\x7f\x00\x41\xff\xff\xff\xff\xff\x7f\x0b\x00asm\x01\x00\x00\x00\x06\x10\x01\x7e\x00\x42\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x00\x0b\x00asm\x01\x00\x00\x00\x06\x10\x01\x7e\x00\x42\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x7f\x0b\x00asm\x01\x00\x00\x00\x05\x07\x01\x00\x82\x80\x80\x80\x7\x00asm\x01\x00\x00\x00\x05\x07\x01\x00\x82\x80\x80\x80\x4\x00asm\x01\x00\x00\x00\x06\x0a\x01\x7f\x00\x41\x80\x80\x80\x80\x70\x0b\x00asm\x01\x00\x00\x00\x06\x0a\x01\x7f\x00\x41\xff\xff\xff\xff\x0f\x0b\x00asm\x01\x00\x00\x00\x06\x0a\x01\x7f\x00\x41\x80\x80\x80\x80\x1f\x0b\x00asm\x01\x00\x00\x00\x06\x0a\x01\x7f\x00\x41\xff\xff\xff\xff\x4f\x0b\x00asm\x01\x00\x00\x00\x06\x0f\x01\x7e\x00\x42\x80\x80\x80\x80\x80\x80\x80\x80\x80\x7e\x0b\x00asm\x01\x00\x00\x00\x06\x0f\x01\x7e\x00\x42\xff\xff\xff\xff\xff\xff\xff\xff\xff\x01\x0b\x00asm\x01\x00\x00\x00\x06\x0f\x01\x7e\x00\x42\x80\x80\x80\x80\x80\x80\x80\x80\x80\x02\x0b\x00asm\x01\x00\x00\x00\x06\x0f\x01\x7e\x00\x42\xff\xff\xff\xff\xff\xff\xff\xff\xff\x41\x0b\x00asm\x01\x00\x00\x00\x01\x04\x01\x60\x00\x00\x03\x02\x01\x00\x04\x04\x01\x70\x00\x00\x0a\x09\x01\x07\x00\x41\x00\x11\x00\x01\x0b\x00asm\x01\x00\x00\x00\x01\x04\x01\x60\x00\x00\x03\x02\x01\x00\x04\x04\x01\x70\x00\x00\x0a\x0a\x01\x07\x00\x41\x00\x11\x00\x80\x00\x0b\x00asm\x01\x00\x00\x00\x01\x04\x01\x60\x00\x00\x03\x02\x01\x00\x04\x04\x01\x70\x00\x00\x0a\x0b\x01\x08\x00\x41\x00\x11\x00\x80\x80\x00\x0b\x00asm\x01\x00\x00\x00\x03\x01\x00unctio";
 std::vector<uint8_t> code;
 //for (int i=0; i < strlen(_wasm); i++) {
    //   code.push_back((uint8_t)_wasm[i]);
 //}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+0, 6};
   module mod;
   bp.parse_module(cp, 6, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+6, 18};
   module mod;
   bp.parse_module(cp, 18, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+24, 18};
   module mod;
   bp.parse_module(cp, 18, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+42, 18};
   module mod;
   bp.parse_module(cp, 18, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+60, 16};
   module mod;
   bp.parse_module(cp, 16, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+76, 18};
   module mod;
   bp.parse_module(cp, 18, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+94, 18};
   module mod;
   bp.parse_module(cp, 18, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+112, 18};
   module mod;
   bp.parse_module(cp, 18, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+130, 18};
   module mod;
   bp.parse_module(cp, 18, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 24};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 24, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 27};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 27, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 6};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 6, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 9};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 9, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 15};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 15, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 18};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 18, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+148, 36};
   module mod;
   bp.parse_module(cp, 36, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+184, 45};
   module mod;
   bp.parse_module(cp, 45, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+229, 44};
   module mod;
   bp.parse_module(cp, 44, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+273, 45};
   module mod;
   bp.parse_module(cp, 45, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+318, 54};
   module mod;
   bp.parse_module(cp, 54, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+372, 54};
   module mod;
   bp.parse_module(cp, 54, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+426, 45};
   module mod;
   bp.parse_module(cp, 45, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+471, 45};
   module mod;
   bp.parse_module(cp, 45, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+516, 69};
   module mod;
   bp.parse_module(cp, 69, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+585, 69};
   module mod;
   bp.parse_module(cp, 69, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+654, 59};
   module mod;
   bp.parse_module(cp, 59, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+713, 62};
   module mod;
   bp.parse_module(cp, 62, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 48};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 48, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 57};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 57, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 57};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 57, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 72};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 72, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 72};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 72, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 44};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 44, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 44};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 44, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 54};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 54, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 54};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 54, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 54};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 54, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 54};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 54, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 69};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 69, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 69};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 69, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 69};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 69, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 69};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 69, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 99};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 99, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 102};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 102, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 105};
   module mod;
   CHECK_THROWS_AS(bp.parse_module(cp, 105, mod), wasm_parse_exception);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+775, 27};
   module mod;
   bp.parse_module(cp, 27, mod);
}
{
   binary_parser bp;
   wasm_code_ptr cp{code.data()+802, 6};
   module mod;
   bp.parse_module(cp, 6, mod);
}

} FC_LOG_AND_RETHROW()
}
#endif

TEST_CASE("Testing wasm blocks", "[blocks_tests]") {
   auto code = backend_t::read_wasm( blocks_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "empty"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singular")) == (uint32_t)7);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multi")) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "deep")) == (uint32_t)150);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-mid")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-mid")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-last")) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-then")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-else")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-last")) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-return-value")) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_global-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-load-operand")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-operand")) == (uint32_t)12);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-bare")) == (uint32_t)19);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-value")) == (uint32_t)18);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-repeated")) == (uint32_t)18);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-inner")) == (uint32_t)0xf);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "effects")) == (uint32_t)1);

}

TEST_CASE("Testing wasm branching", "[br_tests]") {
   auto code = backend_t::read_wasm( br_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "type-i32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-i64"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f64"));

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-i32-value")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-i64-value")) == (uint64_t)2);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32-value")) == (float)3);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64-value")) == (double)4);

   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-mid"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-last"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-value")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-first")) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-mid")) == (uint32_t)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-last")) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == (uint32_t)9);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_if-cond"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-value")) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-value-cond")) == (uint32_t)9);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_table-index"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-value")) == (uint32_t)10);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-value-index")) == (uint32_t)11);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-return-value")) == (uint64_t)7);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-cond")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)1, (uint32_t)6)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)0, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)0, (uint32_t)6)) == (uint32_t)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)1, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first", (uint32_t)0, (uint32_t)6)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first", (uint32_t)1, (uint32_t)6)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-second", (uint32_t)0, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-second", (uint32_t)1, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-cond")) == (uint32_t)7);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-first")) == (uint32_t)12);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-mid")) == (uint32_t)13);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-last")) == (uint32_t)14);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-func")) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == (uint32_t)21);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-last")) == (uint32_t)23);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value")) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-tee_local-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_global-value")) == (uint32_t)1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-load-address")) == (float)1.7);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-loadN-address")) == (uint64_t)30);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-store-address")) == (uint32_t)30);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-store-value")) == (uint32_t)31);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-storeN-address")) == (uint32_t)32);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-storeN-value")) == (uint32_t)33);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == (float)3.4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == (uint32_t)3);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == (uint64_t)45);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == (uint32_t)43);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == (uint32_t)42);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-size")) == (uint32_t)40);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index")) == (uint32_t)9);

}

TEST_CASE("Testing wasm conditional branching", "[br_if_tests]") {
   auto code = backend_t::read_wasm( br_if_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "type-i32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-i64"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f64"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-i32-value")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-i64-value")) == (uint64_t)2);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32-value")) == (float)3);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64-value")) == (double)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-first", (uint32_t)0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-first", (uint32_t)1)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-mid", (uint32_t)0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-mid", (uint32_t)1)) == (uint32_t)3);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-last", (uint32_t)0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-last", (uint32_t)1));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-first-value", (uint32_t)0)) == (uint32_t)11);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-first-value", (uint32_t)1)) == (uint32_t)10);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-mid-value", (uint32_t)0)) == (uint32_t)21);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-mid-value", (uint32_t)1)) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-last-value", (uint32_t)0)) == (uint32_t)11);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-last-value", (uint32_t)1)) == (uint32_t)11);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-first", (uint32_t)0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-first", (uint32_t)1)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-mid", (uint32_t)0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-mid", (uint32_t)1)) == (uint32_t)4);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-loop-last", (uint32_t)0));
   //CHECK(!bkend.call_with_return(nullptr, "env", "as-loop-last", (uint32_t)1));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_if-cond"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-value-cond", (uint32_t)0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-value-cond", (uint32_t)1)) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_table-index"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-value-index")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-return-value")) == (uint64_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-cond", (uint32_t)0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-cond", (uint32_t)1)) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)0, (uint32_t)0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)4, (uint32_t)0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)0, (uint32_t)1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)4, (uint32_t)1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)0, (uint32_t)0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)3, (uint32_t)0));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)0, (uint32_t)1));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)3, (uint32_t)1));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first", (uint32_t)0)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first", (uint32_t)1)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-second", (uint32_t)0)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-second", (uint32_t)1)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-cond")) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-first")) == (uint32_t)12);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-mid")) == (uint32_t)13);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-last")) == (uint32_t)14);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-func")) == (uint32_t)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == (uint32_t)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == (uint32_t)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-last")) == (uint32_t)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value", (uint32_t)0)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value", (uint32_t)1)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-tee_local-value", (uint32_t)0)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-tee_local-value", (uint32_t)1)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_global-value", (uint32_t)0)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_global-value", (uint32_t)1)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-load-address")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loadN-address")) == (uint32_t)30);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-store-address")) == (uint32_t)30);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-store-value")) == (uint32_t)31);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-storeN-address")) == (uint32_t)32);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-storeN-value")) == (uint32_t)33);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == (double)1.0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-size")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)0)) == (uint32_t)21);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)0)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)0)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)0)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)0)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)0)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)1)) == (uint32_t)9);

}

TEST_CASE("Testing wasm branch table", "[br_table_tests]") {
   auto code = backend_t::read_wasm( br_table_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "type-i32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-i64"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f32"));
   CHECK(!bkend.call_with_return(nullptr, "env", "type-f64"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-i32-value")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-i64-value")) == (uint64_t)2);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32-value")) == (float)3);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64-value")) == (double)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty", (uint32_t)0)) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty", (uint32_t)1)) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty", (uint32_t)11)) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty", (uint32_t)-1)) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty", (uint32_t)-100)) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty", (uint32_t)0xffffffff)) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty-value", (uint32_t)0)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty-value", (uint32_t)1)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty-value", (uint32_t)11)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty-value", (uint32_t)-1)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty-value", (uint32_t)-100)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "empty-value", (uint32_t)0xffffffff)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton", (uint32_t)0)) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton", (uint32_t)1)) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton", (uint32_t)11)) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton", (uint32_t)-1)) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton", (uint32_t)-100)) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton", (uint32_t)0xffffffff)) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton-value", (uint32_t)0)) == (uint32_t)32);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton-value", (uint32_t)1)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton-value", (uint32_t)11)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton-value", (uint32_t)-1)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton-value", (uint32_t)-100)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singleton-value", (uint32_t)0xffffffff)) == (uint32_t)33);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)0)) == (uint32_t)103);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)1)) == (uint32_t)102);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)2)) == (uint32_t)101);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)3)) == (uint32_t)100);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)4)) == (uint32_t)104);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)5)) == (uint32_t)104);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)6)) == (uint32_t)104);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)10)) == (uint32_t)104);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)-1)) == (uint32_t)104);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple", (uint32_t)0xffffffff)) == (uint32_t)104);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)0)) == (uint32_t)213);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)1)) == (uint32_t)212);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)2)) == (uint32_t)211);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)3)) == (uint32_t)210);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)4)) == (uint32_t)214);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)5)) == (uint32_t)214);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)6)) == (uint32_t)214);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)10)) == (uint32_t)214);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)-1)) == (uint32_t)214);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multiple-value", (uint32_t)0xffffffff)) == (uint32_t)214);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)1)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)100)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)101)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)10000)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)10001)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)1000000)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "large", (uint32_t)1000001)) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-mid"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-block-last"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-block-value")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-first")) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-mid")) == (uint32_t)4);
   //CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-loop-last")) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == (uint32_t)9);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_if-cond"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-value")) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-value-cond")) == (uint32_t)9);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-br_table-index"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-value")) == (uint32_t)10);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-value-index")) == (uint32_t)11);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-return-value")) == (uint64_t)7);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-cond")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)1, (uint32_t)6)) == (uint32_t)3);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-then", (uint32_t)0, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)0, (uint32_t)6)) == (uint32_t)4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-else", (uint32_t)1, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first", (uint32_t)0, (uint32_t)6)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first", (uint32_t)1, (uint32_t)6)) == (uint32_t)5);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-second", (uint32_t)0, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-second", (uint32_t)1, (uint32_t)6)) == (uint32_t)6);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-cond")) == (uint32_t)7);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-first")) == (uint32_t)12);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-mid")) == (uint32_t)13);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-last")) == (uint32_t)14);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == (uint32_t)20);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == (uint32_t)21);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-last")) == (uint32_t)22);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-func")) == (uint32_t)23);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value")) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-tee_local-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_global-value")) == (uint32_t)1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-load-address")) == (float)1.7);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-loadN-address")) == (uint64_t)30);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-store-address")) == (uint32_t)30);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-store-value")) == (uint32_t)31);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-storeN-address")) == (uint32_t)32);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-storeN-value")) == (uint32_t)33);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == (float)3.4);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == (uint32_t)3);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == (uint64_t)45);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == (uint32_t)43);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == (uint32_t)42);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-convert-operand")) == (uint32_t)41);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-size")) == (uint32_t)40);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)0)) == (uint32_t)19);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)1)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)2)) == (uint32_t)16);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)10)) == (uint32_t)16);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)-1)) == (uint32_t)16);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-block-value", (uint32_t)100000)) == (uint32_t)16);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)0)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)2)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)11)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)-4)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br-value", (uint32_t)10213210)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)0)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)2)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)9)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)-9)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value", (uint32_t)999999)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)0)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)1)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)2)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)3)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)-1000000)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_if-value-cond", (uint32_t)9423975)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)0)) == (uint32_t)17);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)1)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)2)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)9)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)-9)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value", (uint32_t)999999)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)0)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)1)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)2)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)3)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)-1000000)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-value-index", (uint32_t)9423975)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested-br_table-loop-block", (uint32_t)1)) == (uint32_t)3);

}

TEST_CASE("Testing wasm branching breaks", "[break_drop_tests]") {
   auto code = backend_t::read_wasm( break_drop_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "br"));
   CHECK(!bkend.call_with_return(nullptr, "env", "br_if"));
   CHECK(!bkend.call_with_return(nullptr, "env", "br_table"));

}

TEST_CASE("Testing wasm calls", "[call_tests]") {
   auto code = backend_t::read_wasm( call_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-i32")) == (uint32_t)0x132);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-i64")) == (uint64_t)0x164);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32")) == (float)0xf32);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64")) == (double)0xf64);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-first-i32")) == (uint32_t)32);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-first-i64")) == (uint64_t)64);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-first-f32")) == (float)1.32);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-first-f64")) == (double)1.64);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-second-i32")) == (uint32_t)32);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-second-i64")) == (uint64_t)64);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-second-f32")) == (float)32);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-second-f64")) == (double)64.1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac", (uint64_t)0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac", (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac", (uint64_t)2)) == (uint64_t)2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac", (uint64_t)5)) == (uint64_t)120);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac", (uint64_t)25)) == (uint64_t)7034535277573963776);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-acc", (uint64_t)0, (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-acc", (uint64_t)1, (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-acc", (uint64_t)5, (uint64_t)1)) == (uint64_t)120);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-acc", (uint64_t)25, (uint64_t)1)) == (uint64_t)7034535277573963776);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib", (uint64_t)0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib", (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib", (uint64_t)2)) == (uint64_t)2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib", (uint64_t)5)) == (uint64_t)8);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib", (uint64_t)20)) == (uint64_t)10946);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint64_t)0)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint64_t)1)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint64_t)100)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint64_t)77)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint64_t)0)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint64_t)1)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint64_t)200)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint64_t)77)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-mid")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-condition")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-first")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-first")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == (uint32_t)2);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-return-value")) == (uint32_t)0x132);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-tee_local-value")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_global-value")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-load-operand")) == (uint32_t)'A');
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == (float)0x0p+0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == (uint32_t)11);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-convert-operand")) == (uint64_t)1);

   //bkend.call_with_return(nullptr, "env", "runaway");
}

TEST_CASE("Testing wasm call indirect", "[call_indirect_tests]") {
   auto code = backend_t::read_wasm( call_indirect_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-i32")) == (uint32_t)0x132);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-i64")) == (uint64_t)0x164);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-f32")) == (float)0xf32);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-f64")) == (double)0xf64);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-index")) == (uint64_t)100);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-first-i32")) == (uint32_t)32);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-first-i64")) == (uint64_t)64);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-first-f32")) == (float)1.32);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-first-f64")) == (double)1.64);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "type-second-i32")) == (uint32_t)32);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "type-second-i64")) == (uint64_t)64);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "type-second-f32")) == (float)32);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "type-second-f64")) == (double)64.1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch", (uint32_t)5, (uint64_t)2)) == (uint64_t)2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch", (uint32_t)5, (uint64_t)5)) == (uint64_t)5);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch", (uint32_t)12, (uint64_t)5)) == (uint64_t)120);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch", (uint32_t)13, (uint64_t)5)) == (uint64_t)8);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch", (uint32_t)20, (uint64_t)2)) == (uint64_t)2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", (uint32_t)5)) == (uint64_t)9);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", (uint32_t)12)) == (uint64_t)362880);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", (uint32_t)13)) == (uint64_t)55);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i64", (uint32_t)20)) == (uint64_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", (uint32_t)4)) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", (uint32_t)23)) == (uint32_t)362880);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", (uint32_t)26)) == (uint32_t)55);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-i32", (uint32_t)19)) == (uint32_t)9);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", (uint32_t)6)) == (float)9.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", (uint32_t)24)) == (float)362880.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", (uint32_t)27)) == (float)55.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f32", (uint32_t)21)) == (float)9.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", (uint32_t)7)) == (double)9.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", (uint32_t)25)) == (double)362880.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", (uint32_t)28)) == (double)55.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "dispatch-structural-f64", (uint32_t)22)) == (double)9.0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-i64", (uint64_t)0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-i64", (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-i64", (uint64_t)5)) == (uint64_t)120);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-i64", (uint64_t)25)) == (uint64_t)7034535277573963776);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fac-i32", (uint32_t)0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fac-i32", (uint32_t)1)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fac-i32", (uint32_t)5)) == (uint32_t)120);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fac-i32", (uint32_t)10)) == (uint32_t)3628800);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", (float)0.0)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", (float)1.0)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", (float)5.0)) == (float)120.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fac-f32", (float)10.0)) == (float)3628800.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", (double)0.0)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", (double)1.0)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", (double)5.0)) == (double)120.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fac-f64", (double)10.0)) == (double)3628800.0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib-i64", (uint64_t)0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib-i64", (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib-i64", (uint64_t)2)) == (uint64_t)2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib-i64", (uint64_t)5)) == (uint64_t)8);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fib-i64", (uint64_t)20)) == (uint64_t)10946);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fib-i32", (uint32_t)0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fib-i32", (uint32_t)1)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fib-i32", (uint32_t)2)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fib-i32", (uint32_t)5)) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "fib-i32", (uint32_t)20)) == (uint32_t)10946);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", (float)0.0)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", (float)1.0)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", (float)2.0)) == (float)2.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", (float)5.0)) == (float)8.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "fib-f32", (float)20.0)) == (float)10946.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", (double)0.0)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", (double)1.0)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", (double)2.0)) == (double)2.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", (double)5.0)) == (double)8.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "fib-f64", (double)20.0)) == (double)10946.0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint32_t)0)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint32_t)1)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint32_t)100)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "even", (uint32_t)77)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint32_t)0)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint32_t)1)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint32_t)200)) == (uint32_t)99);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "odd", (uint32_t)77)) == (uint32_t)44);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first")) == (uint32_t)0x132);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-mid")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-condition")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-br_if-first")) == (uint64_t)0x164);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-last")) == (uint32_t)2);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-br_table-first")) == (float)0xf32);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-last")) == (uint32_t)2);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-return-value")) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == (float)1);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "as-set_local-value")) == (double)1);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "as-tee_local-value")) == (double)1);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "as-set_global-value")) == (double)1.0);
   // TODO fix this like call_tests
   //CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-load-operand")) == (uint32_t)'A');
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == (float)0x0p+0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-left")) == (uint32_t)11);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-right")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-left")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-right")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "as-convert-operand")) == (uint64_t)1);
}

TEST_CASE("Testing wasm conversions", "[conversions_tests]") {
   auto code = backend_t::read_wasm( conversions_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", (uint32_t)0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", (uint32_t)10000)) == (uint64_t)10000);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", (uint32_t)-10000)) == (uint64_t)-10000);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", (uint32_t)-1)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", (uint32_t)0x7fffffff)) == (uint64_t)0x000000007fffffff);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_s", (uint32_t)0x80000000)) == (uint64_t)0xffffffff80000000);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", (uint32_t)0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", (uint32_t)10000)) == (uint64_t)10000);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", (uint32_t)-10000)) == (uint64_t)0x00000000ffffd8f0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", (uint32_t)-1)) == (uint64_t)0xffffffff);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", (uint32_t)0x7fffffff)) == (uint64_t)0x000000007fffffff);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.extend_i32_u", (uint32_t)0x80000000)) == (uint64_t)0x0000000080000000);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)-1)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)-100000)) == (uint32_t)-100000);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0x80000000)) == (uint32_t)0x80000000);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0xffffffff7fffffff)) == (uint32_t)0x7fffffff);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0xffffffff00000000)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0xfffffffeffffffff)) == (uint32_t)0xffffffff);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0xffffffff00000001)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)1311768467463790320)) == (uint32_t)0x9abcdef0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0x00000000ffffffff)) == (uint32_t)0xffffffff);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0x0000000100000000)) == (uint32_t)0x00000000);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.wrap_i64", (uint64_t)0x0000000100000001)) == (uint32_t)0x00000001);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)0x1p-149)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-0x1p-149)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)1.0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)0x1.19999ap+0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)1.5)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-1.0)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-0x1.19999ap+0)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-1.5)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-1.9)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-2.0)) == (uint32_t)-2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)2147483520.0)) == (uint32_t)2147483520);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_s", (float)-2147483648.0)) == (uint32_t)-2147483648);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)-0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)0x1p-149)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)-0x1p-149)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)1.0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)0x1.19999ap+0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)1.5)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)1.9)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)2.0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)2147483648)) == (uint32_t)-2147483648);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)4294967040.0)) == (uint32_t)-256);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)-0x1.ccccccp-1)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f32_u", (float)-0x1.fffffep-1)) == (uint32_t)0);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)0x0.0000000000001p-1022)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-0x0.0000000000001p-1022)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)1.0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)0x1.199999999999ap+0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)1.5)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-1.0)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-0x1.199999999999ap+0)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-1.5)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-1.9)) == (uint32_t)-1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-2.0)) == (uint32_t)-2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)2147483647.0)) == (uint32_t)2147483647);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_s", (double)-2147483648.0)) == (uint32_t)-2147483648);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)-0.0)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)0x0.0000000000001p-1022)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)-0x0.0000000000001p-1022)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)1.0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)0x1.199999999999ap+0)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)1.5)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)1.9)) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)2.0)) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)2147483648)) == (uint32_t)2147483648);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)4294967295.0)) == (uint32_t)4294967295);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)-0x1.ccccccccccccdp-1)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)-0x1.fffffffffffffp-1)) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "i32.trunc_f64_u", (double)1e8)) == (uint32_t)100000000);

   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)0x1p-149)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-0x1p-149)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)1.0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)0x1.19999ap+0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)1.5)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-1.0)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-0x1.19999ap+0)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-1.5)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-1.9)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-2.0)) == (uint64_t)-2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)4294967296)) == (uint64_t)4294967296);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-4294967296)) == (uint64_t)-4294967296);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)9223371487098961920.0)) == (uint64_t)9223371487098961920ull);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_s", (float)-9223372036854775808.0)) == std::numeric_limits<int64_t>::min());

   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)-0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)0x1p-149)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)-0x1p-149)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)1.0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)0x1.19999ap+0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)1.5)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)4294967296)) == (uint64_t)4294967296);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)18446742974197923840.0)) == (uint64_t)18446742974197923840ull);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)-0x1.ccccccp-1)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f32_u", (float)-0x1.fffffep-1)) == (uint64_t)0);

   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)0x0.0000000000001p-1022)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-0x0.0000000000001p-1022)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)1.0)) == (uint64_t)1.0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)0x1.199999999999ap+0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)1.5)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-1.0)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-0x1.199999999999ap+0)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-1.5)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-1.9)) == (uint64_t)-1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-2.0)) == (uint64_t)-2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)4294967296)) == (uint64_t)4294967296);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-4294967296)) == (uint64_t)-4294967296);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)9223372036854774784.0)) == (uint64_t)9223372036854774784);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_s", (double)-9223372036854775808.0)) == std::numeric_limits<int64_t>::min());

   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)-0.0)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)0x0.0000000000001p-1022)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)-0x0.0000000000001p-1022)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)1.0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)0x1.199999999999ap+0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)1.5)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)4294967295)) == (uint64_t)4294967295);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)4294967296)) == (uint64_t)4294967296);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)18446744073709549568.0)) == (uint64_t)18446744073709549568ull);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)-0x1.ccccccccccccdp-1)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)-0x1.fffffffffffffp-1)) == (uint64_t)0);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)1e8)) == (uint64_t)100000000);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)1e16)) == (uint64_t)10000000000000000);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "i64.trunc_f64_u", (double)9223372036854775808.0)) == std::numeric_limits<int64_t>::min());

   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)1)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)-1)) == (float)-1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)0)) == (float)0.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)2147483647)) == (float)2147483648);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)-2147483648)) == (float)-2147483648);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)1234567890)) == (float)0x1.26580cp+30);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)16777217)) == (float)16777216.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)-16777217)) == (float)-16777216.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)16777219)) == (float)16777220.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_s", (uint32_t)-16777219)) == (float)-16777220.0);

   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)1)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)-1)) == (float)-1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)0)) == (float)0.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)9223372036854775807)) == (float)9223372036854775807);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)std::numeric_limits<int64_t>::min())) == (float)-9223372036854775808.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)314159265358979)) == (float)0x1.1db9e8p+48);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)16777217)) == (float)16777216.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)-16777217)) == (float)-16777216.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)16777219)) == (float)16777220);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_s", (uint64_t)-16777219)) == (float)-16777220);

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", (uint32_t)1)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", (uint32_t)-1)) == (double)-1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", (uint32_t)0)) == (double)0.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", (uint32_t)2147483647)) == (double)2147483647);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", (uint32_t)-2147483648)) == (double)-2147483648);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_s", (uint32_t)987654321)) == (double)987654321);

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)1)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)-1)) == (double)-1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)0)) == (double)0.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)9223372036854775807)) == (double)9223372036854775807);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)-9223372036854775807)) == (double)-9223372036854775807);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)4669201609102990)) == (double)4669201609102990);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)9007199254740993)) == (double)9007199254740992);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)-9007199254740993)) == (double)-9007199254740992);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)9007199254740995)) == (double)9007199254740996);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_s", (uint64_t)-9007199254740995)) == (double)-9007199254740996);

   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)1)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0)) == (float)0.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)2147483647)) == (float)2147483648);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)-2147483648)) == (float)2147483648);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0x12345678)) == (float)0x1.234568p+28);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0xffffffff)) == (float)4294967296.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0x80000080)) == (float)0x1.000000p+31);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0x80000081)) == (float)0x1.000002p+31);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0x80000082)) == (float)0x1.000002p+31);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0xfffffe80)) == (float)0x1.fffffcp+31);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0xfffffe81)) == (float)0x1.fffffep+31);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)0xfffffe82)) == (float)0x1.fffffep+31);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)16777217)) == (float)16777216.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i32_u", (uint32_t)16777219)) == (float)16777220.0);

   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", (uint64_t)1)) == (float)1.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", (uint64_t)0)) == (float)0.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", (uint64_t)9223372036854775807)) == (float)9223372036854775807);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", (int64_t)std::numeric_limits<int64_t>::min())) == (float)-9223372036854775808.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", (uint64_t)0xffffffffffffffff)) == (float)18446744073709551616.0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", (uint64_t)16777217)) == (float)16777216);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32.convert_i64_u", (uint64_t)16777219)) == (float)16777220);

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", (uint32_t)1)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", (uint32_t)0)) == (double)0.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", (uint32_t)2147483647)) == (double)2147483647);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", (uint32_t)-2147483648)) == (double)2147483648);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i32_u", (uint32_t)0xffffffff)) == (double)4294967295.0);

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)1)) == (double)1);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0)) == (double)0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)9223372036854775807)) == (double)9223372036854775807);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (int64_t)std::numeric_limits<int64_t>::min())) == (double)-9223372036854775808.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0xffffffffffffffff)) == (double)18446744073709551616.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0x8000000000000400)) == (double)0x1.0000000000000p+63);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0x8000000000000401)) == (double)0x1.0000000000001p+63);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0x8000000000000402)) == (double)0x1.0000000000001p+63);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0xfffffffffffff400)) == (double)0x1.ffffffffffffep+63);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0xfffffffffffff401)) == (double)0x1.fffffffffffffp+63);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)0xfffffffffffff402)) == (double)0x1.fffffffffffffp+63);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)9007199254740993)) == (double)9007199254740992);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.convert_i64_u", (uint64_t)9007199254740995)) == (double)9007199254740996);

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)0.0)) == (double)0.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)-0.0)) == (double)-0.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)0x1p-149)) == (double)0x1p-149);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)-0x1p-149)) == (double)-0x1p-149);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)1.0)) == (double)1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)-1.0)) == (double)-1.0);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)-0x1.fffffep+127)) == (double)-0x1.fffffep+127);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)0x1.fffffep+127)) == (double)0x1.fffffep+127);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)0x1p-119)) == (double)0x1p-119);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64.promote_f32", (float)0x1.8f867ep+125)) == (double)6.6382536710104395e+37);
}

TEST_CASE("Testing wasm endianness", "[endianness_tests]") {
   auto code = backend_t::read_wasm( endianness_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_s", (int32_t)-1)) == (int32_t)65535);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_s", (int32_t)-4242)) == (int32_t)61294);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_s", (int32_t)42)) == (int32_t)42);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_s", (int32_t)0x3210)) == (int32_t)0x3210);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_u", (int32_t)-1)) == (uint16_t)0xFFFF);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_u", (int32_t)-4242)) == (uint16_t)61294);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_u", (int32_t)42)) == (uint16_t)42);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load16_u", (int32_t)0xCAFE)) == (uint16_t)0xCAFE);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load", (int32_t)-1)) == (uint32_t)-1);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load", (int32_t)-42424242)) == (uint32_t)-42424242);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load", (int32_t)42424242)) == (uint32_t)42424242);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_load", (int32_t)0xABAD1DEA)) == (uint32_t)0xABAD1DEA);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_s", (int64_t)-1)) == (int32_t)65535);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_s", (int64_t)-4242)) == (int32_t)61294);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_s", (int64_t)42)) == (int32_t)42);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_s", (int64_t)0x3210)) == (int32_t)0x3210);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_u", (int64_t)-1)) == (uint32_t)65535);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_u", (int64_t)-4242)) == (uint32_t)61294);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_u", (int64_t)42)) == (uint32_t)42);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load16_u", (int64_t)0xCAFE)) == (uint32_t)0xCAFE);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_s", (int64_t)-1)) == 0xFFFFFFFF);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_s", (int64_t)-42424242)) == 0xfd78a84e);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_s", (int64_t)42424242)) == (int64_t)42424242);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_s", (int64_t)0x12345678)) == (int64_t)0x12345678);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_u", (int64_t)-1)) == (uint32_t)-1);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_u", (int64_t)-42424242)) == (uint32_t)-42424242);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_u", (int64_t)42424242)) == (uint32_t)42424242);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load32_u", (int64_t)0xABAD1DEA)) == (uint32_t)0xABAD1DEA);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load", (int64_t)-1)) == (uint64_t)-1);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load", (int64_t)-42424242)) == (uint64_t)-42424242);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load", (int64_t)0xABAD1DEA)) == (uint64_t)0xABAD1DEA);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_load", (int64_t)0xABADCAFEDEAD1DEA)) == (uint64_t)0xABADCAFEDEAD1DEA);

   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_load", (float)-1)) == (float)-1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_load", (float)1234e-5)) == (float)1234e-5);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_load", (float)4242.4242)) == (float)4242.4242);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_load", (float)0x1.fffffep+127)) == (float)0x1.fffffep+127);

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_load", (double)-1)) == (double)-1);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_load", (double)123456789e-5)) == (double)123456789e-5);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_load", (double)424242.424242)) == (double)424242.424242);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_load", (double)0x1.fffffffffffffp+1023)) == (double)0x1.fffffffffffffp+1023);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store16", (int32_t)-1)) == (uint32_t)0xFFFF);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store16", (int32_t)-4242)) == (uint32_t)61294);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store16", (int32_t)42)) == (uint32_t)42);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store16", (int32_t)0xCAFE)) == (uint32_t)0xCAFE);

   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store", (int32_t)-1)) == (uint32_t)-1);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store", (int32_t)-4242)) == (uint32_t)-4242);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store", (int32_t)42424242)) == (uint32_t)42424242);
   CHECK(to_i32(*bkend.call_with_return(nullptr, "env", "i32_store", (int32_t)0xDEADCAFE)) == (uint32_t)0xDEADCAFE);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store16", (int64_t)-1)) == 0xffff);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store16", (int64_t)-4242)) == 0xef6e);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store16", (int64_t)42)) == (uint64_t)42);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store16", (int64_t)0xCAFE)) == (uint64_t)0xCAFE);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store32", (int64_t)-1)) == 0xffffffff);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store32", (int64_t)-4242)) == 0xffffef6e);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store32", (int64_t)42424242)) == (uint64_t)42424242);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store32", (int64_t)0xDEADCAFE)) == (uint64_t)0xDEADCAFE);

   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store", (int64_t)-1)) == (uint64_t)-1);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store", (int64_t)-42424242)) == (uint64_t)-42424242);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store", (int64_t)0xABAD1DEA)) == (uint64_t)0xABAD1DEA);
   CHECK(to_i64(*bkend.call_with_return(nullptr, "env", "i64_store", (int64_t)0xABADCAFEDEAD1DEA)) == (uint64_t)0xABADCAFEDEAD1DEA);

   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_store", (float)-1)) == (float)-1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_store", (float)1234e-5)) == (float)1234e-5);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_store", (float)4242.4242)) == (float)4242.4242);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "f32_store", (float)0x1.fffffep+127)) == (float)0x1.fffffep+127);

   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_store", (double)-1)) == (double)-1);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_store", (double)123456789e-5)) == (double)123456789e-5);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_store", (double)424242.424242)) == (double)424242.424242);
   CHECK(to_f64(*bkend.call_with_return(nullptr, "env", "f64_store", (double)0x1.fffffffffffffp+1023)) == (double)0x1.fffffffffffffp+1023);
}

TEST_CASE("Testing wasm factorial", "[fac_tests]") {
   auto code = backend_t::read_wasm( fac_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-rec", (uint64_t)25)) == (uint64_t)7034535277573963776);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-iter", (uint64_t)25)) == (uint64_t)7034535277573963776);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-rec-named", (uint64_t)25)) == (uint64_t)7034535277573963776);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-iter-named", (uint64_t)25)) == (uint64_t)7034535277573963776);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "fac-opt", (uint64_t)25)) == (uint64_t)7034535277573963776);
}

TEST_CASE("Testing wasm loops", "[loop_tests]") {
   auto code = backend_t::read_wasm( loop_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(!bkend.call_with_return(nullptr, "env", "empty"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "singular")) == (uint32_t)7);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "multi")) == (uint32_t)8);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "nested")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "deep")) == (uint32_t)150);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-mid")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-select-last")) == (uint32_t)2);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-if-condition"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-then")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-if-else")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_if-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br_table-last")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-first")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-mid")) == (uint32_t)2);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call_indirect-last")) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-first"));
   CHECK(!bkend.call_with_return(nullptr, "env", "as-store-last"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-memory.grow-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-call-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-return-value")) == (uint32_t)1);
   CHECK(!bkend.call_with_return(nullptr, "env", "as-drop-operand"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-br-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_local-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-tee_local-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-set_global-value")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-load-operand")) == (uint32_t)1);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-unary-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-binary-operand")) == (uint32_t)12);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-test-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "as-compare-operand")) == (uint32_t)0);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-bare")) == (uint32_t)19);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-value")) == (uint32_t)18);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-repeated")) == (uint32_t)18);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "break-inner")) == (uint32_t)0x1f);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "effects")) == (uint32_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "while", (uint64_t)0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "while", (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "while", (uint64_t)2)) == (uint64_t)2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "while", (uint64_t)3)) == (uint64_t)6);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "while", (uint64_t)5)) == (uint64_t)120);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "while", (uint64_t)20)) == (uint64_t)2432902008176640000);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "for", (uint64_t)0)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "for", (uint64_t)1)) == (uint64_t)1);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "for", (uint64_t)2)) == (uint64_t)2);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "for", (uint64_t)3)) == (uint64_t)6);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "for", (uint64_t)5)) == (uint64_t)120);
   CHECK(to_ui64(*bkend.call_with_return(nullptr, "env", "for", (uint64_t)20)) == (uint64_t)2432902008176640000);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)0, (float)7)) == (float)0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)7, (float)0)) == (float)0);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)1, (float)1)) == (float)1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)1, (float)2)) == (float)2);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)1, (float)3)) == (float)4);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)1, (float)4)) == (float)6);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)1, (float)100)) == (float)2550);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)1, (float)101)) == (float)2601);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)2, (float)1)) == (float)1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)3, (float)1)) == (float)1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)10, (float)1)) == (float)1);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)2, (float)2)) == (float)3);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)2, (float)3)) == (float)4);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)7, (float)4)) == (float)10.3095235825);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)7, (float)100)) == (float)4381.54785156);
   CHECK(to_f32(*bkend.call_with_return(nullptr, "env", "nesting", (float)7, (float)101)) == (float)2601);
}

TEST_CASE("Testing wasm stack unwinding" "[unwind_tests]") {
   auto code = backend_t::read_wasm( unwind_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "func-unwind-by-unreachable"), eosio::vm::wasm_interpreter_exception);
   CHECK(!bkend.call_with_return(nullptr, "env", "func-unwind-by-br"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "func-unwind-by-br-value")) == (uint32_t)9);
   CHECK(!bkend.call_with_return(nullptr, "env", "func-unwind-by-br_if"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "func-unwind-by-br_if-value")) == (uint32_t)9);
   CHECK(!bkend.call_with_return(nullptr, "env", "func-unwind-by-br_table"));
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "func-unwind-by-br_table-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "func-unwind-by-return")) == (uint32_t)9);
   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "block-unwind-by-unreachable"), eosio::vm::wasm_interpreter_exception);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-unwind-by-br")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-unwind-by-br-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-unwind-by-br_if")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-unwind-by-br_if-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-unwind-by-br_table")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-unwind-by-br_table-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-unwind-by-return")) == (uint32_t)9);
   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-unreachable"), eosio::vm::wasm_interpreter_exception);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_if")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_if-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_table")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-br_table-value")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-nested-unwind-by-return")) == (uint32_t)9);
   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "unary-after-unreachable"), eosio::vm::wasm_interpreter_exception);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "unary-after-br")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "unary-after-br_if")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "unary-after-br_table")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "unary-after-return")) == (uint32_t)9);
   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "binary-after-unreachable"), eosio::vm::wasm_interpreter_exception);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "binary-after-br")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "binary-after-br_if")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "binary-after-br_table")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "binary-after-return")) == (uint32_t)9);
   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "select-after-unreachable"), eosio::vm::wasm_interpreter_exception);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "select-after-br")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "select-after-br_if")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "select-after-br_table")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "select-after-return")) == (uint32_t)9);
   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "block-value-after-unreachable"), eosio::vm::wasm_interpreter_exception);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-value-after-br")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-value-after-br_if")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-value-after-br_table")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "block-value-after-return")) == (uint32_t)9);
   CHECK_THROWS_AS(bkend.call_with_return(nullptr, "env", "loop-value-after-unreachable"), eosio::vm::wasm_interpreter_exception);

   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "loop-value-after-br")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "loop-value-after-br_if")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "loop-value-after-br_table")) == (uint32_t)9);
   CHECK(to_ui32(*bkend.call_with_return(nullptr, "env", "loop-value-after-return")) == (uint32_t)9);
}
