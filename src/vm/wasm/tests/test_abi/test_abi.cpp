#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <boost/test/unit_test.hpp>
#pragma GCC diagnostic pop

#include <fstream>
#include <iostream>
#include <string>
#include <chrono>

#include "json/json_spirit.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer.h"
#include "wasm/abi_serializer.hpp"
#include "wasm/wasm_variant.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/exception/exceptions.hpp"

#include "wasm/wasm_log.hpp"

#include "large_nested.abi.hpp"
#include "deep_nested.abi.hpp"


using std::chrono::microseconds;
using std::chrono::system_clock;

using namespace std;
using namespace json_spirit;
using namespace wasm;

const static auto max_serialization_time_testing            = microseconds(25 * 1000);


#define WASM_TEST( expr, msg )               \
if(! (expr)){                                \
    WASM_TRACE("%s%s", msg , "[ failed ]")   \
    assert(false);                           \
} else {                                     \
    WASM_TRACE("%s%s", msg , "[ passed ]")   \
}


#define WASM_CHECK( expr, msg )              \
if(! (expr)){                                \
    WASM_TRACE("%s%s", msg , "[ failed ]")   \
    assert(false);                           \
}

#define WASM_CHECK_EXCEPTION( expr, passed, ex, msg ) \
 passed = false;                              \
 try{                                         \
     expr;                                    \
 } catch(wasm_chain::exception &e){                 \
     if( ex(CHAIN_LOG_MESSAGE( log_level::warn, msg )).code() == e.code()) {        \
         WASM_TRACE("%s%s \nexception: %s\n", msg , "[ passed ]", e.to_detail_string())   \
         passed = true;                       \
     }else {                                  \
          WASM_TRACE("%s\n", e.to_detail_string())        \
     }                                        \
 }  catch(...){                               \
    WASM_TRACE("%s", "exception")             \
 }                                            \
 if (!passed) {                               \
     WASM_TRACE("%s%s", msg, "[ failed ]")    \
     assert(false);                           \
 }


template<typename T>
static inline string ToHex( const T &t, string separator = "" ) {
    const std::string hex = "0123456789abcdef";
    std::ostringstream o;

    for (std::string::size_type i = 0; i < t.size(); ++i)
        o << hex[(unsigned char) t[i] >> 4] << hex[(unsigned char) t[i] & 0xf] << separator;

    return o.str();

}

wasm::variant
verify_byte_round_trip_conversion( const wasm::abi_serializer &abis, const type_name &type, const wasm::variant &var ) {
    auto bytes = abis.variant_to_binary(type, var, max_serialization_time_testing);
    auto var2 = abis.binary_to_variant(type, bytes, max_serialization_time_testing);

    auto bytes2 = abis.variant_to_binary(type, var2, max_serialization_time_testing);
    WASM_CHECK(ToHex(bytes) == ToHex(bytes2), "verify_byte_round_trip_conversion_fail")

    return var2;
}

void verify_round_trip_conversion( const wasm::abi_serializer &abis, const type_name &type, const std::string &json,
                                   const std::string &hex, const std::string &expected_json,const std::string &msg ) {
    string message = msg;

    wasm::variant var;
    json_spirit::read_string(json, var);

    auto bytes = abis.variant_to_binary(type, var, max_serialization_time_testing);

    WASM_CHECK(ToHex(bytes, "") == hex, "verify_round_trip_conversion_fail variant_to_binary_1")

    auto var2 = abis.binary_to_variant(type, bytes, max_serialization_time_testing);
    WASM_CHECK(json_spirit::write(var2) == expected_json, "verify_round_trip_conversion_fail binary_to_variant_2");

    auto bytes2 = abis.variant_to_binary(type, var2, max_serialization_time_testing);
    WASM_CHECK(ToHex(bytes2, "") == hex, "verify_round_trip_conversion_fail variant_to_binary_3");

    WASM_TRACE("%s %s", message, "[ passed ]");

}

void verify_round_trip_conversion( const abi_serializer &abis, const type_name &type, const std::string &json,
                                   const std::string &hex, const std::string &msg ) {
    verify_round_trip_conversion(abis, type, json, hex, json, msg);
}

BOOST_AUTO_TEST_SUITE( test_api )

