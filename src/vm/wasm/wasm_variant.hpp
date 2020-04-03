#pragma once
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

#include <chrono>

#include "wasm/types/types.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/types/varint.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/types/regid.hpp"
#include "commons/json/json_spirit.h"
#include "commons/json/json_spirit_value.h"
//#include "wasm/exceptions.hpp"
#include "wasm/abi_def.hpp"
#include "wasm/wasm_constants.hpp"
#include "wasm/wasm_log.hpp"

#include "wasm/exception/exceptions.hpp"

namespace wasm {
    using namespace json_spirit;
    using namespace wasm;
    using std::chrono::system_clock;

    using variant = json_spirit::Value;
    using object = json_spirit::Object;
    using array = json_spirit::Array;

    typedef Config::Value_type::Config_type Config_type;

    static inline string from_hex( string str ) {

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
        std::ostringstream o;

        for (std::string::size_type i = 0; i < str.size();) {

            //uint8_t t = hex[(char)str[i]] | hex[(char)str[i + 1]] << 4;
            uint8_t h = hex[(char) str[i]];
            uint8_t l = hex[(char) str[i + 1]];
            uint8_t t = l | h << 4;
            o << t;

            i += 2;
        }

        return o.str();

    }

    template<typename T>
    static inline string to_hex( const T &t, string separator = "" ) {
        const std::string hex = "0123456789abcdef";
        std::ostringstream o;

        for (std::string::size_type i = 0; i < t.size(); ++i)
            o << hex[(unsigned char) t[i] >> 4] << hex[(unsigned char) t[i] & 0xf] << separator;

        return o.str();

    }

    static inline string from_time( const std::time_t &t ) {

        char szTime[128];
        std::tm *p;
        p = localtime(&t);
        sprintf(szTime, "%4d-%2d-%2dT%2d:%2d:%2d", p->tm_year + 1900, p->tm_mon + 1, p->tm_mday, p->tm_hour, p->tm_min,
                p->tm_sec);

        return string(szTime);
    }

