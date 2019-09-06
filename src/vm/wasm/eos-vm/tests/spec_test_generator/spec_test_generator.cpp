#include "picojson.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

const string test_includes = "#include <algorithm>\n#include <vector>\n#include <iostream>\n#include "
                             "<iterator>\n#include <cmath>\n#include <cstdlib>\n#include <catch2/catch.hpp>\n"
                             "#include <utils.hpp>\n#include <wasm_config.hpp>\n#include "
                             "<eosio/vm/backend.hpp>\n\nusing namespace eosio;\nusing namespace eosio::vm;\n"
                             "extern wasm_allocator wa;\nusing backend_t = backend<std::nullptr_t>;\n\n";
const string test_preamble_0 = "auto code = backend_t::read_wasm( ";
const string test_preamble_1 = "backend_t bkend( code );\n   bkend.set_wasm_allocator( &wa );\n   bkend.initialize(nullptr);";

string generate_test_call(picojson::object obj, string expected_t, string expected_v) {
   stringstream ss;

   if (expected_t == "i32") {
      ss << "bkend.call_with_return(nullptr, \"env\", ";
   } else if (expected_t == "i64") {
      ss << "bkend.call_with_return(nullptr, \"env\", ";
   } else if (expected_t == "f32") {
      ss << "bit_cast<uint32_t>(bkend.call_with_return(nullptr, \"env\", ";
   } else if (expected_t == "f64") {
      ss << "bit_cast<uint64_t>(bkend.call_with_return(nullptr, \"env\", ";
   } else {
      ss << "!bkend.call_with_return(nullptr, \"env\", ";
   }

   ss << "\"" << obj["field"].to_str() << "\"";

   for (picojson::value argv : obj["args"].get<picojson::array>()) {
      ss << ", ";
      picojson::object arg = argv.get<picojson::object>();
      if (arg["type"].to_str() == "i32")
         ss << "UINT32_C(" << arg["value"].to_str() << ")";
      else if (arg["type"].to_str() == "i64")
         ss << "UINT64_C(" << arg["value"].to_str() << ")";
      else if (arg["type"].to_str() == "f32")
         ss << "bit_cast<float>(UINT32_C(" << arg["value"].to_str() << "))";
      else
         ss << "bit_cast<double>(UINT64_C(" << arg["value"].to_str() << "))";
   }
   if (expected_t == "i32") {
      ss << ")->to_ui32() == ";
      ss << "UINT32_C(" << expected_v << ")";
   } else if (expected_t == "i64") {
      ss << ")->to_ui64() == ";
      ss << "UINT32_C(" << expected_v << ")";
   } else if (expected_t == "f32") {
      ss << ")->to_f32()) == ";
      ss << "UINT32_C(" << expected_v << ")";
   } else if (expected_t == "f64") {
      ss << ")->to_f64()) == ";
      ss << "UINT64_C(" << expected_v << ")";
   } else {
      ss << ")";
   }
   return ss.str();
}

string generate_trap_call(picojson::object obj) {
   stringstream ss;

   ss << "bkend(nullptr, \"env\", ";
   ss << "\"" << obj["field"].to_str() << "\"";

   for (picojson::value argv : obj["args"].get<picojson::array>()) {
      ss << ", ";
      picojson::object arg = argv.get<picojson::object>();
      if (arg["type"].to_str() == "i32")
         ss << "UINT32_C(" << arg["value"].to_str() << ")";
      else if (arg["type"].to_str() == "i64")
         ss << "UINT64_C(" << arg["value"].to_str() << ")";
      else if (arg["type"].to_str() == "f32")
         ss << "bit_cast<float>(UINT32_C(" << arg["value"].to_str() << "))";
      else
         ss << "bit_cast<double>(UINT64_C(" << arg["value"].to_str() << "))";
   }
   ss << "), std::exception";
   return ss.str();
}