BOOST_AUTO_TEST_CASE( abi_cycle )
{

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
                                 abis(def, max_serialization_time_testing), passed, duplicate_abi_type_def_exception,
                         "typedef_cycle_abi")

    wasm::variant var2;
    json_spirit::read_string(std::string(struct_cycle_abi), var2);
    wasm::abi_def def2;
    wasm::from_variant(var2, def2);

    abi_serializer abis;
    WASM_CHECK_EXCEPTION(abis.set_abi(def2, max_serialization_time_testing), passed, abi_circular_def_exception,
                         "struct_cycle_abi")

}


BOOST_AUTO_TEST_CASE( abi_type_repeat ) {

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
    WASM_CHECK_EXCEPTION(abis.set_abi(def, max_serialization_time_testing), passed, duplicate_abi_type_def_exception,
                         "abi_type_repeat")
}

BOOST_AUTO_TEST_CASE( abi_struct_repeat ) {

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
    WASM_CHECK_EXCEPTION(abis.set_abi(def, max_serialization_time_testing), passed, duplicate_abi_struct_def_exception,
                         "abi_struct_repeat")

}

BOOST_AUTO_TEST_CASE( abi_action_repeat ) {

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
    WASM_CHECK_EXCEPTION(abis.set_abi(def, max_serialization_time_testing), passed, duplicate_abi_action_def_exception,
                         "abi_action_repeat")

}


BOOST_AUTO_TEST_CASE( abi_table_repeat ) {

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
    WASM_CHECK_EXCEPTION(abis.set_abi(def, max_serialization_time_testing), passed, duplicate_abi_table_def_exception,
                         "abi_table_repeat")
}

BOOST_AUTO_TEST_CASE( abi_type_def ) {

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
    abis.set_abi(def, max_serialization_time_testing);

    WASM_CHECK(abis.is_type("name", max_serialization_time_testing), "abis.is_type name");
    WASM_CHECK(abis.is_type("account_name", max_serialization_time_testing), "abis.is_type account_name");

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


BOOST_AUTO_TEST_CASE( abi_type_redefine ) {

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

    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed, invalid_type_inside_abi,
                          "abi_type_redefine")


}


BOOST_AUTO_TEST_CASE( abi_type_redefine_to_name ) {

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

    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed, duplicate_abi_type_def_exception,
                          "abi_type_redefine_to_name")


}


BOOST_AUTO_TEST_CASE( abi_type_nested_in_vector ) {

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

    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed, abi_circular_def_exception,
                          "abi_type_nested_in_vector")

}



BOOST_AUTO_TEST_CASE( abi_large_array ) {

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
    wasm::abi_serializer abis(def, max_serialization_time_testing);

    bytes bin = {static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0xff),
                 static_cast<char>(0x08)};

    WASM_CHECK_EXCEPTION( abis.binary_to_variant("hi[]", bin, max_serialization_time_testing), passed,
                          array_size_exceeds_exception, "abi_large_array")


}


BOOST_AUTO_TEST_CASE( abi_is_type_recursion ) {

    const char *abi_str = R"=====(
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

    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed,
                          abi_serialization_deadline_exception, "abi_is_type_recursion")


}


BOOST_AUTO_TEST_CASE( abi_recursive_structs ) {

    const char *abi_str = R"=====(
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
    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed,
                          abi_circular_def_exception, "abi_recursive_structs")


}


BOOST_AUTO_TEST_CASE( abi_very_deep_structs ) {

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(large_nested_abi), var);

    wasm::abi_def def;
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time_testing);

    string hi_data = "{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":{\"f1\":0}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}}";
    wasm::variant var2;
    json_spirit::read_string(hi_data, var2);

    WASM_CHECK_EXCEPTION( abis.variant_to_binary("s98", var2, max_serialization_time_testing), passed,
                          abi_serialization_deadline_exception, "abi_very_deep_structs")

}


BOOST_AUTO_TEST_CASE( abi_very_deep_structs_1us ) {

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(large_nested_abi), var);

    wasm::abi_def def;
    wasm::from_variant(var, def);

    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, microseconds(1000)), passed,
                          abi_serialization_deadline_exception, "abi_very_deep_structs_1us")

}


