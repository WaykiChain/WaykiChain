#pragma once

#include "wasm/types/types.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/types/varint.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
#include "json/json_spirit.h"
#include "json/json_spirit_value.h"
#include "wasm/exceptions.hpp"
#include "wasm/abi_def.hpp"
// #include "wasm/wasmvariant.hpp"
#include "wasm/wasmconfig.hpp"

namespace wasm {
    using namespace json_spirit;
    using namespace wasm;
   
    using variant = json_spirit::Value;
    using Object = json_spirit::Object;
    using Array = json_spirit::Array;

    typedef Config::Value_type::Config_type Config_type;

    static inline string FromHex( string str ) {

        std::map<char, uint8_t> hex = {
                {'0', 0x00},
                {'1', 0x01},
                {'2', 0x02},
                {'3', 0x03},
                {'4', 0x04},
                {'5', 0x05},
                {'6', 0x06},
                {'7', 0x07},
                {'8', 0x08},
                {'9', 0x09},
                {'a', 0x0a},
                {'b', 0x0b},
                {'c', 0x0c},
                {'d', 0x0d},
                {'e', 0x0e},
                {'f', 0x0f}
        };
        std::stringstream ss;

        for (std::string::size_type i = 0; i < str.size();) {

            //uint8_t t = hex[(char)str[i]] | hex[(char)str[i + 1]] << 4;
            uint8_t h = hex[(char) str[i]];
            uint8_t l = hex[(char) str[i + 1]];
            uint8_t t = l | h << 4;
            ss << t;

            i += 2;
        }

        return ss.str();

    }


    template<typename T>
    static inline string ToHex( const T &t, string separator = " " ) {
        const std::string hex = "0123456789abcdef";
        std::stringstream ss;

        for (std::string::size_type i = 0; i < t.size(); ++i)
            ss << hex[(unsigned char) t[i] >> 4] << hex[(unsigned char) t[i] & 0xf] << separator;

        return ss.str();

    }

    static inline void to_variant( const std::string &t, wasm::variant &v ) {
        v = wasm::variant(t);
    }

    static inline void to_variant( const wasm::name &t, wasm::variant &v ) {
        v = wasm::variant(t.to_string());
    }

    // static inline void to_variant( const wasm::permission &t, wasm::variant &v ) {

    //     wasm::Object obj;

    //     wasm::variant val;
    //     to_variant(wasm::name(t.account), val);
    //     json_spirit::Config::add(obj, "account", val);

    //     to_variant(wasm::name(t.perm), val);
    //     json_spirit::Config::add(obj, "permission", val);

    //     v = obj;
    // }


    static inline void to_variant( const wasm::bytes &t, wasm::variant &v ) {
        string str(t.begin(), t.end());
        v = wasm::variant(str);
    }

    static inline void to_variant( const wasm::signed_int &t, wasm::variant &v ) {
        v = wasm::variant(static_cast< int64_t >(t));
    }

    static inline void to_variant( const wasm::unsigned_int &t, wasm::variant &v ) {
        v = wasm::variant(static_cast< uint64_t >(t));
    }

    static inline void to_variant( const bool &t, wasm::variant &v ) {
        v = wasm::variant(t);
    }

    template<typename T, std::enable_if_t <std::is_floating_point<T>::value> * = nullptr>
    static inline void to_variant( const T &t, wasm::variant &v ) {
        v = wasm::variant(t);
    }

    template<typename T, std::enable_if_t <std::is_integral<T>::value> * = nullptr>
    static inline void to_variant( const T &t, wasm::variant &v ) {
        v = wasm::variant(static_cast< int64_t >(t));
    }


    template<typename T>
    static inline void to_variant( const std::vector <T> &ts, wasm::variant &v ) {

        wasm::Array var;
        for (const T &t: ts) {
            wasm::variant tmp;
            to_variant(t, tmp);
            var.push_back(tmp);
        }

        v = var;
    }

    template<typename T>
    static inline void to_variant( const std::optional <T> &t, wasm::variant &v ) {
        if (t.has_value()) {
            to_variant(t, v);
            return;
        }
        v = wasm::variant();
    }

