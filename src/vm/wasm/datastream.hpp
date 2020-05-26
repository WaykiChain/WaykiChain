#pragma once

#include <list>
#include <queue>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <string>
#include <optional>
#include <variant>
#include <cstring>
#include <sstream>
#include <chrono>

#include "wasm/types/types.hpp"
#include "wasm/types/symbol.hpp"
#include "wasm/types/varint.hpp"
//#include "wasm/types/inline_transaction.hpp"


namespace wasm {

    using std::chrono::system_clock;

/**
 *  A data stream for reading and writing data in the form of bytes
 *
 *  @tparam T - Type of the datastream buffer
 */
    template<typename T>
    class datastream {

    public:
        typedef int wasm;

    public:
        /**
         * @brief Construct a new datastream object
         *
         * @details Construct a new datastream object given the size of the buffer and start position of the buffer
         * @param start - The start position of the buffer
         * @param s - The size of the buffer
         */
        datastream( T start, size_t s )
                : _start(start), _pos(start), _end(start + s) {}

        /**
         *  Skips a specified number of bytes from this stream
         *
         *  @param s - The number of bytes to skip
         */
        inline void skip( size_t s ) { _pos += s; }

        /**
         *  Reads a specified number of bytes from the stream into a buffer
         *
         *  @param d - The pointer to the destination buffer
         *  @param s - the number of bytes to read
         *  @return true
         */
        inline bool read( char *d, size_t s ) {
            check( size_t(_end - _pos) >= (size_t)s, "datastream read out of bounds" );
            memcpy(d, _pos, s);
            _pos += s;
            return true;
        }

        /**
         *  Writes a specified number of bytes into the stream from a buffer
         *
         *  @param d - The pointer to the source buffer
         *  @param s - The number of bytes to write
         *  @return true
         */
        inline bool write( const char *d, size_t s ) {
            check( _end - _pos >= (int32_t)s, "datastream write out of bounds" );
            memcpy((void *) _pos, d, s);
            _pos += s;
            return true;
        }

        /**
         *  Writes a byte into the stream
         *
         *  @brief Writes a byte into the stream
         *  @param c byte to write
         *  @return true
         */
        inline bool put( char c ) {
            check( _pos < _end, "datastream put out of bounds" );
            *_pos = c;
            ++_pos;
            return true;
        }

        /**
         *  Reads a byte from the stream
         *
         *  @brief Reads a byte from the stream
         *  @param c - The reference to destination byte
         *  @return true
         */
        inline bool get( unsigned char &c ) { return get(*(char *) &c); }

        /**
         *  Reads a byte from the stream
         *
         *  @brief Reads a byte from the stream
         *  @param c - The reference to destination byte
         *  @return true
         */
        inline bool get( char &c ) {
            check( _pos < _end, "datastream get out of bounds" );
            c = *_pos;
            ++_pos;
            return true;
        }

        /**
         *  Retrieves the current position of the stream
         *
         *  @brief Retrieves the current position of the stream
         *  @return T - The current position of the stream
         */
        T pos() const { return _pos; }

        inline bool valid() const { return _pos <= _end && _pos >= _start; }

        /**
         *  Sets the position within the current stream
         *
         *  @brief Sets the position within the current stream
         *  @param p - The offset relative to the origin
         *  @return true if p is within the range
         *  @return false if p is not within the rawnge
         */
        inline bool seekp( size_t p ) {
            _pos = _start + p;
            return _pos <= _end;
        }

        /**
         *  Gets the position within the current stream
         *
         *  @brief Gets the position within the current stream
         *  @return p - The position within the current stream
         */
        inline size_t tellp() const { return size_t(_pos - _start); }

        /**
         *  Returns the number of remaining bytes that can be read/skipped
         *
         *  @brief Returns the number of remaining bytes that can be read/skipped
         *  @return size_t - The number of remaining bytes
         */
        inline size_t remaining() const { return _end - _pos; }

    private:
        /**
         * The start position of the buffer
         *
         * @brief The start position of the buffer
         */
        T _start;
        /**
         * The current position of the buffer
         *
         * @brief The current position of the buffer
         */
        T _pos;
        /**cl
         * The end position of the buffer
         *
         * @brief The end position of the buffer
         */
        T _end;
    };

/**
 * @brief Specialization of datastream used to help determine the final size of a serialized value.
 * Specialization of datastream used to help determine the final size of a serialized value
 */
    template<>
    class datastream<size_t> {
    public:
        typedef int wasm;

