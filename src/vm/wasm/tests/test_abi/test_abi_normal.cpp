
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

#include "large_nested.abi.hpp"


using std::chrono::microseconds;
using std::chrono::system_clock;

using namespace std;
using namespace json_spirit;
using namespace wasm;


//#define WASM_TEST(expr, ...) WASM_ASSERT(expr, wasm_exception, __VA_ARGS__ )

#define WASM_CHECK( expr, msg )              \
if(! (expr)){                                \
    WASM_TRACE("%s%s", msg , "[ failed ]")   \
    assert(false);                           \
}

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
 }  catch(...){                               \
    WASM_TRACE("%s", "exception")             \
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
    WASM_CHECK(ToHex(bytes) == ToHex(bytes2), "verify_byte_round_trip_conversion_fail")

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


void abi_type_repeat() {

    const char *repeat_abi = R"=====(
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

void abi_struct_repeat() {

    const char *repeat_abi = R"=====(
    {
     "version": "wasm::abi/1.0",
     "types": [{
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
         "name": "transfer",
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
                         "abi_struct_repeat")

}

void abi_action_repeat() {

    const char *repeat_abi = R"=====(
   {
     "version": "wasm::abi/1.0",
     "types": [{
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
       },{
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
                         "abi_action_repeat")

}

void abi_table_repeat() {

    const char *repeat_abi = R"=====(
   {
     "version": "wasm::abi/1.0",
     "types": [{
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
         "type": "transfer",
         "ricardian_contract": "transfer contract"
       }
     ],
     "tables": [{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       },{
         "name": "account",
         "type": "account",
         "index_type": "i64",
         "key_names" : ["account"],
         "key_types" : ["name"]
       }
     ]
   }
   )=====";


    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(repeat_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);


    abi_serializer abis;
    WASM_CHECK_EXCEPTION(abis.set_abi(def, max_serialization_time), passed, duplicate_abi_def_exception,
                         "abi_table_repeat")
}

void abi_type_def() {

    const char *repeat_abi = R"=====(
   {
     "version": "wasm::abi/1.0",
     "types": [{
         "new_type_name": "account_name",
         "type": "name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "account_name"
         },{
            "name": "to",
            "type": "name"
         },{
            "name": "amount",
            "type": "uint64"
         }]
       }
     ],
     "actions": [{
         "name": "transfer",
         "type": "transfer",
         "ricardian_contract": "transfer contract"
       }
     ],
     "tables": []
   }
   )=====";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(repeat_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);

    abi_serializer abis;
    abis.set_abi(def, max_serialization_time);

    WASM_CHECK(abis.is_type("name", max_serialization_time), "abis.is_type name");
    WASM_CHECK(abis.is_type("account_name", max_serialization_time), "abis.is_type naaccount_nameme");

    const char *test_data = R"=====(
    {
     "from" : "kevin",
     "to" : "dan",
     "amount" : 16
    }
    )=====";

    wasm::variant var2;
    json_spirit::read_string(std::string(test_data), var2);
    verify_byte_round_trip_conversion(abis, "transfer", var2);

    WASM_TRACE("%s%s", "abi_type_def", "[ passed ]")
}


void abi_type_redefine() {

    const char *repeat_abi = R"=====(
   {
     "version": "wasm::abi/1.0",
     "types": [{
         "new_type_name": "account_name",
         "type": "account_name"
       }
     ],
     "structs": [{
         "name": "transfer",
         "base": "",
         "fields": [{
            "name": "from",
            "type": "account_name"
         },{
            "name": "to",
            "type": "name"
         },{
            "name": "amount",
            "type": "uint64"
         }]
       }
     ],
     "actions": [{
         "name": "transfer",
         "type": "transfer",
         "ricardian_contract": "transfer contract"
       }
     ],
     "tables": []
   }
   )=====";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(repeat_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);

    WASM_CHECK_EXCEPTION(wasm::abi_serializer
                                 abis(def, max_serialization_time), passed, invalid_type_inside_abi,
                         "abi_type_redefine")


}

void abi_type_redefine_to_name() {

    const char *repeat_abi = R"=====(
   {
     "version": "wasm::abi/1.0",
     "types": [{
         "new_type_name": "name",
         "type": "name"
       }
     ],
     "structs": [],
     "actions": [],
     "tables": []
   }
   )=====";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(repeat_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);

    WASM_CHECK_EXCEPTION(wasm::abi_serializer
                                 abis(def, max_serialization_time), passed, duplicate_abi_def_exception,
                         "abi_type_redefine_to_name")


}