    static inline std::time_t to_time( const std::string &t ) {

        int year, month, day, hour, minute, second;
        sscanf((char *) t.data(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
        std::tm time = {};
        time.tm_year = year - 1900;
        time.tm_mon = month - 1;
        time.tm_mday = day;
        time.tm_hour = hour;
        time.tm_min = minute;
        time.tm_sec = second;
        time.tm_isdst = 0;
        time_t t_ = mktime(&time);

        return t_;
    }

    static inline void to_variant( const std::string &t, wasm::variant &v ) {
        v = wasm::variant(t);
    }

    static inline void to_variant( const wasm::name &t, wasm::variant &v ) {
        v = wasm::variant(t.to_string());
    }

    static inline void to_variant( const wasm::regid &t, wasm::variant &v ) {
        v = wasm::variant(t.to_string());
    }

    static inline void to_variant( const wasm::bytes &t, wasm::variant &v ) {
        string str(t.begin(), t.end());
        v = wasm::variant(str);
    }

    static inline void to_variant( const int128_t &t, wasm::variant &v ) {

        bool is_negative = (t < 0);
        uint128_t val_magnitude;
        if (is_negative)
            val_magnitude = static_cast<uint128_t>(-t); // Works even if val is at the lowest possible value of a int128_t
        else
            val_magnitude = static_cast<uint128_t>(t);

        uint128 u(val_magnitude);

        if (!is_negative) {
            if (u.hi == 0) {
                v = wasm::variant(u.lo);
                return;
            }
            v = wasm::variant(string(u));
            return;
        } else {
            if (u.hi == 0) {
                int64_t lo = u.lo;
                v = wasm::variant(-lo);
                return;
            }
            v = wasm::variant("-" + string(u));
            return;
        }

    }

    static inline void to_variant( const uint128_t &t, wasm::variant &v ) {
        uint128 u(t);

        if (u.hi == 0) {
            v = wasm::variant(u.lo);
            return;
        }

        v = wasm::variant(string(u));
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

    static inline void to_variant( const system_clock::time_point &t, wasm::variant &v ) {

        std::time_t time = std::chrono::system_clock::to_time_t(t);
        v = wasm::variant(from_time(time));
    }


    template<typename T>
    static inline void to_variant( const std::vector <T> &ts, wasm::variant &v ) {

        wasm::array var;
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
            to_variant(t.value(), v);
            return;
        }
        v = wasm::variant();
    }

    static inline void to_variant( const wasm::checksum160_type &t, wasm::variant &v ) {
        //to_variant(t.hash, v);
        string str(&t.hash[0], &t.hash[sizeof(t.hash) / sizeof(t.hash[0])]);
        v = wasm::variant(to_hex(str, ""));


    }

    static inline void to_variant( const wasm::checksum256_type &t, wasm::variant &v ) {
        string str(&t.hash[0], &t.hash[sizeof(t.hash) / sizeof(t.hash[0])]);
        v = wasm::variant(to_hex(str, ""));

    }

    static inline void to_variant( const wasm::checksum512_type &t, wasm::variant &v ) {
        string str(&t.hash[0], &t.hash[sizeof(t.hash) / sizeof(t.hash[0])]);

        v = wasm::variant(to_hex(str, ""));
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

    static inline void to_variant( const wasm::type_def &t, wasm::variant &v ) {

        wasm::object obj;

        obj.push_back(Pair("new_type_name", wasm::variant(t.new_type_name)));
        obj.push_back(Pair("type", wasm::variant(t.type)));

        v = obj;
    }

    static inline void to_variant( const wasm::field_def &t, wasm::variant &v ) {
        wasm::object obj;

        obj.push_back(Pair("name", wasm::variant(t.name)));
        obj.push_back(Pair("type", wasm::variant(t.type)));

        v = obj;
    }

    static inline void to_variant( const wasm::struct_def &t, wasm::variant &v ) {

        wasm::object obj;

        obj.push_back(Pair("name", wasm::variant(t.name)));
        obj.push_back(Pair("base", wasm::variant(t.base)));

        wasm::variant fields;
        to_variant(t.fields, fields);
        obj.push_back(Pair("fields", fields));

        v = obj;
    }

    static inline void to_variant( const wasm::action_def &t, wasm::variant &v ) {
        wasm::object obj;

        obj.push_back(Pair("name", wasm::variant(t.name)));
        obj.push_back(Pair("type", wasm::variant(t.type)));
        obj.push_back(Pair("ricardian_contract", wasm::variant(t.ricardian_contract)));

        v = obj;
    }

    static inline void to_variant( const wasm::table_def &t, wasm::variant &v ) {

        wasm::object obj;

        obj.push_back(Pair("name", wasm::variant(t.name)));
        obj.push_back(Pair("index_type", wasm::variant(t.index_type)));

        wasm::variant key_names;
        to_variant(t.key_names, key_names);
        obj.push_back(Pair("key_names", key_names));

        wasm::variant key_types;
        to_variant(t.key_types, key_types);
        obj.push_back(Pair("key_types", key_types));

        obj.push_back(Pair("type", wasm::variant(t.type)));

        v = obj;
    }

    static inline void to_variant( const wasm::clause_pair &t, wasm::variant &v ) {

        wasm::object obj;

        obj.push_back(Pair("id", wasm::variant(t.id)));
        obj.push_back(Pair("body", wasm::variant(t.body)));

        v = obj;

    }

    static inline void to_variant( const wasm::abi_def &t, wasm::variant &v ) {

        wasm::object obj;

        obj.push_back(Pair("version", wasm::variant(t.version)));

        wasm::variant types;
        to_variant(t.types, types);
        obj.push_back(Pair("types", types));

        wasm::variant structs;
        to_variant(t.structs, structs);
        obj.push_back(Pair("structs", structs));

        wasm::variant actions;
        to_variant(t.actions, actions);
        obj.push_back(Pair("actions", actions));

        wasm::variant tables;
        to_variant(t.tables, tables);
        obj.push_back(Pair("tables", tables));

        wasm::variant ricardian_clauses;
        to_variant(t.ricardian_clauses, ricardian_clauses);
        obj.push_back(Pair("ricardian_clauses", ricardian_clauses));

        v = obj;

    }

    static inline void from_variant( const wasm::variant &v, std::string &t ) {
        if (v.type() == json_spirit::str_type) {
            t = v.get_str();
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }

    static inline void from_variant( const wasm::variant &v, wasm::name &t ) {
        if (v.type() == json_spirit::str_type) {
            t = wasm::name(v.get_str());
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }

    static inline void from_variant( const wasm::variant &v, wasm::regid &t ) {
        if (v.type() == json_spirit::str_type) {
            t = wasm::regid(v.get_str());
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }

    static inline void from_variant( const wasm::variant &v, wasm::bytes &t ) {
        if (v.type() == json_spirit::str_type) {
            string str = v.get_str();
            t.insert(t.begin(), str.begin(), str.end());
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")

    }

    static inline void from_variant( const wasm::variant &v, wasm::int128_t &t ) {

        if (v.type() == json_spirit::str_type) {
            string str = v.get_str();

            bool is_negative = (str[0] == '-') ? true : false;
            if (is_negative)
                str.substr(1, str.size() - 1);
            uint128 u(str);
            t = is_negative ? (-wasm::uint128_t(u)) : wasm::uint128_t(u);
            return;

        } else if (v.type() == json_spirit::int_type) {
            t = static_cast< wasm::int128_t >(v.get_int64());
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string or int")

    }

    static inline void from_variant( const wasm::variant &v, wasm::uint128_t &t ) {

        if (v.type() == json_spirit::str_type) {
            string str = v.get_str();
            uint128 u(str);
            t = wasm::uint128_t(u);
            return;
        } else if (v.type() == json_spirit::int_type) {
            t = static_cast< wasm::uint128_t >(v.get_uint64());
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string or int")
    }


    static inline void from_variant( const wasm::variant &v, wasm::signed_int &t ) {
        if (v.type() == json_spirit::int_type) {
            t = static_cast< int32_t >(v.get_int64());
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an int")
    }

    static inline void from_variant( const wasm::variant &v, wasm::unsigned_int &t ) {
        if (v.type() == json_spirit::int_type) {
            t = static_cast< uint32_t >(v.get_int64());
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an int")
    }


    static inline void from_variant( const wasm::variant &v, bool &t ) {
        if (v.type() == json_spirit::bool_type) {
            t = v.get_bool();
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a boolean")
    }

    template<typename T, std::enable_if_t <std::is_floating_point<T>::value> * = nullptr>
    static inline void from_variant( const wasm::variant &v, T &t ) {
        if (v.type() == json_spirit::real_type) {
            t = v.get_real();
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a real (floating point)")
    }


    template<typename T, std::enable_if_t <std::is_integral<T>::value> * = nullptr>
    static inline void from_variant( const wasm::variant &v, T &t ) {
        if (v.type() == json_spirit::int_type) {
            t = v.get_int64();
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an int")
    }

    static inline void from_variant( const wasm::variant &v, system_clock::time_point &t ) {
        if (v.type() == json_spirit::str_type) {
            t = std::chrono::system_clock::from_time_t(to_time(v.get_str()));
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }

    template<typename T>
    static inline void from_variant( const wasm::variant &v, vector <T> &ts ) {
        if (v.type() == json_spirit::array_type) {

            auto a = v.get_array();
            for (wasm::array::const_iterator i = a.begin(); i != a.end(); ++i) {
                T t;
                from_variant(*i, t);
                ts.push_back(t);
            }
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a array[]")

    }

    template<typename T>
    static inline void from_variant( const wasm::variant &v, optional <T> &opt ) {

        if (v.is_null()) {
            opt = std::nullopt;
            return;
        }

        T t;
        from_variant(v, t);
        opt = t; 
        //opt = std::optional<T>(t);   
    }

    // static inline void from_variant( const wasm::variant &v, std::time_point_sec &t ) {
    //     if (v.type() == json_spirit::str_type) {

    //     }
    // }

    static inline void from_variant( const wasm::variant &v, wasm::checksum160_type &t ) {
        if (v.type() == json_spirit::str_type) {
            from_hex(v.get_str()).copy((char *) &t, sizeof(t));
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }

    static inline void from_variant( const wasm::variant &v, wasm::checksum256_type &t ) {
        if (v.type() == json_spirit::str_type) {
            from_hex(v.get_str()).copy((char *) &t, sizeof(t));
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }


    static inline void from_variant( const wasm::variant &v, checksum512_type &t ) {
        if (v.type() == json_spirit::str_type) {
            from_hex(v.get_str()).copy((char *) &t, sizeof(t));
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }


    static inline void from_variant( const wasm::variant &v, wasm::symbol_code &t ) {
        if (v.type() == json_spirit::str_type) {
            t = wasm::symbol_code(v.get_str());
            return;
        }
        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string")
    }

    static inline void from_variant( const wasm::variant &v, wasm::symbol &t ) {
        if (v.type() == json_spirit::obj_type) {
            wasm::symbol_code symbol_code;
            uint8_t precision;
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
                    case ID_Symbolcode : {
                        from_variant(Config_type::get_value(*i), symbol_code);
                        break;
                    }
                    case ID_Precision : {
                        from_variant(Config_type::get_value(*i), precision);
                        break;
                    }
                    default :
                        break;
                }
            }
            t = wasm::symbol(symbol_code, precision);
            return;

        } else if (v.type() == json_spirit::str_type) {
            t = wasm::symbol::from_string(v.get_str());
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string or object")
    }

    static inline void from_variant( const wasm::variant &v, wasm::asset &t ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
                    case ID_Amount : {
                        from_variant(Config_type::get_value(*i), t.amount);
                        break;
                    }
                    case ID_Symbol : {
                        from_variant(Config_type::get_value(*i), t.symbol);
                        break;
                    }
                    default :
                        break;
                }
            }
            return;
        } else if (v.type() == json_spirit::str_type) { //"100.0000 SYS"
            wasm::asset a;
            t = a.from_string(v.get_str());
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be a string or object")
    }


    static inline void from_variant( const wasm::variant &v, field_def &field ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
                    case ID_Name : {
                        from_variant(Config_type::get_value(*i), field.name);
                        break;
                    }
                    case ID_Type : {
                        from_variant(Config_type::get_value(*i), field.type);
                        break;
                    }
                    default :
                        break;
                }
            }
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an object")
    }

    static inline void from_variant( const wasm::variant &v, struct_def &s ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
                    case ID_Name : {
                        from_variant(Config_type::get_value(*i), s.name);
                        break;
                    }
                    case ID_Base : {
                        from_variant(Config_type::get_value(*i), s.base);
                        break;
                    }
                    case ID_Fields : {
                        from_variant(Config_type::get_value(*i), s.fields);
                        break;
                    }
                    default :
                        break;
                }
            }
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an object")

    }


    static inline void from_variant( const wasm::variant &v, action_def &a ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
                    case ID_Name : {
                        from_variant(Config_type::get_value(*i), a.name);
                        break;
                    }
                    case ID_Type : {
                        from_variant(Config_type::get_value(*i), a.type);
                        break;
                    }
                    case ID_Ricardian_contract : {
                        from_variant(Config_type::get_value(*i), a.ricardian_contract);
                        break;
                    }
                    default :
                        break;
                }
            }

            return;

        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an object")

    }


    static inline void from_variant( const wasm::variant &v, table_def &table ) {
        if (v.type() == json_spirit::obj_type) {

            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
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
            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an object")

    }


    static inline void from_variant( const wasm::variant &v, clause_pair &c ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
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

            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an object")

    }


    static inline void from_variant( const wasm::variant &v, type_def &type ) {
        if (v.type() == json_spirit::obj_type) {
            auto o = v.get_obj();
            for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                string key = Config_type::get_name(*i);
                abi_typeid_t type_id = string2typeid[key];
                switch (type_id) {
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

            return;
        }

        CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an object")
    }


    static inline void from_variant( const wasm::variant &v, abi_def &abi ) {

        try {
            if (v.type() == json_spirit::obj_type) {
                auto o = v.get_obj();

                for (wasm::Object::const_iterator i = o.begin(); i != o.end(); ++i) {
                    string key = Config_type::get_name(*i);
                    abi_typeid_t type_id = string2typeid[key];
                    switch (type_id) {
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
                return;
            }

            CHAIN_THROW(wasm_chain::abi_parse_exception, "abi parse fail:%s", "json variant must be an object")
        }

        CHAIN_RETHROW_EXCEPTIONS(wasm_chain::abi_parse_exception, "%s", "abi parse fail")
    }


} //wasm