    public:
        /**
         * Construct a new specialized datastream object given the initial size
         *
         * @brief Construct a new specialized datastream object
         * @param init_size - The initial size
         */
        datastream( size_t init_size = 0 ) : _size(init_size) {}

        /**
         *  Increment the size by s. This behaves the same as write( const char* ,size_t s ).
         *
         *  @brief Increase the size by s
         *  @param s - The amount of size to increase
         *  @return true
         */
        inline bool skip( size_t s ) {
            _size += s;
            return true;
        }

        /**
         *  Increment the size by s. This behaves the same as skip( size_t s )
         *
         *  @brief Increase the size by s
         *  @param s - The amount of size to increase
         *  @return true
         */
        inline bool write( const char *, size_t s ) {
            _size += s;
            return true;
        }

        /**
         *  Increment the size by one
         *
         *  @brief Increase the size by one
         *  @return true
         */
        inline bool put( char ) {
            ++_size;
            return true;
        }

        /**
         *  Check validity. It's always valid
         *
         *  @brief Check validity
         *  @return true
         */
        inline bool valid() const { return true; }

        /**
         * Set new size
         *
         * @brief Set new size
         * @param p - The new size
         * @return true
         */
        inline bool seekp( size_t p ) {
            _size = p;
            return true;
        }

        /**
         * Get the size
         *
         * @brief Get the size
         * @return size_t - The size
         */
        inline size_t tellp() const { return _size; }

        /**
         * Always returns 0
         *
         * @brief Always returns 0
         * @return size_t - 0
         */
        inline size_t remaining() const { return 0; }

