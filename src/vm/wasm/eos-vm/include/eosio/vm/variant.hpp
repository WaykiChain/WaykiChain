#pragma once

// temporarily use exceptions
#include <eosio/vm/exceptions.hpp>
#include <eosio/vm/outcome.hpp>
#include <eosio/vm/utils.hpp>

#include <algorithm>
#include <iostream>
#include <tuple>
#include <type_traits>
#include <variant>
#include <functional>

namespace eosio { namespace vm {

   // forward declaration
   template <typename... Alternatives>
   class variant;

   // implementation details
   namespace detail {

      template <typename... Ts>
      constexpr std::size_t max_layout_size_v = std::max({sizeof(Ts)...});

      template <typename... Ts>
      constexpr std::size_t max_alignof_v = std::max({alignof(Ts)...});

      template <typename T, typename... Alternatives>
      constexpr bool is_valid_alternative_v = (... + (std::is_same_v<T, Alternatives>?1:0)) != 0;

      template <typename T, typename Alternative, typename... Alternatives>
      constexpr std::size_t get_alternatives_index_v =
               std::is_same_v<T, Alternative> ? 0 : get_alternatives_index_v<T, Alternatives...> + 1;

      template <typename T, typename Alternative>
      constexpr std::size_t get_alternatives_index_v<T, Alternative> = 0;

      template <std::size_t I, typename... Alternatives>
      using get_alternative_t = std::tuple_element_t<I, std::tuple<Alternatives...>>;

      template <bool Valid, typename Ret>
      struct dispatcher;

      template <typename Ret>
      struct dispatcher<false, Ret> {
         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _case(Vis&&, Var&&) {
            __builtin_unreachable();
         }
         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _switch(Vis&&, Var&&) {
            __builtin_unreachable();
         }
      };

      template <typename Ret>
      struct dispatcher<true, Ret> {
         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _case(Vis&& vis, Var&& var) {
            return std::invoke(std::forward<Vis>(vis), std::forward<Var>(var).template get<I>());
         }

