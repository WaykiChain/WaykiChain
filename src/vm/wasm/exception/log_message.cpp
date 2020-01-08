#include"wasm/exception/log_message.hpp"


namespace wasm_chain {
    using namespace std;


	string log_level::to_string()const 
	{
		switch( value )
		{
			case log_level::all:
			return "all";
			case log_level::debug:
			return "debug";
			case log_level::info:
			return "info";
			case log_level::warn:
			return "warn";
			case log_level::error:
			return "error";
			case log_level::off:
			return "off";
		}
		return "unknown";
	}

   log_context::~log_context(){}
   log_context::log_context() {}
   log_context::log_context( log_level level, const char* file, uint64_t line, 
                                              const char* method )  
   {
	   _level  = level;
	   _file   = file;
	   _line   = line;
	   _method = method;
   } 

   string log_context::to_string()const
   {
   	  std::ostringstream o;
   	  o << _file << ":" << _line << ":" << _method;
      return o.str();
   }

   void log_context::append_context( const string& s )
   {
        if (!_context.empty())
          _context += " -> ";
        _context += s;
   }

   string     log_context::get_file()const        { return _file;    }
   uint64_t   log_context::get_line_number()const { return _line;    }
   string     log_context::get_method()const      { return _method;  }
   log_level  log_context::get_log_level()const   { return _level;   }
   string     log_context::get_context()const     { return _context; }
   //time_point  log_context::get_timestamp()const  { return my->timestamp; }

   log_message::~log_message(){}
   log_message::log_message() {}
   log_message::log_message( log_context ctx, string msg )
   {
	     _context = ctx;
	     _message = msg;
   }

   log_context log_message::get_context()const { return _context; }
   string      log_message::get_message()const { return _message; }


} //chain
