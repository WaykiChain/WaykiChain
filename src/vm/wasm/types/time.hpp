#pragma once
#include <stdint.h>
#include <string>
#include "wasm/wasm_serialize_reflect.hpp"
// #include <boost/date_time/posix_time/posix_time.hpp>

namespace wasm {
  /**
   *  @defgroup time
   *  @ingroup core
   *  @brief Classes for working with time.
   */

  class microseconds_t {
    public:
        explicit microseconds_t( int64_t c = 0) :_count(c){}

        /// @cond INTERNAL
        static microseconds_t maximum() { return microseconds_t(0x7fffffffffffffffll); }
        friend microseconds_t operator + (const  microseconds_t& l, const microseconds_t& r ) { return microseconds_t(l._count+r._count); }
        friend microseconds_t operator - (const  microseconds_t& l, const microseconds_t& r ) { return microseconds_t(l._count-r._count); }


        bool operator==(const microseconds_t& c)const { return _count == c._count; }
        bool operator!=(const microseconds_t& c)const { return _count != c._count; }
        friend bool operator>(const microseconds_t& a, const microseconds_t& b){ return a._count > b._count; }
        friend bool operator>=(const microseconds_t& a, const microseconds_t& b){ return a._count >= b._count; }
        friend bool operator<(const microseconds_t& a, const microseconds_t& b){ return a._count < b._count; }
        friend bool operator<=(const microseconds_t& a, const microseconds_t& b){ return a._count <= b._count; }
        microseconds_t& operator+=(const microseconds_t& c) { _count += c._count; return *this; }
        microseconds_t& operator-=(const microseconds_t& c) { _count -= c._count; return *this; }
        int64_t count()const { return _count; }
        int64_t to_seconds()const { return _count/1000000; }

        int64_t _count;

        WASM_REFLECT( microseconds_t, (_count) )

        /**  Serialize a symbol_code into a stream
        *
        *  @brief Serialize a symbol_code
        *  @param ds - The stream to write
        *  @param sym - The value to serialize
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        // template<typename DataStream>
        // friend inline DataStream &operator<<( DataStream &ds, const wasm::microseconds_t &t ) {
        //     int64_t raw = t._count;
        //     ds.write((const char *) &raw, sizeof(int64_t));
        //     return ds;
        // }

        /**
        *  Deserialize a symbol_code from a stream
        *
        *  @brief Deserialize a symbol_code
        *  @param ds - The stream to read
        *  @param symbol - The destination for deserialized value
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        // template<typename DataStream>
        // friend inline DataStream &operator>>( DataStream &ds, wasm::microseconds_t &t ) {
        //     int64_t raw = 0;
        //     ds.read((char *) &raw, sizeof(int64_t));
        //     t.value = raw;
        //     return ds;
        // }
    private:
        friend class time_point_t;
  };

  inline microseconds_t seconds( int64_t s ) { return microseconds_t( s * 1000000 ); }
  inline microseconds_t milliseconds( int64_t s ) { return microseconds_t( s * 1000 ); }
  inline microseconds_t minutes(int64_t m) { return seconds(60*m); }
  inline microseconds_t hours(int64_t h) { return minutes(60*h); }
  inline microseconds_t days(int64_t d) { return hours(24*d); }

  /**
   *  High resolution time point in microseconds_t
   *
   *  @ingroup time
   */
  class time_point {
    public:
        explicit time_point( microseconds_t e = microseconds_t() ) :elapsed(e){}
        const microseconds_t& time_since_epoch()const { return elapsed; }
        uint32_t            sec_since_epoch()const  { return uint32_t(elapsed.count() / 1000000); }
        static time_point from_iso_string( const std::string& s );
        std::string to_iso_string() const;

        /// @cond INTERNAL
        bool   operator > ( const time_point& t )const                              { return elapsed._count > t.elapsed._count; }
        bool   operator >=( const time_point& t )const                              { return elapsed._count >=t.elapsed._count; }
        bool   operator < ( const time_point& t )const                              { return elapsed._count < t.elapsed._count; }
        bool   operator <=( const time_point& t )const                              { return elapsed._count <=t.elapsed._count; }
        bool   operator ==( const time_point& t )const                              { return elapsed._count ==t.elapsed._count; }
        bool   operator !=( const time_point& t )const                              { return elapsed._count !=t.elapsed._count; }
        time_point&  operator += ( const microseconds_t& m)                           { elapsed+=m; return *this;                 }
        time_point&  operator -= ( const microseconds_t& m)                           { elapsed-=m; return *this;                 }
        time_point   operator + (const microseconds_t& m) const { return time_point(elapsed+m); }
        time_point   operator + (const time_point& m) const { return time_point(elapsed+m.elapsed); }
        time_point   operator - (const microseconds_t& m) const { return time_point(elapsed-m); }
        microseconds_t operator - (const time_point& m) const { return microseconds_t(elapsed.count() - m.elapsed.count()); }
        microseconds_t elapsed;
        /// @endcond

        WASM_REFLECT( time_point, (elapsed) )

