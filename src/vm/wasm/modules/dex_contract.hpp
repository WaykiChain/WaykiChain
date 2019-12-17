// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "wasm/wasm_context.hpp"
#include "wasm/types/asset.hpp"
#include "wasm/abi_def.hpp"
#include "native_contract_base.hpp"

using namespace std;
using namespace wasm;

namespace dex {

////////////////////////////////////////////////////////////////////////////////
////// dex operator

//       field_name       type_name     type             separator          description
//       ----------    ------------ -------------     ---------------    -------------------
#define DEX_OPERATOR_REGISTER(DEFINE, SEPARATOR, SEPARATOR_END) \
    DEFINE( registrant, "name",     uint64_t)           SEPARATOR()       /* from name */ \
    DEFINE( fee,        "asset",    wasm::asset)        SEPARATOR()       /* from nonce  */ \
    DEFINE( owner,      "name",     uint64_t)           SEPARATOR()       /* owner  */ \
    DEFINE( matcher,    "name",     uint64_t)           SEPARATOR()       /* matcher  */ \
    DEFINE( name,       "string",   string)             SEPARATOR()       /* name  */ \
    DEFINE( portal_url, "string",   string)             SEPARATOR()       /* portal url  */ \
    DEFINE( memo,       "string",   string)             SEPARATOR_END()   /* memo  */

#define DEX_OPERATOR_UPDATE(DEFINE, SEPARATOR, SEPARATOR_END) \
    DEFINE( registrant, "name",     uint64_t)           SEPARATOR()       /* from name */ \
    DEFINE( fee,        "asset",    wasm::asset)        SEPARATOR()       /* from nonce  */ \
    DEFINE( id,         "uint32",   uint32_t)           SEPARATOR()       /* exid  */ \
    DEFINE( owner,      "name?",    optional<uint64_t>) SEPARATOR()       /* owner  */ \
    DEFINE( matcher,    "name?",    optional<uint64_t>) SEPARATOR()       /* matcher  */ \
    DEFINE( name,       "string?",  optional<string>)   SEPARATOR()       /* name */ \
    DEFINE( portal_url, "string?",  optional<string>)   SEPARATOR()       /* portal url */ \
    DEFINE( memo,       "string?",  optional<string>)   SEPARATOR_END()   /* memo  */

    const static uint64_t dex_operator     = N(dexoperator);

    DEFINE_WASM_NATIVE_STRUCT(operator_register_t, DEX_OPERATOR_REGISTER)
    DEFINE_WASM_NATIVE_STRUCT(operator_update_t, DEX_OPERATOR_UPDATE)

    inline wasm::abi_def get_operator_abi() {

        wasm::abi_def abi;

        if( abi.version.size() == 0 ) {
            abi.version = "wasm::abi/1.0";
        }


        abi.structs = {
            //  name         base           fields
            //------------  --------  -------------------------
            {
                "register",  "",        operator_register_t::get_abi_fields()
            },
            {
                "update",  "",          operator_update_t::get_abi_fields()
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

//       field_name       type_name     type             separator          description
//       ----------    ------------ -------------     ---------------    -------------------
#define DEX_ORDER_CREATE(DEFINE, SEPARATOR, SEPARATOR_END) \
    DEFINE( from,           "name",      uint64_t)    SEPARATOR()       /* from name */ \
    DEFINE( exid,           "uint32",    uint32_t)    SEPARATOR()       /* exid  */ \
    DEFINE( fee_rate,       "uint64",    uint64_t)    SEPARATOR()       /* fee rate  */ \
    DEFINE( order_type,     "uint8",     uint8_t)     SEPARATOR()       /* order type  */ \
    DEFINE( order_side,     "uint8",     uint8_t)     SEPARATOR()       /* order side  */ \
    DEFINE( coin,           "asset",     wasm::asset) SEPARATOR()       /* coins  */ \
    DEFINE( asset,          "asset",     wasm::asset) SEPARATOR()       /* assets  */ \
    DEFINE( price,          "uint64",    uint64_t)    SEPARATOR()       /* price  */ \
    DEFINE( memo,           "string",    string)      SEPARATOR_END()   /* memo  */ \

//       field_name       type_name     type             separator          description
//       ----------    ------------ -------------     ---------------    -------------------
#define DEX_ORDER_CANCEL(DEFINE, SEPARATOR, SEPARATOR_END) \
    DEFINE( from,         "name",        uint64_t)          SEPARATOR()       /* from name */ \
    DEFINE( exid,         "uint32",      uint32_t)          SEPARATOR()       /* exid  */ \
    DEFINE( order_id,     "checksum256", wasm::checksum256_type) SEPARATOR()       /* exid  */ \
    DEFINE( memo,         "string",      string)            SEPARATOR_END()   /* memo  */

    DEFINE_WASM_NATIVE_STRUCT(order_t, DEX_ORDER_CREATE)
    DEFINE_WASM_NATIVE_STRUCT(order_cancel_t, DEX_ORDER_CANCEL)

    const static uint64_t dex_order     = N(dex.order);

    inline wasm::abi_def get_order_abi() {

        wasm::abi_def abi;

        if( abi.version.size() == 0 ) {
            abi.version = "wasm::abi/1.0";
        }

        abi.structs = {
            struct_def("create",  "", order_t::get_abi_fields()),
            struct_def("cancel",  "", order_cancel_t::get_abi_fields())
        };

        abi.actions = {
            {"create",   "create",    ""},
            {"cancel",   "cancel",    ""}
        };
        return abi;
    }

    void dex_order_create( wasm_context & );
    void dex_order_cancel( wasm_context & );
};