    private:
        /**
         * The size used to determine the final size of a serialized value.
         *
         * @brief The size used to determine the final size of a serialized value.
         */
        size_t _size;
    };

/**
 *  Serialize an std::list into a stream
 *
 *  @brief Serialize an std::list
 *  @param ds - The stream to write
 *  @param opt - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename T>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const std::list <T> &l ) {
        ds << unsigned_int(l.size());
        for (auto elem : l)
            ds << elem;
        return ds;
    }

/**
 *  Deserialize an std::list from a stream
 *
 *  @brief Deserialize an std::list
 *  @param ds - The stream to read
 *  @param opt - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename T>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, std::list <T> &l ) {
        unsigned_int s;
        ds >> s;
        l.resize(s.value);
        for (auto &i : l)
            ds >> i;
        return ds;
    }

/**
 *  Serialize an std::deque into a stream
 *
 *  @brief Serialize an std::queue
 *  @param ds - The stream to write
 *  @param opt - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename T>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const std::deque <T> &d ) {
        ds << unsigned_int(d.size());
        for (auto elem : d)
            ds << elem;
        return ds;
    }

/**
 *  Deserialize an std::deque from a stream
 *
 *  @brief Deserialize an std::deque
 *  @param ds - The stream to read
 *  @param opt - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename T>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, std::deque <T> &d ) {
        unsigned_int s;
        ds >> s;
        d.resize(s.value);
        for (auto &i : d)
            ds >> i;
        return ds;
    }

/**
 *  Serialize an std::variant into a stream
 *
 *  @brief Serialize an std::variant
 *  @param ds - The stream to write
 *  @param opt - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename... Ts>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const std::variant<Ts...> &var ) {
        unsigned_int index = var.index();
        ds << index;
        std::visit([&ds]( auto &val ) { ds << val; }, var);
        return ds;
    }

    template<int I, typename Stream, typename... Ts>
    void deserialize( datastream<Stream> &ds, std::variant<Ts...> &var, int i ) {
        if constexpr (I < std::variant_size_v < std::variant < Ts...>>) {
            if (i == I) {
                std::variant_alternative_t <I, std::variant<Ts...>> tmp;
                ds >> tmp;
                var = std::move(tmp);
            } else {
                deserialize<I + 1>(ds, var, i);
            }
        } else {
            //check(false, "invalid variant index");
        }
    }

/**
 *  Deserialize an std::variant from a stream
 *
 *  @brief Deserialize an std::variant
 *  @param ds - The stream to read
 *  @param opt - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename... Ts>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, std::variant<Ts...> &var ) {
        unsigned_int index;
        ds >> index;
        deserialize<0>(ds, var, index);
        return ds;
    }

/**
 *  Serialize an std::pair
 *
 *  @brief Serialize an std::pair
 *  @param ds - The stream to write
 *  @param t - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam Args - Type of the objects contained in the tuple
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T1, typename T2>
    DataStream &operator<<( DataStream &ds, const std::pair <T1, T2> &t ) {
        ds << std::get<0>(t);
        ds << std::get<1>(t);
        return ds;
    }

/**
 *  Deserialize an std::pair
 *
 *  @brief Deserialize an std::pair
 *  @param ds - The stream to read
 *  @param t - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam Args - Type of the objects contained in the tuple
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T1, typename T2>
    DataStream &operator>>( DataStream &ds, std::pair <T1, T2> &t ) {
        T1 t1;
        T2 t2;
        ds >> t1;
        ds >> t2;
        t = std::pair < T1, T2 > {t1, t2};
        return ds;
    }

/**
 *  Serialize an optional into a stream
 *
 *  @brief Serialize an optional
 *  @param ds - The stream to write
 *  @param opt - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename T>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const std::optional <T> &opt ) {
        char valid = opt.has_value();
        ds << valid;
        if (valid)
            ds << *opt;
        return ds;
    }

/**
 *  Deserialize an optional from a stream
 *
 *  @brief Deserialize an optional
 *  @param ds - The stream to read
 *  @param opt - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream, typename T>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, std::optional <T> &opt ) {
        char valid = 0;
        ds >> valid;
        if (valid) {
            T val;
            ds >> val;
            opt = val;
        }
        return ds;
    }

/**
 *  Serialize a bool into a stream
 *
 *  @brief  Serialize a bool into a stream
 *  @param ds - The stream to read
 *  @param d - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const bool &d ) {
        return ds << uint8_t(d);
    }

/**
 *  Deserialize a bool from a stream
 *
 *  @brief Deserialize a bool
 *  @param ds - The stream to read
 *  @param d - The destination for deserialized value
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, bool &d ) {
        uint8_t t;
        ds >> t;
        d = t;
        return ds;
    }


/**
 *  Serialize a fixed size std::array
 *
 *  @brief Serialize a fixed size std::array
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::size_t N>
    DataStream &operator<<( DataStream &ds, const std::array <T, N> &v ) {
        for (const auto &i : v)
            ds << i;
        return ds;
    }


/**
 *  Deserialize a fixed size std::array
 *
 *  @brief Deserialize a fixed size std::array
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::size_t N>
    DataStream &operator>>( DataStream &ds, std::array <T, N> &v ) {
        for (auto &i : v)
            ds >> i;
        return ds;
    }

    namespace _datastream_detail {
        /**
         * Check if type T is a pointer
         *
         * @brief Check if type T is a pointer
         * @tparam T - The type to be checked
         * @return true if T is a pointer
         * @return false otherwise
         */
        template<typename T>
        constexpr bool is_pointer() {
            return std::is_pointer<T>::value ||
                   std::is_null_pointer<T>::value ||
                   std::is_member_pointer<T>::value;
        }

