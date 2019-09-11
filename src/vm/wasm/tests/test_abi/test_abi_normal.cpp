
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

#include "json/json_spirit.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer.h"
// #include "wasm/abi_def.hpp"
#include "wasm/abi_serializer.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/wasmvariant.hpp"
#include "wasm/wasmconfig.hpp"


using std::chrono::microseconds;
using std::chrono::system_clock;

using namespace std;
using namespace json_spirit;
using namespace wasm;


//#define WASM_TEST(expr, ...) WASM_ASSERT(expr, wasm_exception, __VA_ARGS__ )

#define WASM_CHECK_EXCEPTION( expr, passed, ex, msg ) \
 std::cout << msg << "[ testing... ]" << std::endl;  \
 passed = false;                              \
 try{                                         \
     expr  ;                                  \
 } catch(wasm::exception &e){                 \
     if( ex(msg).code() == e.code()) {        \
         passed = true;                       \
     }else {                                     \
         std::cout << "e code:" << e.code() << std::endl;  \
         std::cout << "e:" << e.detail() << std::endl;  \
     }           \
 }                                            \
 if (!passed) {                               \
   std::cout << msg << "[ failed ]" << std::endl;  \
   assert(false);                             \
  }                                           \
  std::cout << msg << "[ passed ]" << std::endl;


wasm::variant
verify_byte_round_trip_conversion( const wasm::abi_serializer &abis, const type_name &type, const wasm::variant &var ) {
    auto bytes = abis.variant_to_binary(type, var, max_serialization_time);
    auto var2 = abis.binary_to_variant(type, bytes, max_serialization_time);

    auto bytes2 = abis.variant_to_binary(type, var2, max_serialization_time);
    //WASM_TEST( ToHex(bytes) == ToHex(bytes2), "verify_byte_round_trip_conversion_fail" )

    return var2;
}

void verify_round_trip_conversion( const wasm::abi_serializer &abis, const type_name &type, const std::string &json,
                                   const std::string &hex, const std::string &expected_Json ) {

    wasm::variant var;
    json_spirit::read_string(json, var);

    auto bytes = abis.variant_to_binary(type, var, max_serialization_time);
    //WASM_TEST( ToHex(bytes) == hex, "verify_round_trip_conversion_fail variant_to_binary_1" )


    auto var2 = abis.binary_to_variant(type, bytes, max_serialization_time);
    //WASM_TEST(json_spirit::write(var2) == expected_json, "verify_round_trip_conversion_fail binary_to_variant_2" );

    auto bytes2 = abis.variant_to_binary(type, var2, max_serialization_time);
    //WASM_TEST(ToHex(bytes2) == hex, "verify_round_trip_conversion_fail variant_to_binary_3" ); 

}

void verify_round_trip_conversion( const abi_serializer &abis, const type_name &type, const std::string &json,
                                   const std::string &hex ) {
    verify_round_trip_conversion(abis, type, json, hex, json);
}

void abi_cycle() {

    const char *typedef_cycle_abi = R"=====(
     {
        "version": "wasm::abi/1.0",
         "types": [{
            "new_type_name": "A",
            "type": "name"
          },{
            "new_type_name": "name",
            "type": "A"
          }],
         "structs": [],
         "actions": [],
         "tables": [],
         "ricardian_clauses": []
     }
     )=====";

    const char *struct_cycle_abi = R"=====(
     {
         "version": "wasm::abi/1.0",
         "types": [],
         "structs": [{
           "name": "A",
           "base": "B",
           "fields": []
         },{
           "name": "B",
           "base": "C",
           "fields": []
         },{
           "name": "C",
           "base": "A",
           "fields": []
         }],
         "actions": [],
         "tables": [],
         "ricardian_clauses": []
     }
     )=====";


    //std::cout << "testing:" << "duplicate_abi_type_def_exception" << std::endl;
    wasm::variant var;
    json_spirit::read_string(std::string(typedef_cycle_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);


    bool passed = false;
    WASM_CHECK_EXCEPTION(wasm::abi_serializer
                                 abis(def, max_serialization_time), passed, duplicate_abi_def_exception,
                         "duplicate_abi_type_def_exception")


    //std::cout << "testing:" << "abi_circular_def_exception" << std::endl;
    wasm::variant var2;
    json_spirit::read_string(std::string(struct_cycle_abi), var2);
    wasm::abi_def def2;
    wasm::from_variant(var2, def2);

    // std::cout << "def:" << def.version << std::endl;
    // for (auto t : def.structs){
    //   std::cout << "name:" << t.name << std::endl;
    //   std::cout << "base:" << t.base << std::endl;
    // }


    abi_serializer abis;

    // auto f2 = [&]() {
    //     abis.set_abi(def, max_serialization_time);
    // };

    WASM_CHECK_EXCEPTION(abis.set_abi(def2, max_serialization_time), passed, abi_circular_def_exception,
                         "abi_circular_def_exception")

}


int main( int argc, char **argv ) {

    abi_cycle();

    return 0;

}




