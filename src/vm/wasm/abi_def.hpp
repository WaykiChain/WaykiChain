#pragma once

#include "wasm/types/types.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/types/varint.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
// #include "wasm/exceptions.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/datastream.hpp"
#include "wasm/wasm_serialize_reflect.hpp"

#include "commons/json/json_spirit.h"
#include "commons/json/json_spirit_value.h"



namespace wasm {
    using namespace json_spirit;
    using namespace wasm;

//
    typedef Config::Value_type::Config_type Config_type;


    using type_name      = string;
    using field_name     = string;

    struct type_def {
        type_def() = default;

        type_def( const type_name &new_type_name, const type_name &type )
                : new_type_name(new_type_name), type(type) {}

        type_name new_type_name;
        type_name type;

        WASM_REFLECT( type_def, (new_type_name)(type) )
    };

    struct field_def {
        field_def() = default;

        field_def( const field_name &name, const type_name &type )
                : name(name), type(type) {}

        field_name name;
        type_name  type;

        bool operator==( const field_def &other ) const {
            return std::tie(name, type) == std::tie(other.name, other.type);
        }

        WASM_REFLECT( field_def, (name)(type) )
    };

    struct struct_def {
        struct_def() = default;

        struct_def( const type_name &name, const type_name &base, const vector <field_def> &fields )
                : name(name), base(base), fields(fields) {}

        type_name          name;
        type_name          base;
        vector <field_def> fields;

        bool operator==( const struct_def &other ) const {
            return std::tie(name, base, fields) == std::tie(other.name, other.base, other.fields);
        }

        WASM_REFLECT( struct_def, (name)(base)(fields) )
    };

    struct action_def {
        action_def() = default;

        action_def( const type_name &name, const type_name &type, const string &ricardian_contract )
                : name(name), type(type), ricardian_contract(ricardian_contract) {}

        //action_name name;
        type_name name;
        type_name type;
        string    ricardian_contract;

        WASM_REFLECT( action_def, (name)(type)(ricardian_contract) )
    };

    struct table_def {
        table_def() = default;

        table_def( const type_name &name, const type_name &index_type, const vector <field_name> &key_names,
                   const vector <type_name> &key_types, const type_name &type )
                : name(name), index_type(index_type), key_names(key_names), key_types(key_types), type(type) {}

        //table_name         name;       // the name of the table
        type_name           name;        // the name of the table
        type_name           index_type;  // the kind of index, i64, i128i128, etc
        vector <field_name> key_names;   // names for the keys defined by key_types
        vector <type_name>  key_types;   // the type of key parameters
        type_name           type;        // type of binary data stored in this table

        WASM_REFLECT( table_def, (name)(index_type)(key_names)(key_types)(type) )
    };

    struct clause_pair {
        clause_pair() = default;

        clause_pair( const string &id, const string &body )
                : id(id), body(body) {}

        string id;
        string body;

        WASM_REFLECT( clause_pair, (id)(body) )
    };

    struct error_message {
        error_message() = default;

        error_message( uint64_t error_code, const string &error_msg )
                : error_code(error_code), error_msg(error_msg) {}

        uint64_t error_code;
        string error_msg;

        WASM_REFLECT( error_message, (error_code)(error_msg) )
    };

    struct abi_def {
        abi_def() = default;

        abi_def( const vector <type_def> &types, const vector <struct_def> &structs, const vector <action_def> &actions,
                 const vector <table_def> &tables, const vector <clause_pair> &clauses,
                 const vector <error_message> &error_msgs )
                : types(types), structs(structs), actions(actions), tables(tables), ricardian_clauses(clauses) {}

        string version = "";
        vector <type_def> types;
        vector <struct_def> structs;
        vector <action_def> actions;
        vector <table_def> tables;
        vector <clause_pair> ricardian_clauses;
        //vector<error_message> error_messages;

        WASM_REFLECT( abi_def, (version)(types)(structs)(actions)(tables)(ricardian_clauses) )

    };

    enum abi_typeid_t {
        ID_Comment = 0,
        ID_Version,
        ID_Structs,
        ID_Types,
        ID_Actions,
        ID_Tables,
        ID_Ricardian_clauses,
        ID_Variants,
        ID_Abi_extensions,
        ID_Name,
        ID_Base,
        ID_Fields,
        ID_Type,
        ID_Ricardian_contract,
        ID_ID,
        ID_Body,
        ID_Index_type,
        ID_Key_names,
        ID_Key_types,
        ID_New_type_name,
        ID_Symbolcode,
        ID_Precision,
        ID_Symbol,
        ID_Amount,
    };

    static std::map <std::string, abi_typeid_t> string2typeid = {
            {"____comment",        ID_Comment},
            {"version",            ID_Version},
            {"structs",            ID_Structs},
            {"types",              ID_Types},
            {"actions",            ID_Actions},
            {"tables",             ID_Tables},
            {"ricardian_clauses",  ID_Ricardian_clauses},
            {"variants",           ID_Variants},
            {"abi_extensions",     ID_Abi_extensions},
            {"name",               ID_Name},
            {"base",               ID_Base},
            {"fields",             ID_Fields},
            {"type",               ID_Type},
            {"ricardian_contract", ID_Ricardian_contract},
            {"id",                 ID_ID},
            {"body",               ID_Body},
            {"index_type",         ID_Index_type},
            {"key_names",          ID_Key_names},
            {"types",              ID_Key_types},
            {"new_type_name",      ID_New_type_name},
            {"symbol_code",        ID_Symbolcode},
            {"precision",          ID_Precision},
            {"symbol",             ID_Symbol},
            {"amount",             ID_Amount},
    };


} //wasm


