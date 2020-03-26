#pragma once

#include <eosio/vm/config.hpp>

#include <string>

#ifdef EOS_VM_USE_BOOST
#   include <boost/system/error_code.hpp>
#   include <boost/type_traits.hpp>
using error_category_t = boost::system::error_category;
using error_code_t     = boost::system::error_code;
#   define ERROR_CODE_NAMESPACE boost::system
#   define TRUE_TYPE boost::true_type
#else
#   include <system_error>
#   include <type_traits>
using error_category_t = std::error_category;
using error_code_t     = std::error_code;
#   define ERROR_CODE_NAMESPACE std
#   define TRUE_TYPE std::true_type
#endif

#define GENERATE_ERROR_CATEGORY(CATEGORY, NAME)                                                                        \
   namespace eosio { namespace vm {                                                                                    \
         struct CATEGORY##_category : error_category_t {                                                               \
            const char* name() const noexcept override { return #NAME; }                                               \
            std::string message(int ev) const override;                                                                \
         };                                                                                                            \
         const CATEGORY##_category __##CATEGORY##_category{};                                                          \
         template <typename T>                                                                                         \
         static inline constexpr auto is_a(const error_code_t& ec)                                                     \
               -> std::enable_if_t<std::is_same_v<T, CATEGORY##_category>, bool> {                                     \
            return ec.category() == __##CATEGORY##_category;                                                           \
         }                                                                                                             \
      }                                                                                                                \
   }

#define GENERATE_ENUM_ELEM(PARENT, ITEM) ITEM,

#define GENERATE_STR_ELEM(PARENT, ITEM)                                                                                \
   case PARENT::ITEM:                                                                                                  \
      return #ITEM;

#define CREATE_ERROR_CODES(CATEGORY, ERRORS)                                                                           \
   namespace eosio { namespace vm {                                                                                    \
         enum class CATEGORY { ERRORS(GENERATE_ENUM_ELEM) };                                                           \
      }                                                                                                                \
   }                                                                                                                   \
   namespace ERROR_CODE_NAMESPACE {                                                                                    \
      template <>                                                                                                      \
      struct is_error_code_enum<eosio::vm::CATEGORY> : TRUE_TYPE {};                                                   \
   }                                                                                                                   \
   namespace eosio { namespace vm {                                                                                    \
         inline std::string CATEGORY##_category::message(int ev) const {                                               \
            switch (static_cast<CATEGORY>(ev)) { ERRORS(GENERATE_STR_ELEM) }                                           \
            return "";                                                                                                 \
         }                                                                                                             \
         inline error_code_t make_error_code(CATEGORY e) noexcept {                                                    \
            return { static_cast<int>(e), __##CATEGORY##_category };                                                   \
         }                                                                                                             \
      }                                                                                                                \
   }
