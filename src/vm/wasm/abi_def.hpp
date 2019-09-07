#pragma once

#include "wasm/types/types.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/types/varint.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
#include "json/json_spirit.h"
#include "json/json_spirit_value.h"
#include "wasm/exceptions.hpp"

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
    };

    struct field_def {
        field_def() = default;

        field_def( const field_name &name, const type_name &type )
                : name(name), type(type) {}

        field_name name;
        type_name type;

        bool operator==( const field_def &other ) const {
            return std::tie(name, type) == std::tie(other.name, other.type);
        }
    };

    struct struct_def {
        struct_def() = default;

        struct_def( const type_name &name, const type_name &base, const vector <field_def> &fields )
                : name(name), base(base), fields(fields) {}

        type_name name;
        type_name base;
        vector <field_def> fields;

        bool operator==( const struct_def &other ) const {
            return std::tie(name, base, fields) == std::tie(other.name, other.base, other.fields);
        }
    };

    struct action_def {
        action_def() = default;

        action_def( const type_name &name, const type_name &type, const string &ricardian_contract )
                : name(name), type(type), ricardian_contract(ricardian_contract) {}

        //action_name name;
        type_name name;
        type_name type;
        string ricardian_contract;
    };

    struct table_def {
        table_def() = default;

        table_def( const type_name &name, const type_name &index_type, const vector <field_name> &key_names,
                   const vector <type_name> &key_types, const type_name &type )
                : name(name), index_type(index_type), key_names(key_names), key_types(key_types), type(type) {}

        //table_name         name;        // the name of the table
        type_name name;        // the name of the table
        type_name index_type;  // the kind of index, i64, i128i128, etc
        vector <field_name> key_names;   // names for the keys defined by key_types
        vector <type_name> key_types;   // the type of key parameters
        type_name type;        // type of binary data stored in this table
    };

    struct clause_pair {
        clause_pair() = default;

        clause_pair( const string &id, const string &body )
                : id(id), body(body) {}

        string id;
        string body;
    };

    struct error_message {
        error_message() = default;

        error_message( uint64_t error_code, const string &error_msg )
                : error_code(error_code), error_msg(error_msg) {}

        uint64_t error_code;
        string error_msg;
    };

// struct variant_def {
//    type_name            name;
//    vector<type_name>    types;
// };

