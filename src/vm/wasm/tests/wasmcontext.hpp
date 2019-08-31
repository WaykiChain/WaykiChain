#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>
#include <iostream>

#include "inlinetransaction.hpp"
#include "name.hpp"
#include <asset.hpp>
// #include "softfloat.hpp"
// #include "compiler_builtins.hpp"
#include "wasminterface.hpp"
#include "datastream.hpp"
#include "wasmcontextinterface.hpp"

using namespace std;
using namespace wasm;
namespace wasm {


class CWasmDbWrapper{
public:
    CWasmDbWrapper(){}
    bool GetContractData(const uint64_t &contract, const string &key, string &value){

        vector<char> buffer(sizeof(uint64_t) + key.size());
        wasm::datastream<char *> ds(buffer.data(),buffer.size());
        ds.write((char *)&contract, sizeof(uint64_t));
        ds.write(key.data(), key.size());
        string k(buffer.data(), buffer.size());

        auto iter = database.find(k);
        if( iter != database.end()){
        	value = iter->second;
        	return true;
        }
        
        return false;
    }

    bool SetContractData(const uint64_t &contract, const string &key, const string &value){

        vector<char> buffer(sizeof(uint64_t) + key.size());
        wasm::datastream<char *> ds(buffer.data(),buffer.size());
        ds.write((char *)&contract, sizeof(uint64_t));
        ds.write(key.data(), key.size());
        string k(buffer.data(), buffer.size());

        database[k] = value;

        return true;
    }

    bool EraseContractData(const uint64_t &contract, const string &key ){

        vector<char> buffer(sizeof(uint64_t) + key.size());
        wasm::datastream<char *> ds(buffer.data(),buffer.size());
        ds.write((char *)&contract, sizeof(uint64_t));
        ds.write(key.data(), key.size());
        string k(buffer.data(), buffer.size());

        database.erase(k);

        return true;
    }

    void print() {
    	for(auto iter = database.begin(); iter != database.end(); iter++) 
           std::cout << "key:" << iter->first << " value:" << iter->second<<std::endl ;
    }

public:

	map<string, string> database;
};

// class CWasmContext;
// using nativeHandler = std::function<void(CWasmContext&)>;

class CInlineTransactionsQueue {

public:
	CInlineTransactionsQueue(){
		trxNumber = 0;
	};
	~CInlineTransactionsQueue(){};

	CInlineTransaction popFront(){
		CInlineTransaction trx = queue.front();
		queue.pop();
		return  trx;
	}

	bool pushBack(CInlineTransaction trx){
		trxNumber ++;
		queue.push(trx);
		return true;
	}

    int size(){
        return queue.size();
    }
public:
	uint32_t trxNumber;
	std::queue<CInlineTransaction> queue;
};


class CWasmContext : public CWasmContextInterface {
// class CWasmContext {

public:
	//CWasmContext(){};
	CWasmContext(CInlineTransactionsQueue& q, CWasmDbWrapper& c)
	:queue(q)
	,db(c)
    {
        // queue = q;
        // db = std::move(CWasmDbWrapper(c));
        // controlTx = ctrl;
        // state = s;
        trx = queue.popFront();
        receiver = trx.contract;
        notified.push_back(receiver);

        //wasmInterface.Initialize(wasm::vmType::eosvm);

        // RegisterNativeHandler(name("wasmio").value, name("setcode").value, wasmNativeSetcode);
    }
	~CWasmContext(){}

public:
    vector<uint8_t> GetCode(uint64_t account);
    void ExecuteOne();

    // void RegisterNativeHandler(uint64_t receiver, uint64_t action, nativeHandler v);
    // nativeHandler* FindNativeHandle(uint64_t receiver, uint64_t action);

public:
    void ExecuteInline(CInlineTransaction t);
    bool HasRecipient( uint64_t account ) const;
    void RequireRecipient( uint64_t recipient );

    uint64_t Receiver() { return receiver;} 
    uint64_t Contract() { return trx.contract;} 
    uint64_t Action() { return trx.action;} 
    const char* GetActionData() {return trx.data.data();}
    uint32_t GetActionDataSize() {return trx.data.size();}

    bool SetData( uint64_t contract, string k ,string v) { return db.SetContractData(contract, k, v);} 
    bool GetData( uint64_t contract, string k ,string& v) { return db.GetContractData(contract, k, v);} 
    bool EraseData( uint64_t contract, string k ) { return db.EraseContractData(contract, k );} 
       
public:
	//std::string  sParam = "hello world";
	uint64_t receiver;
    vector<uint64_t> notified;

    CInlineTransactionsQueue& queue;
	CWasmDbWrapper& db;

    CInlineTransaction trx;

    //CWasmInterface wasmInterface;
    // map< pair<uint64_t,uint64_t>, nativeHandler>   nativeHandlers;

};
}
