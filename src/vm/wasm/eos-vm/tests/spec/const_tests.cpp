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

BACKEND_TEST_CASE( "Testing wasm <const_0_wasm>", "[const_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.0.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_1_wasm>", "[const_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.1.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_100_wasm>", "[const_100_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.100.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_101_wasm>", "[const_101_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.101.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_102_wasm>", "[const_102_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.102.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_103_wasm>", "[const_103_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.103.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_104_wasm>", "[const_104_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.104.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_105_wasm>", "[const_105_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.105.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_106_wasm>", "[const_106_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.106.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_107_wasm>", "[const_107_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.107.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_108_wasm>", "[const_108_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.108.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_109_wasm>", "[const_109_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.109.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_110_wasm>", "[const_110_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.110.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_111_wasm>", "[const_111_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.111.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_112_wasm>", "[const_112_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.112.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_113_wasm>", "[const_113_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.113.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_114_wasm>", "[const_114_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.114.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_115_wasm>", "[const_115_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.115.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_116_wasm>", "[const_116_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.116.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_117_wasm>", "[const_117_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.117.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_118_wasm>", "[const_118_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.118.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922819));
}

BACKEND_TEST_CASE( "Testing wasm <const_119_wasm>", "[const_119_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.119.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406467));
}

BACKEND_TEST_CASE( "Testing wasm <const_12_wasm>", "[const_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.12.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_120_wasm>", "[const_120_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.120.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922816));
}

BACKEND_TEST_CASE( "Testing wasm <const_121_wasm>", "[const_121_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.121.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406464));
}

BACKEND_TEST_CASE( "Testing wasm <const_122_wasm>", "[const_122_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.122.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_123_wasm>", "[const_123_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.123.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_124_wasm>", "[const_124_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.124.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_125_wasm>", "[const_125_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.125.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_126_wasm>", "[const_126_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.126.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_127_wasm>", "[const_127_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.127.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_128_wasm>", "[const_128_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.128.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783616));
}

BACKEND_TEST_CASE( "Testing wasm <const_129_wasm>", "[const_129_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.129.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267264));
}

BACKEND_TEST_CASE( "Testing wasm <const_13_wasm>", "[const_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.13.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_130_wasm>", "[const_130_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.130.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_131_wasm>", "[const_131_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.131.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_132_wasm>", "[const_132_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.132.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_133_wasm>", "[const_133_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.133.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_134_wasm>", "[const_134_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.134.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_135_wasm>", "[const_135_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.135.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_136_wasm>", "[const_136_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.136.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_137_wasm>", "[const_137_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.137.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_138_wasm>", "[const_138_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.138.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_139_wasm>", "[const_139_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.139.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_140_wasm>", "[const_140_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.140.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_141_wasm>", "[const_141_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.141.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_142_wasm>", "[const_142_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.142.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_143_wasm>", "[const_143_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.143.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_144_wasm>", "[const_144_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.144.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_145_wasm>", "[const_145_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.145.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_146_wasm>", "[const_146_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.146.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_147_wasm>", "[const_147_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.147.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_148_wasm>", "[const_148_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.148.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_149_wasm>", "[const_149_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.149.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_150_wasm>", "[const_150_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.150.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_151_wasm>", "[const_151_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.151.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_152_wasm>", "[const_152_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.152.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_153_wasm>", "[const_153_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.153.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_154_wasm>", "[const_154_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.154.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783619));
}

BACKEND_TEST_CASE( "Testing wasm <const_155_wasm>", "[const_155_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.155.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267267));
}

BACKEND_TEST_CASE( "Testing wasm <const_156_wasm>", "[const_156_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.156.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783616));
}

BACKEND_TEST_CASE( "Testing wasm <const_157_wasm>", "[const_157_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.157.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267264));
}

