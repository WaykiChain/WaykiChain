#pragma once

/**
 * @defgroup varint Variable Length Integer Type
 * @brief Defines variable length integer type which provides more efficient serialization
 * @ingroup types
 * @{
 */
/**
 * Variable Length Unsigned Integer. This provides more efficient serialization of 32-bit unsigned int.
 * It serialuzes a 32-bit unsigned integer in as few bytes as possible
 * `varuint32` is unsigned and uses [VLQ or Base-128 encoding](https://en.wikipedia.org/wiki/Variable-length_quantity)
 *
 * @brief Variable Length Unsigned Integer
 */
namespace wasm {
struct unsigned_int {
    /**
     * Construct a new unsigned int object
     *
     * @param v - Source
     */
    unsigned_int( uint32_t v = 0 ):value(v){}

    /**
     * Construct a new unsigned int object from a type that is convertible to uint32_t
     *
     * @tparam T - Type of the source
     * @param v - Source
     * @pre T must be convertible to uint32_t
     */
    template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
    unsigned_int( T v ):value(v){}

    //operator uint32_t()const { return value; }
    //operator uint64_t()const { return value; }

    /**
     * Convert unsigned_int as T
     *
     * @tparam T - Target type of conversion
     * @return T - Converted target
     */
    template<typename T>
    operator T()const { return static_cast<T>(value); }

    /**
     * Assign 32-bit unsigned integer
     *
     * @param v - Soruce
     * @return unsigned_int& - Reference to this object
     */
    unsigned_int& operator=( uint32_t v ) { value = v; return *this; }

    /**
     * Contained value
     */
    uint32_t value;

    /**
     * Check equality between a unsigned_int object and 32-bit unsigned integer
     *
     * @param i - unsigned_int object to compare
     * @param v - 32-bit unsigned integer to compare
     * @return true - if equal
     * @return false - otherwise
     */
    friend bool operator==( const unsigned_int& i, const uint32_t& v )     { return i.value == v; }

    /**
     * Check equality between 32-bit unsigned integer and  a unsigned_int object
     *
     * @param i - 32-bit unsigned integer to compare
     * @param v - unsigned_int object to compare
     * @return true - if equal
     * @return false - otherwise
     */
    friend bool operator==( const uint32_t& i, const unsigned_int& v )     { return i       == v.value; }

    /**
     * Check equality between two unsigned_int objects
     *
     * @param i - First unsigned_int object to compare
     * @param v - Second unsigned_int object to compare
     * @return true - if equal
     * @return false - otherwise
     */
    friend bool operator==( const unsigned_int& i, const unsigned_int& v ) { return i.value == v.value; }

    /**
     * Check inequality between a unsigned_int object and 32-bit unsigned integer
     *
     * @param i - unsigned_int object to compare
     * @param v - 32-bit unsigned integer to compare
     * @return true - if inequal
     * @return false - otherwise
     */
    friend bool operator!=( const unsigned_int& i, const uint32_t& v )     { return i.value != v; }

    /**
     * Check inequality between 32-bit unsigned integer and  a unsigned_int object
     *
     * @param i - 32-bit unsigned integer to compare
     * @param v - unsigned_int object to compare
     * @return true - if unequal
     * @return false - otherwise
     */
    friend bool operator!=( const uint32_t& i, const unsigned_int& v )     { return i       != v.value; }

    /**
     * Check inequality between two unsigned_int objects
     *
     * @param i - First unsigned_int object to compare
     * @param v - Second unsigned_int object to compare
     * @return true - if inequal
     * @return false - otherwise
     */
    friend bool operator!=( const unsigned_int& i, const unsigned_int& v ) { return i.value != v.value; }

    /**
     * Check if the given unsigned_int object is less than the given 32-bit unsigned integer
     *
     * @param i - unsigned_int object to compare
     * @param v - 32-bit unsigned integer to compare
     * @return true - if i less than v
     * @return false - otherwise
     */
    friend bool operator<( const unsigned_int& i, const uint32_t& v )      { return i.value < v; }

    /**
     * Check if the given 32-bit unsigned integer is less than the given unsigned_int object
     *
     * @param i - 32-bit unsigned integer to compare
     * @param v - unsigned_int object to compare
     * @return true -  if i less than v
     * @return false - otherwise
     */
    friend bool operator<( const uint32_t& i, const unsigned_int& v )      { return i       < v.value; }

    /**
     * Check if the first given unsigned_int is less than the second given unsigned_int object
     *
     * @param i - First unsigned_int object to compare
     * @param v - Second unsigned_int object to compare
     * @return true -  if i less than v
     * @return false - otherwise
     */
    friend bool operator<( const unsigned_int& i, const unsigned_int& v )  { return i.value < v.value; }

    /**
     * Check if the given unsigned_int object is greater or equal to the given 32-bit unsigned integer
     *
     * @param i - unsigned_int object to compare
     * @param v - 32-bit unsigned integer to compare
     * @return true - if i is greater or equal to v
     * @return false - otherwise
     */
    friend bool operator>=( const unsigned_int& i, const uint32_t& v )     { return i.value >= v; }

    /**
     * Check if the given 32-bit unsigned integer is greater or equal to the given unsigned_int object
     *
     * @param i - 32-bit unsigned integer to compare
     * @param v - unsigned_int object to compare
     * @return true -  if i is greater or equal to v
     * @return false - otherwise
     */
    friend bool operator>=( const uint32_t& i, const unsigned_int& v )     { return i       >= v.value; }

