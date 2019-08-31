#pragma once
#include <vector>
//#include "wasmcontextinterface.hpp"
#include "wasmcontextinterface.hpp"

namespace wasm {

// class CWasmContext;

enum class vmType {
            eosvm,
            wabt
         };

class CWasmInterface {

	public:

        CWasmInterface();
	    //CWasmInterface(vmType type);
	    ~CWasmInterface();
    
    public:
        void Initialize(vmType type);
		void Execute(vector<uint8_t> code, CWasmContextInterface* pWasmContext);
		//void validate(const vector<uint8_t> &code );
		//void exit();

};
}