BOOST_AUTO_TEST_CASE( abi_deep_structs_validate ) {

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(deep_nested_abi), var);

    wasm::abi_def def;
    wasm::from_variant(var, def);

    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed,
                          abi_serialization_deadline_exception, "abi_deep_structs_validate")

}


BOOST_AUTO_TEST_CASE( version ) {

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(R"({})"), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);
    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed,
                          unsupported_abi_version_exception, "version")

    json_spirit::read_string(std::string(R"({"version": ""})"), var);
    wasm::from_variant(var, def);
    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed,
                          unsupported_abi_version_exception, "version")

    json_spirit::read_string(std::string(R"({"version": "wasm::abi/9.0"})"), var);
    wasm::from_variant(var, def);
    WASM_CHECK_EXCEPTION( wasm::abi_serializer
                          abis(def, max_serialization_time_testing), passed,
                          unsupported_abi_version_exception, "version")

    json_spirit::read_string(std::string(R"({"version": "wasm::abi/1.0"})"), var);
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time_testing);

    json_spirit::read_string(std::string(R"({"version": "wasm::abi/1.1"})"), var);
    wasm::from_variant(var, def);
    wasm::abi_serializer abis2(def, max_serialization_time_testing);

}


BOOST_AUTO_TEST_CASE( abi_serialize_incomplete_json_array ) {

    auto abi = R"({
      "version": "wasm::abi/1.0",
      "structs": [
         {"name": "s",
          "base": "",
          "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"},
            {"name": "i2", "type": "int8"}
         ]}
      ]
    })";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time_testing);


    wasm::variant var1;
    json_spirit::read_string(string(R"([])"), var1);
    WASM_CHECK_EXCEPTION( abis.variant_to_binary(string("s"), var1, max_serialization_time_testing), passed,
                          wasm_chain::pack_exception, "Early end to input array specifying the fields of struct")

    wasm::variant var2;
    json_spirit::read_string(string(R"([1,2])"), var2);
    WASM_CHECK_EXCEPTION( abis.variant_to_binary(string("s"), var2, max_serialization_time_testing), passed,
                          wasm_chain::pack_exception, "Early end to input array specifying the fields of struct")

    //string msg = string("abi_serialize_incomplete_json_array");
    verify_round_trip_conversion( abis, "s", R"([1,2,3])", "010203", R"({"i0":1,"i1":2,"i2":3})",
                                  "abi_serialize_incomplete_json_array");

}


BOOST_AUTO_TEST_CASE( abi_serialize_incomplete_json_object ) {

    auto abi = R"({
      "version": "wasm::abi/1.0",
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"}
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "f0", "type": "s1"},
            {"name": "i2", "type": "int8"}
         ]}
      ]
   })";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time_testing);


    wasm::variant var2;
    json_spirit::read_string(string(R"({})"), var2);
    WASM_CHECK_EXCEPTION(abis.variant_to_binary(string("s2"), var2, max_serialization_time_testing), passed,
                         wasm_chain::pack_exception, "Missing field 'f0' in input object")

    json_spirit::read_string(string(R"({"f0":{"i0":1}})"), var2);
    WASM_CHECK_EXCEPTION(abis.variant_to_binary(string("s2"), var2, max_serialization_time_testing), passed,
                         wasm_chain::pack_exception, "Missing field 'i1' in input object")

    verify_round_trip_conversion(abis, "s2", R"({"f0":{"i0":1,"i1":2},"i2":3})", "010203",
                                 "abi_serialize_incomplete_json_object");

}


BOOST_AUTO_TEST_CASE( abi_serialize_json_mismatching_type ) {

    auto abi = R"({
      "version": "wasm::abi/1.0",
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "i0", "type": "int8"}
         ]},
         {"name": "s2", "base": "", "fields": [
            {"name": "f0", "type": "s1"},
            {"name": "i1", "type": "int8"}
         ]}
      ]
   })";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time_testing);

    wasm::variant var2;
    json_spirit::read_string(string(R"({"f0":1,"i1":2})"), var2);
    WASM_CHECK_EXCEPTION(abis.variant_to_binary(string("s2"), var2, max_serialization_time_testing), passed,
                         wasm_chain::pack_exception, "Unexpected input encountered while processing struct 's2.f0'")

    verify_round_trip_conversion(abis, "s2", R"({"f0":{"i0":1},"i1":2})", "0102",
                                 "abi_serialize_json_mismatching_type");

}



