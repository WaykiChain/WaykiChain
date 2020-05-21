#pragma once

#include <string_view>
#include<iostream>
#include <cstdlib>
#include <algorithm>

#include "check.hpp"
#include "wasm/exception/exceptions.hpp"

namespace wasm {


    inline string ltrim( const string &str ) {
        int h = 0, t = str.size();
        while (h < t && (str[h] == ' ' || str[h] == '\t')) h++;
        return str.substr(h, t - h);
    }

    inline string rtrim( const string &str ) {
        int h = 0, t = str.size();
        // trim right
        while (h < t && (str[t - 1] == ' ' || str[t - 1] == '\t')) t--;
        return str.substr(h, t - h);
    }

    inline string trim( const string &s ) {
        string str = s;
        ltrim(rtrim(str));
        return str;
    }

    inline constexpr bool valid_character_in_symbol_name(const char& c){
        if( c >= '0' && c <= '9' ) return true;
        if( c >= 'A' && c <= 'Z' ) return true;
        if( c >= 'a' && c <= 'z' ) return true;
        if( c == '_' ) return true;
        if( c == '@' ) return true;
        if( c == '.' ) return true;
        if( c == '#' ) return true;

        return false;

    }

    static uint64_t string_to_symbol( uint8_t precision, const char *str ) {
        try {
            uint32_t len = 0;
            while (str[len]) ++len;

            if (len > 7) {
                check( false, "the length of symbol_code must be <= 7" );
            }

            uint64_t result = 0;
            for (uint32_t i = 0; i < len; ++i) {
                // All characters must be upper case alphabets
                //WASM_ASSERT (str[i] >= 'A' && str[i] <= 'Z', symbol_type_exception, "%s", "invalid character in symbol name");
                check (valid_character_in_symbol_name(str[i]),  "invalid character in symbol name, the character must be '0~9' 'A~Z' '_' '.' '#' '@'");
                result |= (uint64_t(str[i]) << (8 * (i + 1)));
            }
            result |= uint64_t(precision);
            return result;
        }

        CHAIN_CAPTURE_AND_RETHROW( "symbol convert error: %s", str )
    }

    /**
     *  @addtogroup symbol Symbol C++ API
     *  @ingroup core
     *  @brief Defines C++ API for managing symbols
     *  @{
     */

    /**
     * @class Stores the symbol code
     * @brief Stores the symbol code as a uint64_t value
     */
    class symbol_code {
    public:

        /**
         * Default constructor, construct a new symbol_code
         *
         * @brief Construct a new symbol_code object defaulting to a value of 0
         *
         */
        constexpr symbol_code() : value(0) {}

        /**
         * Construct a new symbol_code given a scoped enumerated type of raw (uint64_t).
         *
         * @brief Construct a new symbol_code object initialising value with raw
         * @param raw - The raw value which is a scoped enumerated type of unit64_t
         *
         */
        constexpr explicit symbol_code( uint64_t raw )
                : value(raw) {}

        /**
         * Construct a new symbol_code given an string.
         *
         * @brief Construct a new symbol_code object initialising value with str
         * @param str - The string value which validated then converted to unit64_t
         *
         */
        constexpr explicit symbol_code( std::string_view str )
                : value(0) {
            if (str.size() > 7) {
                check( false, "the length of symbol_code must be <= 7" );
                return;
            }
            for (auto itr = str.rbegin(); itr != str.rend(); ++itr) {
                // if (*itr < 'A' || *itr > 'Z') {
                //     check( false, "only uppercase letters allowed in symbol_code string" );
                //     return;
                // }
                check (valid_character_in_symbol_name(*itr), strprintf("invalid char=%c in symbol", *itr));
                value <<= 8;
                value |= *itr;
            }
        }

        /**
         * Checks if the symbol code is valid
         * @return true - if symbol is valid
         */
        constexpr bool is_valid() const {
            auto sym = value;
            for (int i = 0; i < 7; i++) {
                char c = (char) (sym & 0xFF);
                //if (!('A' <= c && c <= 'Z')) return false;
                if(!valid_character_in_symbol_name(c)) return false;
                sym >>= 8;
                if (!(sym & 0xFF)) {
                    do {
                        sym >>= 8;
                        if ((sym & 0xFF)) return false;
                        i++;
                    } while (i < 7);
                }
            }
            return true;
        }

        /**
         * Returns the character length of the provided symbol
         *
         * @return length - character length of the provided symbol
         */
        constexpr uint32_t length() const {
            auto sym = value;
            uint32_t len = 0;
            while (sym & 0xFF && len <= 7) {
                len++;
                sym >>= 8;
            }
            return len;
        }

        /**
         * Casts a symbol code to raw
         *
         * @return Returns an instance of raw based on the value of a symbol_code
         */
        constexpr uint64_t raw() const { return value; }