BACKEND_TEST_CASE( "Testing wasm <const_158_wasm>", "[const_158_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.158.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_159_wasm>", "[const_159_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.159.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_16_wasm>", "[const_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.16.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_160_wasm>", "[const_160_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.160.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_161_wasm>", "[const_161_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.161.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_162_wasm>", "[const_162_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.162.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_163_wasm>", "[const_163_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.163.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_164_wasm>", "[const_164_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.164.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_165_wasm>", "[const_165_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.165.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_166_wasm>", "[const_166_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.166.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_167_wasm>", "[const_167_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.167.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_168_wasm>", "[const_168_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.168.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_169_wasm>", "[const_169_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.169.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_17_wasm>", "[const_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.17.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_170_wasm>", "[const_170_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.170.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783616));
}

BACKEND_TEST_CASE( "Testing wasm <const_171_wasm>", "[const_171_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.171.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267264));
}

BACKEND_TEST_CASE( "Testing wasm <const_172_wasm>", "[const_172_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.172.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_173_wasm>", "[const_173_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.173.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_174_wasm>", "[const_174_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.174.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783617));
}

BACKEND_TEST_CASE( "Testing wasm <const_175_wasm>", "[const_175_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.175.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267265));
}

BACKEND_TEST_CASE( "Testing wasm <const_176_wasm>", "[const_176_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.176.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1484783618));
}

BACKEND_TEST_CASE( "Testing wasm <const_177_wasm>", "[const_177_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.177.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3632267266));
}

BACKEND_TEST_CASE( "Testing wasm <const_178_wasm>", "[const_178_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.178.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <const_179_wasm>", "[const_179_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.179.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483648));
}

