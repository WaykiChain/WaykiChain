#pragma once
#include "wasm/wasmcontext.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

//	struct SetCode {
//		uint64_t account;
//		string code;
//		string abi;
//		string memo;
//	};
//
//      template<typename DataStream>
//      inline DataStream& operator<<(DataStream& ds, const wasm::SetCode& setcode) {
//        ds << setcode.account ;
//        ds << setcode.code;
//        ds << setcode.abi;
//        ds << setcode.memo;
//         return ds;
//      }
//
//      template<typename DataStream>
//      inline DataStream& operator>>(DataStream& ds, wasm::SetCode& setcode) {
//        ds >> setcode.account ;
//        ds >> setcode.code;
//        ds >> setcode.abi;
//        ds >> setcode.memo;
//
//         return ds;
//      }

	class CWasmContext;
	void WasmNativeSetcode(CWasmContext&);

};