          /**  Serialize a symbol_code into a stream
        *
        *  @brief Serialize a symbol_code
        *  @param ds - The stream to write
        *  @param sym - The value to serialize
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        // template<typename DataStream>
        // friend inline DataStream &operator<<( DataStream &ds, const wasm::time_point &t ) {
        //     // uint32_t raw = t.utc_seconds;
        //     // ds.write((const char *) &raw, sizeof(uint32_t));
        //     ds << t.elapsed
        //     return ds;
        // }

        /**
        *  Deserialize a symbol_code from a stream
        *
        *  @brief Deserialize a symbol_code
        *  @param ds - The stream to read
        *  @param symbol - The destination for deserialized value
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        // template<typename DataStream>
        // friend inline DataStream &operator>>( DataStream &ds, wasm::time_point &t ) {
        //     // uint32_t raw = 0;
        //     // ds.read((char *) &raw, sizeof(uint32_t));
        //     // t.utc_seconds = raw;
        //     ds >> t.elapsed;
        //     return ds;
        // }

  };

  /**
   *  A lower resolution time_point accurate only to seconds from 1970
   *
   *  @ingroup time
   */
  class time_point_sec {
    public:
        time_point_sec()
        :utc_seconds(0){}

        explicit time_point_sec(uint64_t seconds )
        :utc_seconds(seconds){}

        // explicit time_point_sec(uint32_t seconds )
        // :utc_seconds(seconds){}

        time_point_sec( const time_point& t )
        :utc_seconds( uint32_t(t.time_since_epoch().count() / 1000000ll) ){}

        static time_point_sec maximum() { return time_point_sec(0xffffffff); }
        static time_point_sec min() { return time_point_sec(0); }

        operator time_point()const { return time_point( wasm::seconds( utc_seconds) ); }
        uint32_t sec_since_epoch()const { return utc_seconds; }

        static time_point_sec from_iso_string( const std::string& s );
        std::string to_iso_string() const;


        // time_point_sec time_point_sec::from_iso_string( const std::string& s )
        // { 
        //     static boost::posix_time::ptime epoch = boost::posix_time::from_time_t( 0 );
        //     boost::posix_time::ptime pt;
        //     if( s.size() >= 5 && s.at( 4 ) == '-' ) // http://en.wikipedia.org/wiki/ISO_8601
        //         pt = boost::date_time::parse_delimited_time<boost::posix_time::ptime>( s, 'T' );
        //     else
        //         pt = boost::posix_time::from_iso_string( s );
        //     return wasm::time_point_sec( (pt - epoch).total_seconds() );
        // } 

        // std::string to_iso_string(){
        //    const auto ptime = boost::posix_time::from_time_t( time_t( sec_since_epoch() ) );
        //    return boost::posix_time::to_iso_extended_string( ptime );
        // }

        // void print(){
        //   str::string str = to_string();
        //   print(str);
        // }

        /// @cond INTERNAL
        time_point_sec operator = ( const wasm::time_point& t )
        {
          utc_seconds = uint32_t(t.time_since_epoch().count() / 1000000ll);
          return *this;
        }
        friend bool      operator < ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds < b.utc_seconds; }
        friend bool      operator > ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds > b.utc_seconds; }
        friend bool      operator <= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds <= b.utc_seconds; }
        friend bool      operator >= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds >= b.utc_seconds; }
        friend bool      operator == ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds == b.utc_seconds; }
        friend bool      operator != ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds != b.utc_seconds; }
        time_point_sec&  operator += ( uint32_t m ) { utc_seconds+=m; return *this; }
        time_point_sec&  operator += ( microseconds_t m ) { utc_seconds+=m.to_seconds(); return *this; }
        time_point_sec&  operator += ( time_point_sec m ) { utc_seconds+=m.utc_seconds; return *this; }
        time_point_sec&  operator -= ( uint32_t m ) { utc_seconds-=m; return *this; }
        time_point_sec&  operator -= ( microseconds_t m ) { utc_seconds-=m.to_seconds(); return *this; }
        time_point_sec&  operator -= ( time_point_sec m ) { utc_seconds-=m.utc_seconds; return *this; }
        time_point_sec   operator +( uint32_t offset )const { return time_point_sec(utc_seconds + offset); }
        time_point_sec   operator -( uint32_t offset )const { return time_point_sec(utc_seconds - offset); }

        friend time_point   operator + ( const time_point_sec& t, const microseconds_t& m )   { return time_point(t) + m;             }
        friend time_point   operator - ( const time_point_sec& t, const microseconds_t& m )   { return time_point(t) - m;             }
        friend microseconds_t operator - ( const time_point_sec& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }
        friend microseconds_t operator - ( const time_point& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }
        uint32_t utc_seconds;

        /// @endcond

        WASM_REFLECT( time_point_sec, (utc_seconds) )

        /**  Serialize a symbol_code into a stream
        *
        *  @brief Serialize a symbol_code
        *  @param ds - The stream to write
        *  @param sym - The value to serialize
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        // template<typename DataStream>
        // friend inline DataStream &operator<<( DataStream &ds, const wasm::time_point_sec &t ) {
        //     uint32_t raw = t.utc_seconds;
        //     ds.write((const char *) &raw, sizeof(uint32_t));
        //     return ds;
        // }

        /**
        *  Deserialize a symbol_code from a stream
        *
        *  @brief Deserialize a symbol_code
        *  @param ds - The stream to read
        *  @param symbol - The destination for deserialized value
        *  @tparam DataStream - Type of datastream buffer
        *  @return DataStream& - Reference to the datastream
        */
        // template<typename DataStream>
        // friend inline DataStream &operator>>( DataStream &ds, wasm::time_point_sec &t ) {
        //     uint32_t raw = 0;
        //     ds.read((char *) &raw, sizeof(uint32_t));
        //     t.utc_seconds = raw;
        //     return ds;
        // }
  };


    // inline std::string to_string(const wasm::microseconds_t& v){
    //   return v.to_iso_string();
    // }

    // inline std::string to_string(const wasm::time_point& v){
    //     return v.to_iso_string();
    // }

    // inline std::string to_string(const wasm::time_point_sec& v){
    //   return v.to_iso_string();

    // }



} // namespace wasm
