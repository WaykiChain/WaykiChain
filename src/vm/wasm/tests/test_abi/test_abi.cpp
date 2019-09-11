
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

using std::chrono::microseconds;
using std::chrono::system_clock;

namespace wasm {
  using variant = json_spirit::Value;
}

using namespace std;
using namespace json_spirit;
using namespace wasm;

//template<typename T>
string VectorToHexString(std::vector<char> str, string separator = " ")
{

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;
 
    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned uint8_t)str[i] >> 4] << hex[(unsigned uint8_t)str[i] & 0xf] << separator;

    return ss.str();

}

// auto max_serialization_time = microseconds(15*1000);

void abi_token(){

  try {
    string abiJson;

    char byte;
    ifstream f("token.abi", ios::binary);
    while(f.get(byte))  abiJson.push_back(byte);

    wasm::variant v;
    json_spirit::read_string(abiJson, v);

    wasm::abi_def abi_d;
    wasm::from_variant(v, abi_d);

    //string dataJson("{\"issuer\":\"walker\",\"maximum_supply\":{\"amount\":10000000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}}}");
    string dataJson("{\"issuer\":\"walker\",\"maximum_supply\":\"1000000.0000 BTC \"}");
    //string dataJson("{\"to\":\"walker\",\"quantity\":{\"amount\":10000000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}},\"memo\":\"issue to walker\"}");
    //string dataJson("{\"quantity\":{\"amount\":100000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}},\"memo\":\"retire BTC\"}");
    //string dataJson("{\"from\":\"xiaoyu\",\"to\":\"walker\",\"quantity\":{\"amount\":100000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}},\"memo\":\"transfer BTC\"}");
    //string dataJson("{\"owner\":\"walker\",\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8},\"ram_payer\":\"xiaoyu\"}");
    //string dataJson("{\"owner\":\"walker\",\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}}");
    //string dataJson("{\"owner\":\"walker\",\"symbol\":\"8,BTC\" }");

    wasm::variant dataVariant;
    json_spirit::read_string(dataJson, dataVariant);
    std::cout << json_spirit::write(dataVariant) << std::endl ;

    wasm::abi_serializer abi_serializer(abi_d, max_serialization_time);
    vector<char> dataAction = abi_serializer.variant_to_binary("create", dataVariant, max_serialization_time);


    std::cout << VectorToHexString(dataAction) << std::endl ;

   }catch( wasm::exception &e ) {
    std::cout << e.detail() << std::endl ;
   }

}


void abi_serialize_incomplete_json_array(){

// try {
    auto abi = R"({
      "version": "wasm::abi/1.0",
      "structs": [
         {"name": "s", "base": "", "fields": [
            {"name": "i0", "type": "int8"},
            {"name": "i1", "type": "int8"},
            {"name": "i2", "type": "int8"}
         ]}
      ],
   })";

   try {
        wasm::variant abi_v;
        json_spirit::read_string(string(abi), abi_v);
        wasm::abi_def def;
        wasm::from_variant(abi_v, def);
        wasm::abi_serializer abis(def, max_serialization_time);

        std::cout << "-----------------------------------------" << std::endl ;

        wasm::variant dataV;
        json_spirit::read_string(string(R"({"i0":1,"i1":2,"i2":3})"), dataV);
        vector<char> data = abis.variant_to_binary("s", dataV, max_serialization_time);

        std::cout << VectorToHexString(data) << std::endl ;

       }catch( wasm::exception &e ) {
        std::cout << e.detail() << std::endl ;
       }
}

