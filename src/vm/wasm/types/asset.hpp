#pragma once

#include "symbol.hpp"
#include<iostream>
#include <cstdlib>
#include <cerrno>

#include "check.hpp"
#include "types.hpp"

#include "wasm/wasm_serialize_reflect.hpp"

namespace wasm {
    /**
     *  Defines C++ API for managing assets
     *  @addtogroup asset Asset C++ API
     *  @ingroup core
     *  @{
     */

    /**
     * @struct Stores information for owner of asset
     */

    struct asset {
        /**
         * The amount of the asset
         */
        int64_t amount = 0;

        /**
         * The symbol name of the asset
         */
        class symbol symbol;

        /**
         * Maximum amount possible for this asset. It's capped to 90 billion
         */
        static constexpr int64_t max_amount = 900LL * 100000000 * 100000000; // 90 billion

        asset() {}

        /**
         * Construct a new asset given the symbol name and the amount
         *
         * @param a - The amount of the asset
         * @param s - The name of the symbol
         */
        asset( int64_t a, class symbol s )
                : amount(a), symbol{s} {
            check(is_amount_within_range(), "magnitude of asset amount must be less than 9,000,000,000,000,000,000");
            check(symbol.is_valid(), "invalid symbol name");
        }

        /**
         * Check if the amount doesn't exceed the max amount
         *
         * @return true - if the amount doesn't exceed the max amount
         * @return false - otherwise
         */
        bool is_amount_within_range() const { return -max_amount <= amount && amount <= max_amount; }

        /**
         * Check if the asset is valid. %A valid asset has its amount <= max_amount and its symbol name valid
         *
         * @return true - if the asset is valid
         * @return false - otherwise
         */
        bool is_valid() const { return is_amount_within_range() && symbol.is_valid(); }

        /**
         * Set the amount of the asset
         *
         * @param a - New amount for the asset
         */
        void set_amount( int64_t a ) {
            amount = a;
            check(is_amount_within_range(), "magnitude of asset amount must be less than 2^62");
        }

        /**
         * Unary minus operator
         *
         * @return asset - New asset with its amount is the negative amount of this asset
         */
        asset operator-() const {
            asset r = *this;
            r.amount = -r.amount;
            return r;
        }

        /**
         * Subtraction assignment operator
         *
         * @param a - Another asset to subtract this asset with
         * @return asset& - Reference to this asset
         * @post The amount of this asset is subtracted by the amount of asset a
         */
        asset &operator-=( const asset &a ) {
            check(a.symbol == symbol, "attempt to subtract asset with different symbol");
            amount -= a.amount;
            check(-max_amount <= amount, "subtraction underflow");
            check(amount <= max_amount, "subtraction overflow");
            return *this;
        }


        /**
         * Addition Assignment  operator
         *
         * @param a - Another asset to subtract this asset with
         * @return asset& - Reference to this asset
         * @post The amount of this asset is added with the amount of asset a
         */
        asset &operator+=( int64_t a ) {
            //check( a.sym == sym, "attempt to add asset with different symbol" );
            amount += a;
            check(-max_amount <= amount, "addition underflow");
            check(amount <= max_amount, "addition overflow");
            return *this;
        }

        /**
         * Addition Assignment  operator
         *
         * @param a - Another asset to subtract this asset with
         * @return asset& - Reference to this asset
         * @post The amount of this asset is added with the amount of asset a
         */
        asset &operator+=( const asset &a ) {
            check(a.symbol == symbol, "attempt to add asset with different symbol");
            amount += a.amount;
            check(-max_amount <= amount, "addition underflow");
            check(amount <= max_amount, "addition overflow");
            return *this;
        }

        /**
         * Addition operator
         *
         * @param a - The first asset to be added
         * @param b - The second asset to be added
         * @return asset - New asset as the result of addition
         */
        inline friend asset operator+( const asset &a, const asset &b ) {
            asset result = a;
            result += b;
            return result;
        }

        /**
         * Subtraction operator
         *
         * @param a - The asset to be subtracted
         * @param b - The asset used to subtract
         * @return asset - New asset as the result of subtraction of a with b
         */
        inline friend asset operator-( const asset &a, const asset &b ) {
            asset result = a;
            result -= b;
            return result;
        }

        /**
         * @brief Multiplication assignment operator, with a number
         *
         * @details Multiplication assignment operator. Multiply the amount of this asset with a number and then assign the value to itself.
         * @param a - The multiplier for the asset's amount
         * @return asset - Reference to this asset
         * @post The amount of this asset is multiplied by a
         */
        asset &operator*=( int64_t a ) {
            //int64_t tmp = amount * a;
            int128_t tmp = (int128_t)amount * (int128_t)a;
            check( tmp <= max_amount, "multiplication overflow" );
            check( tmp >= -max_amount, "multiplication underflow" );
            amount = (int64_t) tmp;
            return *this;
        }

