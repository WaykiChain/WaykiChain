#pragma once


namespace wasm { namespace rpc{

    const char *submit_wasm_contract_deploy_tx_rpc_help_message = R"=====(
        submitwasmcontractdeploytx "sender" "contract" "wasm_file" "abi_file" [symbol:fee:unit]
        deploy code and abi to an account as contract
        Arguments:
        1."sender":          (string required) contract owner address from this wallet
        2."contract":        (string required), contract name
        3."wasm_file":       (string required), the file path of the contract wasm code
        4."abi_file":        (string required), the file path of the contract abi
        5."symbol:fee:unit": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi
        Result:
        "txhash":            (string)
        Examples:
        > ./coind submitwasmcontractdeploytx tokenbank999 tokenbank999 /tmp/token.wasm /tmp/token.abi
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"submitwasmcontractdeploytx", "params":["tokenbank999", "tokenbank999", "/tmp/token.wasm", "/tmp/token.bai"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *submit_wasm_contract_call_tx_rpc_help_message = R"=====(
        submitwasmcontractcalltx "sender" "contract" "action" "data" "fee"
        1."sender ":         (string, required) sender name
        2."contract":        (string, required) contract name
        3."action":          (string, required) action name
        4."data":            (json string, required) action data
        5."symbol:fee:unit": (numeric, optional) pay to miner
        Result:
        "txid":       (string)
        Examples: 
        > ./coind submitwasmcontractcalltx alice1234555 tokenbank999 transfer '["alice1234555", "bob123455555", "100.00000000 WICC","transfer to bob"]'
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"submitwasmcontractcalltx", "params":["alice1234555", "tokenbank999", "transfer", '["alice1234555", "bob123455555", "100000000 WICC","transfer to bob"]']}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_table_wasm_rpc_help_message = R"=====(
        gettablewasm "contract" "table" "numbers" "begin_key"
        1."contract": (string, required) contract name"
        2."table":    (string, required) table name"
        3."numbers":  (numberic, optional) numbers"
        4."begin_key":(string, optional) smallest key in Hex"
        Result:"
        "rows":       (string)"
        "more":       (bool)"
        nExamples: 
        > ./coind gettablewasm tokenbank999 accounts 
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"gettablewasm", "params":["tokenbank999", "accounts"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *json_to_bin_wasm_rpc_help_message = R"=====(
        jsontobinwasm "contract" "action" "data"
        1."contract": (string, required) contract name
        2."action"  : (string, required) action name
        3."data"    : (json string, required) action data in json
        Result:
        "data":       (string in hex)
        Examples: 
        > ./coind jsontobinwasm tokenbank999 transfer '["alice1234555", "bob123455555", "100.00000000 WICC","transfer to bob"]'
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"jsontobinwasm", "params":["tokenbank999","transfer",'["alice1234555","bob123455555", "100.00000000 WICC", "transfer to bob"]']}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *bin_to_json_wasm_rpc_help_message = R"=====(
        bintojsonwasm "contract" "action" "data"
        1."contract": (string, required) contract name
        2."action"  : (string, required) action name
        3."data"    : (binary hex string, required) action data in hex
        Result:
        "data":       (string in json)
        Examples: 
        > ./coind bintojsonwasm tokenbank999 transfer 504a214304855c34504a29850c110e3d00e40b540200000008574943430000000f7472616e7366657220746f20626f62
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"bintojsonwasm", "params":["tokenbank999","transfer", "504a214304855c34504a29850c110e3d00e40b540200000008574943430000000f7472616e7366657220746f20626f62"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_code_wasm_rpc_help_message = R"=====(
        getcodewasm "contract" 
        1."contract": (string, required) contract name
        Result:
        "code":        (string in hex)
        Examples:
        > ./coind getcodewasm tokenbank999
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"getcodewasm", "params":["tokenbank999"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_abi_wasm_rpc_help_message = R"=====(
        getabiwasm "contract" 
        1."contract": (string, required) contract name
        Result:
        "code":        (string)
        Examples:
        > ./coind getabiwasm tokenbank999"
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"getabiwasm", "params":["tokenbank999"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_tx_trace_rpc_help_message = R"=====(
        gettxtrace "trxid" 
        1."txid": (string, required)  The hash of transaction
        Result an object of the transaction detail
        "nResult":
        Examples:
        > ./coind gettxtrace 68feb6a4097a45d6e56f5b84f6c381b0c638a1306eb95b7ee2354e19838461e4
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"gettxtrace", "params":"68feb6a4097a45d6e56f5b84f6c381b0c638a1306eb95b7ee2354e19838461e4"}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *abi_def_json_to_bin_wasm_rpc_help_message = R"=====(
        abijsontobinwasm "abijson" 
        1."abijson": (string, required) abi json file from cdt
        Result:
        "data":       (string in hex)
        Examples: 
        > ./coind abijsontobinwasm '{"____comment": "This file was generated with wasm-abigen. DO NOT EDIT ",...}' 
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"abijsontobinwasm", "params":{"____comment": "This file was generated with wasm-abigen. DO NOT EDIT ",...}}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

} // rpc
} // wasm
