#pragma once

#include <string>
#include <string_view>
#include <sstream>

#include "check.hpp"
#include "wasm/wasm_log.hpp"

namespace wasm {

    constexpr bool is_digit(char c) {
        return c >= '0' && c <= '9';
    }

    constexpr int stoi_impl(const char* str, int value = 0) {
        return *str ?
                is_digit(*str) ?
                    stoi_impl(str + 1, (*str - '0') + value * 10)
                    : throw "compile-time-error: not a digit"
                : value;
    }

    constexpr int stoi(const char* str) {
        return stoi_impl(str);
    }

    constexpr inline uint64_t string_to_regid( const string &sRegId ) {
        int pos = sRegId.find('-');

        uint64_t height   = stoi(sRegId.substr(0, pos).c_str());
        uint64_t index    = stoi(sRegId.substr(pos + 1).c_str());

        return (height << 20) + index;
    }


    #define REGID( X ) string_to_regid(#X)

    //for account identity in wasm
    struct regid {
    public:
        enum class raw : uint64_t {
        };

        /**
         * Construct a new regid
         *
         * @brief Construct a new regid object defaulting to a value of 0
         *
         */
        constexpr regid() : value(0) {}

        /**
         * Construct a new regid given a unit64_t value
         *
         * @brief Construct a new regid object initialising value with v
         * @param v - The unit64_t value
         *
         */
        constexpr explicit regid( uint64_t v )
                : value(v) {}

        /**
         * Construct a new regid given a scoped enumerated type of raw (uint64_t).
         *
         * @brief Construct a new regid object initialising value with r
         * @param r - The raw value which is a scoped enumerated type of unit64_t
         *
         */
        constexpr explicit regid( regid::raw r )
                : value(static_cast<uint64_t>(r)) {}

        /**
         * Construct a new regid given an string.
         *
         * @brief Construct a new regid object initialising value with str
         * @param str - The string value which validated then converted to unit64_t
         *
         */
        constexpr explicit regid( std::string_view str )
        // explicit regid( std::string str )
                : value(0) {

               auto pos = str.find('-');
               check( pos > 0, "'-' must be between two numbers, ex. '999-80'");

               uint64_t height   = atoi(str.substr(0, pos).c_str());
               uint64_t index    = atoi(str.substr(pos + 1).c_str());

               value = (height << 20) + index;
        }


        /**
         * Casts a regid to raw
         *
         * @return Returns an instance of raw based on the value of a regid
         */
        constexpr operator raw() const { return raw(value); }

        /**
         * Explicit cast to bool of the uint64_t value of the regid
         *
         * @return Returns true if the regid is set to the default value of 0 else true.
         */
        constexpr explicit operator bool() const { return value != 0; }

        /**
         *  Returns the regid as a string.
         *
         *  @brief Returns the regid value as a string by calling write_as_string() and returning the buffer produced by write_as_string()
         */
        std::string to_string() const {
            uint64_t height = value >> 20;
            uint64_t index  = value & 0xFFFFF;

            char buffer[64];
            sprintf(buffer, "%ld-%ld", height, index);
            return std::string(buffer);
        }

        /**
         * Equivalency operator. Returns true if a == b (are the same)
         *
         * @return boolean - true if both provided %regid values are the same
         */
        friend constexpr bool operator==( const regid &a, const regid &b ) {
            return a.value == b.value;
        }

        /**
         * Inverted equivalency operator. Returns true if a != b (are different)
         *
         * @return boolean - true if both provided %regid values are not the same
         */
        friend constexpr bool operator!=( const regid &a, const regid &b ) {
            return a.value != b.value;
        }

        /**
         * Less than operator. Returns true if a < b.
         *
         * @return boolean - true if %regid `a` is less than `b`
         */
        friend constexpr bool operator<( const regid &a, const regid &b ) {
            return a.value < b.value;
        }


        uint64_t value = 0;

        /**  Serialize a symbol_code into a stream
        *
        *  @brief Serialize a symbol_code
        *  @param ds - The stream to write
        *  @param sym - The value to serialize
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        template<typename DataStream>
        friend inline DataStream &operator<<( DataStream &ds, const wasm::regid &regid ) {
            uint64_t raw = regid.value;
            ds.write((const char *) &raw, sizeof(uint64_t));
            return ds;
        }

        /**
        *  Deserialize a symbol_code from a stream
        *
        *  @brief Deserialize a symbol_code
        *  @param ds - The stream to read
        *  @param symbol - The destination for deserialized value
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        template<typename DataStream>
        friend inline DataStream &operator>>( DataStream &ds, wasm::regid &regid ) {
            uint64_t raw = 0;
            ds.read((char *) &raw, sizeof(uint64_t));
            regid.value = raw;
            return ds;
        }

    };


} /// regidspace wayki
