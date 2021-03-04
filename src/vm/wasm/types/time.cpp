#include <time.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <sstream>
#include <string>

namespace wasm {

  namespace bch = boost::chrono;

  time_point time_point::now()
  {
     return time_point( microseconds_t( bch::duration_cast<bch::microseconds>( bch::system_clock::now().time_since_epoch() ).count() ) );
  }

  std::string time_point_sec::to_non_delimited_iso_string()const
  {
    const auto ptime = boost::posix_time::from_time_t( time_t( sec_since_epoch() ) );
    return boost::posix_time::to_iso_string( ptime );
  }

  std::string time_point_sec::to_iso_string()const
  {
    const auto ptime = boost::posix_time::from_time_t( time_t( sec_since_epoch() ) );
    return boost::posix_time::to_iso_extended_string( ptime );
  }

  time_point_sec::operator std::string()const
  {
      return this->to_iso_string();
  }

  time_point_sec time_point_sec::from_iso_string( const std::string& s )
  {
      static boost::posix_time::ptime epoch = boost::posix_time::from_time_t( 0 );
      boost::posix_time::ptime pt;
      char sep = 'T';
      if (s.find(sep) == std::string::npos) sep = ' ';

      if( s.size() >= 5 && s.at( 4 ) == '-' ) // http://en.wikipedia.org/wiki/ISO_8601
          pt = boost::date_time::parse_delimited_time<boost::posix_time::ptime>( s, sep );
      else
          pt = boost::date_time::parse_iso_time<boost::posix_time::ptime>(s, sep);
      return wasm::time_point_sec( (pt - epoch).total_seconds() );
  }

   std::string time_point::to_iso_string()const
   {
      auto count = elapsed.count();
      if (count >= 0) {
         uint64_t secs = (uint64_t)count / 1000000ULL;
         uint64_t msec = ((uint64_t)count % 1000000ULL) / 1000ULL;
         std::string padded_ms = std::to_string((uint64_t)(msec + 1000ULL)).substr(1);
         const auto ptime = boost::posix_time::from_time_t(time_t(secs));
         return boost::posix_time::to_iso_extended_string(ptime) + "." + padded_ms;
      } else {
         // negative time_points serialized as "durations" in the ISO form with boost
         // this is not very human readable but fits the precedent set by the above
         auto as_duration = boost::posix_time::microseconds(count);
         return boost::posix_time::to_iso_string(as_duration);
      }
   }

  time_point time_point::from_iso_string( const std::string& s )
  {
      auto dot = s.find( '.' );
      if( dot == std::string::npos )
         return time_point( time_point_sec::from_iso_string( s ) );
      else {
         auto ms = s.substr( dot );
         ms[0] = '1';
         while( ms.size() < 4 ) ms.push_back('0');
         return time_point( time_point_sec::from_iso_string( s ) ) + milliseconds( std::stoll(ms) - 1000 );
      }
  }

  // inspired by show_date_relative() in git's date.c
  std::string get_approximate_relative_time_string(const time_point_sec& event_time,
                                              const time_point_sec& relative_to_time /* = wasm::time_point::now() */,
                                              const std::string& default_ago /* = " ago" */) {


    std::string ago = default_ago;
    int32_t seconds_ago = relative_to_time.sec_since_epoch() - event_time.sec_since_epoch();
    if (seconds_ago < 0)
    {
       ago = " in the future";
       seconds_ago = -seconds_ago;
    }
    std::stringstream result;
    if (seconds_ago < 90)
    {
      result << seconds_ago << " second" << (seconds_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t minutes_ago = (seconds_ago + 30) / 60;
    if (minutes_ago < 90)
    {
      result << minutes_ago << " minute" << (minutes_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t hours_ago = (minutes_ago + 30) / 60;
    if (hours_ago < 90)
    {
      result << hours_ago << " hour" << (hours_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t days_ago = (hours_ago + 12) / 24;
    if (days_ago < 90)
    {
      result << days_ago << " day" << (days_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t weeks_ago = (days_ago + 3) / 7;
    if (weeks_ago < 70)
    {
      result << weeks_ago << " week" << (weeks_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t months_ago = (days_ago + 15) / 30;
    if (months_ago < 12)
    {
      result << months_ago << " month" << (months_ago > 1 ? "s" : "") << ago;
      return result.str();
    }
    uint32_t years_ago = days_ago / 365;
    result << years_ago << " year" << (months_ago > 1 ? "s" : "");
    if (months_ago < 12 * 5)
    {
      uint32_t leftover_days = days_ago - (years_ago * 365);
      uint32_t leftover_months = (leftover_days + 15) / 30;
      if (leftover_months)
        result << leftover_months <<  " month" << (months_ago > 1 ? "s" : "");
    }
    result << ago;
    return result.str();
  }
  std::string get_approximate_relative_time_string(const time_point& event_time,
                                              const time_point& relative_to_time /* = wasm::time_point::now() */,
                                              const std::string& ago /* = " ago" */) {
    return get_approximate_relative_time_string(time_point_sec(event_time), time_point_sec(relative_to_time), ago);
  }

} //namespace wasm