        /**
         * Multiplication operator, with a number proceeding
         *
         * @brief Multiplication operator, with a number proceeding
         * @param a - The asset to be multiplied
         * @param b - The multiplier for the asset's amount
         * @return asset - New asset as the result of multiplication
         */
        friend asset operator*( const asset &a, int64_t b ) {
            asset result = a;
            result *= b;
            return result;
        }


        /**
         * Multiplication operator, with a number preceeding
         *
         * @param a - The multiplier for the asset's amount
         * @param b - The asset to be multiplied
         * @return asset - New asset as the result of multiplication
         */
        friend asset operator*( int64_t b, const asset &a ) {
            asset result = a;
            result *= b;
            return result;
        }

        /**
         * @brief Division assignment operator, with a number
         *
         * @details Division assignment operator. Divide the amount of this asset with a number and then assign the value to itself.
         * @param a - The divisor for the asset's amount
         * @return asset - Reference to this asset
         * @post The amount of this asset is divided by a
         */
        asset &operator/=( int64_t a ) {
            check(a != 0, "divide by zero");
            check(!(amount == std::numeric_limits<int64_t>::min() && a == -1), "signed division overflow");
            amount /= a;
            return *this;
        }

        /**
         * Division operator, with a number proceeding
         *
         * @param a - The asset to be divided
         * @param b - The divisor for the asset's amount
         * @return asset - New asset as the result of division
         */
        friend asset operator/( const asset &a, int64_t b ) {
            asset result = a;
            result /= b;
            return result;
        }

        /**
         * Division operator, with another asset
         *
         * @param a - The asset which amount acts as the dividend
         * @param b - The asset which amount acts as the divisor
         * @return int64_t - the resulted amount after the division
         * @pre Both asset must have the same symbol
         */
        friend int64_t operator/( const asset &a, const asset &b ) {
            check(b.amount != 0, "divide by zero");
            check(a.symbol == b.symbol, "comparison of assets with different symbols is not allowed");
            return a.amount / b.amount;
        }

        /**
         * Equality operator
         *
         * @param a - The first asset to be compared
         * @param b - The second asset to be compared
         * @return true - if both asset has the same amount
         * @return false - otherwise
         * @pre Both asset must have the same symbol
         */
        friend bool operator==( const asset &a, const asset &b ) {
            check(a.symbol == b.symbol, "comparison of assets with different symbols is not allowed");
            return a.amount == b.amount;
        }

        /**
         * Inequality operator
         *
         * @param a - The first asset to be compared
         * @param b - The second asset to be compared
         * @return true - if both asset doesn't have the same amount
         * @return false - otherwise
         * @pre Both asset must have the same symbol
         */
        friend bool operator!=( const asset &a, const asset &b ) {
            return !(a == b);
        }

        /**
         * Less than operator
         *
         * @param a - The first asset to be compared
         * @param b - The second asset to be compared
         * @return true - if the first asset's amount is less than the second asset amount
         * @return false - otherwise
         * @pre Both asset must have the same symbol
         */
        friend bool operator<( const asset &a, const asset &b ) {
            check(a.symbol == b.symbol, "comparison of assets with different symbols is not allowed");
            return a.amount < b.amount;
        }

        /**
         * Less or equal to operator
         *
         * @param a - The first asset to be compared
         * @param b - The second asset to be compared
         * @return true - if the first asset's amount is less or equal to the second asset amount
         * @return false - otherwise
         * @pre Both asset must have the same symbol
         */
        friend bool operator<=( const asset &a, const asset &b ) {
            check(a.symbol == b.symbol, "comparison of assets with different symbols is not allowed");
            return a.amount <= b.amount;
        }

        /**
         * Greater than operator
         *
         * @param a - The first asset to be compared
         * @param b - The second asset to be compared
         * @return true - if the first asset's amount is greater than the second asset amount
         * @return false - otherwise
         * @pre Both asset must have the same symbol
         */
        friend bool operator>( const asset &a, const asset &b ) {
            check(a.symbol == b.symbol, "comparison of assets with different symbols is not allowed");
            return a.amount > b.amount;
        }

        /**
         * Greater or equal to operator
         *
         * @param a - The first asset to be compared
         * @param b - The second asset to be compared
         * @return true - if the first asset's amount is greater or equal to the second asset amount
         * @return false - otherwise
         * @pre Both asset must have the same symbol
         */
        friend bool operator>=( const asset &a, const asset &b ) {
            check(a.symbol == b.symbol, "comparison of assets with different symbols is not allowed");
            return a.amount >= b.amount;
        }


        friend bool operator<( const asset& a, int64_t b ) {
           return (a.amount < b);
        }

        friend bool operator<=( const asset& a, int64_t b ) {
           return (a.amount <= b);
        }

        friend bool operator>( const asset& a, int64_t b ) {
           return (a.amount > b);
        }

        friend bool operator>=( const asset& a, int64_t b ) {
           return (a.amount >= b);
        }