        /**
         * Explicit cast to bool of the symbol_code
         *
         * @return Returns true if the symbol_code is set to the default value of 0 else true.
         */
        constexpr explicit operator bool() const { return value != 0; }

        /**
         *  Writes the symbol_code as a string to the provided char buffer
         *
         *
         *  @brief Writes the symbol_code as a string to the provided char buffer
         *  @pre Appropriate Size Precondition: (begin + 7) <= end and (begin + 7) does not overflow
         *  @pre Valid Memory Region Precondition: The range [begin, end) must be a valid range of memory to write to.
         *  @param begin - The start of the char buffer
         *  @param end - Just past the end of the char buffer
         *  @return char* - Just past the end of the last character written (returns begin if the Appropriate Size Precondition is not satisfied)
         *  @post If the Appropriate Size Precondition is satisfied, the range [begin, returned pointer) contains the string representation of the symbol_code.
         */
        char *write_as_string( char *begin, char *end ) const {
            constexpr uint64_t mask = 0xFFull;

            if ((begin + 7) < begin || (begin + 7) > end) return begin;

            auto v = value;
            for (auto i = 0; i < 7; ++i, v >>= 8) {
                if (v == 0) return begin;

                *begin = static_cast<char>(v & mask);
                ++begin;
            }

            return begin;
        }

        /**
         *  Returns the symbol_code as a string.
         *
         *  @brief Returns the name value as a string by calling write_as_string() and returning the buffer produced by write_as_string()
         */
        std::string to_string() const {
            char buffer[7];
            auto end = write_as_string(buffer, buffer + sizeof(buffer));
            return {buffer, end};
        }

        /**
         * Equivalency operator. Returns true if a == b (are the same)
         *
         * @brief Equivalency operator
         * @return boolean - true if both provided symbol_codes are the same
         */
        friend constexpr bool operator==( const symbol_code &a, const symbol_code &b ) {
            return a.value == b.value;
        }

        /**
         * Inverted equivalency operator. Returns true if a != b (are different)
         *
         * @brief Inverted equivalency operator
         * @return boolean - true if both provided symbol_codes are not the same
         */
        friend constexpr bool operator!=( const symbol_code &a, const symbol_code &b ) {
            return a.value != b.value;
        }

        /**
         * Less than operator. Returns true if a < b.
         * @brief Less than operator
         * @return boolean - true if symbol_code `a` is less than `b`
         */
        friend constexpr bool operator<( const symbol_code &a, const symbol_code &b ) {
            return a.value < b.value;
        }


    private:
        uint64_t value = 0;
    };