BACKEND_TEST_CASE( "Testing wasm <const_18_wasm>", "[const_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.18.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_180_wasm>", "[const_180_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.180.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_181_wasm>", "[const_181_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.181.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

BACKEND_TEST_CASE( "Testing wasm <const_182_wasm>", "[const_182_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.182.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_183_wasm>", "[const_183_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.183.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

BACKEND_TEST_CASE( "Testing wasm <const_184_wasm>", "[const_184_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.184.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_185_wasm>", "[const_185_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.185.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

BACKEND_TEST_CASE( "Testing wasm <const_186_wasm>", "[const_186_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.186.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_187_wasm>", "[const_187_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.187.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

BACKEND_TEST_CASE( "Testing wasm <const_188_wasm>", "[const_188_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.188.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_189_wasm>", "[const_189_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.189.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483649));
}

BACKEND_TEST_CASE( "Testing wasm <const_19_wasm>", "[const_19_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.19.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_190_wasm>", "[const_190_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.190.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_191_wasm>", "[const_191_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.191.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

BACKEND_TEST_CASE( "Testing wasm <const_192_wasm>", "[const_192_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.192.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_193_wasm>", "[const_193_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.193.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

BACKEND_TEST_CASE( "Testing wasm <const_194_wasm>", "[const_194_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.194.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_195_wasm>", "[const_195_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.195.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

BACKEND_TEST_CASE( "Testing wasm <const_196_wasm>", "[const_196_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.196.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_197_wasm>", "[const_197_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.197.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

BACKEND_TEST_CASE( "Testing wasm <const_198_wasm>", "[const_198_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.198.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_199_wasm>", "[const_199_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.199.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

BACKEND_TEST_CASE( "Testing wasm <const_20_wasm>", "[const_20_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.20.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_200_wasm>", "[const_200_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.200.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_201_wasm>", "[const_201_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.201.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

BACKEND_TEST_CASE( "Testing wasm <const_202_wasm>", "[const_202_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.202.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_203_wasm>", "[const_203_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.203.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483650));
}

BACKEND_TEST_CASE( "Testing wasm <const_204_wasm>", "[const_204_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.204.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(3));
}

BACKEND_TEST_CASE( "Testing wasm <const_205_wasm>", "[const_205_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.205.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2147483651));
}

BACKEND_TEST_CASE( "Testing wasm <const_206_wasm>", "[const_206_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.206.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2139095039));
}

BACKEND_TEST_CASE( "Testing wasm <const_207_wasm>", "[const_207_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.207.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(4286578687));
}

BACKEND_TEST_CASE( "Testing wasm <const_208_wasm>", "[const_208_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.208.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2139095039));
}

BACKEND_TEST_CASE( "Testing wasm <const_209_wasm>", "[const_209_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.209.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(4286578687));
}

BACKEND_TEST_CASE( "Testing wasm <const_21_wasm>", "[const_21_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.21.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_210_wasm>", "[const_210_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.210.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2139095039));
}

BACKEND_TEST_CASE( "Testing wasm <const_211_wasm>", "[const_211_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.211.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(4286578687));
}

BACKEND_TEST_CASE( "Testing wasm <const_212_wasm>", "[const_212_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.212.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719808));
}

BACKEND_TEST_CASE( "Testing wasm <const_213_wasm>", "[const_213_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.213.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495616));
}

BACKEND_TEST_CASE( "Testing wasm <const_214_wasm>", "[const_214_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.214.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_215_wasm>", "[const_215_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.215.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_216_wasm>", "[const_216_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.216.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_217_wasm>", "[const_217_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.217.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_218_wasm>", "[const_218_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.218.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_219_wasm>", "[const_219_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.219.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_22_wasm>", "[const_22_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.22.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_220_wasm>", "[const_220_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.220.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_221_wasm>", "[const_221_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.221.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_222_wasm>", "[const_222_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.222.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_223_wasm>", "[const_223_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.223.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_224_wasm>", "[const_224_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.224.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_225_wasm>", "[const_225_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.225.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_226_wasm>", "[const_226_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.226.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_227_wasm>", "[const_227_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.227.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_228_wasm>", "[const_228_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.228.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_229_wasm>", "[const_229_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.229.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_23_wasm>", "[const_23_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.23.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_230_wasm>", "[const_230_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.230.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_231_wasm>", "[const_231_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.231.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_232_wasm>", "[const_232_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.232.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_233_wasm>", "[const_233_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.233.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_234_wasm>", "[const_234_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.234.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_235_wasm>", "[const_235_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.235.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_236_wasm>", "[const_236_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.236.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719811));
}

BACKEND_TEST_CASE( "Testing wasm <const_237_wasm>", "[const_237_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.237.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495619));
}

BACKEND_TEST_CASE( "Testing wasm <const_238_wasm>", "[const_238_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.238.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719808));
}

BACKEND_TEST_CASE( "Testing wasm <const_239_wasm>", "[const_239_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.239.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495616));
}

BACKEND_TEST_CASE( "Testing wasm <const_24_wasm>", "[const_24_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.24.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_240_wasm>", "[const_240_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.240.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_241_wasm>", "[const_241_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.241.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_242_wasm>", "[const_242_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.242.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_243_wasm>", "[const_243_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.243.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_244_wasm>", "[const_244_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.244.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_245_wasm>", "[const_245_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.245.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_246_wasm>", "[const_246_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.246.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_247_wasm>", "[const_247_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.247.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_248_wasm>", "[const_248_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.248.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719809));
}

BACKEND_TEST_CASE( "Testing wasm <const_249_wasm>", "[const_249_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.249.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495617));
}

BACKEND_TEST_CASE( "Testing wasm <const_25_wasm>", "[const_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.25.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_250_wasm>", "[const_250_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.250.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_251_wasm>", "[const_251_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.251.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_252_wasm>", "[const_252_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.252.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_253_wasm>", "[const_253_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.253.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_254_wasm>", "[const_254_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.254.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_255_wasm>", "[const_255_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.255.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_256_wasm>", "[const_256_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.256.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_257_wasm>", "[const_257_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.257.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_258_wasm>", "[const_258_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.258.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_259_wasm>", "[const_259_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.259.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_260_wasm>", "[const_260_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.260.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719810));
}

BACKEND_TEST_CASE( "Testing wasm <const_261_wasm>", "[const_261_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.261.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495618));
}

BACKEND_TEST_CASE( "Testing wasm <const_262_wasm>", "[const_262_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.262.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1905022642377719811));
}

BACKEND_TEST_CASE( "Testing wasm <const_263_wasm>", "[const_263_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.263.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(11128394679232495619));
}

BACKEND_TEST_CASE( "Testing wasm <const_264_wasm>", "[const_264_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.264.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142912));
}

BACKEND_TEST_CASE( "Testing wasm <const_265_wasm>", "[const_265_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.265.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918720));
}

BACKEND_TEST_CASE( "Testing wasm <const_266_wasm>", "[const_266_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.266.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142913));
}

BACKEND_TEST_CASE( "Testing wasm <const_267_wasm>", "[const_267_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.267.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918721));
}

BACKEND_TEST_CASE( "Testing wasm <const_268_wasm>", "[const_268_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.268.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142913));
}

BACKEND_TEST_CASE( "Testing wasm <const_269_wasm>", "[const_269_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.269.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918721));
}

BACKEND_TEST_CASE( "Testing wasm <const_270_wasm>", "[const_270_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.270.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9106278446543142914));
}

BACKEND_TEST_CASE( "Testing wasm <const_271_wasm>", "[const_271_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.271.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18329650483397918722));
}

BACKEND_TEST_CASE( "Testing wasm <const_272_wasm>", "[const_272_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.272.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315008));
}

BACKEND_TEST_CASE( "Testing wasm <const_273_wasm>", "[const_273_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.273.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090816));
}

BACKEND_TEST_CASE( "Testing wasm <const_274_wasm>", "[const_274_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.274.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

BACKEND_TEST_CASE( "Testing wasm <const_275_wasm>", "[const_275_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.275.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

BACKEND_TEST_CASE( "Testing wasm <const_276_wasm>", "[const_276_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.276.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

BACKEND_TEST_CASE( "Testing wasm <const_277_wasm>", "[const_277_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.277.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

BACKEND_TEST_CASE( "Testing wasm <const_278_wasm>", "[const_278_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.278.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

BACKEND_TEST_CASE( "Testing wasm <const_279_wasm>", "[const_279_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.279.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

BACKEND_TEST_CASE( "Testing wasm <const_280_wasm>", "[const_280_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.280.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

BACKEND_TEST_CASE( "Testing wasm <const_281_wasm>", "[const_281_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.281.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

BACKEND_TEST_CASE( "Testing wasm <const_282_wasm>", "[const_282_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.282.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315009));
}

BACKEND_TEST_CASE( "Testing wasm <const_283_wasm>", "[const_283_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.283.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090817));
}

BACKEND_TEST_CASE( "Testing wasm <const_284_wasm>", "[const_284_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.284.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

BACKEND_TEST_CASE( "Testing wasm <const_285_wasm>", "[const_285_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.285.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

BACKEND_TEST_CASE( "Testing wasm <const_286_wasm>", "[const_286_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.286.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

BACKEND_TEST_CASE( "Testing wasm <const_287_wasm>", "[const_287_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.287.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

BACKEND_TEST_CASE( "Testing wasm <const_288_wasm>", "[const_288_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.288.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

BACKEND_TEST_CASE( "Testing wasm <const_289_wasm>", "[const_289_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.289.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

BACKEND_TEST_CASE( "Testing wasm <const_290_wasm>", "[const_290_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.290.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

BACKEND_TEST_CASE( "Testing wasm <const_291_wasm>", "[const_291_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.291.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

BACKEND_TEST_CASE( "Testing wasm <const_292_wasm>", "[const_292_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.292.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

BACKEND_TEST_CASE( "Testing wasm <const_293_wasm>", "[const_293_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.293.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

BACKEND_TEST_CASE( "Testing wasm <const_294_wasm>", "[const_294_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.294.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

BACKEND_TEST_CASE( "Testing wasm <const_295_wasm>", "[const_295_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.295.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

BACKEND_TEST_CASE( "Testing wasm <const_296_wasm>", "[const_296_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.296.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315010));
}

BACKEND_TEST_CASE( "Testing wasm <const_297_wasm>", "[const_297_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.297.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090818));
}

BACKEND_TEST_CASE( "Testing wasm <const_298_wasm>", "[const_298_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.298.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(7309342195222315011));
}

BACKEND_TEST_CASE( "Testing wasm <const_299_wasm>", "[const_299_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.299.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(16532714232077090819));
}

BACKEND_TEST_CASE( "Testing wasm <const_30_wasm>", "[const_30_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.30.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_300_wasm>", "[const_300_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.300.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955520));
}

BACKEND_TEST_CASE( "Testing wasm <const_301_wasm>", "[const_301_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.301.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731328));
}

BACKEND_TEST_CASE( "Testing wasm <const_302_wasm>", "[const_302_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.302.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

BACKEND_TEST_CASE( "Testing wasm <const_303_wasm>", "[const_303_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.303.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

BACKEND_TEST_CASE( "Testing wasm <const_304_wasm>", "[const_304_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.304.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

BACKEND_TEST_CASE( "Testing wasm <const_305_wasm>", "[const_305_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.305.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

BACKEND_TEST_CASE( "Testing wasm <const_306_wasm>", "[const_306_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.306.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

BACKEND_TEST_CASE( "Testing wasm <const_307_wasm>", "[const_307_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.307.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

BACKEND_TEST_CASE( "Testing wasm <const_308_wasm>", "[const_308_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.308.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

BACKEND_TEST_CASE( "Testing wasm <const_309_wasm>", "[const_309_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.309.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

BACKEND_TEST_CASE( "Testing wasm <const_31_wasm>", "[const_31_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.31.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_310_wasm>", "[const_310_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.310.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955521));
}

BACKEND_TEST_CASE( "Testing wasm <const_311_wasm>", "[const_311_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.311.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731329));
}

BACKEND_TEST_CASE( "Testing wasm <const_312_wasm>", "[const_312_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.312.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

BACKEND_TEST_CASE( "Testing wasm <const_313_wasm>", "[const_313_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.313.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

BACKEND_TEST_CASE( "Testing wasm <const_314_wasm>", "[const_314_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.314.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

BACKEND_TEST_CASE( "Testing wasm <const_315_wasm>", "[const_315_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.315.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

BACKEND_TEST_CASE( "Testing wasm <const_316_wasm>", "[const_316_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.316.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

BACKEND_TEST_CASE( "Testing wasm <const_317_wasm>", "[const_317_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.317.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

BACKEND_TEST_CASE( "Testing wasm <const_318_wasm>", "[const_318_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.318.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

BACKEND_TEST_CASE( "Testing wasm <const_319_wasm>", "[const_319_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.319.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

BACKEND_TEST_CASE( "Testing wasm <const_320_wasm>", "[const_320_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.320.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

BACKEND_TEST_CASE( "Testing wasm <const_321_wasm>", "[const_321_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.321.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

BACKEND_TEST_CASE( "Testing wasm <const_322_wasm>", "[const_322_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.322.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

BACKEND_TEST_CASE( "Testing wasm <const_323_wasm>", "[const_323_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.323.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

BACKEND_TEST_CASE( "Testing wasm <const_324_wasm>", "[const_324_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.324.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955522));
}

BACKEND_TEST_CASE( "Testing wasm <const_325_wasm>", "[const_325_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.325.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731330));
}

BACKEND_TEST_CASE( "Testing wasm <const_326_wasm>", "[const_326_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.326.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(5044031582654955523));
}

BACKEND_TEST_CASE( "Testing wasm <const_327_wasm>", "[const_327_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.327.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14267403619509731331));
}

BACKEND_TEST_CASE( "Testing wasm <const_328_wasm>", "[const_328_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.328.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247168));
}

BACKEND_TEST_CASE( "Testing wasm <const_329_wasm>", "[const_329_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.329.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022976));
}

BACKEND_TEST_CASE( "Testing wasm <const_330_wasm>", "[const_330_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.330.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247169));
}

BACKEND_TEST_CASE( "Testing wasm <const_331_wasm>", "[const_331_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.331.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022977));
}

BACKEND_TEST_CASE( "Testing wasm <const_332_wasm>", "[const_332_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.332.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247169));
}

BACKEND_TEST_CASE( "Testing wasm <const_333_wasm>", "[const_333_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.333.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022977));
}

BACKEND_TEST_CASE( "Testing wasm <const_334_wasm>", "[const_334_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.334.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4877398396442247170));
}

BACKEND_TEST_CASE( "Testing wasm <const_335_wasm>", "[const_335_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.335.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(14100770433297022978));
}

BACKEND_TEST_CASE( "Testing wasm <const_336_wasm>", "[const_336_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.336.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(0));
}

BACKEND_TEST_CASE( "Testing wasm <const_337_wasm>", "[const_337_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.337.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775808));
}

BACKEND_TEST_CASE( "Testing wasm <const_338_wasm>", "[const_338_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.338.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_339_wasm>", "[const_339_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.339.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

BACKEND_TEST_CASE( "Testing wasm <const_34_wasm>", "[const_34_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.34.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_340_wasm>", "[const_340_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.340.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_341_wasm>", "[const_341_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.341.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

BACKEND_TEST_CASE( "Testing wasm <const_342_wasm>", "[const_342_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.342.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_343_wasm>", "[const_343_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.343.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

BACKEND_TEST_CASE( "Testing wasm <const_344_wasm>", "[const_344_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.344.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_345_wasm>", "[const_345_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.345.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

BACKEND_TEST_CASE( "Testing wasm <const_346_wasm>", "[const_346_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.346.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(1));
}

BACKEND_TEST_CASE( "Testing wasm <const_347_wasm>", "[const_347_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.347.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775809));
}

BACKEND_TEST_CASE( "Testing wasm <const_348_wasm>", "[const_348_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.348.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_349_wasm>", "[const_349_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.349.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

BACKEND_TEST_CASE( "Testing wasm <const_35_wasm>", "[const_35_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.35.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_350_wasm>", "[const_350_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.350.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_351_wasm>", "[const_351_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.351.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

BACKEND_TEST_CASE( "Testing wasm <const_352_wasm>", "[const_352_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.352.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_353_wasm>", "[const_353_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.353.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

BACKEND_TEST_CASE( "Testing wasm <const_354_wasm>", "[const_354_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.354.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_355_wasm>", "[const_355_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.355.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

BACKEND_TEST_CASE( "Testing wasm <const_356_wasm>", "[const_356_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.356.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_357_wasm>", "[const_357_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.357.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

BACKEND_TEST_CASE( "Testing wasm <const_358_wasm>", "[const_358_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.358.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_359_wasm>", "[const_359_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.359.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

BACKEND_TEST_CASE( "Testing wasm <const_360_wasm>", "[const_360_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.360.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(2));
}

BACKEND_TEST_CASE( "Testing wasm <const_361_wasm>", "[const_361_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.361.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9223372036854775810));
}

BACKEND_TEST_CASE( "Testing wasm <const_362_wasm>", "[const_362_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.362.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(4503599627370499));
}

BACKEND_TEST_CASE( "Testing wasm <const_363_wasm>", "[const_363_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.363.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9227875636482146307));
}

BACKEND_TEST_CASE( "Testing wasm <const_364_wasm>", "[const_364_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.364.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9218868437227405311));
}

BACKEND_TEST_CASE( "Testing wasm <const_365_wasm>", "[const_365_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.365.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18442240474082181119));
}

BACKEND_TEST_CASE( "Testing wasm <const_366_wasm>", "[const_366_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.366.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(9218868437227405311));
}

BACKEND_TEST_CASE( "Testing wasm <const_367_wasm>", "[const_367_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.367.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint64_t>(bkend.call_with_return(nullptr, "env", "f")->to_f64()) == UINT64_C(18442240474082181119));
}

BACKEND_TEST_CASE( "Testing wasm <const_38_wasm>", "[const_38_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.38.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_39_wasm>", "[const_39_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.39.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_4_wasm>", "[const_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.4.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_40_wasm>", "[const_40_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.40.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_41_wasm>", "[const_41_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.41.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_42_wasm>", "[const_42_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.42.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_43_wasm>", "[const_43_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.43.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_44_wasm>", "[const_44_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.44.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_45_wasm>", "[const_45_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.45.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_5_wasm>", "[const_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.5.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_50_wasm>", "[const_50_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.50.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_51_wasm>", "[const_51_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.51.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_54_wasm>", "[const_54_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.54.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_55_wasm>", "[const_55_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.55.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_58_wasm>", "[const_58_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.58.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_59_wasm>", "[const_59_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.59.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_60_wasm>", "[const_60_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.60.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_61_wasm>", "[const_61_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.61.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_68_wasm>", "[const_68_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.68.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922816));
}

BACKEND_TEST_CASE( "Testing wasm <const_69_wasm>", "[const_69_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.69.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406464));
}

BACKEND_TEST_CASE( "Testing wasm <const_70_wasm>", "[const_70_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.70.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_71_wasm>", "[const_71_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.71.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_72_wasm>", "[const_72_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.72.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_73_wasm>", "[const_73_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.73.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_74_wasm>", "[const_74_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.74.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_75_wasm>", "[const_75_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.75.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_76_wasm>", "[const_76_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.76.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_77_wasm>", "[const_77_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.77.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_78_wasm>", "[const_78_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.78.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_79_wasm>", "[const_79_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.79.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

BACKEND_TEST_CASE( "Testing wasm <const_8_wasm>", "[const_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.8.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_80_wasm>", "[const_80_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.80.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_81_wasm>", "[const_81_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.81.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_82_wasm>", "[const_82_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.82.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_83_wasm>", "[const_83_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.83.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_84_wasm>", "[const_84_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.84.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_85_wasm>", "[const_85_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.85.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_86_wasm>", "[const_86_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.86.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_87_wasm>", "[const_87_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.87.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_88_wasm>", "[const_88_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.88.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_89_wasm>", "[const_89_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.89.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_9_wasm>", "[const_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.9.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

}

BACKEND_TEST_CASE( "Testing wasm <const_90_wasm>", "[const_90_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.90.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_91_wasm>", "[const_91_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.91.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_92_wasm>", "[const_92_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.92.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922818));
}

BACKEND_TEST_CASE( "Testing wasm <const_93_wasm>", "[const_93_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.93.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406466));
}

BACKEND_TEST_CASE( "Testing wasm <const_94_wasm>", "[const_94_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.94.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922819));
}

BACKEND_TEST_CASE( "Testing wasm <const_95_wasm>", "[const_95_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.95.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406467));
}

BACKEND_TEST_CASE( "Testing wasm <const_96_wasm>", "[const_96_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.96.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922816));
}

BACKEND_TEST_CASE( "Testing wasm <const_97_wasm>", "[const_97_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.97.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406464));
}

BACKEND_TEST_CASE( "Testing wasm <const_98_wasm>", "[const_98_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.98.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(645922817));
}

BACKEND_TEST_CASE( "Testing wasm <const_99_wasm>", "[const_99_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "const.99.wasm");
   backend_t bkend( code );
   bkend.set_wasm_allocator( &wa );
   bkend.initialize(nullptr);

   CHECK(bit_cast<uint32_t>(bkend.call_with_return(nullptr, "env", "f")->to_f32()) == UINT32_C(2793406465));
}

