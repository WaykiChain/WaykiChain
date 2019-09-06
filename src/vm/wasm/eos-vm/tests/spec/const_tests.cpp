#include <algorithm>
#include <vector>
#include <iostream>
#include <iterator>
#include <cmath>
#include <cstdlib>
#include <catch2/catch.hpp>
#include <utils.hpp>
#include <wasm_config.hpp>
#include <eosio/vm/backend.hpp>

using namespace eosio;
using namespace eosio::vm;
extern wasm_allocator wa;
using backend_t = backend<std::nullptr_t>;

TEST_CASE( "Testing wasm <const_0_wasm>", "[const_0_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_0_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_100_wasm>", "[const_100_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_100_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_101_wasm>", "[const_101_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_101_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_102_wasm>", "[const_102_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_102_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_103_wasm>", "[const_103_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_103_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_104_wasm>", "[const_104_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_104_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_105_wasm>", "[const_105_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_105_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_106_wasm>", "[const_106_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_106_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_107_wasm>", "[const_107_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_107_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_108_wasm>", "[const_108_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_108_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_109_wasm>", "[const_109_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_109_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_110_wasm>", "[const_110_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_110_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_111_wasm>", "[const_111_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_111_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_112_wasm>", "[const_112_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_112_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_113_wasm>", "[const_113_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_113_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_114_wasm>", "[const_114_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_114_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_115_wasm>", "[const_115_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_115_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_116_wasm>", "[const_116_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_116_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_117_wasm>", "[const_117_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_117_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_118_wasm>", "[const_118_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_118_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922819));
}

TEST_CASE( "Testing wasm <const_119_wasm>", "[const_119_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_119_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406467));
}

TEST_CASE( "Testing wasm <const_120_wasm>", "[const_120_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_120_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922816));
}

TEST_CASE( "Testing wasm <const_121_wasm>", "[const_121_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_121_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406464));
}

TEST_CASE( "Testing wasm <const_122_wasm>", "[const_122_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_122_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_123_wasm>", "[const_123_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_123_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_124_wasm>", "[const_124_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_124_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_125_wasm>", "[const_125_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_125_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_126_wasm>", "[const_126_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_126_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_127_wasm>", "[const_127_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_127_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_128_wasm>", "[const_128_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_128_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783616));
}

TEST_CASE( "Testing wasm <const_129_wasm>", "[const_129_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_129_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267264));
}

TEST_CASE( "Testing wasm <const_12_wasm>", "[const_12_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_12_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_130_wasm>", "[const_130_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_130_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_131_wasm>", "[const_131_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_131_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_132_wasm>", "[const_132_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_132_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_133_wasm>", "[const_133_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_133_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_134_wasm>", "[const_134_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_134_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_135_wasm>", "[const_135_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_135_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_136_wasm>", "[const_136_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_136_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_137_wasm>", "[const_137_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_137_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_138_wasm>", "[const_138_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_138_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_139_wasm>", "[const_139_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_139_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_13_wasm>", "[const_13_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_13_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_140_wasm>", "[const_140_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_140_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_141_wasm>", "[const_141_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_141_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_142_wasm>", "[const_142_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_142_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_143_wasm>", "[const_143_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_143_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_144_wasm>", "[const_144_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_144_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_145_wasm>", "[const_145_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_145_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_146_wasm>", "[const_146_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_146_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_147_wasm>", "[const_147_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_147_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_148_wasm>", "[const_148_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_148_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_149_wasm>", "[const_149_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_149_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_150_wasm>", "[const_150_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_150_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_151_wasm>", "[const_151_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_151_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_152_wasm>", "[const_152_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_152_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_153_wasm>", "[const_153_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_153_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_154_wasm>", "[const_154_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_154_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783619));
}

TEST_CASE( "Testing wasm <const_155_wasm>", "[const_155_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_155_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267267));
}

TEST_CASE( "Testing wasm <const_156_wasm>", "[const_156_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_156_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783616));
}

TEST_CASE( "Testing wasm <const_157_wasm>", "[const_157_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_157_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267264));
}

