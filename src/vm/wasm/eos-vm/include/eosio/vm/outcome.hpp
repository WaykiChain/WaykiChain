#pragma once

#include <eosio/vm/error_codes.hpp>

/// wrapper header to use either the built in outcome library or an external Boost v1.70 outcome library
#ifdef EOS_VM_USE_EXTERNAL_OUTCOME
#   include <outcome.hpp>
namespace outcome = OUTCOME_V2_NAMESPACE;
#else
#   include <boost/outcome.hpp>
namespace outcome = BOOST_OUTCOME_V2_NAMESPACE;
#endif

// tag struct to represent a void function for outcome::result
struct result_void {};
