#include "config/errorcode.h"
#include "wasm/wasmnativecontract.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/datastream.hpp"
#include "persistence/cachewrapper.h"

using namespace std;
using namespace wasm;

namespace wasm {

void WasmNativeSetcode(CWasmContext& context) {

    CAccount sender;
    WASM_ASSERT(context.cache.accountCache.GetAccount(context.control_trx.txUid, sender), UPDATE_ACCOUNT_FAIL, "bad-read-accountdb",
       "wasmnativecontract.Setcode, sender account does not exist, sender Id = %s",context.control_trx.txUid.ToString().data())


    WASM_ASSERT(sender.OperateBalance(SYMB::WICC, BalanceOpType::SUB_FREE, context.control_trx.llFees), UPDATE_ACCOUNT_FAIL, "operate-account-failed",
       "wasmnativecontract.Setcode, operate account failed ,regId=%s", context.control_trx.txUid.ToString().data())


    WASM_ASSERT(context.cache.accountCache.SetAccount(CUserID(sender.keyid), sender), UPDATE_ACCOUNT_FAIL, "bad-save-accountdb",
       "wasmnativecontract.Setcode, save account info error")

    SetCode setcode = wasm::unpack<SetCode>(context.trx.data);


    CAccount account;
    CRegID contractRegId = wasm::Name2RegID(setcode.account);
    // if (!context.cache.accountCache.GetAccount(contractRegId, account)){
    //   return;
    // }

    CUniversalContract contract;
    context.cache.contractCache.GetContract(contractRegId, contract);

    if (contract.vm_type != VMType::WASM_VM) contract.vm_type = VMType::WASM_VM;

    if (setcode.code.size() > 0)  contract.code = setcode.code;
    if (setcode.abi.size() > 0) contract.abi = setcode.abi;
    if (setcode.memo.size() > 0) contract.memo = setcode.memo;

    WASM_ASSERT(context.cache.contractCache.SaveContract(contractRegId, contract), UPDATE_ACCOUNT_FAIL, "bad-save-scriptdb",
       "wasmnativecontract.Setcode, save account info error")

    // CUniversalContract contractTest;
    // context.cache.contractCache.GetContract(contractRegId, contractTest);

    std::cout << "WasmNativeSetcode ----------------------------"
         <<"  contract:"<< setcode.account
         << " contractRegId:"<< contractRegId.ToString()
         //<< " abi:"<< contractTest.abi
         // << " data:"<< params[3].get_str()
         << " \n";

    //context.control_trx.nRunStep = contract.GetContractSize();
    //if (!SaveTxAddresses(height, index, cw, state, {txUid})) return false;
}

}