// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "wasm/wasm_context.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/abi_def.hpp"

using namespace std;
using namespace wasm;

namespace dex {

////////////////////////////////////////////////////////////////////////////////
////// dex operator
    const static uint64_t dex_operator     = N(dexoperator);

    inline wasm::abi_def get_operator_abi() {

        wasm::abi_def abi;

        if( abi.version.size() == 0 ) {
            abi.version = "wasm::abi/1.0";
        }

        abi.structs = {
            //  name         base           fields
            //------------  --------  -------------------------
            {
                "register",  "",   {    //field_name    type
                                        {"registrant",  "name"},
                                        {"fee",         "asset"},
                                        {"owner",       "name"},
                                        {"matcher",     "name"},
                                        {"name",        "string"},
                                        {"portal_url",  "string"},
                                        {"memo",        "string"}
                                    }
            },
            {
                "update",  "",      {    //field_name    type
                                        {"registrant",  "name"},
                                        {"fee",         "asset"},
                                        {"id",          "uint32"},
                                        {"owner",       "name?"},
                                        {"name",        "string?"},
                                        {"matcher",     "name?"},
                                        {"portal_url",  "string?"},
                                        {"memo",        "string?"}
                                    }
            }
        };

        abi.actions = {
            //  name         type        ricardian_contract
            //------------  -----------  ------------------
            {"register",    "register",  ""},
            {"update",      "update",  ""}
        };
        return abi;

    }

    void dex_operator_register( wasm_context & );
    void dex_operator_update( wasm_context & );


////////////////////////////////////////////////////////////////////////////////
////// dex order

//       field_name       type_name     type                        description
//       ----------    ------------ -------------  -----------------------------------
#define WASM_STRUCT_ORDERS(DEFINE, SEPARATOR, SEPARATOR_END) \
    DEFINE( from,         "name",      uint64_t)    SEPARATOR()    /* from name */ \
    DEFINE( from_nonce,   "uint64",    uint64_t)    SEPARATOR()     /* from nonce  */ \
    DEFINE( exid,         "uint32",    uint32_t)    SEPARATOR()     /* exid  */ \
    DEFINE( fee_rate,     "uint64",    uint64_t)    SEPARATOR()     /* fee rate  */ \
    DEFINE( order_type,   "uint8",     uint8_t)     SEPARATOR()     /* order type  */ \
    DEFINE( order_side,   "uint8",     uint8_t)     SEPARATOR()     /* order side  */ \
    DEFINE( coin,        "asset",      wasm::asset) SEPARATOR()     /* coins  */ \
    DEFINE( asset,       "asset",      wasm::asset) SEPARATOR()    /* assets  */ \
    DEFINE( price,        "uint64",    uint64_t)    SEPARATOR()    /* price  */ \
    DEFINE( memo,         "string",    string)      SEPARATOR_END()    /* memo  */ \

    #define DEFINE_STRUCT_FIELD_ENUM(field_name, type_name, type) __enum_##field_name
    #define DEFINE_STRUCT_TYPE(field_name, type_name, type) type
    #define DEFINE_STRUCT_ABI_FIELD(field_name, type_name, type) { #field_name, type_name }
    #define SEPARATOR_END_EMPTY()
    #define SEPARATOR_COMMA() ,
    #define DEFINE_STRUCT_FIELD_FUNC(field_name, type_name, type) \
    type & field_name() { \
        return std::get<__field_enum::__enum_##field_name>(_data); \
    }

    class order_t {
    public:
        enum __field_enum {
            WASM_STRUCT_ORDERS(DEFINE_STRUCT_FIELD_ENUM, SEPARATOR_COMMA, SEPARATOR_END_EMPTY)
        };

        typedef std::tuple<
            WASM_STRUCT_ORDERS(DEFINE_STRUCT_TYPE, SEPARATOR_COMMA, SEPARATOR_END_EMPTY)
        > pack_type;

        WASM_STRUCT_ORDERS(DEFINE_STRUCT_FIELD_FUNC, SEPARATOR_END_EMPTY, SEPARATOR_END_EMPTY)

        static vector <field_def> get_abi_fields() {
            return {
                //DEFINE_STRUCT_ABI_FIELDS(aaa, "abc", int)
                WASM_STRUCT_ORDERS(DEFINE_STRUCT_ABI_FIELD, SEPARATOR_COMMA, SEPARATOR_END_EMPTY)
            };
        };

        void unpack_from_bin(const std::vector<char> &bin_data) {
            wasm::unpack<pack_type>(bin_data, _data);
        }

        std::vector<char> pack_to_bin() {
            return wasm::pack<pack_type>(_data);
        }
    private:
        pack_type _data;
    };


    const static uint64_t dex_order     = N(dex.order);

    inline wasm::abi_def get_order_abi() {

        wasm::abi_def abi;

        if( abi.version.size() == 0 ) {
            abi.version = "wasm::abi/1.0";
        }

        abi.structs = { struct_def("create",  "", order_t::get_abi_fields())};
            //  name         base           fields
            //------------  --------  -------------------------
            // {
            //     "order",  "",           order_t::get_abi_fields()
            // }
        // };

        abi.actions = {
            //  name         type        ricardian_contract
            //------------  -----------  ------------------
            {"create",   "create",    ""}
        };
        return abi;
    }

    void dex_order_create( wasm_context & );
};