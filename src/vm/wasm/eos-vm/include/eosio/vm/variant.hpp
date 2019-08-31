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

namespace eosio { namespace vm {

   // // forward declaration
   // template <typename... Alternatives>
   // class variant;

   // // implementation details
   // namespace detail {

   //    template <typename T, typename... Ts>
   //    struct max_layout_size {
   //       static constexpr auto value = std::max(sizeof(T), max_layout_size<Ts...>::value);
   //    };

   //    template <typename T>
   //    struct max_layout_size<T> {
   //       static constexpr auto value = sizeof(T);
   //    };

   //    template <typename T, typename... Ts>
   //    struct max_alignof {
   //       static constexpr auto value = std::max(alignof(T), max_layout_size<Ts...>::value);
   //    };

   //    template <typename T>
   //    struct max_alignof<T> {
   //       static constexpr auto value = alignof(T);
   //    };

   //    template <typename T, typename Alternative, typename... Alternatives>
   //    struct is_valid_alternative {
   //       static constexpr auto value =
   //             std::is_same<T, Alternative>::value ? true : is_valid_alternative<T, Alternatives...>::value;
   //    };

   //    template <typename T, typename Alternative>
   //    struct is_valid_alternative<T, Alternative> {
   //       static constexpr auto value = std::is_same<T, Alternative>::value;
   //    };

   //    template <size_t N, typename T, typename Alternative, typename... Alternatives>
   //    struct get_alternatives_index {
   //       static constexpr auto value =
   //             std::is_same<T, Alternative>::value ? N : get_alternatives_index<N + 1, T, Alternatives...>::value;
   //    };

   //    template <size_t N, typename T, typename Alternative>
   //    struct get_alternatives_index<N, T, Alternative> {
   //       static constexpr auto value = N;
   //    };

   //    template <typename Alternative, typename... Alternatives>
   //    struct get_first_alternative {
   //       using type = Alternative;
   //    };

   //    template <size_t I, size_t N, typename... Alternatives>
   //    struct get_alternative {
   //       using type = typename get_alternative<I + 1, N, Alternatives...>::type;
   //    };

   //    template <size_t I, typename... Alternatives>
   //    struct get_alternative<I, I, Alternatives...> {
   //       using type = typename get_first_alternative<Alternatives...>::type;
   //    };

   //    template <typename Ret, size_t I, size_t Sz, typename Alt, typename... Alts>
   //    constexpr  Ret type_at(uint16_t index) {
   //       if constexpr (I < Sz-1) {
   //          if (I == index)
   //             return Alt{};
   //          else
   //             return type_at<I+1, Sz, Alts...>(index);
   //       } else {
   //          throw wasm_invalid_element("type_at failure");
   //       }
   //    }

   //    template <bool Valid, typename Ret>
   //    struct dispatcher;

   //    template <typename Ret>
   //    struct dispatcher<false, Ret> {
   //       template <std::size_t I, typename Vis, typename Var>
   //       static constexpr Ret _case(Vis&&, Var&&) {
   //          __builtin_unreachable();
   //       }
   //       template <std::size_t I, typename Vis, typename Var>
   //       static constexpr Ret _switch(Vis&&, Var&&) {
   //          __builtin_unreachable();
   //       }
   //    };

   //    template <typename Ret>
   //    struct dispatcher<true, Ret> {
   //       template <std::size_t I, typename Vis, typename Var>
   //       static constexpr Ret _case(Vis&& vis, Var&& var) {
   //          return std::invoke(std::forward<Vis>(vis), var.template get<I>());
   //       }

