
#include <fstream>
#include <name.hpp>
#include <asset.hpp>
#include <boost/test/unit_test.hpp>
#include"tester.hpp"
#include <chrono>

using namespace wasm;

namespace wasm {


    string stToString( const string &value ) {
        std::vector<char> v(value.begin(), value.end());
        auto
        t = wasm::unpack<std::tuple < asset, asset, name>>
        (v);


        string ret;

        asset supply = std::get<0>(t);
        ret = "supply:";
        ret += supply.to_string();

        ret += " max_supply:";
        asset max_supply = std::get<1>(t);
        ret += max_supply.to_string();

        ret += " issuer:";
        name issuer = std::get<2>(t);
        ret += issuer.to_string();

        return ret;

    }

    string accountToString( const string &value ) {
        std::vector<char> v(value.begin(), value.end());

        asset a = wasm::unpack<asset>(v);
        return a.to_string();
    }

    string StringToHexString( string str, string separator = " " ) {

        const std::string hex = "0123456789abcdef";
        std::stringstream ss;

        for (std::string::size_type i = 0; i < str.size(); ++i)
            ss << hex[(unsigned char) str[i] >> 4] << hex[(unsigned char) str[i] & 0xf] << separator;

        return ss.str();

    }
    //template<typename T>
    string VectorToHexString( std::vector<char> str, string separator = " " ) {

        const std::string hex = "0123456789abcdef";
        std::stringstream ss;

        for (std::string::size_type i = 0; i < str.size(); ++i)
            ss << hex[(unsigned
        uint8_t)str[i] >> 4] << hex[(unsigned
        uint8_t)str[i] & 0xf] << separator;

        return ss.str();

    }

    template<typename T>
    std::string tostring( const T &t ) {
        std::ostringstream ss;
        ss << t;
        return ss.str();
    }

    void print( const CCacheWrapper &cache ) {

        std::cout << std::string("cache:") << std::endl;
        for (auto iter = cache.database.begin(); iter != cache.database.end(); iter++) {
            if (iter->second.size() != 16) {
                std::cout << std::string("key:") << StringToHexString(iter->first, "") << std::string(" value:")
                          << StringToHexString(iter->second, "") << std::string("(") << stToString(iter->second)
                          << std::string(")") << std::endl;
            } else {

                std::cout << std::string("key:") << StringToHexString(iter->first, "") << std::string(" value:")
                          << StringToHexString(iter->second, "") << std::string("(") << accountToString(iter->second)
                          << std::string(")") << std::endl;
            }
        }

    }


}


// int main( int argc, char **argv ) {

//     std::vector <uint8_t> code;

//     char byte;
//     ifstream f("wasm/token.wasm", ios::binary);
//     while (f.get(byte)) code.push_back(byte);


//     CWasmInterface wasmInterface;
//     wasmInterface.Initialize(vmType::eos_vm_jit);
//     CWasmDbWrapper cache;
//     inline_transactionsQueue queue;

//     name receiver = name("wasm.token");
//     name contract = name("wasm.token");
//     name action = name("create");

//     inline_transaction trx;
//     trx.contract = contract.value;
//     trx.action = action.value;

//     //create
//     std::cout << std::string("-------------------------------------------------------------------------------------")
//               << std::endl;
//     std::cout << std::string("1.wasm.token->create issuer:walker maximum_supply:asset{1000000000, symbol('BTC', 4)}")
//               << std::endl;
//     name issuer = name("walker");
//     asset maximum_supply = asset{1000000000, symbol("BTC", 4)};
//     //create_args args{issuer,maximum_supply};
//     //trx.data = pack(args);
//     trx.data = pack(std::tuple(issuer, maximum_supply));
//     queue.pushBack(trx);
//     //inline_transaction tx = queue.queue.front();

//     CWasmContext wasmContextCreate(queue, cache);

//     // std::cout << std::string("receiver:") << receiver.to_string()<< std::endl
//     //           << std::string("contract:") << contract.to_string()<< std::endl
//     //           << std::string("action:") << action.to_string()<< std::endl
//     //           << std::string("data:") << VectorToHexString(tx.data) << std::endl;