    /**
    *  Serialize a symbol_code into a stream
    *
    *  @brief Serialize a symbol_code
    *  @param ds - The stream to write
    *  @param sym - The value to serialize
    *  @tparam DataStream - Type of datastream buffer
    *  @return DataStream& - Reference to the datastream
    */
    template<typename DataStream>
    inline DataStream &operator<<( DataStream &ds, const wasm::symbol_code sym_code ) {
        uint64_t raw = sym_code.raw();
        ds.write((const char *) &raw, sizeof(raw));
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
    inline DataStream &operator>>( DataStream &ds, wasm::symbol_code &sym_code ) {
        uint64_t raw = 0;
        ds.read((char *) &raw, sizeof(raw));
        sym_code = symbol_code(raw);
        return ds;
    }


    /**
     * @struct Stores information about a symbol, the symbol can be 7 characters long.
     *
     * @brief Stores information about a symbol
     */
    class symbol {
    public:

        static constexpr uint8_t max_precision = 18;

        /**
         * Default constructor, construct a new symbol
         *
         * @brief Construct a new symbol object defaulting to a value of 0
         *
         */
        constexpr symbol() : value(0) {}

        /**
         * Construct a new symbol given a scoped enumerated type of raw (uint64_t).
         *
         * @brief Construct a new symbol object initialising value with raw
         * @param raw - The raw value which is a scoped enumerated type of unit64_t
         *
         */
        constexpr explicit symbol( uint64_t raw ) : value(raw) {}

        /**
         * Construct a new symbol given a symbol_code and a uint8_t precision.
         *
         * @brief Construct a new symbol object initialising value with a symbol maximum of 7 characters, packs the symbol and precision into the uint64_t value.
         * @param sc - The symbol_code
         * @param precision - The number of decimal places used for the symbol
         *
         */
        constexpr symbol( symbol_code sc, uint8_t precision )
                : value((sc.raw() << 8) | static_cast<uint64_t>(precision)) {
                    CHAIN_ASSERT( precision <= max_precision, symbol_type_exception, "precision '%d' should be <= 18", precision);
                }
        /**
         * Construct a new symbol given a string and a uint8_t precision.
         *
         * @brief Construct a new symbol object initialising value with a symbol maximum of 7 characters, packs the symbol and precision into the uint64_t value.
         * @param ss - The string containing the symbol
         * @param precision - The number of decimal places used for the symbol
         *
         */
        constexpr symbol( std::string_view ss, uint8_t precision )
                : value((symbol_code(ss).raw() << 8) | static_cast<uint64_t>(precision)) {
                    CHAIN_ASSERT( precision <= max_precision, symbol_type_exception, "precision '%d' should be <= 18", precision);
                }

        /**
         * Is this symbol valid
         */
        constexpr bool is_valid() const { return code().is_valid() && precision() <= max_precision; }

        /**
         * This symbol's precision
         */
        constexpr uint8_t precision() const { return static_cast<uint8_t>( value & 0xFFull ); }

        /**
         * Returns representation of symbol name
         */
        constexpr symbol_code code() const { return symbol_code{value >> 8}; }

        /**
         * Returns uint64_t repreresentation of the symbol
         */
        constexpr uint64_t raw() const { return value; }

        constexpr explicit operator bool() const { return value != 0; }

        // uint8_t decimals() const { return value & 0xFF; }
        uint64_t precision_in_10() const {
            CHAIN_ASSERT( precision() <= max_precision, symbol_type_exception, "precision %d should be <= 18", precision() );
            uint64_t p10 = 1;
            uint64_t p = precision();
            while (p > 0) {
                p10 *= 10;
                --p;
            }
            return p10;
        }

        /**
         * %Print the symbol
         *
         * @brief %Print the symbol
         */
        std::string to_string() const {
            // if( show_precision ){
            //    printui( static_cast<uint64_t>(precision()) );
            //    prints(",");
            // }
            // char buffer[7];
            // auto end = code().write_as_string( buffer, buffer + sizeof(buffer) );
            // if( buffer < end )
            //    prints_l( buffer, (end-buffer) );

            char str[1 + 1 + 7];
            snprintf(str, sizeof(str), "%d%s%s",
                     precision(),
                     ",",
                     code().to_string().c_str());
            return {str};

        }

        static symbol from_string( const string &from ) {
            try {
                string s = trim(from);
                CHAIN_ASSERT(!s.empty(), symbol_type_exception, "creating symbol from empty string");
                auto comma_pos = s.find(',');
                CHAIN_ASSERT(comma_pos != string::npos, symbol_type_exception,"missing comma in symbol. eg. '%s'","8,WICC");
                auto prec_part = s.substr(0, comma_pos);
                uint8_t p = atoi(prec_part.data());
                string name_part = s.substr(comma_pos + 1);
                CHAIN_ASSERT( p <= max_precision, symbol_type_exception, "precision '%d' should be <= 18", p);
                return symbol(string_to_symbol(p, name_part.c_str()));
            }

            CHAIN_CAPTURE_AND_RETHROW( "symbol convert error:'%s'", from.c_str() )
        }

        /**
         * Equivalency operator. Returns true if a == b (are the same)
         *
         * @brief Equivalency operator
         * @return boolean - true if both provided symbols are the same
         */
        friend constexpr bool operator==( const symbol &a, const symbol &b ) {
            return a.value == b.value;
        }

        /**
         * Inverted equivalency operator. Returns true if a != b (are different)
         *
         * @brief Inverted equivalency operator
         * @return boolean - true if both provided symbols are not the same
         */
        friend constexpr bool operator!=( const symbol &a, const symbol &b ) {
            return a.value != b.value;
        }

        /**
         * Less than operator. Returns true if a < b.
         * @brief Less than operator
         * @return boolean - true if symbol `a` is less than `b`
         */
        friend constexpr bool operator<( const symbol &a, const symbol &b ) {
            return a.value < b.value;
        }

    private:
        uint64_t value = 0;
    };

    /**
     *  Serialize a symbol into a stream
     *
     *  @brief Serialize a symbol
     *  @param ds - The stream to write
     *  @param sym - The value to serialize
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
        template<typename DataStream>
        inline DataStream &operator<<( DataStream &ds, const wasm::symbol sym ) {
            uint64_t raw = sym.raw();
            ds.write((const char *) &raw, sizeof(raw));
            return ds;
        }

    /**
     *  Deserialize a symbol from a stream
     *
     *  @brief Deserialize a symbol
     *  @param ds - The stream to read
     *  @param symbol - The destination for deserialized value
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
        template<typename DataStream>
        inline DataStream &operator>>( DataStream &ds, wasm::symbol &sym ) {
            uint64_t raw = 0;
            ds.read((char *) &raw, sizeof(raw));
            sym = symbol(raw);
            return ds;
        }

    /// @}
}