void general (){
    const char* my_abi = R"=====(
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

   // const char *my_other = R"=====(
   //  {
   //    "asset"         : "100.0000 SYS",
   //    "asset_arr"     : ["100.0000 SYS","100.0000 SYS"],
   //  }  
   //  )=====";

      // "uint128"           : 128,
      // "uint128_arr"       : ["0x00000000000000000000000000000080",129],
   const char *my_other = R"=====(
    {
      "string"            : "ola ke ase",
      "string_arr"        : ["ola ke ase","ola ke desi"],
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
      "int8"              : 108,
      "int8_arr"          : [108,109],
      "int16"             : 116,
      "int16_arr"         : [116,117],
      "int32"             : 132,
      "int32_arr"         : [132,133],
      "int64"             : 164,
      "int64_arr"         : [164,165],
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
      "asset_arr"     : ["100.0000 SYS","100.0000 SYS"],
    }  
    )=====";

    try {
        // wasm::variant abi_v;
        // json_spirit::read_string(string(my_abi), abi_v);
        // std::cout << json_spirit::write(abi_v) << std::endl ;
        // wasm::abi_def def;
        // wasm::from_variant(abi_v, def);
        // wasm::abi_serializer abis(def, max_serialization_time);

        // wasm::variant data_v;
        // json_spirit::read_string(string(my_other), data_v);
        // std::cout << json_spirit::write(data_v) << std::endl ;
        // vector<char> data = abis.variant_to_binary("A", data_v, max_serialization_time);
      vector<char> data = wasm::abi_serializer::pack(my_abi, "A", my_other, max_serialization_time);
      std::cout << VectorToHexString(data) << std::endl ;

      json_spirit::Value value = wasm::abi_serializer::unpack(my_abi, "A", data, max_serialization_time);
      std::cout << json_spirit::write(value) << std::endl ;

    } catch (wasm::exception& e) {
        std::cout << e.detail() << std::endl;
    }


}


void abi_type_redefine(){

    try {
       const char* abi = R"=====(
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


      wasm::variant abi_v;
      json_spirit::read_string(string(abi), abi_v);
      wasm::abi_def def;
      wasm::from_variant(abi_v, def);
      wasm::abi_serializer abis(def, max_serialization_time);

     }catch( wasm::exception &e ) {
      std::cout << e.detail() << std::endl ;
     }
}

void abi_recursive_structs(){

    try {
      const char* abi = R"=====(
      {
        "version": "wasm::abi/1.0",
        "types": [],
        "structs": [
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


      // wasm::variant abi_v;
      // json_spirit::read_string(string(abi), abi_v);
      // //std::cout << json_spirit::write(abi_v) << std::endl ;

      // wasm::abi_def def;
      // wasm::from_variant(abi_v, def);
      // wasm::abi_serializer abis(def, max_serialization_time);
 
      // wasm::variant data_v;
      // json_spirit::read_string(string("{\"user\":\"wasm\"}"), data_v);
      // std::cout << json_spirit::write(data_v) << std::endl ;
      // vector<char> data = abis.variant_to_binary("hi2", data_v, max_serialization_time);

      vector<char> data = wasm::abi_serializer::pack(abi, string("hi2"), string("{\"user\":\"wasm\"}"), max_serialization_time);

      std::cout << VectorToHexString(data) << std::endl ;

     }catch( wasm::exception &e ) {
      std::cout << e.detail() << std::endl ;
     }
}

void abi_to_variant(){

  try {
      string abi;

      char byte;
      ifstream f("token.abi", ios::binary);
      while(f.get(byte))  abi.push_back(byte);

      //string json("{\"issuer\":\"walker\",\"maximum_supply\":{\"amount\":10000000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}}}");
      //string json("{\"issuer\":\"walker\",\"maximum_supply\":\"1000000.0000 BTC\"}");
      //string json("{\"to\":\"walker\",\"quantity\":{\"amount\":10000000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}},\"memo\":\"issue to walker\"}");
      //string json("{\"quantity\":{\"amount\":100000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}},\"memo\":\"retire BTC\"}");
      string json("{\"from\":\"xiaoyu\",\"to\":\"walker\",\"quantity\":{\"amount\":100000000,\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}},\"memo\":\"transfer BTC\"}");
      //string json("{\"owner\":\"walker\",\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8},\"ram_payer\":\"xiaoyu\"}");
      //string json("{\"owner\":\"walker\",\"symbol\":{\"symbol_code\":\"BTC\",\"precision\":8}}");
      //string json("{\"owner\":\"walker\",\"symbol\":\"8,BTC\" }");
      vector<char> data = wasm::abi_serializer::pack(abi, string("transfer"), json, max_serialization_time);
      std::cout << VectorToHexString(data) << std::endl ;

      json_spirit::Value value = wasm::abi_serializer::unpack(abi, string("transfer"), data, max_serialization_time);
      std::cout << json_spirit::write(value) << std::endl ;
   }catch( wasm::exception &e ) {
      std::cout << e.detail() << std::endl ;
   }

}



int main(int argc, char** argv) {

    //abi_token();
    //abi_serialize_incomplete_json_array();
    general ();
    //abi_type_redefine();
    //abi_recursive_structs();

    //abi_to_variant();
    return 0;

}