//     //system_clock::time_point start = system_clock::now();
//     wasmInterface.Execute(code, &wasmContextCreate);
//     //system_clock::time_point end = system_clock::now();
//     //std::cerr <<std::string("duration:") <<std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() <<std::endl ;
//     print(cache);

//     //issue
//     std::cout << std::string("---------------------------------------------------------------------------")
//               << std::endl;
//     std::cout << std::string("2.1 wasm.token->issue to:walker quantity:asset{800000000, symbol('BTC', 4)}")
//               << std::endl;

//     name to = name("walker");
//     action = name("issue");
//     trx.contract = contract.value;
//     trx.action = action.value;

//     asset quantity = asset{800000000, symbol("BTC", 4)};
//     // std::string memo("issue to walker 800000000 BTC");
//     // vector<char> memo(str.begin(), str.end());
//     // trx.data = pack(std::tuple(to, quantity, memo));
//     std::string str("issue to walker 800000000 BTC");
//     trx.data = pack(std::tuple(to, quantity, std::string("issue to walker 800000000 BTC")));
//     queue.pushBack(trx);

//     CWasmContext wasmContextIssue(queue, cache);
//     wasmInterface.Execute(code, &wasmContextIssue);
//     print(cache);


//     std::cout << std::string("---------------------------------------------------------------------------")
//               << std::endl;
//     std::cout << std::string("2.2 wasm.token->issue to:xiaoyu quantity:asset{100000000, symbol('BTC', 4)}")
//               << std::endl;

//     to = name("xiaoyu");
//     action = name("issue");
//     trx.contract = contract.value;
//     trx.action = action.value;

//     quantity = asset{100000000, symbol("BTC", 4)};
//     // std::string strIssue("issue to xiaoyu 100000000 BTC");
//     // vector<char> memoIssue(strIssue.begin(), strIssue.end());
//     trx.data = pack(std::tuple(to, quantity, string("issue to xiaoyu 100000000 BTC")));
//     queue.pushBack(trx);

//     //start = system_clock::now();
//     while (queue.size()) {
//         CWasmContext wasmContextIssue2(queue, cache);
//         wasmInterface.Execute(code, &wasmContextIssue2);
//     }

//     //end = system_clock::now();
//     //std::cerr <<std::string("duration:") <<std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() <<std::endl ;

//     print(cache);

//     //retire
//     std::cout << std::string("-----------------------------------------------------------------") << std::endl;
//     std::cout << std::string("3. wasm.token->retire quantity:asset{100000000, symbol('BTC', 4)}") << std::endl;

//     action = name("retire");
//     trx.contract = contract.value;
//     trx.action = action.value;

//     quantity = asset{100000000, symbol("BTC", 4)};
//     // std::string strRetire("issue 100000000 BTC");
//     // vector<char> memoRetire(strRetire.begin(), strRetire.end());

//     trx.data = pack(std::tuple(quantity, string("issue 100000000 BTC")));
//     queue.pushBack(trx);

//     //std::cout << std::string("queue size:")<< queue.queue.size()<<std::endl ;
//     CWasmContext wasmContextRetire(queue, cache);
//     wasmInterface.Execute(code, &wasmContextRetire);
//     print(cache);

//     //transfer
//     std::cout
//             << std::string("------------------------------------------------------------------------------------------")
//             << std::endl;
//     std::cout
//             << std::string("4.1 wasm.token->transfer from:walker to:xiaoyu quantity:asset{100000000, symbol('BTC', 4)}")
//             << std::endl;

//     action = name("transfer");
//     trx.contract = contract.value;
//     trx.action = action.value;

//     name from = name("walker");
//     to = name("xiaoyu");
//     quantity = asset{100000000, symbol("BTC", 4)};
//     // std::string strTransfer("issue 100000000 BTC");
//     // vector<char> memoTransfer(strTransfer.begin(), strTransfer.end());

//     trx.data = pack(std::tuple(from, to, quantity, string("issue 100000000 BTC")));
//     queue.pushBack(trx);

