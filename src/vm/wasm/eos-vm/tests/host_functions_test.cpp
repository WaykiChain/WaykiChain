#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>
#include <fstream>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/test/framework.hpp>

#include <eosio/wasm_backend/leb128.hpp>
#include <eosio/wasm_backend/wasm_interpreter.hpp>
#include <eosio/wasm_backend/types.hpp>
#include <eosio/wasm_backend/opcodes.hpp>
#include <eosio/wasm_backend/parser.hpp>
#include <eosio/wasm_backend/constants.hpp>
#include <eosio/wasm_backend/sections.hpp>
//#include <eosio/wasm_backend/disassembly_visitor.hpp>
//#include <eosio/wasm_backend/interpret_visitor.hpp>

using namespace eosio;
using namespace eosio::wasm_backend;

BOOST_AUTO_TEST_SUITE(host_functions_tests)
BOOST_AUTO_TEST_CASE(host_functions_test) { 
   try {  
      {
         /*
         memory_manager::set_memory_limits( 32*1024*1024 );
         binary_parser bp;
         module mod;
         wasm_code code = read_wasm( "test.wasm" );
         wasm_code_ptr code_ptr(code.data(), 0);
         bp.parse_module( code, mod );

         struct test {
            void hello() { std::cout << "hello\n"; }
            static void hello2() { std::cout << "Hello2\n"; }
            static void func() { std::cout << "func\n"; }
         };

         test t;
         registered_member_function<test, &test::hello, decltype("hello"_hfn)> rf;
         std::invoke(rf.function, t);

         registered_function<&test::hello2, decltype("hello2"_hfn)> rf2;
         std::invoke(rf2.function);

         registered_function<&test::func, decltype("eosio_assert"_hfn)> rf3;

         using rhf = registered_host_functions<decltype(rf), decltype(rf2), decltype(rf3)>;
         rhf::resolve(mod);
         execution_context ec(mod);
         interpret_visitor v(ec);
         uint32_t index = mod.get_exported_function("apply");
         int i = 0;
         while (ec.executing()) {
         for (uint32_t i=0; i < mod.code[index].code.size(); i++) {
            ec.set_pc(i);
            std::visit(v, mod.code[index].code[i]);
         }
         std::cout << v.dbg_output.str() << '\n';
         */
      }

   } FC_LOG_AND_RETHROW() 
}
BOOST_AUTO_TEST_SUITE_END()