    static inline void to_variant( const wasm::symbol_code &t, wasm::variant &v ) {
        v = wasm::variant(t.to_string());

    }

    static inline void to_variant( const wasm::symbol &t, wasm::variant &v ) {
        v = wasm::variant(t.to_string());

    }

    static inline void to_variant( const wasm::asset &t, wasm::variant &v ) {
        v = wasm::variant(t.to_string());

    }

    static inline void from_variant( const wasm::variant &v, std::string &t ) {
        if (v.type() == json_spirit::str_type) {
            t = v.get_str();
            //std::cout << "-" << t << std::endl ;
        }
    }

    static inline void from_variant( const wasm::variant &v, wasm::name &t ) {
        if (v.type() == json_spirit::str_type) {
            t = wasm::name(v.get_str());
            //std::cout << "-" << t.to_string() << std::endl ;
        }
    }

    static inline void from_variant( const wasm::variant &v, wasm::bytes &t ) {
        if (v.type() == json_spirit::str_type) {
            string str = v.get_str();
            t.insert(t.begin(), str.begin(), str.end());
        }
    }

    static inline void from_variant( const wasm::variant &v, wasm::signed_int &t ) {
        if (v.type() == json_spirit::int_type) {
            t = static_cast< int32_t >(v.get_int64());
        }
    }

    static inline void from_variant( const wasm::variant &v, wasm::unsigned_int &t ) {
        if (v.type() == json_spirit::int_type) {
            t = static_cast< uint32_t >(v.get_int64());
        }
    }


    static inline void from_variant( const wasm::variant &v, bool &t ) {
        if (v.type() == json_spirit::bool_type) {
            t = v.get_bool();
        }
    }

    template<typename T, std::enable_if_t <std::is_floating_point<T>::value> * = nullptr>
    static inline void from_variant( const wasm::variant &v, T &t ) {
        if (v.type() == json_spirit::int_type) {
            t = v.get_real();
        }
    }


    template<typename T, std::enable_if_t <std::is_integral<T>::value> * = nullptr>
    static inline void from_variant( const wasm::variant &v, T &t ) {
        if (v.type() == json_spirit::int_type) {
            t = v.get_int64();
        }
    }

    template<typename T>
    static inline void from_variant( const wasm::variant &v, vector <T> &ts ) {
        if (v.type() == json_spirit::array_type) {

            auto a = v.get_array();
            for (wasm::Array::const_iterator i = a.begin(); i != a.end(); ++i) {
                T t;
                from_variant(*i, t);
                ts.push_back(t);
            }

        }

    }

    template<typename T>
    static inline void from_variant( const wasm::variant &v, optional <T> &opt ) {

        if (v.type() == json_spirit::null_type) {
            return;
        }

        T t;
        from_variant(v, t);
        opt = t;
    }


    static inline void from_variant( const wasm::variant &v, wasm::symbol_code &t ) {
        if (v.type() == json_spirit::str_type) {
            t = wasm::symbol_code(v.get_str());
        }
    }

    static inline void from_variant( const wasm::variant &v, wasm::symbol &t ) {
        if (v.type() == json_spirit::obj_type) {
            wasm::symbol_code symbol_code;
            uint8_t precision;
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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

    static inline void from_variant( const wasm::variant &v, wasm::asset &t ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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


    static inline void from_variant( const wasm::variant &v, field_def &field ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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

    static inline void from_variant( const wasm::variant &v, struct_def &s ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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


    static inline void from_variant( const wasm::variant &v, action_def &a ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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


    static inline void from_variant( const wasm::variant &v, table_def &table ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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


    static inline void from_variant( const wasm::variant &v, clause_pair &c ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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


    static inline void from_variant( const wasm::variant &v, type_def &type ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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


    static inline void from_variant( const wasm::variant &v, abi_def &abi ) {

        try {
            if (v.type() == json_spirit::obj_type) {
                auto o = v.get_obj();

                for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
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

        //WASM_RETHROW_EXCEPTIONS(ABI_PARSE_FAIL, "ABI_PARSE_FAIL", "abi parse fail ")
        WASM_RETHROW_EXCEPTIONS(abi_parse_exception, "abi parse fail ")
    }


} //wasm