BOOST_AUTO_TEST_CASE( abi_serialize_json_empty_name ) {

    auto abi = R"({
      "version": "wasm::abi/1.0",
      "structs": [
         {"name": "s1", "base": "", "fields": [
            {"name": "", "type": "int8"},
         ]}
      ],
   })";

    bool passed = false;

    wasm::variant var;
    json_spirit::read_string(std::string(abi), var);
    wasm::abi_def def;
    wasm::from_variant(var, def);
    wasm::abi_serializer abis(def, max_serialization_time_testing);

    wasm::variant var2;
    json_spirit::read_string(string(R"({"":1})"), var2);
    WASM_CHECK_EXCEPTION(abis.variant_to_binary(string("s2"), var2, max_serialization_time_testing), passed,
        wasm_chain::invalid_type_inside_abi, "abi_serialize_json_empty_name")

    verify_round_trip_conversion(abis, "s1", R"({"":1})", "01",
                                 "abi_serialize_json_empty_name");

}



BOOST_AUTO_TEST_CASE( general ) {
    const char *my_abi = R"=====(
{
   "version": "wasm::abi/1.0",
   "types": [{
      "new_type_name": "type_name",
      "type": "string"
   },{
      "new_type_name": "field_name",
      "type": "string"
   },{
      "new_type_name": "fields",
      "type": "field_def[]"
   },{
      "new_type_name": "scope_name",
      "type": "name"
   }],
   "structs": [{
      "name": "abi_extension",
      "base": "",
      "fields": [{
         "name": "type",
         "type": "uint16"
      },{
         "name": "data",
         "type": "bytes"
      }]
   },{
      "name": "type_def",
      "base": "",
      "fields": [{
         "name": "new_type_name",
         "type": "type_name"
      },{
         "name": "type",
         "type": "type_name"
      }]
   },{
      "name": "field_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "field_name"
      },{
         "name": "type",
         "type": "type_name"
      }]
   },{
      "name": "struct_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "type_name"
      },{
         "name": "base",
         "type": "type_name"
      },{
         "name": "fields",
         "type": "field_def[]"
      }]
   },{
      "name": "action_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "action_name"
      },{
         "name": "type",
         "type": "type_name"
      },{
         "name": "ricardian_contract",
         "type": "string"
      }]
   },{
      "name": "table_def",
      "base": "",
      "fields": [{
         "name": "name",
         "type": "table_name"
      },{
         "name": "index_type",
         "type": "type_name"
      },{
         "name": "key_names",
         "type": "field_name[]"
      },{
         "name": "key_types",
         "type": "type_name[]"
      },{
         "name": "type",
         "type": "type_name"
      }]
   },{
     "name": "clause_pair",
     "base": "",
     "fields": [{
         "name": "id",
         "type": "string"
     },{
         "name": "body",
         "type": "string"
     }]
   },{
      "name": "abi_def",
      "base": "",
      "fields": [{
         "name": "version",
         "type": "string"
      },{
         "name": "types",
         "type": "type_def[]"
      },{
         "name": "structs",
         "type": "struct_def[]"
      },{
         "name": "actions",
         "type": "action_def[]"
      },{
         "name": "tables",
         "type": "table_def[]"
      },{
         "name": "ricardian_clauses",
         "type": "clause_pair[]"
      },{
         "name": "abi_extensions",
         "type": "abi_extension[]"
      }]
   },{
      "name"  : "A",
      "base"  : "AssetTypes",
      "fields": []
   },{
      "name": "AssetTypes",
      "base" : "NativeTypes",
      "fields": [{
         "name": "asset",
         "type": "asset"
      },{
         "name": "asset_arr",
         "type": "asset[]"
      }]
    },{
      "name": "NativeTypes",
      "fields" : [{
         "name": "string",
         "type": "string"
      },{
         "name": "string_arr",
         "type": "string[]"
      },{
         "name": "time_point",
         "type": "time_point"
      },{
         "name": "time_point_arr",
         "type": "time_point[]"
      },{
         "name": "checksum160",
         "type": "checksum160"
      },{
         "name": "checksum160_arr",
         "type": "checksum160[]"
      },{
         "name": "checksum256",
         "type": "checksum256"
      },{
         "name": "checksum256_arr",
         "type": "checksum256[]"
      },{
         "name": "checksum512",
         "type": "checksum512"
      },{
         "name": "checksum512_arr",
         "type": "checksum512[]"
      },{
         "name": "fieldname",
         "type": "field_name"
      },{
         "name": "fieldname_arr",
         "type": "field_name[]"
      },{
         "name": "typename",
         "type": "type_name"
      },{
         "name": "typename_arr",
         "type": "type_name[]"
      },{
         "name": "uint8",
         "type": "uint8"
      },{
         "name": "uint8_arr",
         "type": "uint8[]"
      },{
         "name": "uint16",
         "type": "uint16"
      },{
         "name": "uint16_arr",
         "type": "uint16[]"
      },{
         "name": "uint32",
         "type": "uint32"
      },{
         "name": "uint32_arr",
         "type": "uint32[]"
      },{
         "name": "uint64",
         "type": "uint64"
      },{
         "name": "uint64_arr",
         "type": "uint64[]"
      },{
         "name": "uint128",
         "type": "uint128"
      },{
         "name": "uint128_arr",
         "type": "uint128[]"
      },{
         "name": "int8",
         "type": "int8"
      },{
         "name": "int8_arr",
         "type": "int8[]"
      },{
         "name": "int16",
         "type": "int16"
      },{
         "name": "int16_arr",
         "type": "int16[]"
      },{
         "name": "int32",
         "type": "int32"
      },{
         "name": "int32_arr",
         "type": "int32[]"
      },{
         "name": "int64",
         "type": "int64"
      },{
         "name": "int64_arr",
         "type": "int64[]"
      },{
         "name": "int128",
         "type": "int128"
      },{
         "name": "int128_arr",
         "type": "int128[]"
      },{
         "name": "name",
         "type": "name"
      },{
         "name": "name_arr",
         "type": "name[]"
      },{
         "name": "field",
         "type": "field_def"
      },{
         "name": "field_arr",
         "type": "field_def[]"
      },{
         "name": "struct",
         "type": "struct_def"
      },{
         "name": "struct_arr",
         "type": "struct_def[]"
      },{
         "name": "fields",
         "type": "fields"
      },{
         "name": "fields_arr",
         "type": "fields[]"
      },{
         "name": "typedef",
         "type": "type_def"
      },{
         "name": "typedef_arr",
         "type": "type_def[]"
      },{
         "name": "actiondef",
         "type": "action_def"
      },{
         "name": "actiondef_arr",
         "type": "action_def[]"
      },{
         "name": "tabledef",
         "type": "table_def"
      },{
         "name": "tabledef_arr",
         "type": "table_def[]"
      },{
         "name": "abidef",
         "type": "abi_def"
      },{
         "name": "abidef_arr",
         "type": "abi_def[]"
      }]
    }
  ],
  "actions": [],
  "tables": [],
  "ricardian_clauses": [{"id":"clause A","body":"clause body A"},
              {"id":"clause B","body":"clause body B"}],
  "abi_extensions": []
}
)=====";

    const char *my_other = R"=====(
    {
      "string"            : "ola ke ase",
      "string_arr"        : ["ola ke ase","ola ke desi"],
      "time_point"        : "2021-12-20T15:30:30",
      "time_point_arr"    : ["2021-12-20T15:30:30","2021-12-20T15:31:30"],
      "checksum160"       : "ba7816bf8f01cfea414140de5dae2223b00361a3",
      "checksum160_arr"   : ["ba7816bf8f01cfea414140de5dae2223b00361a3","ba7816bf8f01cfea414140de5dae2223b00361a3"],
      "checksum256"       : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
      "checksum256_arr"   : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"],
      "checksum512"       : "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
      "checksum512_arr"   : ["ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015adba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"],
      "fieldname"         : "name1",
      "fieldname_arr"     : ["name1","name2"],
      "typename"          : "name3",
      "typename_arr"      : ["name4","name5"],
      "uint8"             : 8,
      "uint8_arr"         : [8,9],
      "uint16"            : 16,
      "uint16_arr"        : [16,17],
      "uint32"            : 32,
      "uint32_arr"        : [32,33],
      "uint64"            : 64,
      "uint64_arr"        : [64,65],
      "uint128"           : 128,
      "uint128_arr"       : [128,129],
      "int8"              : 108,
      "int8_arr"          : [108,109],
      "int16"             : 116,
      "int16_arr"         : [116,117],
      "int32"             : 132,
      "int32_arr"         : [132,133],
      "int64"             : 164,
      "int64_arr"         : [164,165],
      "int128"            : -128,
      "int128_arr"        : [-128,-129],
      "name"              : "xname1",
      "name_arr"          : ["xname2","xname3"],
      "field"             : {"name":"name1", "type":"type1"},
      "field_arr"         : [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}],
      "struct"            : {"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}]},
      "struct_arr"        : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}]},{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}]}],
      "fields"            : [{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}],
      "fields_arr"        : [[{"name":"name1", "type":"type1"}, {"name":"name2", "type":"type2"}],[{"name":"name3", "type":"type3"}, {"name":"name4", "type":"type4"}]],
      "typedef" : {"new_type_name":"new", "type":"old"},
      "typedef_arr": [{"new_type_name":"new", "type":"old"},{"new_type_name":"new", "type":"old"}],
      "actiondef"       : {"name":"actionname1", "type":"type1", "ricardian_contract":"ricardian1"},
      "actiondef_arr"   : [{"name":"actionname1", "type":"type1","ricardian_contract":"ricardian1"},{"name":"actionname2", "type":"type2","ricardian_contract":"ricardian2"}],
      "tabledef": {"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"},
      "tabledef_arr": [
         {"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"},
         {"name":"table2","index_type":"indextype2","key_names":["keyname2"],"key_types":["typename2"],"type":"type2"}
      ],
      "abidef":{
        "version": "wasm::abi/1.0",
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract":""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": [],
        "abi_extensions": []
      },
      "abidef_arr": [{
        "version": "wasm::abi/1.0",
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract":""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": [],
        "abi_extensions": []
      },{
        "version": "wasm::abi/1.0",
        "types" : [{"new_type_name":"new", "type":"old"}],
        "structs" : [{"name":"struct1", "base":"base1", "fields": [{"name":"name1", "type": "type1"}, {"name":"name2", "type": "type2"}] }],
        "actions" : [{"name":"action1","type":"type1", "ricardian_contract": ""}],
        "tables" : [{"name":"table1","index_type":"indextype1","key_names":["keyname1"],"key_types":["typename1"],"type":"type1"}],
        "ricardian_clauses": [],
        "abi_extensions": []
      }],
      "asset"         : "100.0000 SYS",
      "asset_arr"     : ["100.0000 SYS","100.0000 SYS"]
    }
    )=====";



    wasm::variant var_abi;
    json_spirit::read_string(std::string(my_abi), var_abi);
    wasm::abi_def def;
    wasm::from_variant(var_abi, def);
    auto abi = wasm::pack<wasm::abi_def>(def);


    vector<char> data = wasm::abi_serializer::pack(abi, string("A"), string(my_other), max_serialization_time_testing);
    json_spirit::Value value = wasm::abi_serializer::unpack(abi, string("A"), data, max_serialization_time_testing);
    //WASM_TRACE("%s", json_spirit::write(value).c_str())

    json_spirit::Value v;
    json_spirit::read_string(string(my_other), v);
    // std::cout << json_spirit::write(v) << std::endl;
    // std::cout << json_spirit::write(value) << std::endl;

    WASM_TEST(json_spirit::write(v) == json_spirit::write(value), "general")
}