        /**
         * Check if type T is a primitive type
         *
         * @brief Check if type T is a primitive type
         * @tparam T - The type to be checked
         * @return true if T is a primitive type
         * @return false otherwise
         */
        template<typename T>
        constexpr bool is_primitive() {
            return std::is_arithmetic<T>::value ||
                   std::is_enum<T>::value;
        }
    }


    /**
 *  Serialize a uint128_t into a stream
 *
 *  @brief Serialize a uint128_t
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const uint128_t &v ) {

        ds.write((const char *) &v, sizeof(uint128_t));
        return ds;
    }

/**
 *  Deserialize a int128_t from a stream
 *
 *  @brief Deserialize a int128_t
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, uint128_t &v ) {

        ds.read((char *) &v, sizeof(uint128_t));
        return ds;
    }


    /**
 *  Serialize a uint128_t into a stream
 *
 *  @brief Serialize a uint128_t
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const int128_t &v ) {

        ds.write((const char *) &v, sizeof(int128_t));
        return ds;
    }

/**
 *  Deserialize a int128_t from a stream
 *
 *  @brief Deserialize a int128_t
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, int128_t &v ) {

        ds.read((char *) &v, sizeof(int128_t));
        return ds;
    }


/**
 *  Serialize a string into a stream
 *
 *  @brief Serialize a string
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const std::string &v ) {
        ds << unsigned_int(v.size());
        if (v.size())
            ds.write(v.data(), v.size());
        return ds;
    }

/**
 *  Deserialize a string from a stream
 *
 *  @brief Deserialize a string
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, std::string &v ) {
        unsigned_int s;
        ds >> s;
        v.resize(s, '0');
        ds.read(v.data(), v.size());
        return ds;
    }

/**
 *  Pointer should not be serialized, so this function will always throws an error
 *
 *  @brief Deserialize a a pointer
 *  @param ds - The stream to read
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the pointer
 *  @return DataStream& - Reference to the datastream
 *  @post Throw an exception if it is a pointer
 */
    template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_pointer<T>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, T ) {
        static_assert(!_datastream_detail::is_pointer<T>(), "Pointers should not be serialized");
        return ds;
    }

/**
 *  Serialize a fixed size C array of non-primitive and non-pointer type
 *
 *  @brief Serialize a fixed size C array of non-primitive and non-pointer type
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the pointer
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::size_t N,
            std::enable_if_t<!_datastream_detail::is_primitive<T>() &&
                             !_datastream_detail::is_pointer<T>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const T (&v)[N] ) {
        ds << unsigned_int(N);
        for (uint32_t i = 0; i < N; ++i)
            ds << v[i];
        return ds;
    }

/**
 *  Serialize a fixed size C array of primitive type
 *
 *  @brief Serialize a fixed size C array of primitive type
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the pointer
 *  @return DataStream& - Reference to the datastream
 */
// template<typename DataStream, typename T, std::size_t N,
//          std::enable_if_t<_datastream_detail::is_primitive<T>()>* = nullptr>

    template<typename DataStream, typename T, std::size_t N,
            std::enable_if_t<_datastream_detail::is_primitive<T>() &&
                             _datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const T (&v)[N] ) {
        ds << unsigned_int(N);
        ds.write((char *) &v[0], sizeof(v));
        return ds;
    }

