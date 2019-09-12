
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
#include "wasm/wasm_variant.hpp"
#include "wasm/wasm_config.hpp"

#include "wasm/wasm_log.hpp"


using std::chrono::microseconds;
using std::chrono::system_clock;

using namespace std;
using namespace json_spirit;
using namespace wasm;


//#define WASM_TEST(expr, ...) WASM_ASSERT(expr, wasm_exception, __VA_ARGS__ )

#define WASM_CHECK_EXCEPTION( expr, passed, ex, msg ) \
 passed = false;                              \
 try{                                         \
     expr  ;                                  \
 } catch(wasm::exception &e){                 \
     if( ex(msg).code() == e.code()) {        \
         WASM_TRACE("%s%s exception: %s", msg , "[ passed ]", e.detail())   \
         passed = true;                       \
     }else {                                  \
          WASM_TRACE("%s", e.detail())        \
     }                                        \
 }                                            \
 if (!passed) {                               \
     WASM_TRACE("%s%s", msg, "[ failed ]")    \
     assert(false);                           \
 }                                            


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

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(typedef_cycle_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);

    WASM_CHECK_EXCEPTION(wasm::abi_serializer
                                 abis(def, max_serialization_time), passed, duplicate_abi_def_exception,
                         "typedef_cycle_abi")


    wasm::variant var2;
    json_spirit::read_string(std::string(struct_cycle_abi), var2);
    wasm::abi_def def2;
    wasm::from_variant(var2, def2);


    abi_serializer abis;
    WASM_CHECK_EXCEPTION(abis.set_abi(def2, max_serialization_time), passed, abi_circular_def_exception,
                         "struct_cycle_abi")

}


void abi_type_repeat(){

   const char* repeat_abi = R"=====(
   {
     "version": "wasm::abi/1.0",
     "types": [{
         "new_type_name": "actor_name",
         "type": "name"
       },{
         "new_type_name": "actor_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "actor_name"
         },{
            "name": "to",
            "type": "actor_name"
         },{
            "name": "amount",
            "type": "uint64"
         }]
       },{
         "name": "account",
         "base": "",
         "fields": [{
            "name": "account",
            "type": "name"
         },{
            "name": "balance",
            "type": "uint64"
         }]
       }
     ],
     "actions": [{
         "name": "transfer",
         "type": "transfer"
       }
     ],
     "tables": [{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ],
    "ricardian_clauses": []
   }
   )=====";


    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(repeat_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);


    abi_serializer abis;
    WASM_CHECK_EXCEPTION(abis.set_abi(def, max_serialization_time), passed, duplicate_abi_def_exception,
                         "abi_type_repeat")  


}


int main( int argc, char **argv ) {

    abi_cycle();
    abi_type_repeat();

    return 0;

}




