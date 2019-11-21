#pragma once

#define EOS_VM_OPEN_NAMESPACE namespace eosio::vm {
#define EOS_VM_CLOSE_NAMESPACE }

#include "error_codes_pp.hpp"

// create a category for parser error codes
GENERATE_ERROR_CATEGORY(parser_errors, "eos-vm parser errors")
GENERATE_ERROR_CATEGORY(memory_errors, "eos-vm memory errors")
GENERATE_ERROR_CATEGORY(system_errors, "eos-vm system errors")
GENERATE_ERROR_CATEGORY(auxiliary_errors, "eos-vm auxiliary errors")

// clang-format off
#define PARSER_ERRORS(macro)                      \
   macro(parser_errors, invalid_magic_number)     \
   macro(parser_errors, invalid_version)          \
   macro(parser_errors, invalid_section_id)       \
   macro(parser_errors, general_parsing_failure)

#define MEMORY_ERRORS(macro)        \
  macro(memory_errors, bad_alloc)   \
  macro(memory_errors, double_free)

#define SYSTEM_ERRORS(macro)                  \
  macro(system_errors, constructor_failure)   \
  macro(system_errors, unimplemented_failure)

#define AUXILIARY_ERRORS(macro)      \
  macro(auxiliary_errors, checktime)

// clang-format on

CREATE_ERROR_CODES(parser_errors, PARSER_ERRORS)
CREATE_ERROR_CODES(memory_errors, MEMORY_ERRORS)
CREATE_ERROR_CODES(system_errors, SYSTEM_ERRORS)
CREATE_ERROR_CODES(auxiliary_errors, AUXILIARY_ERRORS)

#define _EOS_VM_ASSERT(expr, err_type)                                                                                  \
   if (!UNLIKELY(expr)) {                                                                                              \
      return err_type;                                                                                                 \
   }

// assert but invalidate the enclosing class
#define EOS_VM_ASSERT_INVALIDATE(expr, err_type)                                                                       \
   if (_valid = false; !UNLIKELY(expr)) {                                                                              \
      return err_type;                                                                                                 \
   }