TEST_CASE( "Testing wasm <const_158_wasm>", "[const_158_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_158_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_159_wasm>", "[const_159_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_159_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_160_wasm>", "[const_160_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_160_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_161_wasm>", "[const_161_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_161_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_162_wasm>", "[const_162_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_162_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_163_wasm>", "[const_163_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_163_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_164_wasm>", "[const_164_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_164_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_165_wasm>", "[const_165_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_165_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_166_wasm>", "[const_166_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_166_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_167_wasm>", "[const_167_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_167_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_168_wasm>", "[const_168_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_168_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_169_wasm>", "[const_169_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_169_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_16_wasm>", "[const_16_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_16_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_170_wasm>", "[const_170_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_170_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783616));
}

TEST_CASE( "Testing wasm <const_171_wasm>", "[const_171_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_171_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267264));
}

TEST_CASE( "Testing wasm <const_172_wasm>", "[const_172_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_172_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_173_wasm>", "[const_173_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_173_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_174_wasm>", "[const_174_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_174_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

TEST_CASE( "Testing wasm <const_175_wasm>", "[const_175_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_175_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

TEST_CASE( "Testing wasm <const_176_wasm>", "[const_176_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_176_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

TEST_CASE( "Testing wasm <const_177_wasm>", "[const_177_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_177_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

TEST_CASE( "Testing wasm <const_178_wasm>", "[const_178_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_178_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(0));
}

TEST_CASE( "Testing wasm <const_179_wasm>", "[const_179_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_179_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483648));
}

TEST_CASE( "Testing wasm <const_17_wasm>", "[const_17_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_17_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_180_wasm>", "[const_180_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_180_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

TEST_CASE( "Testing wasm <const_181_wasm>", "[const_181_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_181_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

TEST_CASE( "Testing wasm <const_182_wasm>", "[const_182_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_182_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

TEST_CASE( "Testing wasm <const_183_wasm>", "[const_183_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_183_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

TEST_CASE( "Testing wasm <const_184_wasm>", "[const_184_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_184_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

TEST_CASE( "Testing wasm <const_185_wasm>", "[const_185_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_185_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

TEST_CASE( "Testing wasm <const_186_wasm>", "[const_186_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_186_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

TEST_CASE( "Testing wasm <const_187_wasm>", "[const_187_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_187_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

TEST_CASE( "Testing wasm <const_188_wasm>", "[const_188_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_188_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

TEST_CASE( "Testing wasm <const_189_wasm>", "[const_189_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_189_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

TEST_CASE( "Testing wasm <const_18_wasm>", "[const_18_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_18_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_190_wasm>", "[const_190_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_190_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

TEST_CASE( "Testing wasm <const_191_wasm>", "[const_191_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_191_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

TEST_CASE( "Testing wasm <const_192_wasm>", "[const_192_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_192_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

TEST_CASE( "Testing wasm <const_193_wasm>", "[const_193_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_193_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

TEST_CASE( "Testing wasm <const_194_wasm>", "[const_194_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_194_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

TEST_CASE( "Testing wasm <const_195_wasm>", "[const_195_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_195_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

TEST_CASE( "Testing wasm <const_196_wasm>", "[const_196_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_196_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

TEST_CASE( "Testing wasm <const_197_wasm>", "[const_197_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_197_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

TEST_CASE( "Testing wasm <const_198_wasm>", "[const_198_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_198_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

TEST_CASE( "Testing wasm <const_199_wasm>", "[const_199_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_199_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

TEST_CASE( "Testing wasm <const_19_wasm>", "[const_19_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_19_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_1_wasm>", "[const_1_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_1_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_200_wasm>", "[const_200_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_200_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

TEST_CASE( "Testing wasm <const_201_wasm>", "[const_201_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_201_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

TEST_CASE( "Testing wasm <const_202_wasm>", "[const_202_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_202_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

TEST_CASE( "Testing wasm <const_203_wasm>", "[const_203_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_203_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

TEST_CASE( "Testing wasm <const_204_wasm>", "[const_204_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_204_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3));
}

TEST_CASE( "Testing wasm <const_205_wasm>", "[const_205_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_205_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483651));
}

TEST_CASE( "Testing wasm <const_206_wasm>", "[const_206_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_206_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2139095039));
}

TEST_CASE( "Testing wasm <const_207_wasm>", "[const_207_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_207_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(4286578687));
}

TEST_CASE( "Testing wasm <const_208_wasm>", "[const_208_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_208_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2139095039));
}

TEST_CASE( "Testing wasm <const_209_wasm>", "[const_209_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_209_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(4286578687));
}

TEST_CASE( "Testing wasm <const_20_wasm>", "[const_20_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_20_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_210_wasm>", "[const_210_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_210_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2139095039));
}

TEST_CASE( "Testing wasm <const_211_wasm>", "[const_211_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_211_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(4286578687));
}

TEST_CASE( "Testing wasm <const_212_wasm>", "[const_212_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_212_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719808));
}

TEST_CASE( "Testing wasm <const_213_wasm>", "[const_213_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_213_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495616));
}

TEST_CASE( "Testing wasm <const_214_wasm>", "[const_214_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_214_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_215_wasm>", "[const_215_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_215_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_216_wasm>", "[const_216_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_216_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_217_wasm>", "[const_217_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_217_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_218_wasm>", "[const_218_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_218_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_219_wasm>", "[const_219_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_219_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_21_wasm>", "[const_21_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_21_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_220_wasm>", "[const_220_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_220_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_221_wasm>", "[const_221_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_221_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_222_wasm>", "[const_222_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_222_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_223_wasm>", "[const_223_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_223_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_224_wasm>", "[const_224_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_224_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_225_wasm>", "[const_225_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_225_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_226_wasm>", "[const_226_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_226_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_227_wasm>", "[const_227_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_227_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_228_wasm>", "[const_228_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_228_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_229_wasm>", "[const_229_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_229_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_22_wasm>", "[const_22_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_22_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_230_wasm>", "[const_230_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_230_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_231_wasm>", "[const_231_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_231_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_232_wasm>", "[const_232_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_232_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_233_wasm>", "[const_233_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_233_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_234_wasm>", "[const_234_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_234_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_235_wasm>", "[const_235_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_235_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_236_wasm>", "[const_236_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_236_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719811));
}

TEST_CASE( "Testing wasm <const_237_wasm>", "[const_237_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_237_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495619));
}

TEST_CASE( "Testing wasm <const_238_wasm>", "[const_238_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_238_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719808));
}

TEST_CASE( "Testing wasm <const_239_wasm>", "[const_239_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_239_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495616));
}

TEST_CASE( "Testing wasm <const_23_wasm>", "[const_23_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_23_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_240_wasm>", "[const_240_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_240_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_241_wasm>", "[const_241_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_241_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_242_wasm>", "[const_242_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_242_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_243_wasm>", "[const_243_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_243_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_244_wasm>", "[const_244_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_244_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_245_wasm>", "[const_245_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_245_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_246_wasm>", "[const_246_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_246_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_247_wasm>", "[const_247_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_247_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_248_wasm>", "[const_248_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_248_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

TEST_CASE( "Testing wasm <const_249_wasm>", "[const_249_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_249_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

TEST_CASE( "Testing wasm <const_24_wasm>", "[const_24_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_24_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_250_wasm>", "[const_250_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_250_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_251_wasm>", "[const_251_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_251_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_252_wasm>", "[const_252_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_252_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_253_wasm>", "[const_253_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_253_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_254_wasm>", "[const_254_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_254_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_255_wasm>", "[const_255_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_255_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_256_wasm>", "[const_256_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_256_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_257_wasm>", "[const_257_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_257_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_258_wasm>", "[const_258_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_258_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_259_wasm>", "[const_259_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_259_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_25_wasm>", "[const_25_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_25_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_260_wasm>", "[const_260_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_260_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

TEST_CASE( "Testing wasm <const_261_wasm>", "[const_261_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_261_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

TEST_CASE( "Testing wasm <const_262_wasm>", "[const_262_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_262_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719811));
}

TEST_CASE( "Testing wasm <const_263_wasm>", "[const_263_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_263_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495619));
}

TEST_CASE( "Testing wasm <const_264_wasm>", "[const_264_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_264_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142912));
}

TEST_CASE( "Testing wasm <const_265_wasm>", "[const_265_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_265_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918720));
}

TEST_CASE( "Testing wasm <const_266_wasm>", "[const_266_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_266_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142913));
}

TEST_CASE( "Testing wasm <const_267_wasm>", "[const_267_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_267_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918721));
}

TEST_CASE( "Testing wasm <const_268_wasm>", "[const_268_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_268_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142913));
}

TEST_CASE( "Testing wasm <const_269_wasm>", "[const_269_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_269_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918721));
}

TEST_CASE( "Testing wasm <const_270_wasm>", "[const_270_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_270_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142914));
}

TEST_CASE( "Testing wasm <const_271_wasm>", "[const_271_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_271_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918722));
}

TEST_CASE( "Testing wasm <const_272_wasm>", "[const_272_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_272_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315008));
}

TEST_CASE( "Testing wasm <const_273_wasm>", "[const_273_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_273_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090816));
}

TEST_CASE( "Testing wasm <const_274_wasm>", "[const_274_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_274_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

TEST_CASE( "Testing wasm <const_275_wasm>", "[const_275_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_275_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

TEST_CASE( "Testing wasm <const_276_wasm>", "[const_276_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_276_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

TEST_CASE( "Testing wasm <const_277_wasm>", "[const_277_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_277_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

TEST_CASE( "Testing wasm <const_278_wasm>", "[const_278_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_278_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

TEST_CASE( "Testing wasm <const_279_wasm>", "[const_279_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_279_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

TEST_CASE( "Testing wasm <const_280_wasm>", "[const_280_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_280_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

TEST_CASE( "Testing wasm <const_281_wasm>", "[const_281_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_281_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

TEST_CASE( "Testing wasm <const_282_wasm>", "[const_282_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_282_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

TEST_CASE( "Testing wasm <const_283_wasm>", "[const_283_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_283_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

TEST_CASE( "Testing wasm <const_284_wasm>", "[const_284_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_284_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

TEST_CASE( "Testing wasm <const_285_wasm>", "[const_285_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_285_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

TEST_CASE( "Testing wasm <const_286_wasm>", "[const_286_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_286_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

TEST_CASE( "Testing wasm <const_287_wasm>", "[const_287_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_287_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

TEST_CASE( "Testing wasm <const_288_wasm>", "[const_288_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_288_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

TEST_CASE( "Testing wasm <const_289_wasm>", "[const_289_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_289_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

TEST_CASE( "Testing wasm <const_290_wasm>", "[const_290_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_290_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

TEST_CASE( "Testing wasm <const_291_wasm>", "[const_291_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_291_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

TEST_CASE( "Testing wasm <const_292_wasm>", "[const_292_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_292_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

TEST_CASE( "Testing wasm <const_293_wasm>", "[const_293_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_293_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

TEST_CASE( "Testing wasm <const_294_wasm>", "[const_294_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_294_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

TEST_CASE( "Testing wasm <const_295_wasm>", "[const_295_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_295_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

TEST_CASE( "Testing wasm <const_296_wasm>", "[const_296_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_296_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

TEST_CASE( "Testing wasm <const_297_wasm>", "[const_297_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_297_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

TEST_CASE( "Testing wasm <const_298_wasm>", "[const_298_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_298_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315011));
}

TEST_CASE( "Testing wasm <const_299_wasm>", "[const_299_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_299_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090819));
}

TEST_CASE( "Testing wasm <const_300_wasm>", "[const_300_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_300_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955520));
}

TEST_CASE( "Testing wasm <const_301_wasm>", "[const_301_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_301_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731328));
}

TEST_CASE( "Testing wasm <const_302_wasm>", "[const_302_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_302_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

TEST_CASE( "Testing wasm <const_303_wasm>", "[const_303_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_303_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

TEST_CASE( "Testing wasm <const_304_wasm>", "[const_304_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_304_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

TEST_CASE( "Testing wasm <const_305_wasm>", "[const_305_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_305_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

TEST_CASE( "Testing wasm <const_306_wasm>", "[const_306_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_306_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

TEST_CASE( "Testing wasm <const_307_wasm>", "[const_307_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_307_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

TEST_CASE( "Testing wasm <const_308_wasm>", "[const_308_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_308_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

TEST_CASE( "Testing wasm <const_309_wasm>", "[const_309_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_309_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

TEST_CASE( "Testing wasm <const_30_wasm>", "[const_30_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_30_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_310_wasm>", "[const_310_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_310_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

TEST_CASE( "Testing wasm <const_311_wasm>", "[const_311_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_311_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

TEST_CASE( "Testing wasm <const_312_wasm>", "[const_312_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_312_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

TEST_CASE( "Testing wasm <const_313_wasm>", "[const_313_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_313_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

TEST_CASE( "Testing wasm <const_314_wasm>", "[const_314_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_314_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

TEST_CASE( "Testing wasm <const_315_wasm>", "[const_315_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_315_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

TEST_CASE( "Testing wasm <const_316_wasm>", "[const_316_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_316_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

TEST_CASE( "Testing wasm <const_317_wasm>", "[const_317_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_317_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

TEST_CASE( "Testing wasm <const_318_wasm>", "[const_318_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_318_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

TEST_CASE( "Testing wasm <const_319_wasm>", "[const_319_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_319_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

TEST_CASE( "Testing wasm <const_31_wasm>", "[const_31_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_31_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_320_wasm>", "[const_320_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_320_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

TEST_CASE( "Testing wasm <const_321_wasm>", "[const_321_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_321_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

TEST_CASE( "Testing wasm <const_322_wasm>", "[const_322_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_322_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

TEST_CASE( "Testing wasm <const_323_wasm>", "[const_323_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_323_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

TEST_CASE( "Testing wasm <const_324_wasm>", "[const_324_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_324_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

TEST_CASE( "Testing wasm <const_325_wasm>", "[const_325_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_325_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

TEST_CASE( "Testing wasm <const_326_wasm>", "[const_326_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_326_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955523));
}

TEST_CASE( "Testing wasm <const_327_wasm>", "[const_327_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_327_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731331));
}

TEST_CASE( "Testing wasm <const_328_wasm>", "[const_328_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_328_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247168));
}

TEST_CASE( "Testing wasm <const_329_wasm>", "[const_329_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_329_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022976));
}

TEST_CASE( "Testing wasm <const_330_wasm>", "[const_330_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_330_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247169));
}

TEST_CASE( "Testing wasm <const_331_wasm>", "[const_331_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_331_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022977));
}

TEST_CASE( "Testing wasm <const_332_wasm>", "[const_332_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_332_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247169));
}

TEST_CASE( "Testing wasm <const_333_wasm>", "[const_333_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_333_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022977));
}

TEST_CASE( "Testing wasm <const_334_wasm>", "[const_334_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_334_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247170));
}

TEST_CASE( "Testing wasm <const_335_wasm>", "[const_335_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_335_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022978));
}

TEST_CASE( "Testing wasm <const_336_wasm>", "[const_336_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_336_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(0));
}

TEST_CASE( "Testing wasm <const_337_wasm>", "[const_337_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_337_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775808));
}

TEST_CASE( "Testing wasm <const_338_wasm>", "[const_338_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_338_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

TEST_CASE( "Testing wasm <const_339_wasm>", "[const_339_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_339_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

TEST_CASE( "Testing wasm <const_340_wasm>", "[const_340_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_340_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

TEST_CASE( "Testing wasm <const_341_wasm>", "[const_341_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_341_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

TEST_CASE( "Testing wasm <const_342_wasm>", "[const_342_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_342_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

TEST_CASE( "Testing wasm <const_343_wasm>", "[const_343_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_343_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

TEST_CASE( "Testing wasm <const_344_wasm>", "[const_344_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_344_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

TEST_CASE( "Testing wasm <const_345_wasm>", "[const_345_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_345_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

TEST_CASE( "Testing wasm <const_346_wasm>", "[const_346_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_346_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

TEST_CASE( "Testing wasm <const_347_wasm>", "[const_347_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_347_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

TEST_CASE( "Testing wasm <const_348_wasm>", "[const_348_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_348_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

TEST_CASE( "Testing wasm <const_349_wasm>", "[const_349_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_349_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

TEST_CASE( "Testing wasm <const_34_wasm>", "[const_34_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_34_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_350_wasm>", "[const_350_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_350_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

TEST_CASE( "Testing wasm <const_351_wasm>", "[const_351_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_351_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

TEST_CASE( "Testing wasm <const_352_wasm>", "[const_352_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_352_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

TEST_CASE( "Testing wasm <const_353_wasm>", "[const_353_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_353_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

TEST_CASE( "Testing wasm <const_354_wasm>", "[const_354_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_354_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

TEST_CASE( "Testing wasm <const_355_wasm>", "[const_355_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_355_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

TEST_CASE( "Testing wasm <const_356_wasm>", "[const_356_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_356_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

TEST_CASE( "Testing wasm <const_357_wasm>", "[const_357_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_357_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

TEST_CASE( "Testing wasm <const_358_wasm>", "[const_358_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_358_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

TEST_CASE( "Testing wasm <const_359_wasm>", "[const_359_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_359_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

TEST_CASE( "Testing wasm <const_35_wasm>", "[const_35_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_35_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_360_wasm>", "[const_360_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_360_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

TEST_CASE( "Testing wasm <const_361_wasm>", "[const_361_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_361_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

TEST_CASE( "Testing wasm <const_362_wasm>", "[const_362_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_362_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4503599627370499));
}

TEST_CASE( "Testing wasm <const_363_wasm>", "[const_363_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_363_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9227875636482146307));
}

TEST_CASE( "Testing wasm <const_364_wasm>", "[const_364_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_364_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9218868437227405311));
}

TEST_CASE( "Testing wasm <const_365_wasm>", "[const_365_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_365_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18442240474082181119));
}

TEST_CASE( "Testing wasm <const_366_wasm>", "[const_366_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_366_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9218868437227405311));
}

TEST_CASE( "Testing wasm <const_367_wasm>", "[const_367_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_367_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint64_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18442240474082181119));
}

TEST_CASE( "Testing wasm <const_38_wasm>", "[const_38_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_38_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_39_wasm>", "[const_39_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_39_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_40_wasm>", "[const_40_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_40_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_41_wasm>", "[const_41_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_41_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_42_wasm>", "[const_42_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_42_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_43_wasm>", "[const_43_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_43_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_44_wasm>", "[const_44_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_44_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_45_wasm>", "[const_45_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_45_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_4_wasm>", "[const_4_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_4_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_50_wasm>", "[const_50_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_50_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_51_wasm>", "[const_51_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_51_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_54_wasm>", "[const_54_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_54_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_55_wasm>", "[const_55_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_55_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_58_wasm>", "[const_58_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_58_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_59_wasm>", "[const_59_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_59_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_5_wasm>", "[const_5_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_5_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_60_wasm>", "[const_60_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_60_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_61_wasm>", "[const_61_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_61_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_68_wasm>", "[const_68_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_68_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922816));
}

TEST_CASE( "Testing wasm <const_69_wasm>", "[const_69_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_69_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406464));
}

TEST_CASE( "Testing wasm <const_70_wasm>", "[const_70_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_70_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_71_wasm>", "[const_71_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_71_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_72_wasm>", "[const_72_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_72_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_73_wasm>", "[const_73_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_73_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_74_wasm>", "[const_74_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_74_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_75_wasm>", "[const_75_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_75_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_76_wasm>", "[const_76_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_76_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_77_wasm>", "[const_77_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_77_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_78_wasm>", "[const_78_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_78_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_79_wasm>", "[const_79_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_79_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_80_wasm>", "[const_80_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_80_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_81_wasm>", "[const_81_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_81_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_82_wasm>", "[const_82_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_82_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_83_wasm>", "[const_83_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_83_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_84_wasm>", "[const_84_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_84_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_85_wasm>", "[const_85_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_85_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_86_wasm>", "[const_86_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_86_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_87_wasm>", "[const_87_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_87_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_88_wasm>", "[const_88_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_88_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_89_wasm>", "[const_89_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_89_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_8_wasm>", "[const_8_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_8_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

TEST_CASE( "Testing wasm <const_90_wasm>", "[const_90_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_90_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_91_wasm>", "[const_91_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_91_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_92_wasm>", "[const_92_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_92_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

TEST_CASE( "Testing wasm <const_93_wasm>", "[const_93_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_93_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

TEST_CASE( "Testing wasm <const_94_wasm>", "[const_94_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_94_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922819));
}

TEST_CASE( "Testing wasm <const_95_wasm>", "[const_95_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_95_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406467));
}

TEST_CASE( "Testing wasm <const_96_wasm>", "[const_96_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_96_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922816));
}

TEST_CASE( "Testing wasm <const_97_wasm>", "[const_97_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_97_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406464));
}

TEST_CASE( "Testing wasm <const_98_wasm>", "[const_98_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_98_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

TEST_CASE( "Testing wasm <const_99_wasm>", "[const_99_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_99_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

   CHECK(bit_cast<uint32_t>(bkend.initialize(nullptr).call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

TEST_CASE( "Testing wasm <const_9_wasm>", "[const_9_wasm_tests]" ) {
   auto code = backend_t::read_wasm( const_9_wasm );
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );

}

