#pragma once


namespace wasm { namespace rpc{



    // if (fHelp || params.size() < 4 || params.size() > 7) {
    //     throw runtime_error(
    //             "setcodewasmcontracttx \"sender\" \"contract\" \"wasm_file\" \"abi_file\" [\"memo\"] [symbol:fee:unit]\n"
    //             "\ncreate a transaction of registering a contract app\n"
    //             "\nArguments:\n"
    //             "1.\"sender\": (string required) contract owner address from this wallet\n"
    //             "2.\"contract\": (string required), contract name\n"
    //             "3.\"wasm_file\": (string required), the file path of the contract code\n"
    //             "4.\"abi_file\": (string required), the file path of the contract abi\n"
    //             "5.\"symbol:fee:unit\": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi\n"
    //             "6.\"memo\": (string optional) the memo of contract\n"
    //             "\nResult:\n"
    //             "\"txhash\": (string)\n"
    //             "\nExamples:\n"
    //             + HelpExampleCli("setcodewasmcontracttx",
    //                              "\"10-3\" \"20-3\" \"/tmp/myapp.wasm\" \"/tmp/myapp.bai\"") +
    //             "\nAs json rpc call\n"
    //             + HelpExampleRpc("setcodewasmcontracttx",
    //                              "\"10-3\" \"20-3\" \"/tmp/myapp.wasm\" \"/tmp/myapp.bai\""));
    // }
    const char *submit_wasm_contract_deploy_tx_rpc_help_message = R"=====(
        submitwasmcontractdeploytx "sender" "contract" "wasm_file" "abi_file" ["memo"] [symbol:fee:unit]
        deploy code and abi to an account as contract
        Arguments:
        1."sender":          (string required) contract owner address from this wallet
        2."contract":        (string required), contract name
        3."wasm_file":       (string required), the file path of the contract wasm code
        4."abi_file":        (string required), the file path of the contract abi
        5."symbol:fee:unit": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi
        6."memo":            (string optional) the memo of contract
        Result:
        "txhash":            (string)
        Examples:
        > ./coind submitwasmcontractdeploytx "walker222222" "walker222222" "/tmp/myapp.wasm" "/tmp/myapp.abi"
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"submitwasmcontractdeploytx", "params":["walker222222", "walker222222", "/tmp/myapp.wasm", "/tmp/myapp.bai"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    // if (fHelp || params.size() < 5 || params.size() > 6) {
    //     throw runtime_error(
    //             "callwasmcontracttx \"sender addr\" \"contract\" \"action\" \"data\" \"amount\" \"fee\" \n"
    //             "1.\"sender \": (string, required) tx sender's base58 addr\n"
    //             "2.\"contract\":   (string, required) contract name\n"
    //             "3.\"action\":   (string, required) action name\n"
    //             "4.\"data\":   (json string, required) action data\n"
    //             // "5.\"amount\":      (numeric, required) amount of WICC to be sent to the contract account\n"
    //             "5.\"fee\":         (numeric, required) pay to miner\n"
    //             "\nResult:\n"
    //             "\"txid\":        (string)\n"
    //             "\nExamples:\n" +
    //             HelpExampleCli("callcontracttx",
    //                            "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 100") +
    //             "\nAs json rpc call\n" +
    //             HelpExampleRpc("callcontracttx",
    //                            "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 100"));
    // }
    const char *submit_wasm_contract_call_tx_rpc_help_message = R"=====(
        submitwasmcontractcalltx "sender" "contract" "action" "data" "fee"
        1."sender ":  (string, required) sender name
        2."contract": (string, required) contract name
        3."action":   (string, required) action name
        4."data":     (json string, required) action data
        5."fee":      (numeric, required) pay to miner
        Result:
        "txid":       (string)
        Examples: 
        > ./coind submitwasmcontractcalltx "xiaoyu111111" "walker222222" "transfer" '["xiaoyu111111", "walker222222", "100000000 WICC","transfer to walker222222"]'
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"setcodewasmcontracttx", "params":["xiaoyu111111", "walker222222", "transfer", '["xiaoyu111111", "walker222222", "100000000 WICC","transfer to walker222222"]']}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    // if (fHelp || params.size() < 2 || params.size() > 4) {
    //     throw runtime_error(
    //             "gettablewasmcontracttx \"contract\" \"table\" \"numbers\" \"begin_key\" \n"
    //             "1.\"contract\": (string, required) contract name\n"
    //             "2.\"table\":   (string, required) table name\n"
    //             "3.\"numbers\":   (numberic, optional) numbers\n"
    //             "4.\"begin_key\":   (string, optional) smallest key in Hex\n"
    //             "\nResult:\n"
    //             "\"rows\":        (string)\n"
    //             "\"more\":        (bool)\n"
    //             "\nExamples:\n" +
    //             HelpExampleCli("gettablewasmcontracttx",
    //                            " \"411994-1\" \"stat\" 10") +
    //             "\nAs json rpc call\n" +
    //             HelpExampleRpc("gettablewasmcontracttx",
    //                            "\"411994-1\", \"stat\", 10"));
    //     // 1.contract(id)
    //     // 2.table
    //     // 3.number
    //     // 4.begin_key
    // }
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
        > ./coind gettablewasm "walker222222" "accounts" 
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"gettablewasm", "params":["walker222222", "accounts"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    // if (fHelp || params.size() < 2 || params.size() > 4) {
    //     throw runtime_error(
    //             "gettablewasmcontracttx \"contract\" \"action\" \"data\" \n"
    //             "1.\"contract\": (string, required) contract name\n"
    //             "2.\"action\":   (string, required) action name\n"
    //             "3.\"data\":   (json string, required) action data\n"
    //             "\nResult:\n"
    //             "\"data\":        (string)\n"
    //             "\nExamples:\n" +
    //             HelpExampleCli("gettablewasmcontracttx",
    //                            " \"411994-1\" \"transfer\" \'[\"walk\",\"mark\",\"1000.0000 EOS\",\"transfer to mark\"]\' ") +
    //             "\nAs json rpc call\n" +
    //             HelpExampleRpc("gettablewasmcontracttx",
    //                            "\"411994-1\", \"transfer\", \'[\"walk\",\"mark\",\"1000.0000 EOS\",\"transfer to mark\"]\' "));
    //     // 1.contract(id)
    //     // 2.action
    //     // 3.data
    // }
    const char *json_to_bin_wasm_rpc_help_message = R"=====(
        jsontobinwasm "contract" "action" "data"
        1."contract": (string, required) contract name
        2."action"  : (string, required) action name
        3."data".   : (json string, required) action data in json
        Result:
        "data":       (string in hex)
        Examples: 
        > ./coind jsontobinwasm "walker222222" "transfer" '["xiaoyu111111", "walker222222", "100000000 WICC","transfer to walker222222"]'
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"jsontobinwasm", "params":["walker222222","transfer",'["xiaoyu111111","walker222222", "100000000 WICC", "transfer to walker222222"]']}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    // if (fHelp || params.size() < 2 || params.size() > 4) {
    //     throw runtime_error(
    //             "gettablewasmcontracttx \"contract\" \"action\" \"data\" \n"
    //             "1.\"contract\": (string, required) contract name\n"
    //             "2.\"action\":   (string, required) action name\n"
    //             "3.\"data\":   (binary hex string, required) action data\n"
    //             "\nResult:\n"
    //             "\"data\":        (string)\n"
    //             "\nExamples:\n" +
    //             HelpExampleCli("gettablewasmcontracttx",
    //                            " \"411994-1\" \"transfer\"  \"000000809a438deb000000000000af91809698000000000004454f5300000000107472616e7366657220746f206d61726b\" ") +
    //             "\nAs json rpc call\n" +
    //             HelpExampleRpc("gettablewasmcontracttx",
    //                            "\"411994-1\", \"transfer\", \"000000809a438deb000000000000af91809698000000000004454f5300000000107472616e7366657220746f206d61726b\" "));
    //     // 1.contract(id)
    //     // 2.action
    //     // 3.data
    // }
    const char *bin_to_json_wasm_rpc_help_message = R"=====(
        bintojsonwasm "contract" "action" "data"
        1."contract": (string, required) contract name
        2."action"  : (string, required) action name
        3."data"    : (binary hex string, required) action data in hex
        Result:
        "data":       (string in json)
        Examples: 
        > ./coind bintojsonwasm "walker222222" "transfer" "000000809a438deb000000000000af91809698000000000004454f5300000000107472616e7366657220746f206d61726b"
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"bintojsonwasm", "params":["walker222222","transfer", "000000809a438deb000000000000af91809698000000000004454f5300000000107472616e7366657220746f206d61726b"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    // if (fHelp || params.size() != 1 ) {
    //     throw runtime_error(
    //             "getcodewasmcontracttx \"contract\" \n"
    //             "1.\"contract\": (string, required) contract name\n"
    //             "\nResult:\n"
    //             "\"code\":        (string)\n"
    //             "\nExamples:\n" +
    //             HelpExampleCli("getcodewasmcontracttx",
    //                            " \"411994-1\" ") +
    //             "\nAs json rpc call\n" +
    //             HelpExampleRpc("getcodewasmcontracttx",
    //                            "\"411994-1\""));
    //     // 1.contract(id)
    // }
    const char *get_code_wasm_rpc_help_message = R"=====(
        getcodewasm "contract" 
        1."contract": (string, required) contract name
        Result:
        "code":        (string in hex)
        Examples:
        > ./coind getcodewasm "walker222222" 
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"getcodewasm", "params":["walker222222"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";

    // if (fHelp || params.size() != 1 ) {
    //     throw runtime_error(
    //             "getcodewasmcontracttx \"contract\" \n"
    //             "1.\"contract\": (string, required) contract name\n"
    //             "\nResult:\n"
    //             "\"code\":        (string)\n"
    //             "\nExamples:\n" +
    //             HelpExampleCli("getcodewasmcontracttx",
    //                            " \"411994-1\" ") +
    //             "\nAs json rpc call\n" +
    //             HelpExampleRpc("getcodewasmcontracttx",
    //                            "\"411994-1\""));
    //     // 1.contract(id)
    // }
    const char *get_abi_wasm_rpc_help_message = R"=====(
        getabiwasm "contract" 
        1."contract": (string, required) contract name
        Result:
        "code":        (string)
        Examples:
        > ./coind getabiwasm "walker222222" 
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"getabiwasm", "params":["walker222222"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    )=====";


} // rpc
} // wasm
