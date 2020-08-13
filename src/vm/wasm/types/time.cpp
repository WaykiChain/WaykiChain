#include <time.h>
#include "wasm/types/time.hpp"
#include "wasm/wasm_serialize_reflect.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>

namespace wasm {
 

    time_point time_point::from_iso_string( const std::string& s ) 
    { 
        auto dot = s.find( '.' );
        if( dot == std::string::npos )
           return time_point( time_point_sec::from_iso_string( s ) );
        else {
           auto ms = s.substr( dot );
           ms[0] = '1';
           while( ms.size() < 4 ) ms.push_back('0');
           return time_point( time_point_sec::from_iso_string( s ) ) + milliseconds( std::strtoll(ms.c_str(), nullptr, 10) - 1000 );
        }
    } 

    std::string time_point::to_iso_string() const{
        auto count = elapsed.count();
        if (count >= 0) {
           uint64_t secs = (uint64_t)count / 1000000ULL;
           uint64_t msec = ((uint64_t)count % 1000000ULL) / 1000ULL;
           string padded_ms = to_string((uint64_t)(msec + 1000ULL)).substr(1);

           return time_point_sec(secs).to_iso_string() + "." + padded_ms;
         }

         return std::string();
    }

    time_point_sec time_point_sec::from_iso_string( const std::string& s )
    { 

        int year, month, day, hour, minute, second;
        sscanf((char *) s.data(), "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second);
        std::tm time = {};
        time.tm_year = year - 1900;
        time.tm_mon  = month - 1;
        time.tm_mday = day;
        time.tm_hour = hour;
        time.tm_min  = minute;
        time.tm_sec  = second;
        time.tm_isdst  = 0;
        time_t seconds = mktime(&time);
 
        return time_point_sec(seconds);

    } 

    std::string time_point_sec::to_iso_string() const {

       std::time_t t = time_t(sec_since_epoch() );

       char utc[128];
       std::tm* p = localtime(&t);
       sprintf(utc, "%4d-%02d-%02d %02d:%02d:%02d", 
               p->tm_year + 1900, 
               p->tm_mon + 1, 
               p->tm_mday, 
               p->tm_hour, 
               p->tm_min,
               p->tm_sec);

       return string(utc);

    }

    // inline std::string to_string(const wasm::microseconds& v){
    //   return v.to_iso_string();
    // }

    // inline std::string to_string(const wasm::time_point& v){
    //     return v.to_iso_string();
    // }

    // inline std::string to_string(const wasm::time_point_sec& v){
    //   return v.to_iso_string();

    // }



} // namespace wasm
