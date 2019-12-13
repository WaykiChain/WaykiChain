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

};