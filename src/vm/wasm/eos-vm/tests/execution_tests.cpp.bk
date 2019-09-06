#include <algorithm>
#include <vector>
#include <iterator>
#include <cstdlib>

#include <boost/test/unit_test.hpp>
#include <boost/test/framework.hpp>

#include <eosio/wasm_backend/integer_types.hpp>
#include <eosio/wasm_backend/wasm_interpreter.hpp>
#include <eosio/wasm_backend/types.hpp>
#include <eosio/wasm_backend/execution_engine.hpp>
#include <eosio/wasm_backend/opcodes.hpp>

using namespace eosio;
using namespace eosio::wasm_backend;

BOOST_AUTO_TEST_SUITE(execution_tests)
BOOST_AUTO_TEST_CASE(unreachable_test) { 
   try {
      {
         execution_engine ee;
         wasm_code code = { opcode::nop, 
                            opcode::unreachable, 
                            opcode::nop };
         wasm_code_iterator at = code.begin();
         ee.execute( code, at );
         at++;
         BOOST_CHECK_THROW(ee.execute( code, at ), wasm_unreachable_exception);
         at++;
         ee.execute( code, at );
      }
   } FC_LOG_AND_RETHROW() 
}

BOOST_AUTO_TEST_SUITE_END()