/**
 *  Deserialize a fixed size C array of non-primitive and non-pointer type
 *
 *  @brief Deserialize a fixed size C array of non-primitive and non-pointer type
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
// template<typename DataStream, typename T, std::size_t N,
//          std::enable_if_t<!_datastream_detail::is_primitive<T>() &&
//                           !_datastream_detail::is_pointer<T>()>* = nullptr>
    template<typename DataStream, typename T, std::size_t N,
            std::enable_if_t<!_datastream_detail::is_primitive<T>() &&
                             !_datastream_detail::is_pointer<T>() &&
                             _datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, T (&v)[N] ) {
        unsigned_int s;
        ds >> s;
        //eosio::check( N == s.value, "T[] size and unpacked size don't match");
        for (uint32_t i = 0; i < N; ++i)
            ds >> v[i];
        return ds;
    }

/**
 *  Deserialize a fixed size C array of primitive type
 *
 *  @brief Deserialize a fixed size C array of primitive type
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam T - Type of the object contained in the array
 *  @tparam N - Size of the array
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::size_t N,
            std::enable_if_t<_datastream_detail::is_primitive<T>() &&
                             _datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, T (&v)[N] ) {
        unsigned_int s;
        ds >> s;
        //eosio::check( N == s.value, "T[] size and unpacked size don't match");
        ds.read((char *) &v[0], sizeof(v));
        return ds;
    }

/**
 *  Serialize a vector of char
 *
 *  @brief Serialize a vector of char
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const std::vector<char> &v ) {
        ds << unsigned_int(v.size());
        ds.write(v.data(), v.size());
        return ds;
    }

/**
 *  Serialize a vector
 *
 *  @brief Serialize a vector
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the vector
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const std::vector <T> &v ) {
        ds << unsigned_int(v.size());
        for (const auto &i : v)
            ds << i;
        return ds;
    }

/**
 *  Deserialize a vector of bool
 *
 *  @brief Deserialize a vector of bool
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, std::vector<bool> &v ) {
        unsigned_int s;
        ds >> s;
        v.resize(s.value);
        bool tmp;
        for (size_t i = 0; i < s.value; i++) {
            ds >> tmp;
            v[i] = tmp;
        }
        return ds;
    }

/**
 *  Deserialize a vector of char
 *
 *  @brief Deserialize a vector of char
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, std::vector<char> &v ) {
        unsigned_int s;
        ds >> s;
        v.resize(s.value);
        ds.read(v.data(), v.size());
        return ds;
    }

/**
 *  Deserialize a vector
 *
 *  @brief Deserialize a vector
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the vector
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, std::vector <T> &v ) {
        unsigned_int s;
        ds >> s;
        v.resize(s.value);
        for (auto &i : v)
            ds >> i;
        return ds;
    }

/**
 *  Serialize a set
 *
 *  @brief Serialize a set
 *  @param ds - The stream to write
 *  @param s - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the set
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const std::set <T> &s ) {
        ds << unsigned_int(s.size());
        for (const auto &i : s) {
            ds << i;
        }
        return ds;
    }


/**
 *  Deserialize a set
 *
 *  @brief Deserialize a set
 *  @param ds - The stream to read
 *  @param s - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the object contained in the set
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, std::set <T> &s ) {
        s.clear();
        unsigned_int sz;
        ds >> sz;

        for (uint32_t i = 0; i < sz.value; ++i) {
            T v;
            ds >> v;
            s.emplace(std::move(v));
        }
        return ds;
    }

/**
 *  Serialize a map
 *
 *  @brief Serialize a map
 *  @param ds - The stream to write
 *  @param m - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam K - Type of the key contained in the map
 *  @tparam V - Type of the value contained in the map
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename K, typename V, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const std::map <K, V> &m ) {
        ds << unsigned_int(m.size());
        for (const auto &i : m) {
            ds << i.first << i.second;
        }
        return ds;
    }

/**
 *  Deserialize a map
 *
 *  @brief Deserialize a map
 *  @param ds - The stream to read
 *  @param m - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam K - Type of the key contained in the map
 *  @tparam V - Type of the value contained in the map
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename K, typename V, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, std::map <K, V> &m ) {
        m.clear();
        unsigned_int s;
        ds >> s;

        for (uint32_t i = 0; i < s.value; ++i) {
            K k;
            V v;
            ds >> k >> v;
            m.emplace(std::move(k), std::move(v));
        }
        return ds;
    }


/**
 *  Serialize a tuple
 *
 *  @brief Serialize a tuple
 *  @param ds - The stream to write
 *  @param t - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam Args - Type of the objects contained in the tuple
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename Tuple, size_t N>
    struct tuple_write {
        static void write( const Tuple &t, DataStream &ds ) {
            tuple_write<DataStream, Tuple, N - 1>::write(t, ds);
            ds << std::get<N - 1>(t);
        }
    };


    template<typename DataStream, typename Tuple>
    struct tuple_write<DataStream, Tuple, 1> {
        static void write( const Tuple &t, DataStream &ds ) {
            ds << std::get<0>(t);
        }
    };

    template<typename DataStream, typename... Args>
    DataStream &operator<<( DataStream &ds, const std::tuple<Args...> &t ) {
        tuple_write<DataStream, decltype(t), sizeof...(Args)>::write(t, ds);
        return ds;
    }

    //Deserialize a tuple
    template<typename DataStream, typename Tuple, size_t N>
    struct tuple_read {
        static void read( Tuple &t, DataStream &ds ) {
            tuple_read<DataStream, Tuple, N - 1>::read(t, ds);
            ds >> std::get<N - 1>(t);
        }
    };

    template<typename DataStream, typename Tuple>
    struct tuple_read<DataStream, Tuple, 1> {
        static void read( Tuple &t, DataStream &ds ) {
            ds >> std::get<0>(t);
        }
    };

    template<typename DataStream, typename... Args>
    DataStream &operator>>( DataStream &ds, std::tuple<Args...> &t ) {

        tuple_read<DataStream, decltype(t), sizeof...(Args)>::read(t, ds);

        return ds;
    }


/**
 *  Serialize a primitive type
 *
 *  @brief Serialize a primitive type
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the primitive type
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<T>() &&
                                                               _datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const T &v ) {
        ds.write((const char *) &v, sizeof(T));
        return ds;
    }

/**
 *  Deserialize a primitive type
 *
 *  @brief Deserialize a primitive type
 *  @param ds - The stream to read
 *  @param v - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @tparam T - Type of the primitive type
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, typename T, std::enable_if_t<_datastream_detail::is_primitive<T>() &&
                                                               _datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, T &v ) {
        ds.read((char *) &v, sizeof(T));
        return ds;
    }


/**
 *  Serialize an unsigned_int object with as few bytes as possible
 *
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const unsigned_int &v ) {
        uint64_t val = v.value;
        do {
            uint8_t b = uint8_t(val) & 0x7f;
            val >>= 7;
            b |= ((val > 0) << 7);
            ds.write((char *) &b, 1);//.put(b);
        } while (val);
        return ds;
    }

/**
 *  Deserialize an unsigned_int object
 *
 *  @param ds - The stream to read
 *  @param vi - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, unsigned_int &vi ) {
        uint64_t v = 0;
        char b = 0;
        uint8_t by = 0;
        do {
            ds.get(b);
            v |= uint32_t(uint8_t(b) & 0x7f) << by;
            by += 7;
        } while (uint8_t(b) & 0x80);
        vi.value = static_cast<uint32_t>(v);
        return ds;
    }


/**
 *  Serialize an signed_int object with as few bytes as possible
 *
 *  @param ds - The stream to write
 *  @param v - The value to serialize
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator<<( DataStream &ds, const signed_int &v ) {
        uint32_t val = uint32_t((v.value << 1) ^ (v.value >> 31));    //apply zigzag encoding
        do {                                                      //store 7 bit chunks
            uint8_t b = uint8_t(val) & 0x7f;
            val >>= 7;
            b |= ((val > 0) << 7);
            ds.write((char *) &b, 1);//.put(b);
        } while (val);
        return ds;
    }

/**
 *  Deserialize an signed_int object
 *
 *  @param ds - The stream to read
 *  @param vi - The destination for deserialized value
 *  @tparam DataStream - Type of datastream
 *  @return DataStream& - Reference to the datastream
 */
    template<typename DataStream, std::enable_if_t<_datastream_detail::is_primitive<typename DataStream::wasm>()> * = nullptr>
    DataStream &operator>>( DataStream &ds, signed_int &vi ) {
        uint32_t v = 0;
        char b = 0;
        int by = 0;
        do {                                                      //read 7 bit chunks
            ds.get(b);
            v |= uint32_t(uint8_t(b) & 0x7f) << by;
            by += 7;
        } while (uint8_t(b) & 0x80);
        vi.value = (v >> 1) ^ (~(v & 1) + 1ull);                         //reverse zigzag encoding
        return ds;
    }


    /**
 *  Serialize a checksum160 type
 *
 *  @brief Serializea capi_checksum160 type
 *  @param ds - The stream to write
 *  @param cs - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const system_clock::time_point &t ) {
        ds.write((const char *) &t, sizeof(t));
        return ds;
    }

    /**
     *  Deserialize a checksum160 type
     *
     *  @brief Deserialize a capi_checksum160 type
     *  @param ds - The stream to read
     *  @param cs - The destination for deserialized value
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
    template<typename Stream>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, system_clock::time_point &t ) {
        ds.read((char *) &t, sizeof(t));
        return ds;
    }

/**
 *  Serialize a checksum160 type
 *
 *  @brief Serializea capi_checksum160 type
 *  @param ds - The stream to write
 *  @param cs - The value to serialize
 *  @tparam Stream - Type of datastream buffer
 *  @return datastream<Stream>& - Reference to the datastream
 */
    template<typename Stream>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const checksum160_type &cs ) {
        ds.write((const char *) &cs.hash[0], sizeof(cs.hash));
        return ds;
    }

    /**
     *  Deserialize a checksum160 type
     *
     *  @brief Deserialize a capi_checksum160 type
     *  @param ds - The stream to read
     *  @param cs - The destination for deserialized value
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
    template<typename Stream>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, checksum160_type &cs ) {
        ds.read((char *) &cs.hash[0], sizeof(cs.hash));
        return ds;
    }

    /**
     *  Serialize a checksum256_type type
     *
     *  @brief Serializea capi_checksum256 type
     *  @param ds - The stream to write
     *  @param cs - The value to serialize
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
    template<typename Stream>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const checksum256_type &cs ) {
        ds.write((const char *) &cs.hash[0], sizeof(cs.hash));
        return ds;
    }

    /**
     *  Deserialize a checksum256_type type
     *
     *  @brief Deserialize a checksum256 type
     *  @param ds - The stream to read
     *  @param cs - The destination for deserialized value
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
    template<typename Stream>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, checksum256_type &cs ) {
        ds.read((char *) &cs.hash[0], sizeof(cs.hash));
        return ds;
    }


    /**
     *  Serialize a checksum512 type
     *
     *  @brief Serialize a checksum512 type
     *  @param ds - The stream to write
     *  @param cs - The value to serialize
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
    */
    template<typename Stream>
    inline datastream<Stream> &operator<<( datastream<Stream> &ds, const checksum512_type &cs ) {
        ds.write((const char *) &cs.hash[0], sizeof(cs.hash));
        return ds;
    }

    /**
     *  Deserialize a checksum512 type
     *
     *  @brief Deserialize a checksum512 type
     *  @param ds - The stream to read
     *  @param cs - The destination for deserialized value
     *  @tparam Stream - Type of datastream buffer
     *  @return datastream<Stream>& - Reference to the datastream
     */
    template<typename Stream>
    inline datastream<Stream> &operator>>( datastream<Stream> &ds, checksum512_type &cs ) {
        ds.read((char *) &cs.hash[0], sizeof(cs.hash));
        return ds;
    }


    /**
     * Defines data stream for reading and writing data in the form of bytes
     *
     * @addtogroup datastream Data Stream
     * @ingroup core
     * @{
     */

    /**
     * Unpack data inside a fixed size buffer as T
     *
     * @brief Unpack data inside a fixed size buffer as T
     * @tparam T - Type of the unpacked data
     * @param buffer - Pointer to the buffer
     * @param len - Length of the buffer
     * @return T - The unpacked data
     */
    template<typename T>
    T unpack( const char *buffer, size_t len ) {
        T result;
        datastream<const char *> ds(buffer, len);
        ds >> result;
        return result;
    }

    /**
     * Unpack data inside a fixed size buffer as T
     *
     * @brief Unpack data inside a fixed size buffer as T
     * @tparam T - Type of the unpacked data
     * @param buffer - Pointer to the buffer
     * @param len - Length of the buffer
     * @param T - The unpacked data
     */
    template<typename T>
    void unpack( const char *buffer, size_t len, T &value ) {
        datastream<const char *> ds(buffer, len);
        ds >> value;
    }

    /**
     * Unpack data inside a variable size buffer as T
     *
     * @brief Unpack data inside a variable size buffer as T
     * @tparam T - Type of the unpacked data
     * @param bytes - Buffer
     * @return T - The unpacked data
     */
    template<typename T>
    T unpack( const std::vector<char> &bytes ) {
        return unpack<T>(bytes.data(), bytes.size());
    }

    /**
     * Unpack data inside a variable size buffer as T
     *
     * @brief Unpack data inside a variable size buffer as T
     * @tparam T - Type of the unpacked data
     * @param bytes - Buffer
     * @return T - The unpacked data
     */
    template<typename T>
    void unpack( const std::vector<char> &bytes, T &value ) {
        unpack<T>(bytes.data(), bytes.size(), value);
    }

    /**
     * Get the size of the packed data
     *
     * @brief Get the size of the packed data
     * @tparam T - Type of the data to be packed
     * @param value - Data to be packed
     * @return size_t - Size of the packed data
     */
    template<typename T>
    size_t pack_size( const T &value ) {
        datastream<size_t> ps;
        ps << value;
        return ps.tellp();
    }

    /**
     * Get packed data
     *
     * @brief Get packed data
     * @tparam T - Type of the data to be packed
     * @param value - Data to be packed
     * @return bytes - The packed data
     */
    template<typename T>
    std::vector<char> pack( const T &value ) {
        std::vector<char> result;
        result.resize(pack_size(value));

        datastream<char *> ds(result.data(), result.size());
        ds << value;
        return result;
    }


}
