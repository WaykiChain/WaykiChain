#pragma once

#include <chrono>

#include "wasm/types/types.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/types/varint.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
// #include "wasm/types/inline_transaction.hpp"
#include "json/json_spirit.h"
#include "json/json_spirit_value.h"
#include "wasm/exceptions.hpp"
#include "wasm/abi_def.hpp"
#include "wasm/wasm_config.hpp"
#include "wasm/wasm_log.hpp"
// #include "wasm/wasm_trace.hpp"

namespace wasm {
    using namespace json_spirit;
    using namespace wasm;
    using std::chrono::system_clock;

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

    static inline string FromTime(const std::time_t &t){

        char szTime[128];
        std::tm *p;
        p = localtime(&t);
        sprintf( szTime, "%4d-%2d-%2dT%2d:%2d:%2d",p->tm_year+1900, p->tm_mon+1, p->tm_mday,p->tm_hour, p->tm_min, p->tm_sec); 

        return string(szTime);

    }

    static inline std::time_t ToTime(const std::string &t){
                                   
            int year, month, day, hour, minute, second;
            sscanf((char*)t.data(), "%d-%d-%dT%d:%d:%d", &year, &month, &day, &hour, &minute, &second);
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

    static inline void to_variant( const wasm::bytes &t, wasm::variant &v ) {
        string str(t.begin(), t.end());
        v = wasm::variant(str);
    }

    static inline void to_variant( const int128_t &t, wasm::variant &v ) {

        bool is_negative = ( t < 0);
        uint128_t val_magnitude;
        if (is_negative)
            val_magnitude = static_cast<uint128_t>(-t); // Works even if val is at the lowest possible value of a int128_t
        else
            val_magnitude = static_cast<uint128_t>(t);  

        uint128 u(val_magnitude);

        if(! is_negative){
            if(u.hi == 0){
                v = wasm::variant(u.lo);
                return;           
            }
            v = wasm::variant(string(u));
            return;
        } else {
            if(u.hi == 0){
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

        if(u.hi == 0){
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
          v = wasm::variant(FromTime(time));

          //WASM_TRACE("%s", FromTime(time).c_str());

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

    // static inline void to_variant( const std::time_point_sec &t, wasm::variant &v ) {
    //     // string str(&t.hash[0], &t.hash[sizeof(t.hash)/sizeof(t.hash[0])]);
    //     // v = wasm::variant(ToHex(str,""));

    // }


    static inline void to_variant( const wasm::checksum160_type &t, wasm::variant &v ) {
        //to_variant(t.hash, v);
        string str(&t.hash[0], &t.hash[sizeof(t.hash)/sizeof(t.hash[0])]);
        v = wasm::variant(ToHex(str,""));


    }

    static inline void to_variant( const wasm::checksum256_type &t, wasm::variant &v ) {
        //to_variant(t.hash, v);
        string str(&t.hash[0], &t.hash[sizeof(t.hash)/sizeof(t.hash[0])]);
        //WASM_TRACE("%s", ToHex(str,"").c_str())
        v = wasm::variant(ToHex(str,""));

    }

    static inline void to_variant( const wasm::checksum512_type &t, wasm::variant &v ) {
        string str(&t.hash[0], &t.hash[sizeof(t.hash)/sizeof(t.hash[0])]);

        v = wasm::variant(ToHex(str,""));
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

    static inline void from_variant( const wasm::variant &v, wasm::int128_t &t ) {

        if (v.type() == json_spirit::str_type) {
            string str = v.get_str();
            
            bool is_negative = (str[0] == '-') ? true:false;
            if (is_negative)
                str.substr(1, str.size()-1);
            uint128 u(str);
            t = is_negative? (-wasm::uint128_t(u)) : wasm::uint128_t(u);

        } else if (v.type() == json_spirit::int_type) {
            t = static_cast< wasm::int128_t >(v.get_int64());
        }

    }

    static inline void from_variant( const wasm::variant &v, wasm::uint128_t &t ) {

        if (v.type() == json_spirit::str_type) {
            string str = v.get_str();
            uint128 u(str);
            t = wasm::uint128_t(u);
        } else if (v.type() == json_spirit::int_type) {
            t = static_cast< wasm::uint128_t >(v.get_uint64());
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
        if (v.type() == json_spirit::real_type) {
            t = v.get_real();

            // std::ostringstream o;
            // o.precision(std::numeric_limits<float>::digits10);
            // o << t;
            // WASM_TRACE("%s",o.str().c_str())
        }
    }


    template<typename T, std::enable_if_t <std::is_integral<T>::value> * = nullptr>
    static inline void from_variant( const wasm::variant &v, T &t ) {
        if (v.type() == json_spirit::int_type) {
            t = v.get_int64();
        }
    }

    static inline void from_variant( const wasm::variant &v, system_clock::time_point &t ) {
        if (v.type() == json_spirit::str_type) {

            t = std::chrono::system_clock::from_time_t(ToTime(v.get_str()));

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

    // static inline void from_variant( const wasm::variant &v, std::time_point_sec &t ) {
    //     if (v.type() == json_spirit::str_type) {

    //     }
    // }

    static inline void from_variant( const wasm::variant &v, wasm::checksum160_type &t ) {
        if (v.type() == json_spirit::str_type) {
            FromHex(v.get_str()).copy((char *)&t, sizeof(t));
        }
    }

    static inline void from_variant( const wasm::variant &v, wasm::checksum256_type &t ) {
        if (v.type() == json_spirit::str_type) {
            //WASM_TRACE("%s", v.get_str().c_str())
            FromHex(v.get_str()).copy((char *)&t, sizeof(t));
        }
    }


    static inline void from_variant( const wasm::variant &v, checksum512_type &t ) {
        if (v.type() == json_spirit::str_type) {
            FromHex(v.get_str()).copy((char *)&t, sizeof(t));
        }
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
                        //WASM_TRACE("%s", field.name.c_str())
                        break;
                    }
                    case ID_Type : {
                        from_variant(Config_type::get_value(*i), field.type);
                        //std::cout << field.type <<std::endl ;
                        //WASM_TRACE("%s", field.type.c_str())
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
                         //WASM_TRACE("%s", s.name.c_str())
                        break;
                    }
                    case ID_Base : {
                        from_variant(Config_type::get_value(*i), s.base);
                        //std::cout << s.base <<std::endl ;
                        //WASM_TRACE("%s", s.base.c_str())
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

                    //WASM_TRACE("%s", key.c_str())

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

        WASM_RETHROW_EXCEPTIONS(abi_parse_exception, "abi parse fail ")
    }


} //wasm