         template <std::size_t I, typename Vis, typename Var>
         static constexpr Ret _switch(Vis&& vis, Var&& var) {
            constexpr std::size_t sz = std::decay_t<Var>::variant_size();
            switch (var.index()) {
               case I + 0: {
                  return dispatcher<I + 0 < sz, Ret>::template _case<I + 0>(std::forward<Vis>(vis),
                                                                            std::forward<Var>(var));
               }
               case I + 1: {
                  return dispatcher<I + 1 < sz, Ret>::template _case<I + 1>(std::forward<Vis>(vis),
                                                                            std::forward<Var>(var));
               }
               case I + 2: {
                  return dispatcher<I + 2 < sz, Ret>::template _case<I + 2>(std::forward<Vis>(vis),
                                                                            std::forward<Var>(var));
               }
               case I + 3: {
                  return dispatcher<I + 3 < sz, Ret>::template _case<I + 3>(std::forward<Vis>(vis),
                                                                            std::forward<Var>(var));
               }
               default: {
                  return dispatcher<I + 4 < sz, Ret>::template _switch<I + 4>(std::forward<Vis>(vis),
                                                                              std::forward<Var>(var));
               }
            }
         }
      };

#define V_ELEM(N)                                                       \
      T##N _t##N;                                                       \
      constexpr variant_storage(T##N& arg) : _t##N(arg) {}              \
      constexpr variant_storage(T##N&& arg) : _t##N(std::move(arg)) {}  \
      constexpr variant_storage(const T##N& arg) : _t##N(arg) {}        \
      constexpr variant_storage(const T##N&& arg) : _t##N(std::move(arg)) {}

#define V0 variant_storage() = default;
#define V1 V0 V_ELEM(0)
#define V2 V1 V_ELEM(1)
#define V3 V2 V_ELEM(2)
#define V4 V3 V_ELEM(3)

      template<typename... T>
      union variant_storage;
      template<typename T0, typename T1, typename T2, typename T3, typename... T>
      union variant_storage<T0, T1, T2, T3, T...> {
         V4
         template<typename A>
         constexpr variant_storage(A&& arg) : _tail{static_cast<A&&>(arg)} {}
         variant_storage<T...> _tail;
      };
      template<typename T0>
      union variant_storage<T0> {
         V1
      };
      template<typename T0, typename T1>
      union variant_storage<T0, T1> {
         V2
      };
      template<typename T0, typename T1, typename T2>
      union variant_storage<T0, T1, T2> {
         V3
      };
      template<typename T0, typename T1, typename T2, typename T3>
      union variant_storage<T0, T1, T2, T3> {
         V4
      };

#undef V4
#undef V3
#undef V2
#undef V1
#undef V0
#undef V_ELEM

      template<int I, typename Storage>
      constexpr decltype(auto) variant_storage_get(Storage&& val) {
         if constexpr (I == 0) {
            return (static_cast<Storage&&>(val)._t0);
         } else if constexpr (I == 1) {
            return (static_cast<Storage&&>(val)._t1);
         } else if constexpr (I == 2) {
            return (static_cast<Storage&&>(val)._t2);
         } else if constexpr (I == 3) {
            return (static_cast<Storage&&>(val)._t3);
         } else {
            return detail::variant_storage_get<I - 4>(static_cast<Storage&&>(val)._tail);
         }
      }
   } // namespace detail

   template <class Visitor, typename Variant>
   constexpr auto visit(Visitor&& vis, Variant&& var) {
      using Ret = decltype(std::invoke(std::forward<Visitor>(vis), var.template get<0>()));
      return detail::dispatcher<true, Ret>::template _switch<0>(std::forward<Visitor>(vis), std::forward<Variant>(var));
   }

   template <typename... Alternatives>
   class variant {
      static_assert(sizeof...(Alternatives) <= std::numeric_limits<uint8_t>::max()+1,
                    "eosio::vm::variant can only accept 256 alternatives");
      static_assert((... && (std::is_trivially_copy_constructible_v<Alternatives> && std::is_trivially_move_constructible_v<Alternatives> &&
                    std::is_trivially_copy_assignable_v<Alternatives> && std::is_trivially_move_assignable_v<Alternatives> &&
                    std::is_trivially_destructible_v<Alternatives>)), "Variant requires trivial types");

    public:
      variant() = default;
      variant(const variant& other) = default;
      variant(variant&& other) = default;

      variant& operator=(const variant& other) = default;
      variant& operator=(variant&& other) = default;

      template <typename T, typename = std::enable_if_t<detail::is_valid_alternative_v<std::decay_t<T>, Alternatives...>>>
      constexpr variant(T&& alt) :
         _which(detail::get_alternatives_index_v<std::decay_t<T>, Alternatives...>),
         _storage(static_cast<T&&>(alt)) {
      }

      template <typename T, typename = std::enable_if_t<detail::is_valid_alternative_v<std::decay_t<T>, Alternatives...>>>
      constexpr variant& operator=(T&& alt) {
         _storage = static_cast<T&&>(alt);
         _which = detail::get_alternatives_index_v<std::decay_t<T>, Alternatives...>;
         return *this;
      }

      static inline constexpr size_t variant_size() { return sizeof...(Alternatives); }
      inline constexpr uint16_t      index() const { return _which; }

      template <size_t Index>
      inline constexpr auto&& get_check() {
         // TODO add outcome stuff
         return 3;
      }

      template <size_t Index>
      inline constexpr const auto& get() const & {
         return detail::variant_storage_get<Index>(_storage);
      }

      template <typename Alt>
      inline constexpr const Alt& get() const & {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(_storage);
      }

      template <size_t Index>
      inline constexpr const auto&& get() const && {
         return detail::variant_storage_get<Index>(std::move(_storage));
      }

      template <typename Alt>
      inline constexpr const Alt&& get() const && {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(std::move(_storage));
      }

      template <size_t Index>
      inline constexpr auto&& get() && {
         return detail::variant_storage_get<Index>(std::move(_storage));
      }

      template <typename Alt>
      inline constexpr Alt&& get() && {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(std::move(_storage));
      }

      template <size_t Index>
      inline constexpr auto& get() & {
         return detail::variant_storage_get<Index>(_storage);
      }

      template <typename Alt>
      inline constexpr Alt& get() & {
         return detail::variant_storage_get<detail::get_alternatives_index_v<Alt, Alternatives...>>(_storage);
      }

      template <typename Alt>
      inline constexpr bool is_a() {
         return _which == detail::get_alternatives_index_v<Alt, Alternatives...>;
      }
      inline constexpr void toggle_exiting_which() { _which ^= 0x100; }
      inline constexpr void clear_exiting_which() { _which &= 0xFF; }
      inline constexpr void set_exiting_which() { _which |= 0x100; }

    private:
      static constexpr size_t _sizeof  = detail::max_layout_size_v<Alternatives...>;
      static constexpr size_t _alignof = detail::max_alignof_v<Alternatives...>;
      uint16_t _which                  = 0;
      detail::variant_storage<Alternatives...> _storage;
   };

}} // namespace eosio::vm