string generate_call(picojson::object obj) {
   stringstream ss;

   ss << "bkend(nullptr, \"env\", ";
   ss << "\"" << obj["field"].to_str() << "\"";

   for (picojson::value argv : obj["args"].get<picojson::array>()) {
      ss << ", ";
      picojson::object arg = argv.get<picojson::object>();
      if (arg["type"].to_str() == "i32")
         ss << "UINT32_C(" << arg["value"].to_str() << ")";
      else if (arg["type"].to_str() == "i64")
         ss << "UINT64_C(" << arg["value"].to_str() << ")";
      else if (arg["type"].to_str() == "f32")
         ss << "bit_cast<float>(UINT32_C(" << arg["value"].to_str() << "))";
      else
         ss << "bit_cast<double>(UINT64_C(" << arg["value"].to_str() << "))";
   }
   ss << ")";
   return ss.str();
}

void generate_tests(const map<string, vector<picojson::object>>& mappings) {
   stringstream unit_tests;
   string       exp_t, exp_v;
   unit_tests << test_includes;
   auto grab_expected = [&](auto obj) {
      exp_t = obj["type"].to_str();
      exp_v = obj["value"].to_str();
   };

   for (const auto& [tsn, cmds] : mappings) {
      auto tsn_id = tsn;
      std::replace(tsn_id.begin(), tsn_id.end(), '-', '_');
      unit_tests << "TEST_CASE( \"Testing wasm <" << tsn << ">\", \"[" << tsn << "_tests]\" ) {\n";
      unit_tests << "   " << test_preamble_0 << tsn_id << " );\n";
      unit_tests << "   " << test_preamble_1 << "\n\n";

      for (picojson::object cmd : cmds) {
         if (cmd["type"].to_str() == "assert_return") {
            unit_tests << "   CHECK(";
            exp_t = "";
            exp_v = "";
            if (cmd["expected"].get<picojson::array>().size() > 0) {
               grab_expected(cmd["expected"].get<picojson::array>()[0].get<picojson::object>());
               unit_tests << generate_test_call(cmd["action"].get<picojson::object>(), exp_t, exp_v) << ");\n";
            } else {
               unit_tests << generate_test_call(cmd["action"].get<picojson::object>(), exp_t, exp_v) << ");\n";
            }
         } else if (cmd["type"].to_str() == "assert_trap") {
            unit_tests << "   CHECK_THROWS_AS(";
            unit_tests << generate_trap_call(cmd["action"].get<picojson::object>()) << ");\n";
         } else if (cmd["type"].to_str() == "action") {
            unit_tests << generate_call(cmd["action"].get<picojson::object>()) << ";\n";
         }
      }
      unit_tests << "}\n\n";
      cout << unit_tests.str();
      unit_tests.str("");
   }
}

void usage(const char* name) {
   std::cerr << "Usage:\n"
             << "  " << name << " [json file created by wast2json]\n";
   std::exit(2);
}

int main(int argc, char** argv) {
   ifstream     ifs;
   stringstream ss;
   if(argc != 2 || !strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
      usage(argc?argv[0]:"spec_test_generator");
   }
   ifs.open(argv[1]);
   if(!ifs) {
      std::cerr << "Cannot open file: " << argv[1] << std::endl;
      return EXIT_FAILURE;
   }
   string s;
   while (getline(ifs, s)) ss << s;
   ifs.close();

   picojson::value v;
   picojson::parse(v, ss.str());
   string test_suite_name;

   map<string, vector<picojson::object>> test_mappings;
   const picojson::value::object&        obj = v.get<picojson::object>();
   for (picojson::value::object::const_iterator i = obj.begin(); i != obj.end(); i++) {
      if (i->first == "commands") {
         for (const auto& o : i->second.get<picojson::array>()) {
            picojson::object obj = o.get<picojson::object>();
            if (obj["type"].to_str() == "module") {
               test_suite_name = obj["filename"].to_str();
               [&]() {
                  for (int i = 0; i <= test_suite_name.size(); i++)
                     test_suite_name[i] = test_suite_name[i] == '.' ? '_' : test_suite_name[i];
               }();
               test_mappings[test_suite_name] = {};
            }
            test_mappings[test_suite_name].push_back(obj);
         }
      }
   }

   generate_tests(test_mappings);
}
