#pragma once
#include <string>
#include "commons/tinyformat.h"

namespace wasm_chain {
    using namespace std;

    class log_level
    {
        public:
         /**
          * @brief Define's the various log levels for reporting.  
          *
          * Each log level includes all higher levels such that 
          * Debug includes Error, but Error does not include Debug.
          */
         enum values
         {
             all, 
             debug, 
             info, 
             warn, 
             error, 
             off  
         };
         log_level( values v = off ):value(v){}
         explicit log_level( int v ):value( static_cast<values>(v)){}
         operator int()const { return value; }
         string to_string()const;
         values value;
    };

   // void to_variant( log_level e, wasm::variant& v );
   // void from_variant( const wasm::variant& e, log_level& ll );

    class log_context 
    {
        public:
            log_context();
            log_context(log_level   level,
                        const char* file, 
                        uint64_t    line, 
                        const char* method );
            ~log_context();

            string       get_file()const;
            uint64_t     get_line_number()const;
            string       get_method()const;
            log_level    get_log_level()const;
            string       get_context()const;
            void         append_context( const string& c );
            string       to_string()const;

        public:
            log_level  _level;
            string     _file;
            uint64_t   _line;
            string     _method; 
            string     _context;
            //time_point timestamp;   
    };
    // void to_variant( const log_context& l, wasm::variant& v );
    // void from_variant( const wasm::variant& l, log_context& c );

    class log_message
    {
        public:
            log_message();
            log_message( log_context ctx, string msg );
            ~log_message();
                            
            string      get_message()const;
            log_context get_context()const;
                            
        public:
            log_context     _context;
            string          _message;
    };

    // void to_variant( const log_message& l, wasm::variant& v );
    // void from_variant( const wwasm::variant& l, log_message& c );

    typedef std::vector<log_message> log_messages;

} //wasm_chain


#define CHAIN_LOG_CONTEXT(LOG_LEVEL) \
   wasm_chain::log_context( wasm_chain::log_level::LOG_LEVEL, __FILE__, __LINE__, __FUNCTION__ )

#define CHAIN_LOG_MESSAGE( LOG_LEVEL, ... ) \
   wasm_chain::log_message( CHAIN_LOG_CONTEXT(LOG_LEVEL), tfm::format( __VA_ARGS__ ) )