        /**
         * %asset to std::string
         *
         * @brief %asset to std::string
         */
        std::string to_string() const {
            uint8_t p =  symbol.precision();

            //bool negative = false;
            int64_t invert = 1;

            int64_t p10 = symbol.precision_in_10();

            p =  symbol.precision();

            char fraction[256];
            fraction[p] = '\0';

            if (amount < 0) {
                invert = -1;
                //negative = true;
            }

            auto change = (amount % p10) * invert;

            for (int64_t i = p - 1; i >= 0; --i) {
                fraction[i] = (change % 10) + '0';
                change /= 10;
            }
            char str[256 + 32];
            snprintf(str, sizeof(str), "%ld%s%s %s",
                     (int64_t)(amount / p10),
                     (fraction[0]) ? "." : "",
                     fraction,
                     symbol.code().to_string().c_str());
            return {str};
        }


        asset from_string( const string &from ) {

            try {

                string s = wasm::trim(from);

                // Find space in order to split amount and symbol
                auto space_pos = s.find(' ');
                CHAIN_ASSERT((space_pos != string::npos), asset_type_exception, "Asset's amount and symbol should be separated with space. ex. 98.00000000 WICC");
                auto symbol_str = wasm::trim(s.substr(space_pos + 1));
                auto amount_str = s.substr(0, space_pos);

                // Ensure that if decimal point is used (.), decimal fraction is specified
                auto dot_pos = amount_str.find('.');
                if (dot_pos != string::npos) {
                    CHAIN_ASSERT((dot_pos != amount_str.size() - 1), asset_type_exception, "Missing decimal fraction after decimal point. ex. 98.00000000 WICC");
                }

                // Parse symbol
                string precision_digit_str;
                if (dot_pos != string::npos) {
                    char c[8];
                    sprintf(c, "%ld", amount_str.size() - dot_pos - 1);

                    precision_digit_str = string(c);
                } else {
                    precision_digit_str = "0";
                }

                string symbol_part = precision_digit_str + ',' + symbol_str;
                class symbol sym = symbol::from_string(symbol_part);

                // Parse amount
                bool is_negtive = amount_str.size() > 0 && amount_str[0] == '-';
                int64_t int_part, fract_part = 0;
                // errno can be set to any non-zero value by a library function call
                // regardless of whether there was an error, so it needs to be cleared
                // in order to check the error set by strtoll
                errno = 0;
                if (dot_pos != string::npos) {
                    const auto &int_str = amount_str.substr(0, dot_pos);
                    int_part            = std::strtoll(int_str.c_str(), nullptr, 10);
                    CHAIN_ASSERT(errno != ERANGE && is_negtive == (int_part < 0),
                                 asset_type_exception, "The int part is overflow, str=%s", int_str);

                    const auto &fract_str = amount_str.substr(dot_pos + 1);
                    fract_part            = std::strtoll(fract_str.c_str(), nullptr, 10);
                    CHAIN_ASSERT(errno != ERANGE, asset_type_exception,
                                 "The fract part is overflow, str=%s", fract_str);
                    CHAIN_ASSERT(fract_part >= 0, asset_type_exception,
                                 "The fract part can not be negative, str=%s", fract_str);
                    if (is_negtive) {
                        fract_part *= -1;
                    }
                } else {
                    int_part = std::strtoll(amount_str.c_str(), nullptr, 10);
                    CHAIN_ASSERT(errno != ERANGE && is_negtive == (int_part < 0),
                                 asset_type_exception, "The int part is overflow, str=%s",
                                 amount_str);
                }

                int64_t precision_in_10 = sym.precision_in_10();
                assert(precision_in_10 > 0);
                int64_t amount = int_part * precision_in_10;
                CHAIN_ASSERT(int_part == amount / precision_in_10 , asset_type_exception,
                                "asset '%s' overflow", from);
                amount += fract_part;
                CHAIN_ASSERT(amount >= fract_part, asset_type_exception,
                                "asset '%s' overflow", from);

                return asset(amount, sym);
            }
            CHAIN_CAPTURE_AND_RETHROW( "%s", from.c_str() )

        }


        WASM_REFLECT( asset, (amount)(symbol) )

        // /**
        // *  Serialize a asset into a stream
        // *
        // *  @brief Serialize a asset
        // *  @param ds - The stream to write
        // *  @param sym - The value to serialize
        // *  @tparam DataStream - Type of datastream buffer
        // *  @return DataStream& - Reference to the datastream
        // */
        // template<typename DataStream>
        // friend inline DataStream &operator<<( DataStream &ds, const wasm::asset &asset ) {
        //     ds << asset.amount;
        //     ds << asset.sym;
        //     return ds;
        // }

        // /**
        // *  Deserialize a asset from a stream
        // *
        // *  @brief Deserialize a asset
        // *  @param ds - The stream to read
        // *  @param symbol - The destination for deserialized value
        // *  @tparam DataStream - Type of datastream buffer
        // *  @return DataStream& - Reference to the datastream
        // */
        // template<typename DataStream>
        // friend inline DataStream &operator>>( DataStream &ds, wasm::asset &asset ) {
        //     ds >> asset.amount;
        //     ds >> asset.sym;

        //     return ds;
        // }


    };

/// @} asset type
}
