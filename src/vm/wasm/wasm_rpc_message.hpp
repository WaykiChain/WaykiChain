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

    const char *set_code_wasm_contract_tx_rpc_help_message = R"=====(
    {
        setcodewasmcontracttx "sender" "contract" "wasm_file" "abi_file" ["memo"] [symbol:fee:unit]
        create a transaction of registering a contract app
        Arguments:
        1."sender":          (string required) contract owner address from this wallet
        2."contract":        (string required), contract name
        3."wasm_file":       (string required), the file path of the contract code
        4."abi_file":        (string required), the file path of the contract abi
        5."symbol:fee:unit": (string:numeric:string, optional) fee paid to miner, default is WICC:100000:sawi
        6."memo":            (string optional) the memo of contract
        Result:
        "txhash":            (string)
        Examples:
        > ./coind setcodewasmcontracttx "walker222222" "walker222222" "/tmp/myapp.wasm" "/tmp/myapp.bai"
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"setcodewasmcontracttx", "params":["walker222222", "/tmp/myapp.wasm", "/tmp/myapp.bai"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    }
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

    const char *call_wasm_contract_tx_rpc_help_message = R"=====(
    {
        callwasmcontracttx "sender addr" "contract" "action" "data" "fee"
        1."sender ":  (string, required) tx sender's base58 addr\
        2."contract": (string, required) contract name\
        3."action":   (string, required) action name
        4."data":     (json string, required) action data
        5."fee":      (numeric, required) pay to miner
        Result:
        "txid":       (string)
        Examples: 
        > ./coind setcodewasmcontracttx "wasmio" "transfer" '["xiaoyu111111", "walker222222", "100000000 WICC","transfer to walker222222"]'
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"setcodewasmcontracttx", "params":["wasmio", "transfer", '["xiaoyu111111", "walker222222", "100000000 WICC","transfer to walker222222"]']}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    }
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

    const char *get_table_wasm_contract_tx_rpc_help_message = R"=====(
    {
        gettablewasmcontracttx "contract" "table" "numbers" "begin_key"
        1."contract": (string, required) contract name"
        2."table":    (string, required) table name"
        3."numbers":  (numberic, optional) numbers"
        4."begin_key":(string, optional) smallest key in Hex"
        Result:"
        "rows":       (string)"
        "more":       (bool)"
        nExamples: 
        > ./coind setcodewasmcontracttx "wasmio" "transfer" '["xiaoyu111111", "walker222222", "100000000 WICC","transfer to walker222222"]'
        As json rpc call 
        > curl --user myusername -d '{"jsonrpc": "1.0", "id":"curltest", "method":"gettablewasmcontracttx", "params":["walker222222", "stat", "10"]}' -H 'Content-Type: application/json;' http://127.0.0.1:8332
    }
    )=====";




} // rpc
} // wasm