BOOST_AUTO_TEST_CASE( optional ) {

    const char *my_abi = R"=====(
    {
        "version": "wasm::abi/1.0",
        "types"  : [],
        "structs":[{
          "name"  : "transfer",
          "fields": [{
             "name": "from",
             "type": "string"
          },{
             "name": "to",
             "type": "string"
          },{
             "name": "amount",
             "type": "uint64"
          },{
             "name": "memo",
             "type": "string?"
          }]
       }],
       "actions": [        {
            "name": "transfer",
            "type": "transfer",
            "ricardian_contract": ""
        }],
       "tables": [],
       "ricardian_clauses": [{"id":"clause A","body":"clause body A"},
                  {"id":"clause B","body":"clause body B"}],
       "abi_extensions": []
    }
    )=====";

    string param = string(R"({"from":"walker","to":"xiaoyu","amount":1000,"memo":"transfer to xiaoyu"})");
    WASM_TRACE("param:%s",param.c_str() )

    wasm::variant var_abi;
    json_spirit::read_string(std::string(my_abi), var_abi);
    wasm::abi_def def;
    wasm::from_variant(var_abi, def);
    auto abi = wasm::pack<wasm::abi_def>(def);
    //WASM_TRACE("var_abi:%s",json_spirit::write_formatted(var_abi).c_str() )

    wasm::variant var = wasm::abi_serializer::unpack(abi,
                                        "transfer",
                                        wasm::abi_serializer::pack(abi, "transfer", param, max_serialization_time_testing),
                                        max_serialization_time_testing);
    WASM_TRACE("var:%s",json_spirit::write(var).c_str() )
    WASM_TEST(param == json_spirit::write(var), "optional.transfer")


}