// template<typename T>
// struct may_not_exist {
//    T value{};
// };

    struct abi_def {
        abi_def() = default;

        abi_def( const vector <type_def> &types, const vector <struct_def> &structs, const vector <action_def> &actions,
                 const vector <table_def> &tables, const vector <clause_pair> &clauses,
                 const vector <error_message> &error_msgs )
                : types(types), structs(structs), actions(actions), tables(tables), ricardian_clauses(clauses),
                  error_messages(error_msgs) {}

        string version = "";
        vector <type_def> types;
        vector <struct_def> structs;
        vector <action_def> actions;
        vector <table_def> tables;
        vector <clause_pair> ricardian_clauses;
        vector <error_message> error_messages;
        // extensions_type                     abi_extensions;
        // may_not_exist<vector<variant_def>>  variants;
    };

    enum ABI_Enum {
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

    static std::map <std::string, ABI_Enum> mapStringValues = {
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
            {"sym",                ID_Symbol},
            {"symbol",             ID_Symbol},
            {"amount",             ID_Amount},
    };

    static inline void to_variant( const std::string &t, json_spirit::Value &v ) {
        v = json_spirit::Value(t);
    }

    static inline void to_variant( const wasm::name &t, json_spirit::Value &v ) {
        v = json_spirit::Value(t.to_string());
    }

    static inline void to_variant( const wasm::bytes &t, json_spirit::Value &v ) {
        string str(t.begin(), t.end());
        v = json_spirit::Value(str);
    }

    static inline void to_variant( const wasm::signed_int &t, json_spirit::Value &v ) {
        v = json_spirit::Value(static_cast< int64_t >(t));
    }

    static inline void to_variant( const wasm::unsigned_int &t, json_spirit::Value &v ) {
        v = json_spirit::Value(static_cast< uint64_t >(t));
    }

    static inline void to_variant( const bool &t, json_spirit::Value &v ) {
        v = json_spirit::Value(t);
    }

    template<typename T, std::enable_if_t <std::is_floating_point<T>::value> * = nullptr>
    static inline void to_variant( const T &t, json_spirit::Value &v ) {
        v = json_spirit::Value(t);
    }

    template<typename T, std::enable_if_t <std::is_integral<T>::value> * = nullptr>
    static inline void to_variant( const T &t, json_spirit::Value &v ) {
        v = json_spirit::Value(static_cast< int64_t >(t));
    }


    template<typename T>
    static inline void to_variant( const std::vector <T> &ts, json_spirit::Value &v ) {

        json_spirit::Array var;
        for (const T &t: ts) {
            json_spirit::Value tmp;
            to_variant(t, tmp);
            var.push_back(tmp);
        }

        v = var;
    }

    template<typename T>
    static inline void to_variant( const std::optional <T> &t, json_spirit::Value &v ) {
        if (t.has_value()) {
            to_variant(t, v);
            return;
        }
        v = json_spirit::Value();
    }

    static inline void to_variant( const wasm::symbol_code &t, json_spirit::Value &v ) {
        v = json_spirit::Value(t.to_string());

    }

    static inline void to_variant( const wasm::symbol &t, json_spirit::Value &v ) {
        v = json_spirit::Value(t.to_string());

    }

    static inline void to_variant( const wasm::asset &t, json_spirit::Value &v ) {
        v = json_spirit::Value(t.to_string());

    }


    static inline void from_variant( const json_spirit::Value &v, std::string &t ) {
        if (v.type() == json_spirit::str_type) {
            t = v.get_str();
            //std::cout << "-" << t << std::endl ;
        }
    }

    static inline void from_variant( const json_spirit::Value &v, wasm::name &t ) {
        if (v.type() == json_spirit::str_type) {
            t = wasm::name(v.get_str());
            //std::cout << "-" << t.to_string() << std::endl ;
        }
    }

    static inline void from_variant( const json_spirit::Value &v, wasm::bytes &t ) {
        if (v.type() == json_spirit::str_type) {
            string str = v.get_str();
            t.insert(t.begin(), str.begin(), str.end());
        }
    }

    static inline void from_variant( const json_spirit::Value &v, wasm::signed_int &t ) {
        if (v.type() == json_spirit::int_type) {
            t = static_cast< int32_t >(v.get_int64());
        }
    }

    static inline void from_variant( const json_spirit::Value &v, wasm::unsigned_int &t ) {
        if (v.type() == json_spirit::int_type) {
            t = static_cast< uint32_t >(v.get_int64());
        }
    }


    static inline void from_variant( const json_spirit::Value &v, bool &t ) {
        if (v.type() == json_spirit::bool_type) {
            t = v.get_bool();
        }
    }

    template<typename T, std::enable_if_t <std::is_floating_point<T>::value> * = nullptr>
    static inline void from_variant( const json_spirit::Value &v, T &t ) {
        if (v.type() == json_spirit::int_type) {
            t = v.get_real();
        }
    }

    static inline string VToHexString( std::vector<char> str, string separator = " " ) {

        const std::string hex = "0123456789abcdef";
        std::stringstream ss;

        for (std::string::size_type i = 0; i < str.size(); ++i)
            ss << hex[(unsigned
        uint8_t)str[i] >> 4] << hex[(unsigned
        uint8_t)str[i] & 0xf] << separator;

        return ss.str();

    }


    template<typename T, std::enable_if_t <std::is_integral<T>::value> * = nullptr>
    static inline void from_variant( const json_spirit::Value &v, T &t ) {
        if (v.type() == json_spirit::int_type) {
            t = v.get_int64();
        }
    }

    template<typename T>
    static inline void from_variant( const json_spirit::Value &v, vector <T> &ts ) {
        if (v.type() == json_spirit::array_type) {

            auto a = v.get_array();
            for (json_spirit::Array::const_iterator i = a.begin(); i != a.end(); ++i) {
                T t;
                from_variant(*i, t);
                ts.push_back(t);
            }

        }

    }

    template<typename T>
    static inline void from_variant( const json_spirit::Value &v, optional <T> &opt ) {

        if (v.type() == json_spirit::null_type) {
            return;
        }

        T t;
        from_variant(v, t);
        opt = t;
    }


    static inline void from_variant( const json_spirit::Value &v, wasm::symbol_code &t ) {
        if (v.type() == json_spirit::str_type) {
            t = wasm::symbol_code(v.get_str());
        }
    }

    static inline void from_variant( const json_spirit::Value &v, wasm::symbol &t ) {
        if (v.type() == json_spirit::obj_type) {
            wasm::symbol_code symbol_code;
            uint8_t precision;
            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_Symbolcode : {
                        from_variant(Config_type::get_value(*i), symbol_code);
                        //std::cout << symbol_code <<std::endl ;
                        break;
                    }
                    case ID_Precision : {
                        from_variant(Config_type::get_value(*i), precision);
                        //std::cout << "precision line203" <<std::endl ;
                        break;
                    }
                    default :
                        break;
                }
            }
            t = wasm::symbol(symbol_code, precision);

        } else if (v.type() == json_spirit::str_type) {
            t = wasm::symbol::from_string(v.get_str());
        }
    }

    static inline void from_variant( const json_spirit::Value &v, wasm::asset &t ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_Amount : {
                        from_variant(Config_type::get_value(*i), t.amount);
                        //std::cout << t.amount <<std::endl ;
                        break;
                    }
                    case ID_Symbol : {
                        from_variant(Config_type::get_value(*i), t.sym);
                        //std::cout << t.sym.to_string() <<std::endl ;
                        break;
                    }
                    default :
                        break;
                }
            }

            //std::cout << "-" << t.to_string() << std::endl ;


        } else if (v.type() == json_spirit::str_type) { //"100.0000 SYS"

            wasm::asset a;
            t = a.from_string(v.get_str());
            //std::cout << "-" << t.to_string() << std::endl ;

        }
    }


    static inline void from_variant( const json_spirit::Value &v, field_def &field ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_Name : {
                        from_variant(Config_type::get_value(*i), field.name);
                        //std::cout << field.name <<std::endl ;
                        break;
                    }
                    case ID_Type : {
                        from_variant(Config_type::get_value(*i), field.type);
                        //std::cout << field.type <<std::endl ;
                        break;
                    }
                    default :
                        break;
                }

            }
        }
    }

    static inline void from_variant( const json_spirit::Value &v, struct_def &s ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_Name : {
                        from_variant(Config_type::get_value(*i), s.name);
                        //std::cout << s.name <<std::endl ;
                        break;
                    }
                    case ID_Base : {
                        from_variant(Config_type::get_value(*i), s.base);
                        //std::cout << s.base <<std::endl ;
                        break;
                    }
                    case ID_Fields : {
                        from_variant(Config_type::get_value(*i), s.fields);
                        //std::cout << s.base <<std::endl ;
                        break;
                    }
                    default :
                        break;
                }
            }

        }

    }


    static inline void from_variant( const json_spirit::Value &v, action_def &a ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_Name : {
                        from_variant(Config_type::get_value(*i), a.name);
                        //std::cout << a.name <<std::endl ;
                        break;
                    }
                    case ID_Type : {
                        from_variant(Config_type::get_value(*i), a.type);
                        //std::cout << a.type <<std::endl ;
                        break;
                    }
                    case ID_Ricardian_contract : {
                        from_variant(Config_type::get_value(*i), a.ricardian_contract);
                        //std::cout << a.ricardian_contract <<std::endl ;
                        break;
                    }
                    default :
                        break;
                }
            }

        }

    }


    static inline void from_variant( const json_spirit::Value &v, table_def &table ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_Name : {
                        from_variant(Config_type::get_value(*i), table.name);
                        break;
                    }
                    case ID_Type : {
                        from_variant(Config_type::get_value(*i), table.type);
                        break;
                    }
                    case ID_Index_type : {
                        from_variant(Config_type::get_value(*i), table.index_type);
                        break;
                    }
                    case ID_Key_types : {
                        from_variant(Config_type::get_value(*i), table.key_types);
                        break;
                    }
                    case ID_Key_names : {
                        from_variant(Config_type::get_value(*i), table.key_names);
                        break;
                    }
                    default :
                        break;
                }
            }

        }

    }


    static inline void from_variant( const json_spirit::Value &v, clause_pair &c ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_ID : {
                        from_variant(Config_type::get_value(*i), c.id);
                        break;
                    }
                    case ID_Body : {
                        from_variant(Config_type::get_value(*i), c.body);
                        break;
                    }
                    default :
                        break;
                }
            }

        }

    }


    static inline void from_variant( const json_spirit::Value &v, type_def &type ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];
                switch (abi_key) {
                    case ID_New_type_name : {
                        from_variant(Config_type::get_value(*i), type.new_type_name);
                        break;
                    }
                    case ID_Type : {
                        from_variant(Config_type::get_value(*i), type.type);
                        break;
                    }
                    default :
                        break;
                }
            }

        }

    }


    static inline void from_variant( const json_spirit::Value &v, abi_def &abi ) {

        try {
            if (v.type() == json_spirit::obj_type) {
                auto o = v.get_obj();

                for (json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                    string key = Config_type::get_name(*i);
                    ABI_Enum abi_key = mapStringValues[key];

                    switch (abi_key) {
                        case ID_Comment :
                            break;
                        case ID_Version : {
                            abi.version = Config_type::get_value(*i).get_str();
                            break;
                        }
                        case ID_Structs : {
                            from_variant(Config_type::get_value(*i), abi.structs);
                            break;
                        }
                        case ID_Actions : {
                            from_variant(Config_type::get_value(*i), abi.actions);
                            break;
                        }
                        case ID_Types : {
                            from_variant(Config_type::get_value(*i), abi.types);
                            break;
                        }
                        case ID_Tables : {
                            from_variant(Config_type::get_value(*i), abi.tables);
                            break;
                        }
                        case ID_Ricardian_clauses : {
                            from_variant(Config_type::get_value(*i), abi.ricardian_clauses);
                            break;
                        }
                        case ID_Variants :
                            break;
                        case ID_Abi_extensions :
                            break;
                        default :
                            break;
                    }
                }
            }
        } 

        WASM_RETHROW_EXCEPTIONS(ABI_PARSE_FAIL, "ABI_PARSE_FAIL", "abi parse fail ")
    }


} //wasm


