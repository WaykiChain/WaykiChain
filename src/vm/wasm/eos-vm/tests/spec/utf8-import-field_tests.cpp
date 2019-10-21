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

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_0_wasm>", "[utf8-import-field_0_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.0.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_1_wasm>", "[utf8-import-field_1_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.1.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_10_wasm>", "[utf8-import-field_10_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.10.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_100_wasm>", "[utf8-import-field_100_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.100.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_101_wasm>", "[utf8-import-field_101_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.101.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_102_wasm>", "[utf8-import-field_102_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.102.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_103_wasm>", "[utf8-import-field_103_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.103.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_104_wasm>", "[utf8-import-field_104_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.104.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_105_wasm>", "[utf8-import-field_105_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.105.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_106_wasm>", "[utf8-import-field_106_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.106.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_107_wasm>", "[utf8-import-field_107_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.107.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_108_wasm>", "[utf8-import-field_108_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.108.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_109_wasm>", "[utf8-import-field_109_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.109.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_11_wasm>", "[utf8-import-field_11_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.11.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_110_wasm>", "[utf8-import-field_110_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.110.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_111_wasm>", "[utf8-import-field_111_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.111.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_112_wasm>", "[utf8-import-field_112_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.112.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_113_wasm>", "[utf8-import-field_113_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.113.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_114_wasm>", "[utf8-import-field_114_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.114.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_115_wasm>", "[utf8-import-field_115_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.115.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_116_wasm>", "[utf8-import-field_116_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.116.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_117_wasm>", "[utf8-import-field_117_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.117.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_118_wasm>", "[utf8-import-field_118_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.118.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_119_wasm>", "[utf8-import-field_119_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.119.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_12_wasm>", "[utf8-import-field_12_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.12.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_120_wasm>", "[utf8-import-field_120_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.120.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_121_wasm>", "[utf8-import-field_121_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.121.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_122_wasm>", "[utf8-import-field_122_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.122.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_123_wasm>", "[utf8-import-field_123_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.123.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_124_wasm>", "[utf8-import-field_124_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.124.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_125_wasm>", "[utf8-import-field_125_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.125.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_126_wasm>", "[utf8-import-field_126_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.126.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_127_wasm>", "[utf8-import-field_127_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.127.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_128_wasm>", "[utf8-import-field_128_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.128.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_129_wasm>", "[utf8-import-field_129_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.129.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_13_wasm>", "[utf8-import-field_13_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.13.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_130_wasm>", "[utf8-import-field_130_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.130.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_131_wasm>", "[utf8-import-field_131_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.131.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_132_wasm>", "[utf8-import-field_132_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.132.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_133_wasm>", "[utf8-import-field_133_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.133.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_134_wasm>", "[utf8-import-field_134_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.134.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_135_wasm>", "[utf8-import-field_135_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.135.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_136_wasm>", "[utf8-import-field_136_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.136.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_137_wasm>", "[utf8-import-field_137_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.137.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_138_wasm>", "[utf8-import-field_138_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.138.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_139_wasm>", "[utf8-import-field_139_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.139.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_14_wasm>", "[utf8-import-field_14_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.14.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_140_wasm>", "[utf8-import-field_140_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.140.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_141_wasm>", "[utf8-import-field_141_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.141.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_142_wasm>", "[utf8-import-field_142_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.142.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_143_wasm>", "[utf8-import-field_143_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.143.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_144_wasm>", "[utf8-import-field_144_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.144.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_145_wasm>", "[utf8-import-field_145_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.145.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_146_wasm>", "[utf8-import-field_146_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.146.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_147_wasm>", "[utf8-import-field_147_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.147.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_148_wasm>", "[utf8-import-field_148_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.148.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_149_wasm>", "[utf8-import-field_149_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.149.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_15_wasm>", "[utf8-import-field_15_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.15.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_150_wasm>", "[utf8-import-field_150_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.150.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_151_wasm>", "[utf8-import-field_151_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.151.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_152_wasm>", "[utf8-import-field_152_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.152.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_153_wasm>", "[utf8-import-field_153_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.153.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_154_wasm>", "[utf8-import-field_154_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.154.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_155_wasm>", "[utf8-import-field_155_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.155.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_156_wasm>", "[utf8-import-field_156_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.156.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_157_wasm>", "[utf8-import-field_157_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.157.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_158_wasm>", "[utf8-import-field_158_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.158.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_159_wasm>", "[utf8-import-field_159_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.159.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_16_wasm>", "[utf8-import-field_16_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.16.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_160_wasm>", "[utf8-import-field_160_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.160.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_161_wasm>", "[utf8-import-field_161_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.161.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_162_wasm>", "[utf8-import-field_162_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.162.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_163_wasm>", "[utf8-import-field_163_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.163.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_164_wasm>", "[utf8-import-field_164_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.164.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_165_wasm>", "[utf8-import-field_165_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.165.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_166_wasm>", "[utf8-import-field_166_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.166.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_167_wasm>", "[utf8-import-field_167_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.167.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_168_wasm>", "[utf8-import-field_168_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.168.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_169_wasm>", "[utf8-import-field_169_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.169.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_17_wasm>", "[utf8-import-field_17_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.17.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_170_wasm>", "[utf8-import-field_170_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.170.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_171_wasm>", "[utf8-import-field_171_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.171.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_172_wasm>", "[utf8-import-field_172_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.172.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_173_wasm>", "[utf8-import-field_173_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.173.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_174_wasm>", "[utf8-import-field_174_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.174.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_175_wasm>", "[utf8-import-field_175_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.175.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_18_wasm>", "[utf8-import-field_18_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.18.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_19_wasm>", "[utf8-import-field_19_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.19.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_2_wasm>", "[utf8-import-field_2_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.2.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_20_wasm>", "[utf8-import-field_20_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.20.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_21_wasm>", "[utf8-import-field_21_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.21.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_22_wasm>", "[utf8-import-field_22_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.22.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_23_wasm>", "[utf8-import-field_23_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.23.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_24_wasm>", "[utf8-import-field_24_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.24.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_25_wasm>", "[utf8-import-field_25_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.25.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_26_wasm>", "[utf8-import-field_26_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.26.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_27_wasm>", "[utf8-import-field_27_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.27.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_28_wasm>", "[utf8-import-field_28_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.28.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_29_wasm>", "[utf8-import-field_29_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.29.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_3_wasm>", "[utf8-import-field_3_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.3.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_30_wasm>", "[utf8-import-field_30_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.30.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_31_wasm>", "[utf8-import-field_31_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.31.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_32_wasm>", "[utf8-import-field_32_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.32.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_33_wasm>", "[utf8-import-field_33_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.33.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_34_wasm>", "[utf8-import-field_34_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.34.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_35_wasm>", "[utf8-import-field_35_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.35.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_36_wasm>", "[utf8-import-field_36_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.36.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_37_wasm>", "[utf8-import-field_37_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.37.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_38_wasm>", "[utf8-import-field_38_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.38.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_39_wasm>", "[utf8-import-field_39_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.39.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_4_wasm>", "[utf8-import-field_4_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.4.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_40_wasm>", "[utf8-import-field_40_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.40.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_41_wasm>", "[utf8-import-field_41_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.41.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_42_wasm>", "[utf8-import-field_42_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.42.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_43_wasm>", "[utf8-import-field_43_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.43.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_44_wasm>", "[utf8-import-field_44_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.44.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_45_wasm>", "[utf8-import-field_45_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.45.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_46_wasm>", "[utf8-import-field_46_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.46.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_47_wasm>", "[utf8-import-field_47_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.47.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_48_wasm>", "[utf8-import-field_48_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.48.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_49_wasm>", "[utf8-import-field_49_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.49.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_5_wasm>", "[utf8-import-field_5_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.5.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_50_wasm>", "[utf8-import-field_50_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.50.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_51_wasm>", "[utf8-import-field_51_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.51.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_52_wasm>", "[utf8-import-field_52_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.52.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_53_wasm>", "[utf8-import-field_53_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.53.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_54_wasm>", "[utf8-import-field_54_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.54.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_55_wasm>", "[utf8-import-field_55_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.55.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_56_wasm>", "[utf8-import-field_56_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.56.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_57_wasm>", "[utf8-import-field_57_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.57.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_58_wasm>", "[utf8-import-field_58_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.58.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_59_wasm>", "[utf8-import-field_59_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.59.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_6_wasm>", "[utf8-import-field_6_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.6.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_60_wasm>", "[utf8-import-field_60_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.60.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_61_wasm>", "[utf8-import-field_61_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.61.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_62_wasm>", "[utf8-import-field_62_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.62.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_63_wasm>", "[utf8-import-field_63_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.63.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_64_wasm>", "[utf8-import-field_64_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.64.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_65_wasm>", "[utf8-import-field_65_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.65.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_66_wasm>", "[utf8-import-field_66_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.66.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_67_wasm>", "[utf8-import-field_67_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.67.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_68_wasm>", "[utf8-import-field_68_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.68.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_69_wasm>", "[utf8-import-field_69_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.69.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_7_wasm>", "[utf8-import-field_7_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.7.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_70_wasm>", "[utf8-import-field_70_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.70.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_71_wasm>", "[utf8-import-field_71_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.71.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_72_wasm>", "[utf8-import-field_72_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.72.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_73_wasm>", "[utf8-import-field_73_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.73.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_74_wasm>", "[utf8-import-field_74_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.74.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_75_wasm>", "[utf8-import-field_75_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.75.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_76_wasm>", "[utf8-import-field_76_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.76.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_77_wasm>", "[utf8-import-field_77_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.77.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_78_wasm>", "[utf8-import-field_78_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.78.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_79_wasm>", "[utf8-import-field_79_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.79.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_8_wasm>", "[utf8-import-field_8_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.8.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_80_wasm>", "[utf8-import-field_80_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.80.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_81_wasm>", "[utf8-import-field_81_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.81.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_82_wasm>", "[utf8-import-field_82_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.82.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_83_wasm>", "[utf8-import-field_83_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.83.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_84_wasm>", "[utf8-import-field_84_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.84.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_85_wasm>", "[utf8-import-field_85_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.85.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_86_wasm>", "[utf8-import-field_86_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.86.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_87_wasm>", "[utf8-import-field_87_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.87.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_88_wasm>", "[utf8-import-field_88_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.88.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_89_wasm>", "[utf8-import-field_89_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.89.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_9_wasm>", "[utf8-import-field_9_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.9.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_90_wasm>", "[utf8-import-field_90_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.90.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_91_wasm>", "[utf8-import-field_91_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.91.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_92_wasm>", "[utf8-import-field_92_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.92.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_93_wasm>", "[utf8-import-field_93_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.93.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_94_wasm>", "[utf8-import-field_94_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.94.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_95_wasm>", "[utf8-import-field_95_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.95.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_96_wasm>", "[utf8-import-field_96_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.96.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_97_wasm>", "[utf8-import-field_97_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.97.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_98_wasm>", "[utf8-import-field_98_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.98.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

BACKEND_TEST_CASE( "Testing wasm <utf8-import-field_99_wasm>", "[utf8-import-field_99_wasm_tests]" ) {
   using backend_t = backend<std::nullptr_t, TestType>;
   auto code = backend_t::read_wasm( std::string(wasm_directory) + "utf8-import-field.99.wasm");
   CHECK_THROWS_AS(backend_t(code), std::exception);
}

