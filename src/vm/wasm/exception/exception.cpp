#include"wasm/exception/exception.hpp"

namespace wasm_chain {

	#define CHAIN_CONTEXT_PRINT_LENGTH 64

	exception::exception( int64_t code,
	                      const std::string& name_value,
	                      const std::string& what_value )
	{
		_code = code;
		_what = what_value;
		_name = name_value;
	}

	exception::exception( log_message&& msg, int64_t code,
		                  const std::string& name_value,
		                  const std::string& what_value )
	{
		_code = code;
		_what = what_value;
		_name = name_value;
		_elog.push_back(std::move(msg));
	}

	exception::exception( log_messages&& msgs, int64_t code,
		                  const std::string& name_value,
		                  const std::string& what_value )
	{
		_code = code;
		_what = what_value;
		_name = name_value;
		_elog = std::move(msgs);
	}

	exception::exception( const log_messages& msgs, int64_t code,
		                  const std::string& name_value,
		                  const std::string& what_value )
	{
		_code = code;
		_what = what_value;
		_name = name_value;
		_elog = msgs;
	}

	exception::exception( const exception& c ){ *this = c;            }
	exception::exception( exception&& c )     { *this = std::move(c); }
	exception::~exception(){}

	const char*  exception::name()const throw() { return _name.c_str(); }
	const char*  exception::what()const throw() { return _what.c_str(); }
	int64_t      exception::code()const throw() { return _code;         }

	const log_messages& exception::get_log()const              { return _elog;                   }
	void                exception::append_log( log_message m ) { _elog.emplace_back( std::move(m) ); }


	string exception::to_detail_string( log_level ll  )const
	{
		std::stringstream ss;
		try {
			try {
				ss << "\n(" << _code << ")";
			} catch( std::bad_alloc& ) {
				throw;
			} catch( ... ) {
				ss << "<- exception in to_detail_string.";
			}
			//ss << " " << _name << ": " << _what << "\n";
			ss << " " << _name << "\n";
			for( auto itr = _elog.begin(); itr != _elog.end(); ) {
				try {
					//ss << itr->get_message(); //fc::format_string( itr->get_format(), itr->get_data() ) <<"\n";
					//ss << "    " << json::to_string( itr->get_data()) << "\n";
					string context_str = itr->get_context().to_string();
					std::ostringstream o;
					o << itr->get_context().to_string();
				    while (o.str().size() < CHAIN_CONTEXT_PRINT_LENGTH)
       					 o << " ";

					//ss << itr->get_context().to_string();
       			    ss << o.str();
					ss << itr->get_message();
					++itr;
				} catch( std::bad_alloc& ) {
					throw;
				} catch( ... ) {
					ss << "<- exception in to_detail_string.";
				}
				if( itr != _elog.end()) ss << "\n";
			}
		} catch( std::bad_alloc& ) {
			throw;
		} catch( ... ) {
			ss << "<- exception in to_detail_string.\n";
		}
		ss << "\n";
		return ss.str();
	}

	string exception::to_string( log_level ll )const
	{
		std::stringstream ss;
		try {
			ss << _what;
			try {
				ss << " (" << _code << ")\n";
			} catch( std::bad_alloc& ) {
				throw;
			} catch( ... ) {
				ss << "<- exception in to_string.\n";
			}
			for( auto itr = _elog.begin(); itr != _elog.end(); ++itr ) {
				try {
					ss << itr->get_message() << "\n";
					//      ss << "    " << itr->get_context().to_string() <<"\n";
				} catch( std::bad_alloc& ) {
					throw;
				} catch( ... ) {
					ss << "<- exception in to_string.\n";
				}
			}
			return ss.str();
		} catch( std::bad_alloc& ) {
			throw;
		} catch( ... ) {
			ss << "<- exception in to_string.\n";
		}
		return ss.str();
	}

	string exception::top_message()const
	{
		for( auto itr = _elog.begin(); itr != _elog.end(); ++itr )
		{
			auto s = itr->get_message();
			if (!s.empty()) {
				return s;
			}
		}
		return string();
	}

	exception& exception::operator=( const exception& copy )
	{
	    *this = copy;
	    return *this;
	}

	exception& exception::operator=( exception&& copy )
	{
	    *this = std::move(copy);
	    return *this;
	}

	unhandled_exception::unhandled_exception( log_message&& m, std::exception_ptr e )
	:exception( std::move(m) )
	{
	    _inner = e;
	}

	unhandled_exception::unhandled_exception( const exception& r )
	:exception(r)
	{
	}

	unhandled_exception::unhandled_exception( log_messages m )
	:exception()
	{ _elog = std::move(m); }

	std::exception_ptr unhandled_exception::get_inner_exception()const { return _inner; }



} //chain