//     //std::cout << std::string("queue size:")<< queue.queue.size()<<std::endl ;
//     CWasmContext wasmContextTranser(queue, cache);
//     wasmInterface.Execute(code, &wasmContextTranser);
//     print(cache);

//     //transfer
//     // std::cout << std::string("----------------------------------------------------------------")<<std::endl ;
//     // std::cout << std::string("4.2wasm.token->transfer from:xiaoyu to:mark quantity:asset{100000000, symbol('BTC', 4)}")<<std::endl ;

//     // action = name("transfer");
//     // trx.contract = contract.value;
//     // trx.funcName = action.value;

//     // from = name("xiaoyu");
//     // to = name("mark");
//     // quantity = asset{100000000, symbol("BTC", 4)};
//     // std::string strTransfer2("issue 800000000 BTC");
//     // vector<char> memoTransfer2(strTransfer2.begin(), strTransfer2.end());

//     // trx.data = pack(std::tuple(from, to, quantity, memoTransfer));
//     // queue.pushBack(trx);

//     // //std::cout << std::string("queue size:")<< queue.queue.size()<<std::endl ;
//     // CWasmContext wasmContextTranser2( queue, cache);
//     // wasmInterface.Execute(code, wasmContextTranser2);
//     // for(auto iter = cache.database.begin(); iter != cache.database.end(); iter++)
//     //        std::cout << std::string("key:") << StringToHexString(iter->first,"") << std::string(" value:") << StringToHexString(iter->second,"")<<std::endl ;

//     //open
//     std::cout << std::string("-------------------------------------------------------------------------") << std::endl;
//     std::cout << std::string("5. wasm.token->open owner:mark  symbol:symbol('BTC', 4) ram_payer: walker") << std::endl;

//     action = name("open");
//     trx.contract = contract.value;
//     trx.action = action.value;

//     name owner = name("mark");
//     name ram_payer = name("walker");
//     symbol sym = symbol("BTC", 4);

//     trx.data = pack(std::tuple(owner, sym, ram_payer));
//     queue.pushBack(trx);

//     //std::cout << std::string("queue size:")<< queue.queue.size()<<std::endl ;
//     CWasmContext wasmContextOpen(queue, cache);
//     wasmInterface.Execute(code, &wasmContextOpen);
//     print(cache);

//     //close
//     std::cout << std::string("-------------------------------------------------------") << std::endl;
//     std::cout << std::string("6. wasm.token->close owner:mark  symbol:symbol('BTC', 4)") << std::endl;

//     action = name("close");
//     trx.contract = contract.value;
//     trx.action = action.value;

//     owner = name("mark");
//     sym = symbol("BTC", 4);

//     trx.data = pack(std::tuple(owner, sym));
//     queue.pushBack(trx);

//     //std::cout << std::string("queue size:")<< queue.queue.size()<<std::endl ;
//     CWasmContext wasmContextClose(queue, cache);
//     wasmInterface.Execute(code, &wasmContextClose);
//     print(cache);

//     return 0;
// }

int main( int argc, char **argv ) {

    validating_tester tester;

    set_code(tester, NAME(testapi), "token.wasm");

    name  issuer         = name("walker");
    asset maximum_supply = asset{1000000000, symbol("BTC", 4)};
    vector<char> data = wasm::pack(std::tuple(issuer, maximum_supply));
    CALL_TEST_FUNCTION_ACTION(tester, wasm::name("create").value, data);

    system_clock::time_point start = system_clock::now();
    for(int i = 0 ; i < 10000 ; i ++){
        name  to       = name("walker");
        asset quantity = asset{10000, symbol("BTC", 4)};
        data = wasm::pack(std::tuple(to, quantity, string("issue to xiaoyu")));
        CALL_TEST_FUNCTION_ACTION(tester, wasm::name("issue").value, data);
    }
    std::cerr <<std::string("duration:") <<std::chrono::duration_cast<std::chrono::microseconds>(system_clock::now() - start).count() <<std::endl ;
    //tester.ctrl.cache.print();

    return 0;
}

