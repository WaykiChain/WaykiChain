#include <eosio/vm/error_codes.hpp>
#include <eosio/vm/outcome.hpp>

#include <catch2/catch.hpp>

#include <iostream>
#include <string>
#include <string_view>
#include <type_traits>

using namespace eosio;
using namespace eosio::vm;

TEST_CASE("Testing error_codes", "[error_codes_tests]") {
   {
      std::error_code ec = parser_errors::invalid_magic_number;
      CHECK(ec.message() == "invalid_magic_number");
      CHECK(ec.value() == static_cast<int>(parser_errors::invalid_magic_number));
      CHECK(is_a<parser_errors_category>(ec));
   }
   {
      std::error_code ec = parser_errors::invalid_version;
      CHECK(ec.message() == "invalid_version");
      CHECK(ec.value() == static_cast<int>(parser_errors::invalid_version));
      CHECK(is_a<parser_errors_category>(ec));
   }
   {
      std::error_code ec = parser_errors::general_parsing_failure;
      CHECK(ec.message() == "general_parsing_failure");
      CHECK(ec.value() == static_cast<int>(parser_errors::general_parsing_failure));
      CHECK(is_a<parser_errors_category>(ec));
   }
   {
      std::error_code ec = parser_errors::invalid_section_id;
      CHECK(ec.message() == "invalid_section_id");
      CHECK(ec.value() == static_cast<int>(parser_errors::invalid_section_id));
      CHECK(is_a<parser_errors_category>(ec));
   }

}

outcome::result<std::string> test_foo(std::string_view s) {
   if (s == "fail")
      return parser_errors::general_parsing_failure;
   else
      return "pass";
}

outcome::result<std::string> test_bar(std::string_view s) {
   OUTCOME_TRY( res, test_foo(s) );
   return res+"2";
}

TEST_CASE("Testing outcome", "[outcome_tests]") {
   auto r = test_foo("something");
   CHECK(r);
   CHECK(r.value() == "pass");

   r = test_foo("fail");
   CHECK(!r);
   CHECK(r.error().message() == "general_parsing_failure");

   r = test_bar("something");
   CHECK(r);
   CHECK(r.value() == "pass2");

   r = test_bar("fail");
   CHECK(!r);
   CHECK(r.error().message() == "general_parsing_failure");
}
