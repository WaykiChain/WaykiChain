#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "eosio_test_generator.hpp"

using namespace std;

const string test_includes = "#include <wasm_spec_tests.hpp>\n\n";
const string boost_xrange  = "boost::unit_test::data::xrange";

string convert_to_valid_cpp_identifier(string val) {
   string ret_val = val;
   for (int i = 0; i <= val.size(); i++) {
      if (val[i] == '-' || val[i] == '.') {
         ret_val[i] = '_';
      } else {
         ret_val[i] = val[i];
      }
   }

   return ret_val;
}

string create_passing_data_test_case(string test_name, int start_index, int end_index) {
   stringstream func;

   func << "BOOST_DATA_TEST_CASE(" << test_name << "_pass, " << boost_xrange << "(" << start_index << "," << end_index
        << "), index) { try {\n";
   func << "   TESTER tester;\n";
   func << "   tester.produce_block();\n";
   func << "   tester.create_account( N(wasmtest) );\n";
   func << "   tester.produce_block();\n";
   func << "   tester.set_code(N(wasmtest), wasm_" << test_name << ");\n";
   func << "   tester.produce_block();\n\n";
   func << "   action test;\n";
   func << "   test.account = N(wasmtest);\n";
   func << "   test.name = account_name((uint64_t)index);\n";
   func << "   test.authorization = {{N(wasmtest), config::active_name}};\n\n";
   func << "   push_action(tester, std::move(test), N(wasmtest).to_uint64_t());\n";
   func << "   tester.produce_block();\n";
   func << "   BOOST_REQUIRE_EQUAL( tester.validate(), true );\n";
   func << "} FC_LOG_AND_RETHROW() }\n\n";

   return func.str();
}

string create_check_throw_data_test_case(string test_name, int start_index, int end_index) {
   stringstream func;

   func << "BOOST_DATA_TEST_CASE(" << test_name << "_check_throw, " << boost_xrange << "(" << start_index << "," << end_index
        << "), index) { try {\n";
   func << "   TESTER tester;\n";
   func << "   tester.produce_block();\n";
   func << "   tester.create_account( N(wasmtest) );\n";
   func << "   tester.produce_block();\n";
   func << "   tester.set_code(N(wasmtest), wasm_" << test_name << ");\n";
   func << "   tester.produce_block();\n\n";
   func << "   action test;\n";
   func << "   test.account = N(wasmtest);\n";
   func << "   test.name = account_name((uint64_t)index);\n";
   func << "   test.authorization = {{N(wasmtest), config::active_name}};\n\n";
   func << "   BOOST_CHECK_THROW(push_action(tester, std::move(test), N(wasmtest).to_uint64_t()), "
           "wasm_execution_error);\n";
   func << "   tester.produce_block();\n";
   func << "} FC_LOG_AND_RETHROW() }\n\n";

   return func.str();
}

void write_tests(vector<spec_test> tests) {
   string file_name = "";
   stringstream test_ss;

   for (const auto& t: tests) {
      file_name = t.name.substr(0, t.name.find_last_of('.'));

      string name = convert_to_valid_cpp_identifier(t.name);
      test_ss << "const string wasm_str_" << name << " = base_dir + \"/wasm_spec_tests/wasms/" << t.name << ".wasm\";\n";
      test_ss << "std::vector<uint8_t> wasm_" << name << "= read_wasm(wasm_str_" << name << ".c_str());\n\n";

      if (t.assert_trap_start_index < t.assert_trap_end_index) {
         test_ss << create_check_throw_data_test_case(name, t.assert_trap_start_index, t.assert_trap_end_index);
      }

      if (t.assert_return_start_index < t.assert_return_end_index) {
         test_ss << create_passing_data_test_case(name, t.assert_return_start_index, t.assert_return_end_index);
      }
   }

   ofstream ofs;
   ofs.open(file_name + ".cpp", ofstream::out);

   ofs << test_includes;
   ofs << test_ss.str();
}