void abi_type_nested_in_vector() {

    const char *repeat_abi = R"=====(
   {
     "version": "wasm::abi/1.0",
     "types": [],
     "structs": [{
         "name": "store_t",
         "base": "",
         "fields": [{
            "name": "id",
            "type": "uint64"
         },{
            "name": "childs",
            "type": "store_t[]"
         }],
     "actions": [],
     "tables": []
   }
   )=====";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(repeat_abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);

    //WASM_CHECK_EXCEPTION(wasm::from_variant(var, def),passed, abi_parse_exception, "abi_type_nested_in_vector")

    WASM_CHECK_EXCEPTION(wasm::abi_serializer
                                 abis(def, max_serialization_time), passed, abi_circular_def_exception,
                         "abi_type_nested_in_vector")


}


void abi_large_array() {

    const char *abi_str = R"=====(
      {
        "version": "wasm::abi/1.0",
        "types": [],
        "structs": [{
           "name": "hi",
           "base": "",
           "fields": [
           ]
         }
       ],
       "actions": [{
           "name": "hi",
           "type": "hi[]",
           "ricardian_contract": ""
         }
       ],
       "tables": []
      }
      )=====";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(abi_str), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time);

    bytes bin = {static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0x08)};

    WASM_CHECK_EXCEPTION(abis.binary_to_variant("hi[]", bin, max_serialization_time);, passed,
                         array_size_exceeds_exception, "abi_large_array")


}

void abi_is_type_recursion() {

      const char* abi_str = R"=====(
      {
       "version": "wasm::abi/1.0",
       "types": [
        {
            "new_type_name": "a[]",
            "type": "a[][]"
        }
        ],
        "structs": [
         {
            "name": "a[]",
            "base": "",
            "fields": []
         },
         {
            "name": "hi",
            "base": "",
            "fields": [{
                "name": "user",
                "type": "name"
              }
            ]
          }
        ],
        "actions": [{
            "name": "hi",
            "type": "hi",
            "ricardian_contract": ""
          }
        ],
        "tables": []
      }
      )=====";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(abi_str), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);

    WASM_CHECK_EXCEPTION(wasm::abi_serializer abis(def, max_serialization_time), passed,
                         abi_serialization_deadline_exception, "abi_is_type_recursion")


}

void abi_recursive_structs() {

      const char* abi_str = R"=====(
      {
        "version": "wasm::abi/1.0",
        "types": [],
        "structs": [
          {
            "name": "a",
            "base": "",
            "fields": [
              {
              "name": "user",
              "type": "b"
              }
            ]
          },
          {
            "name": "b",
            "base": "",
            "fields": [
             {
               "name": "user",
               "type": "a"
             }
            ]
          },
          {
            "name": "hi",
            "base": "",
            "fields": [{
                "name": "user",
                "type": "name"
              },
              {
                "name": "arg2",
                "type": "a"
              }
            ]
         },
         {
           "name": "hi2",
           "base": "",
           "fields": [{
               "name": "user",
               "type": "name"
             }
           ]
         }
        ],
        "actions": [{
            "name": "hi",
            "type": "hi",
            "ricardian_contract": ""
          }
        ],
        "tables": []
      }
      )=====";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(abi_str), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);
    WASM_CHECK_EXCEPTION(wasm::abi_serializer abis(def, max_serialization_time), passed,
                         abi_circular_def_exception, "abi_recursive_structs")


}

void abi_very_deep_structs() {

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(large_nested_abi), var);

    wasm::abi_def def;
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time);

    string hi_data = "{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":0}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}";
    wasm::variant var2;
    json_spirit::read_string(hi_data, var2); 

    WASM_CHECK_EXCEPTION(abis.variant_to_binary( "s98", var2, max_serialization_time ), passed,
                         abi_serialization_deadline_exception, "abi_recursive_structs")

}

void abi_very_deep_structs_1us() {

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(large_nested_abi), var);

    wasm::abi_def def;
    wasm::from_variant(var, def);
    

    WASM_CHECK_EXCEPTION(wasm::abi_serializer abis(def, microseconds(1000)), passed,
                         abi_serialization_deadline_exception, "abi_recursive_structs")

}



int main( int argc, char **argv ) {

    // abi_cycle();
    // abi_type_repeat();
    // abi_struct_repeat();
    // abi_action_repeat();
    // abi_table_repeat();
    // abi_type_def();
    // abi_type_redefine();
    // abi_type_redefine_to_name();
    // abi_type_nested_in_vector();
    // abi_large_array();
    // abi_is_type_recursion();
    // abi_recursive_structs();
    // abi_very_deep_structs();

    abi_very_deep_structs_1us();

    return 0;

}




