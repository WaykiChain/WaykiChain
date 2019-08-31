#pragma once
#include "wasmcontext.hpp"
#include "datastream.hpp"
#include "exceptions.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

vector<uint8_t> CWasmContext::GetCode(uint64_t account) {

  vector<uint8_t> code;
  return code;

}

void CWasmContext::ExecuteOne(){

    // auto native = FindNativeHandle(receiver, trx.action);
    // if( native ){
    //    (*native)(*this);
    // }else{
    //     vector<uint8_t> code = GetCode(receiver);
    //     if (code.size() > 0){
    //         try{
    //             wasmInterface.Execute(code, *this);
    //         }catch( CException &e ) {
    //            throw e;
    //         }
    //     }
    // }

}

void CWasmContext::ExecuteInline(CInlineTransaction t){
    //wasm_assert check the code and authorization
    queue.pushBack(t);
}

bool CWasmContext::HasRecipient( uint64_t account ) const {
   for( auto a : notified )
      if( a == account )
         return true;
   return false;
}

void CWasmContext::RequireRecipient( uint64_t recipient ){

  if( !HasRecipient(recipient) ) {
      notified.push_back(recipient);
   }

}

// void CWasmContext::RegisterNativeHandler(uint64_t receiver, uint64_t action, nativeHandler v){
//       nativeHandlers[std::pair(receiver, action)] = v;
// }

// nativeHandler* CWasmContext::FindNativeHandle(uint64_t receiver, uint64_t action){

//   auto handler = nativeHandlers.find(pair(receiver, action));
//   if( handler != nativeHandlers.end()){
//     return &handler->second;
//   }

//   return nullptr;

// }

}