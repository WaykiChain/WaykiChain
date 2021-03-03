#pragma once
#include <stdint.h>
#include <string>
#include <optional>
#include <map>

namespace wasm {

  class microseconds_t {
    public:
        constexpr explicit microseconds_t( int64_t c = 0) :_count(c){}
        static constexpr microseconds_t maximum() { return microseconds_t(0x7fffffffffffffffll); }
        friend constexpr microseconds_t operator + (const  microseconds_t& l, const microseconds_t& r ) { return microseconds_t(l._count+r._count); }
        friend constexpr microseconds_t operator - (const  microseconds_t& l, const microseconds_t& r ) { return microseconds_t(l._count-r._count); }

        constexpr bool operator==(const microseconds_t& c)const { return _count == c._count; }
        constexpr bool operator!=(const microseconds_t& c)const { return _count != c._count; }
        friend constexpr bool operator>(const microseconds_t& a, const microseconds_t& b){ return a._count > b._count; }
        friend constexpr bool operator>=(const microseconds_t& a, const microseconds_t& b){ return a._count >= b._count; }
        constexpr friend bool operator<(const microseconds_t& a, const microseconds_t& b){ return a._count < b._count; }
        constexpr friend bool operator<=(const microseconds_t& a, const microseconds_t& b){ return a._count <= b._count; }
        constexpr microseconds_t& operator+=(const microseconds_t& c) { _count += c._count; return *this; }
        constexpr microseconds_t& operator-=(const microseconds_t& c) { _count -= c._count; return *this; }
        constexpr int64_t count()const { return _count; }
        constexpr int64_t to_seconds()const { return _count/1000000; }
    private:
        friend class time_point;
        int64_t      _count;
  };
  inline constexpr microseconds_t seconds( int64_t s ) { return microseconds_t( s * 1000000 ); }
  inline constexpr microseconds_t milliseconds( int64_t s ) { return microseconds_t( s * 1000 ); }
  inline constexpr microseconds_t minutes(int64_t m) { return seconds(60*m); }
  inline constexpr microseconds_t hours(int64_t h) { return minutes(60*h); }
  inline constexpr microseconds_t days(int64_t d) { return hours(24*d); }

  class time_point {
    public:
        constexpr explicit time_point( microseconds_t e = microseconds_t() ) :elapsed(e){}
        static time_point now();
        static constexpr time_point maximum() { return time_point( microseconds_t::maximum() ); }
        static constexpr time_point min() { return time_point();                      }

        std::string to_iso_string()const;
        static time_point from_iso_string( const std::string& s );

        constexpr const microseconds_t& time_since_epoch()const { return elapsed; }
        constexpr uint32_t            sec_since_epoch()const  { return elapsed.count() / 1000000; }
        constexpr bool   operator > ( const time_point& t )const                              { return elapsed._count > t.elapsed._count; }
        constexpr bool   operator >=( const time_point& t )const                              { return elapsed._count >=t.elapsed._count; }
        constexpr bool   operator < ( const time_point& t )const                              { return elapsed._count < t.elapsed._count; }
        constexpr bool   operator <=( const time_point& t )const                              { return elapsed._count <=t.elapsed._count; }
        constexpr bool   operator ==( const time_point& t )const                              { return elapsed._count ==t.elapsed._count; }
        constexpr bool   operator !=( const time_point& t )const                              { return elapsed._count !=t.elapsed._count; }
        constexpr time_point&  operator += ( const microseconds_t& m)                           { elapsed+=m; return *this;                 }
        constexpr time_point&  operator -= ( const microseconds_t& m)                           { elapsed-=m; return *this;                 }
        constexpr time_point   operator + (const microseconds_t& m) const { return time_point(elapsed+m); }
        constexpr time_point   operator - (const microseconds_t& m) const { return time_point(elapsed-m); }
       constexpr microseconds_t operator - (const time_point& m) const { return microseconds_t(elapsed.count() - m.elapsed.count()); }
    private:
        microseconds_t elapsed;
  };

  /**
   *  A lower resolution time_point accurate only to seconds from 1970
   */
  class time_point_sec
  {
    public:
        constexpr time_point_sec()
        :utc_seconds(0){}

        constexpr explicit time_point_sec(uint32_t seconds )
        :utc_seconds(seconds){}

        constexpr time_point_sec( const time_point& t )
        :utc_seconds( t.time_since_epoch().count() / 1000000ll ){}

        static constexpr time_point_sec maximum() { return time_point_sec(0xffffffff); }
        static constexpr time_point_sec min() { return time_point_sec(0); }

        constexpr operator time_point()const { return time_point( wasm::seconds( utc_seconds) ); }
        constexpr uint32_t sec_since_epoch()const { return utc_seconds; }

        constexpr time_point_sec operator = ( const wasm::time_point& t )
        {
          utc_seconds = t.time_since_epoch().count() / 1000000ll;
          return *this;
        }
        constexpr friend bool      operator < ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds < b.utc_seconds; }
        constexpr friend bool      operator > ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds > b.utc_seconds; }
        constexpr friend bool      operator <= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds <= b.utc_seconds; }
        constexpr friend bool      operator >= ( const time_point_sec& a, const time_point_sec& b )  { return a.utc_seconds >= b.utc_seconds; }
        constexpr friend bool      operator == ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds == b.utc_seconds; }
        constexpr friend bool      operator != ( const time_point_sec& a, const time_point_sec& b ) { return a.utc_seconds != b.utc_seconds; }
        constexpr time_point_sec&  operator += ( uint32_t m ) { utc_seconds+=m; return *this; }
        constexpr time_point_sec&  operator += ( microseconds_t m ) { utc_seconds+=m.to_seconds(); return *this; }
        constexpr time_point_sec&  operator -= ( uint32_t m ) { utc_seconds-=m; return *this; }
        constexpr time_point_sec&  operator -= ( microseconds_t m ) { utc_seconds-=m.to_seconds(); return *this; }
        constexpr time_point_sec   operator +( uint32_t offset )const { return time_point_sec(utc_seconds + offset); }
        constexpr time_point_sec   operator -( uint32_t offset )const { return time_point_sec(utc_seconds - offset); }

        friend constexpr time_point   operator + ( const time_point_sec& t, const microseconds_t& m )   { return time_point(t) + m;             }
        friend constexpr time_point   operator - ( const time_point_sec& t, const microseconds_t& m )   { return time_point(t) - m;             }
        friend constexpr microseconds_t operator - ( const time_point_sec& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }
        friend constexpr microseconds_t operator - ( const time_point& t, const time_point_sec& m ) { return time_point(t) - time_point(m); }

        std::string to_non_delimited_iso_string()const;
        std::string to_iso_string()const;

        operator std::string()const;
        static time_point_sec from_iso_string( const std::string& s );

    private:
        uint32_t utc_seconds;
  };

  typedef std::optional<time_point> otime_point;

}
