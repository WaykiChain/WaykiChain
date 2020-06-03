#pragma once


namespace wasm { namespace rpc{

    const char *submit_setcode_tx_rpc_help_message = R"=====(
        submitsetcodetx "sender" "wasm_file" "abi_file" ["contract"] [symbol:fee:unit]
        deploy code and abi to an account as contract
        Arguments:
        1."sender":          (string, required), contract regid address from this wallet
        2."code_file":       (string, required), the file path of the contract code
        3."abi_file":        (string, required), the file path of the contract abi
        4."contract":        (string, optional), existing contract regid, omitted for new deployment.
        5."symbol:fee:unit": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi
        Result:
        "txhash":            (string)
        Examples:
        > ./coind submitsetcodetx 0-2 /tmp/token.wasm /tmp/token.abi
        > ./coind submitsetcodetx 0-2 /tmp/token.wasm /tmp/token.abi 0-3
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"submitsetcodetx", "params":["0-2", "/tmp/token.wasm", "/tmp/token.abi"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *submit_setcoder_tx_rpc_help_message = R"=====(
        submitsetcodertx "sender" "contract" "maintainer" [symbol:fee:unit]
        set a new maintainer to the contract
        Arguments:
        1."sender":          (string, required) the original maintainer (regid) of the contract
        2."contract":        (string, required) contract regid
        3."maintainer":      (string, required) the new maintainer regid (when set as 0-0, it is to disable further upgrade)
        4."symbol:fee:unit": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi
        Result:
        "txhash":            (string)
        Examples:
        > ./coind submitsetcodertx 0-2 100-3 0-3
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"submitsetcodertx", "params":["0-2", "100-3", "0-3"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *submit_tx_rpc_help_message = R"=====(
        submittx "sender" "contract" "action" "data" "fee"
        1."sender ":         (string, required) sender regid
        2."contract":        (string, required) contract regid
        3."action":          (string, required) action name
        4."data":            (json string, required) action data
        5."symbol:fee:unit": (numeric, optional) pay to miner
        Result:
        "txid":       (string)
        Examples:
        > ./coind submittx 0-2 800-2 transfer '["0-2", "0-3", "100.00000000 WICC","transfer to bob"]'
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"submittx", "params":["0-2", "800-2", "transfer", '["0-2", "0-3", "100.00000000 WICC","transfer to bob"]']}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_table_wasm_rpc_help_message = R"=====(
        wasm_gettable "contract" "table" "numbers" "begin_key"
        1."contract": (string, required) contract regid"
        2."table":    (string, required) table name"
        3."numbers":  (numberic, optional) numbers, defualt is 10"
        4."begin_key":(string, optional) smallest key in Hex, default is empty"
        Result:
        "rows":       (array of object) array of row detail object"
        "more":       (bool)"
        nExamples:
        > ./coind wasm_gettable 0-2 accounts
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_gettable", "params":["0-2", "accounts"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_row_wasm_rpc_help_message = R"=====(
        wasm_getrow "contract" "table" "key"
        1."contract": (string, required) contract regid"
        2."table":    (string, required) table name"
        3."key":      (string, required) key in Hex"
        Result:
        "row detail object"
        nExamples:
        > ./coind wasm_getrow 0-2 accounts 0000000000000001
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_getrow", "params":["0-2", "accounts", "0000000000000001"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *json_to_bin_wasm_rpc_help_message = R"=====(
        wasm_json2bin "contract" "action" "data"
        1."contract": (string, required) contract regid
        2."action"  : (string, required) action name
        3."data"    : (json string, required) action data in json
        Result:
        "data":       (string in hex)
        Examples:
        > ./coind wasm_json2bin 800-2 transfer '["0-2", "0-3", "100.00000000 WICC","transfer to bob"]'
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_json2bin", "params":["800-2","transfer",'["0-2","0-3", "100.00000000 WICC", "transfer to bob"]']}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *bin_to_json_wasm_rpc_help_message = R"=====(
        wasm_bin2json "contract" "action" "data"
        1."contract": (string, required) contract regid
        2."action"  : (string, required) action name
        3."data"    : (binary hex string, required) action data in hex
        Result:
        "data":       (string in json)
        Examples:
        > ./coind wasm_bin2json 800-2 transfer 504a214304855c34504a29850c110e3d00e40b540200000008574943430000000f7472616e7366657220746f20626f62
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_bin2json", "params":["800-2","transfer", "504a214304855c34504a29850c110e3d00e40b540200000008574943430000000f7472616e7366657220746f20626f62"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_code_wasm_rpc_help_message = R"=====(
        wasm_getcode "contract"
        1."contract": (string, required) contract regid
        Result:
        "code":        (string in hex)
        Examples:
        > ./coind wasm_getcode 0-2
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_getcode", "params":["0-2"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_abi_wasm_rpc_help_message = R"=====(
        wasm_getabi "contract"
        1."contract": (string, required) contract regid
        Result:
        "code":        (string)
        Examples:
        > ./coind wasm_getabi 0-2"
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_getabi", "params":["0-2"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *get_tx_trace_rpc_help_message = R"=====(
        wasm_gettxtrace "trxid"
        1."txid": (string, required)  The hash of transaction
        Result an object of the transaction detail
        "nResult":
        Examples:
        > ./coind wasm_gettxtrace 68feb6a4097a45d6e56f5b84f6c381b0c638a1306eb95b7ee2354e19838461e4
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_gettxtrace", "params":"68feb6a4097a45d6e56f5b84f6c381b0c638a1306eb95b7ee2354e19838461e4"}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    const char *abi_def_json_to_bin_wasm_rpc_help_message = R"=====(
        wasm_abidefjson2bin "abijson"
        1."abijson": (string, required) abi json file from cdt
        Result:
        "data":       (string in hex)
        Examples:
        > ./coind wasm_abidefjson2bin '{"____comment": "This file was generated with wasm-abigen. DO NOT EDIT ",...}'
        As json rpc call
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"wasm_abidefjson2bin", "params":{"____comment": "This file was generated with wasm-abigen. DO NOT EDIT ",...}}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

} // rpc
} // wasm