BOOST_AUTO_TEST_CASE( abi_token ) {

    string abi;

    char byte;
    ifstream f("token.abi", ios::binary);
    while (f.get(byte)) abi.push_back(byte);

    wasm::variant var_abi;
    json_spirit::read_string(abi, var_abi);
    wasm::abi_def def;
    wasm::from_variant(var_abi, def);
    auto abiJson = wasm::pack<wasm::abi_def>(def);

    string param = string(R"({"issuer":"walker","maximum_supply":"100.00000000 BTC"})");
    wasm::variant var = wasm::abi_serializer::unpack( abiJson,
                                                      "create",
                                                      wasm::abi_serializer::pack(abiJson, "create", param,
                                                                                 max_serialization_time_testing),
                                                      max_serialization_time_testing);

    //WASM_TRACE("%s",json_spirit::write(var).c_str() )
    WASM_TEST(param == json_spirit::write(var), "abi_token.create")

    param = string(R"({"to":"walker","quantity":"100.00000000 BTC","memo":"issue to walker"})");
    var = wasm::abi_serializer::unpack( abiJson,
                                        "issue",
                                        wasm::abi_serializer::pack(abiJson, "issue", param, max_serialization_time_testing),
                                        max_serialization_time_testing);
    WASM_TEST(param == json_spirit::write(var), "abi_token.issue")


    param = string(R"({"to":"walker","quantity":"100.00000000 BTC","memo":"issue to walker"})");
    var = wasm::abi_serializer::unpack( abiJson,
                                        "issue",
                                        wasm::abi_serializer::pack(abiJson, "issue", param, max_serialization_time_testing),
                                        max_serialization_time_testing);
    WASM_TEST(param == json_spirit::write(var), "abi_token.issue")


    param = string(R"({"quantity":"100.00000000 BTC","memo":"retire BTC"})");
    var = wasm::abi_serializer::unpack( abiJson,
                                        "retire",
                                        wasm::abi_serializer::pack(abiJson, "retire", param, max_serialization_time_testing),
                                        max_serialization_time_testing);
    WASM_TEST(param == json_spirit::write(var), "abi_token.retire")


    param = string(R"({"from":"xiaoyu","to":"walker","quantity":"100.00000000 BTC","memo":"transfer BTC"})");
    var = wasm::abi_serializer::unpack( abiJson,
                                        "transfer",
                                        wasm::abi_serializer::pack(abiJson, "transfer", param, max_serialization_time_testing),
                                        max_serialization_time_testing);
    WASM_TEST(param == json_spirit::write(var), "abi_token.transfer")


    param = string(R"({"owner":"walker","symbol":"8,BTC","payer":"xiaoyu"})");
    var = wasm::abi_serializer::unpack( abiJson,
                                        "open",
                                        wasm::abi_serializer::pack(abiJson, "open", param, max_serialization_time_testing),
                                        max_serialization_time_testing);
    WASM_TEST(param == json_spirit::write(var), "abi_token.open")


    param = string(R"({"owner":"walker","symbol":"8,BTC"})");
    var = wasm::abi_serializer::unpack( abiJson,
                                        "close",
                                        wasm::abi_serializer::pack(abiJson, "close", param, max_serialization_time_testing),
                                        max_serialization_time_testing);
    WASM_TEST(param == json_spirit::write(var), "abi_token.close")

}

BOOST_AUTO_TEST_SUITE_END()





