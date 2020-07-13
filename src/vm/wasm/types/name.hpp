#pragma once

#include <string>
#include <string_view>

#include "check.hpp"
#include "wasm/wasm_log.hpp"

namespace wasm {


    static constexpr uint64_t char_to_symbol( char c ) {
        if (c >= 'a' && c <= 'z')
            return (c - 'a') + 6;
        if (c >= '1' && c <= '5')
            return (c - '1') + 1;
        return 0;
    }

    // Each char of the string is encoded into 5-bit chunk and left-shifted
    // to its 5-bit slot starting with the highest slot for the first char.
    // The 13th char, if str is long enough, is encoded into 4-bit chunk
    // and placed in the lowest 4 bits. 64 = 12 * 5 + 4
    static constexpr uint64_t string_to_name( const char *str ) {
        uint64_t name = 0;
        int i = 0;
        for (; str[i] && i < 12; ++i) {
            // NOTE: char_to_symbol() returns char type, and without this explicit
            // expansion to uint64 type, the compilation fails at the point of usage
            // of string_to_name(), where the usage requires constant (compile time) expression.
            name |= (char_to_symbol(str[i]) & 0x1f) << (64 - 5 * (i + 1));
        }

        // The for-loop encoded up to 60 high bits into uint64 'name' variable,
        // if (strlen(str) > 12) then encode str[12] into the low (remaining)
        // 4 bits of 'name'
        if (i == 12)
            name |= char_to_symbol(str[12]) & 0x0F;
        return name;
    }

#define N( X ) string_to_name(#X)
#define NAME( X ) string_to_name(#X)


    /*
    * Wraps a %uint64_t to ensure it is only passed to methods that expect a %name.
    * Ensures value is only passed to methods that expect a %name and that no mathematical
    * operations occur.  Also enables specialization of print
    */
    struct name {
    public:
        enum class raw : uint64_t {
        };

        /**
         * Construct a new name
         *
         * @brief Construct a new name object defaulting to a value of 0
         *
         */
        constexpr name() : value(0) {}

        /**
         * Construct a new name given a unit64_t value
         *
         * @brief Construct a new name object initialising value with v
         * @param v - The unit64_t value
         *
         */
        constexpr explicit name( uint64_t v )
                : value(v) {}

        /**
         * Construct a new name given a scoped enumerated type of raw (uint64_t).
         *
         * @brief Construct a new name object initialising value with r
         * @param r - The raw value which is a scoped enumerated type of unit64_t
         *
         */
        constexpr explicit name( name::raw r )
                : value(static_cast<uint64_t>(r)) {}

        /**
         * Construct a new name given an string.
         *
         * @brief Construct a new name object initialising value with str
         * @param str - The string value which validated then converted to unit64_t
         *
         */
        // dot change to '_'
        explicit name( std::string_view str )
                : value(0) {
            try {
                if (str.size() > 13) {
                    check( false, "string is too long to be a valid name" );
                }

                if (str.empty()) {
                    return;
                }

                //uint8_t dot = 0;
                auto n = std::min((uint32_t) str.size(), (uint32_t) 12u);
                for (decltype(n) i = 0; i < n; ++i) {
                    value <<= 5;
                    value |= char_to_value(str[i]);
                    //if(str[i] == '.'){
                    // if(str[i] == '_'){
                    //     check( ++dot <= 1, "the amount of dot can not be > 1");
                    // }
                }
                value <<= (4 + 5 * (12 - n));
                if (str.size() == 13) {
                    uint64_t v = char_to_value(str[12]);
                    //if(str[12] == '.'){
                    // if(str[12] == '_'){
                    //     check( ++dot <= 1, "the amount of dot can not be > 1");
                    // }

                    if (v > 0x0Full) {
                        check(false, "thirteenth character in name cannot be a letter that comes after j");
                    }
                    value |= v;
                }
            }
            CHAIN_CAPTURE_AND_RETHROW("'%s' can not convert to name", str)
        }

        /**
         *  Converts a %name Base32 symbol into its corresponding value
         *
         *  @param c - Character to be converted
         *  @return constexpr char - Converted value
         */
        static constexpr uint8_t char_to_value( char c ) {
            //if (c == '.'){
            if (c == '_'){
                return 0;
            }
            else if (c >= '1' && c <= '5'){
                return (c - '1') + 1;
            }
            else if (c >= 'a' && c <= 'z'){
                return (c - 'a') + 6;
            }
            else{
                check( false, strprintf("character '%c(0x%x)' is not in allowed character set for names, the character must be '_' 1~5' 'a-z'", c, c) );
                //check( false, "character is not in allowed character set for names" );
            }

            return 0; // control flow will never reach here; just added to suppress warning
        }

        /**
         *  Returns the length of the %name
         */
        constexpr uint8_t length() const {
            constexpr uint64_t mask = 0xF800000000000000ull;

            if (value == 0)
                return 0;

            uint8_t l = 0;
            uint8_t i = 0;
            for (auto v = value; i < 13; ++i, v <<= 5) {
                if ((v & mask) > 0) {
                    l = i;
                }
            }

            return l + 1;
        }

