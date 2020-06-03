#pragma once

//#include "wasm/wasm_variant.hpp"
#include <boost/preprocessor/stringize.hpp>
#include "log_message.hpp"


namespace wasm_chain {

	enum exception_code
	{
		/** for exceptions we threw that don't have an assigned code */
		unspecified_exception_code        = 0,
		unhandled_exception_code          = 1, ///< for unhandled 3rd party exceptions
		timeout_exception_code            = 2, ///< timeout exceptions
		file_not_found_exception_code     = 3,
		parse_error_exception_code        = 4,
		invalid_arg_exception_code        = 5,
		key_not_found_exception_code      = 6,
		bad_cast_exception_code           = 7,
		out_of_range_exception_code       = 8,
		canceled_exception_code           = 9,
		assert_exception_code             = 10,
		eof_exception_code                = 11,
		std_exception_code                = 13,
		invalid_operation_exception_code  = 14,
		unknown_host_exception_code       = 15,
		null_optional_code                = 16,
		udt_error_code                    = 17,
		aes_error_code                    = 18,
		overflow_code                     = 19,
		underflow_code                    = 20,
		divide_by_zero_code               = 21
	};

	class exception
	{
		public:
			enum code_enum
			{
				code_value = unspecified_exception_code
			};

	        exception( int64_t code = unspecified_exception_code,
	                   const std::string& name_value = "exception",
	                   const std::string& what_value = "unspecified");
	        exception( log_message&&,
	                   int64_t code = unspecified_exception_code,
	                   const std::string& name_value = "exception",
	                   const std::string& what_value = "unspecified");
	        exception( log_messages&&,
	        	       int64_t code = unspecified_exception_code,
	                   const std::string& name_value = "exception",
	                   const std::string& what_value = "unspecified");
	        exception( const log_messages&,
	                   int64_t code = unspecified_exception_code,
	                   const std::string& name_value = "exception",
	                   const std::string& what_value = "unspecified");

			exception( const exception& e );
			exception( exception&& e );
			virtual ~exception();

			const char*          name()const throw();
			int64_t              code()const throw();
			virtual const char*  what()const throw();

			const log_messages&  get_log()const;
			void                 append_log( log_message m );

	        std::string to_detail_string( log_level ll = log_level::all   )const;
	        std::string to_string       ( log_level ll = log_level::info  )const;
	        std::string top_message     (                                 )const;

			// friend void to_variant( const exception& e, variant& v );
			// friend void from_variant( const variant& e, exception& ll );

			exception& operator=( const exception& other );
			exception& operator=( exception&& other      );
		public:
			std::string     _name;
			std::string     _what;
			int64_t         _code;
			log_messages    _elog;
	};

   class unhandled_exception : public exception
   {
      public:
       enum code_enum {
          code_value = unhandled_exception_code,
       };
       unhandled_exception( log_message&& m, std::exception_ptr e = std::current_exception() );
       unhandled_exception( log_messages      );
       unhandled_exception( const exception&  );

       std::exception_ptr get_inner_exception()const;

      private:
       std::exception_ptr _inner;
   };




#define CHAIN_DECLARE_DERIVED_EXCEPTION( TYPE, BASE, CODE, WHAT ) \
	class TYPE : public BASE  \
	{ \
		public: \
			enum code_enum { \
			  code_value = CODE, \
			}; \
			explicit TYPE( int64_t code, const std::string& name_value, const std::string& what_value ) \
			:BASE( code, name_value, what_value ){} \
			explicit TYPE( wasm_chain::log_message&& m, int64_t code, const std::string& name_value, const std::string& what_value ) \
			:BASE( std::move(m), code, name_value, what_value ){} \
			explicit TYPE( wasm_chain::log_messages&& m, int64_t code, const std::string& name_value, const std::string& what_value )\
			:BASE( std::move(m), code, name_value, what_value ){}\
			explicit TYPE( const wasm_chain::log_messages& m, int64_t code, const std::string& name_value, const std::string& what_value )\
			:BASE( m, code, name_value, what_value ){}\
			TYPE( const std::string& what_value, const wasm_chain::log_messages& m ) \
			:BASE( m, CODE, BOOST_PP_STRINGIZE(TYPE), what_value ){} \
			TYPE( wasm_chain::log_message&& m ) \
			:BASE( wasm_chain::move(m), CODE, BOOST_PP_STRINGIZE(TYPE), WHAT ){}\
			TYPE( wasm_chain::log_messages msgs ) \
			:BASE( wasm_chain::move( msgs ), CODE, BOOST_PP_STRINGIZE(TYPE), WHAT ) {} \
			TYPE( const TYPE& c ) \
			:BASE(c){} \
			TYPE( const BASE& c ) \
			:BASE(c){} \
			TYPE():BASE(CODE, BOOST_PP_STRINGIZE(TYPE), WHAT){}\
			\
	};


  #define CHAIN_DECLARE_EXCEPTION( TYPE, CODE, WHAT ) \
      CHAIN_DECLARE_DERIVED_EXCEPTION( TYPE, exception, CODE, WHAT )

  CHAIN_DECLARE_EXCEPTION( timeout_exception,        timeout_exception_code,        "Timeout"          );
  CHAIN_DECLARE_EXCEPTION( file_not_found_exception, file_not_found_exception_code, "File Not Found"   );
  CHAIN_DECLARE_EXCEPTION( parse_error_exception,    parse_error_exception_code,    "Parse Error"      );
  CHAIN_DECLARE_EXCEPTION( invalid_arg_exception,    invalid_arg_exception_code,    "Invalid Argument" );
  CHAIN_DECLARE_EXCEPTION( key_not_found_exception,  key_not_found_exception_code,  "Key Not Found"    );
  CHAIN_DECLARE_EXCEPTION( bad_cast_exception,       bad_cast_exception_code,       "Bad Cast"         );
  CHAIN_DECLARE_EXCEPTION( out_of_range_exception,   out_of_range_exception_code,   "Out of Range"     );
  CHAIN_DECLARE_EXCEPTION( invalid_operation_exception,
  	                       invalid_operation_exception_code,
                           "Invalid Operation" );
  CHAIN_DECLARE_EXCEPTION( unknown_host_exception,
                           unknown_host_exception_code,
                           "Unknown Host" );
  CHAIN_DECLARE_EXCEPTION( canceled_exception,       canceled_exception_code, "Canceled"               );
  CHAIN_DECLARE_EXCEPTION( assert_exception,         assert_exception_code,   "Assert Exception"       );
  CHAIN_DECLARE_EXCEPTION( eof_exception,            eof_exception_code,      "End Of File"            );
  CHAIN_DECLARE_EXCEPTION( null_optional,            null_optional_code,      "null optional"          );
  CHAIN_DECLARE_EXCEPTION( udt_exception,            udt_error_code,          "UDT error"              );
  CHAIN_DECLARE_EXCEPTION( aes_exception,            aes_error_code,          "AES error"              );
  CHAIN_DECLARE_EXCEPTION( overflow_exception,       overflow_code,           "Integer Overflow"       );
  CHAIN_DECLARE_EXCEPTION( underflow_exception,      underflow_code,          "Integer Underflow"      );
  CHAIN_DECLARE_EXCEPTION( divide_by_zero_exception, divide_by_zero_code,     "Integer Divide By Zero" );


} //chain

