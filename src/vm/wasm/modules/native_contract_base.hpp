// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "wasm/wasm_context.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/abi_def.hpp"

using namespace std;

#define DEFINE_STRUCT_FIELD_ENUM(field_name, type_name, type) __enum_##field_name
#define DEFINE_STRUCT_TYPE(field_name, type_name, type) type
#define DEFINE_STRUCT_ABI_FIELD(field_name, type_name, type) \
    { #field_name, type_name }
#define SEPARATOR_END_EMPTY()
#define SEPARATOR_COMMA() ,
#define DEFINE_STRUCT_FIELD_FUNC(field_name, type_name, type) \
type & field_name() { \
    return std::get<__field_enum::__enum_##field_name>(_data); \
}

#define DEFINE_WASM_NATIVE_STRUCT(struct_name, WASM_STRUCT_ORDERS)                             \
    class struct_name {                                                                        \
    public:                                                                                    \
        enum __field_enum {                                                                    \
            WASM_STRUCT_ORDERS(DEFINE_STRUCT_FIELD_ENUM, SEPARATOR_COMMA, SEPARATOR_END_EMPTY) \
        };                                                                                     \
                                                                                               \
        typedef std::tuple<WASM_STRUCT_ORDERS(DEFINE_STRUCT_TYPE, SEPARATOR_COMMA,             \
                                              SEPARATOR_END_EMPTY)>                            \
            pack_type;                                                                         \
                                                                                               \
        WASM_STRUCT_ORDERS(DEFINE_STRUCT_FIELD_FUNC, SEPARATOR_END_EMPTY, SEPARATOR_END_EMPTY) \
                                                                                               \
        static vector<field_def> get_abi_fields() {                                            \
            return {WASM_STRUCT_ORDERS(DEFINE_STRUCT_ABI_FIELD, SEPARATOR_COMMA,               \
                                       SEPARATOR_END_EMPTY)};                                  \
        };                                                                                     \
                                                                                               \
        void unpack_from_bin(const std::vector<char> &bin_data) {                              \
            wasm::unpack<pack_type>(bin_data, _data);                                          \
        }                                                                                      \
                                                                                               \
        std::vector<char> pack_to_bin() { return wasm::pack<pack_type>(_data); }               \
                                                                                               \
    private:                                                                                   \
        pack_type _data;                                                                       \
    };