        /**
         *  Returns the suffix of the %name
         */
        constexpr name suffix() const {
            uint32_t remaining_bits_after_last_actual_dot = 0;
            uint32_t tmp = 0;
            for (int32_t remaining_bits = 59;
                 remaining_bits >= 4; remaining_bits -= 5) { // Note: remaining_bits must remain signed integer
                // Get characters one-by-one in name in order from left to right (not including the 13th character)
                auto c = (value >> remaining_bits) & 0x1Full;
                if (!c) { // if this character is a dot
                    tmp = static_cast<uint32_t>(remaining_bits);
                } else { // if this character is not a dot
                    remaining_bits_after_last_actual_dot = tmp;
                }
            }

            uint64_t thirteenth_character = value & 0x0Full;
            if (thirteenth_character) { // if 13th character is not a dot
                remaining_bits_after_last_actual_dot = tmp;
            }

            if (remaining_bits_after_last_actual_dot ==
                0) // there is no actual dot in the %name other than potentially leading dots
                return name{value};

            // At this point remaining_bits_after_last_actual_dot has to be within the range of 4 to 59 (and restricted to increments of 5).

            // Mask for remaining bits corresponding to characters after last actual dot, except for 4 least significant bits (corresponds to 13th character).
            uint64_t mask = (1ull << remaining_bits_after_last_actual_dot) - 16;
            uint32_t shift = 64 - remaining_bits_after_last_actual_dot;

            return name{((value & mask) << shift) + (thirteenth_character << (shift - 1))};
        }

        /**
         * Casts a name to raw
         *
         * @return Returns an instance of raw based on the value of a name
         */
        constexpr operator raw() const { return raw(value); }

        /**
         * Explicit cast to bool of the uint64_t value of the name
         *
         * @return Returns true if the name is set to the default value of 0 else true.
         */
        constexpr explicit operator bool() const { return value != 0; }

        /**
         *  Writes the %name as a string to the provided char buffer
         *
         *  @pre Appropriate Size Precondition: (begin + 13) <= end and (begin + 13) does not overflow
         *  @pre Valid Memory Region Precondition: The range [begin, end) must be a valid range of memory to write to.
         *  @param begin - The start of the char buffer
         *  @param end - Just past the end of the char buffer
         *  @return char* - Just past the end of the last character written (returns begin if the Appropriate Size Precondition is not satisfied)
         *  @post If the Appropriate Size Precondition is satisfied, the range [begin, returned pointer) contains the string representation of the %name.
         */
        char *write_as_string( char *begin, char *end ) const {
            static const char *charmap = "_12345abcdefghijklmnopqrstuvwxyz";
            constexpr uint64_t mask = 0xF800000000000000ull;

            if ((begin + 13) < begin || (begin + 13) > end) return begin;

            auto v = value;
            for (auto i = 0; i < 13; ++i, v <<= 5) {
                if (v == 0) return begin;

                auto indx = (v & mask) >> (i == 12 ? 60 : 59);
                *begin = charmap[indx];
                ++begin;
            }

            return begin;
        }

        /**
         *  Returns the name as a string.
         *
         *  @brief Returns the name value as a string by calling write_as_string() and returning the buffer produced by write_as_string()
         */
        std::string to_string() const {
            char buffer[13];
            auto end = write_as_string(buffer, buffer + sizeof(buffer));
            return {buffer, end};
        }

        /**
         * Equivalency operator. Returns true if a == b (are the same)
         *
         * @return boolean - true if both provided %name values are the same
         */
        friend constexpr bool operator==( const name &a, const name &b ) {
            return a.value == b.value;
        }

        /**
         * Inverted equivalency operator. Returns true if a != b (are different)
         *
         * @return boolean - true if both provided %name values are not the same
         */
        friend constexpr bool operator!=( const name &a, const name &b ) {
            return a.value != b.value;
        }

        /**
         * Less than operator. Returns true if a < b.
         *
         * @return boolean - true if %name `a` is less than `b`
         */
        friend constexpr bool operator<( const name &a, const name &b ) {
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
        friend inline DataStream &operator<<( DataStream &ds, const wasm::name &name ) {
            uint64_t raw = name.value;
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
        friend inline DataStream &operator>>( DataStream &ds, wasm::name &name ) {
            uint64_t raw = 0;
            ds.read((char *) &raw, sizeof(uint64_t));
            name.value = raw;
            return ds;
        }
    };

    // namespace detail {
    //     template <char... Str>
    //     struct to_const_char_arr {
    //        static constexpr const char value[] = {Str...};
    //     };
    //  } /// namespace detail

} /// namespace wayki


/**
 * %name literal operator
 *
 * @brief "foo"_n is a shortcut for name("foo")
 */
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wgnu-string-literal-operator-template"
// template <typename T, T... Str>
// inline constexpr wayki::name operator""_n() {
//    constexpr auto x = wayki::name{std::string_view{wayki::detail::to_const_char_arr<Str...>::value, sizeof...(Str)}};
//    return x;
// }
// #pragma clang diagnostic pop