   //       template <std::size_t I, typename Vis, typename Var>
   //       static constexpr Ret _switch(Vis&& vis, Var&& var) {
   //          constexpr std::size_t sz = std::decay_t<Var>::variant_size();
   //          switch (var.index()) {
   //             case I + 0: {
   //                return dispatcher<I + 0 < sz, Ret>::template _case<I + 0>(std::forward<Vis>(vis),
   //                                                                          std::forward<Var>(var));
   //             }
   //             case I + 1: {
   //                return dispatcher<I + 1 < sz, Ret>::template _case<I + 1>(std::forward<Vis>(vis),
   //                                                                          std::forward<Var>(var));
   //             }
   //             case I + 2: {
   //                return dispatcher<I + 2 < sz, Ret>::template _case<I + 2>(std::forward<Vis>(vis),
   //                                                                          std::forward<Var>(var));
   //             }
   //             case I + 3: {
   //                return dispatcher<I + 3 < sz, Ret>::template _case<I + 3>(std::forward<Vis>(vis),
   //                                                                          std::forward<Var>(var));
   //             }
   //             default: {
   //                return dispatcher<I + 4 < sz, Ret>::template _case<I + 4>(std::forward<Vis>(vis),
   //                                                                          std::forward<Var>(var));
   //             }
   //          }
   //       }
   //    };
   // } // namespace detail

   // template <class Visitor, typename Variant>
   // constexpr auto visit(Visitor&& vis, Variant&& var) {
   //    using Ret = decltype(std::invoke(std::forward<Visitor>(vis), var.template get<0>()));
   //    return detail::dispatcher<true, Ret>::template _switch<0>(std::forward<Visitor>(vis), std::forward<Variant>(var));
   // }

   // template <typename... Alternatives>
   // class variant {
   //    static_assert(sizeof...(Alternatives) < std::numeric_limits<uint8_t>::max(),
   //                  "eosio::vm::variant can only accept 255 alternatives");

   //  public:
   //    template <typename T>
   //    inline constexpr variant(T&& alt) {
   //       static_assert(detail::is_valid_alternative<std::decay_t<T>, Alternatives...>::value,
   //                     "type not a valid alternative");
   //       new (&_storage) std::decay_t<T>(std::forward<T>(alt));
   //       _which = detail::get_alternatives_index<0, T, Alternatives...>::value;
   //    }

   //    static inline constexpr size_t variant_size() { return sizeof...(Alternatives); }
   //    inline constexpr uint16_t      index() const { return _which; }

   //    template <size_t Index>
   //    inline constexpr auto&& get_check() {
   //       // TODO add outcome stuff
   //       return 3;
   //    }

   //    template <size_t Index>
   //    inline constexpr const auto& get() const {
   //       return reinterpret_cast<const typename std::tuple_element<Index, alternatives_tuple>::type&>(_storage);
   //    }

   //    template <typename Alt>
   //    inline constexpr const auto& get() const {
   //       return reinterpret_cast<const Alt&>(_storage);
   //    }

   //    template <size_t Index>
   //    inline constexpr auto&& get() && {
   //       return reinterpret_cast<typename std::tuple_element<Index, alternatives_tuple>::type&>(_storage);
   //    }

   //    template <typename Alt>
   //    inline constexpr auto&& get() && {
   //       return reinterpret_cast<Alt&>(_storage);
   //    }

   //    template <size_t Index>
   //    inline constexpr auto& get() {
   //       return reinterpret_cast<typename std::tuple_element<Index, alternatives_tuple>::type&>(_storage);
   //    }

   //    template <typename Alt>
   //    inline constexpr auto& get() {
   //       return reinterpret_cast<Alt&>(_storage);
   //    }

   //    template <typename Alt>
   //    inline constexpr bool is_a() {
   //       return _which == detail::get_alternatives_index<0, Alt, Alternatives...>::value;
   //    }
   //    inline constexpr void set_which(uint8_t which) { _which = which; }

   //    static constexpr size_t size() { return std::tuple_size_v<alternatives_tuple>; }

   //  private:
   //    using alternatives_tuple         = std::tuple<Alternatives...>;
   //    static constexpr size_t _sizeof  = detail::max_layout_size<Alternatives...>::value;
   //    static constexpr size_t _alignof = detail::max_alignof<Alternatives...>::value;
   //    uint16_t _which                  = 0;
   //    char     _storage[_sizeof];
   //    // std::aligned_storage<_sizeof, _alignof> _storage;
   // };

}} // namespace eosio::vm