    /**
     * Check if the first given unsigned_int is greater or equal to the second given unsigned_int object
     *
     * @param i - First unsigned_int object to compare
     * @param v - Second unsigned_int object to compare
     * @return true -  if i is greater or equal to v
     * @return false - otherwise
     */
    friend bool operator>=( const unsigned_int& i, const unsigned_int& v ) { return i.value >= v.value; }

};

/**
 * Variable Length Signed Integer. This provides more efficient serialization of 32-bit signed int.
 * It serializes a 32-bit signed integer in as few bytes as possible.
 *
 * @note `varint32' is signed and uses [Zig-Zag encoding](https://developers.google.com/protocol-buffers/docs/encoding#signed-integers)
 */
struct signed_int {
    /**
     * Construct a new signed int object
     *
     * @param v - Source
     */
    signed_int( int32_t v = 0 ):value(v){}

    /**
     * Convert signed_int to primitive 32-bit signed integer
     *
     * @return int32_t - The converted result
     */
    operator int32_t()const { return value; }


    /**
     * Assign an object that is convertible to int32_t
     *
     * @tparam T - Type of the assignment object
     * @param v - Source
     * @return unsigned_int& - Reference to this object
     */
    template<typename T , std::enable_if_t<std::is_integral<T>::value>* = nullptr>
    signed_int& operator=( const T& v ) { value = v; return *this; }

    /**
     * Increment operator
     *
     * @return signed_int - New signed_int with value incremented from the current object's value
     */
    signed_int operator++(int) { return value++; }

    /**
     * Increment operator
     *
     * @return signed_int - Reference to current object
     */
    signed_int& operator++(){ ++value; return *this; }

    /**
     * Contained value
     */
    int32_t value;

    /**
     * Check equality between a signed_int object and 32-bit integer
     *
     * @param i - signed_int object to compare
     * @param v - 32-bit integer to compare
     * @return true - if equal
     * @return false - otherwise
     */
    friend bool operator==( const signed_int& i, const int32_t& v )    { return i.value == v; }

    /**
     * Check equality between 32-bit integer and  a signed_int object
     *
     * @param i - 32-bit integer to compare
     * @param v - signed_int object to compare
     * @return true - if equal
     * @return false - otherwise
     */
    friend bool operator==( const int32_t& i, const signed_int& v )    { return i       == v.value; }

    /**
     * Check equality between two signed_int objects
     *
     * @param i - First signed_int object to compare
     * @param v - Second signed_int object to compare
     * @return true - if equal
     * @return false - otherwise
     */
    friend bool operator==( const signed_int& i, const signed_int& v ) { return i.value == v.value; }


    /**
     * Check inequality between a signed_int object and 32-bit integer
     *
     * @param i - signed_int object to compare
     * @param v - 32-bit integer to compare
     * @return true - if inequal
     * @return false - otherwise
     */
    friend bool operator!=( const signed_int& i, const int32_t& v )    { return i.value != v; }

    /**
     * Check inequality between 32-bit integer and  a signed_int object
     *
     * @param i - 32-bit integer to compare
     * @param v - signed_int object to compare
     * @return true - if unequal
     * @return false - otherwise
     */
    friend bool operator!=( const int32_t& i, const signed_int& v )    { return i       != v.value; }

    /**
     * Check inequality between two signed_int objects
     *
     * @param i - First signed_int object to compare
     * @param v - Second signed_int object to compare
     * @return true - if inequal
     * @return false - otherwise
     */
    friend bool operator!=( const signed_int& i, const signed_int& v ) { return i.value != v.value; }

    /**
     * Check if the given signed_int object is less than the given 32-bit integer
     *
     * @param i - signed_int object to compare
     * @param v - 32-bit integer to compare
     * @return true - if i less than v
     * @return false - otherwise
     */
    friend bool operator<( const signed_int& i, const int32_t& v )     { return i.value < v; }

    /**
     * Check if the given 32-bit integer is less than the given signed_int object
     *
     * @param i - 32-bit integer to compare
     * @param v - signed_int object to compare
     * @return true -  if i less than v
     * @return false - otherwise
     */
    friend bool operator<( const int32_t& i, const signed_int& v )     { return i       < v.value; }

    /**
     * Check if the first given signed_int is less than the second given signed_int object
     *
     * @param i - First signed_int object to compare
     * @param v - Second signed_int object to compare
     * @return true -  if i less than v
     * @return false - otherwise
     */
    friend bool operator<( const signed_int& i, const signed_int& v )  { return i.value < v.value; }


    /**
     * Check if the given signed_int object is greater or equal to the given 32-bit integer
     *
     * @param i - signed_int object to compare
     * @param v - 32-bit integer to compare
     * @return true - if i is greater or equal to v
     * @return false - otherwise
     */
    friend bool operator>=( const signed_int& i, const int32_t& v )    { return i.value >= v; }

    /**
     * Check if the given 32-bit integer is greater or equal to the given signed_int object
     *
     * @param i - 32-bit integer to compare
     * @param v - signed_int object to compare
     * @return true -  if i is greater or equal to v
     * @return false - otherwise
     */
    friend bool operator>=( const int32_t& i, const signed_int& v )    { return i       >= v.value; }

    /**
     * Check if the first given signed_int is greater or equal to the second given signed_int object
     *
     * @param i - First signed_int object to compare
     * @param v - Second signed_int object to compare
     * @return true -  if i is greater or equal to v
     * @return false - otherwise
     */
    friend bool operator>=( const signed_int& i, const signed_int& v ) { return i.value >= v.value; }


};
}

/// @}
