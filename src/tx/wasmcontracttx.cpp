// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "wasmcontracttx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/contractdb.h"
#include "persistence/txdb.h"
#include "config/version.h"
#include <sstream>

#include "wasm/wasmcontext.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/types/name.hpp"

static bool GetKeyId(const CAccountDBCache &view, const string &userIdStr, CKeyID &KeyId) {
    if (userIdStr.size() == 6) {
        CRegID regId(userIdStr);
        KeyId = regId.GetKeyId(view);
    } else if (userIdStr.size() == 34) {
        string addr(userIdStr.begin(), userIdStr.end());
        KeyId = CKeyID(addr);
    } else {
        return false;
    }

    if (KeyId.IsEmpty()) return false;

    return true;
}

bool CWasmContractTx::CheckTx(int nHeight, CCacheWrapper &cw, CValidationState &state) {
    // IMPLEMENT_CHECK_TX_FEE;
    // IMPLEMENT_CHECK_TX_REGID(txUid.type());

    // if (!contract.IsValid()) {
    //     return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, contract is invalid"),
    //                      REJECT_INVALID, "vmscript-invalid");
    // }

    // uint64_t llFuel = GetFuel(GetFuelRate(cw.contractCache));
    // if (llFees < llFuel) {
    //     return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, fee too litter to afford fuel "
    //                      "(actual:%lld vs need:%lld)", llFees, llFuel),
    //                      REJECT_INVALID, "fee-too-litter-to-afford-fuel");
    // }

    // // If valid height range changed little enough(i.e. 3 blocks), remove it.
    // if (GetFeatureForkVersion(height) == MAJOR_VER_R2) {
    //     unsigned int nTxSize = ::GetSerializeSize(SER_NETWORK, PROTOCOL_VERSION);
    //     double dFeePerKb     = double(llFees - llFuel) / (double(nTxSize) / 1000.0);
    //     if (dFeePerKb < CBaseTx::nMinRelayTxFee) {
    //         return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, fee too litter in fees/Kb "
    //                          "(actual:%.4f vs need:%lld)", dFeePerKb, CBaseTx::nMinRelayTxFee),
    //                          REJECT_INVALID, "fee-too-litter-in-fees/Kb");
    //     }
    // }

    // CAccount account;
    // if (!cw.accountCache.GetAccount(txUid, account)) {
    //     return state.DoS(100, ERRORMSG("CWasmContractTx::CheckTx, get account failed"),
    //                      REJECT_INVALID, "bad-getaccount");
    // }
    // if (!account.HaveOwnerPubKey()) {
    //     return state.DoS(
    //         100, ERRORMSG("CWasmContractTx::CheckTx, account unregistered"),
    //         REJECT_INVALID, "bad-account-unregistered");
    // }

    // IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);

    return true;
}

// bool CWasmContractTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cache, CValidationState &state) {
//     CInlineTransaction trx;
//     trx.contract = contract;
//     trx.action = action;
//     trx.data = data;

//    std::cout << "CWasmContractTx ExecuteTx"
//              << " contract:"<< wasm::name(trx.contract).to_string()
//              << " action:"<< wasm::name(trx.action).to_string()
//              // << " data:"<< params[3].get_str()
//              << " \n";

//     wasm::transaction_trace trx_trace;

//     CInlineTransactionsQueue queue;
//     queue.pushBack(trx);

//     try {
//         while(queue.size()){
//             CWasmContext wasmContext(queue, *this, cache, state);
//             wasmContext.Initialize();
//             wasmContext.ExecuteOne();

//             //receipt for accounts in notified
//             for(uint32_t i = 1; i < wasmContext.notified.size(); ++i){
//                 wasmContext.receiver = wasmContext.notified[i];
//                 wasmContext.ExecuteOne();
//             }
//         }
//     } catch( CException& e ) {

//        return state.DoS(100, ERRORMSG(e.errMsg.data()), e.errCode, e.errMsg);
//     }


//     return true;
// }

bool CWasmContractTx::ExecuteTx(int nHeight, int nIndex, CCacheWrapper &cache, CValidationState &state) {

    CInlineTransaction trx;
    trx.contract = contract;
    trx.action = action;
    trx.data = data;

    std::cout << "CWasmContractTx ExecuteTx"
              << " contract:"<< wasm::name(trx.contract).to_string()
              << " action:"<< wasm::name(trx.action).to_string()
              << " \n";


    wasm::transaction_trace trx_trace;
    trx_trace.trx_id = GetHash();
    //trx_trace.block_height = nHeight;
    //trx_trace.block_time =

    try {
        //trx_trace.traces.emplace_back();
        DispatchInlineTransaction( trx_trace.trace, trx, trx.contract, cache, state, 0 );
    } catch( CException& e ) {
       return state.DoS(100, ERRORMSG(e.errMsg.data()), e.errCode, e.errMsg);
    }

    //cache.save(trace)
    return true;
}

void CWasmContractTx::DispatchInlineTransaction( wasm::inline_transaction_trace& trace,
                                                 wasm::CInlineTransaction& trx,
                                                 uint64_t receiver,
                                                 CCacheWrapper& cache,
                                                 CValidationState& state,
                                                 uint32_t recurse_depth) {

    //std::cout << "DispatchInlineTransaction ----------------------------"<< " \n";

    CWasmContext wasmContext(*this, trx, cache, state, recurse_depth);
    wasmContext.receiver = receiver;
    wasmContext.Execute(trace);

}



bool CWasmContractTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    CKeyID senderKeyId;
    if (!cw.accountCache.GetKeyId(txUid, senderKeyId))
        return false;

    keyIds.insert(senderKeyId);
    return true;
}

uint64_t CWasmContractTx::GetFuel(uint32_t nFuelRate) {
    return std::max(uint64_t((nRunStep / 100.0f) * nFuelRate), 1 * COIN);
}

// string ToHex(string str, string separator = " ")
// {

//     const std::string hex = "0123456789abcdef";
//     std::stringstream ss;

//     for (std::string::size_type i = 0; i < str.size(); ++i)
//         ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

//     return ss.str();

// }


string CWasmContractTx::ToString(CAccountDBCache &accountCache) {
    CKeyID senderKeyId;
    accountCache.GetKeyId(txUid, senderKeyId);

    return strprintf("hash=%s, txType=%s, version=%d, contract=%s, senderuserid=%s, senderkeyid=%s, fee=%ld, valid_height=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, contract, txUid.ToString(), senderKeyId.GetHex(), llFees,
                     valid_height);
}

Object CWasmContractTx::ToJson(const CAccountDBCache &accountCache) const{
    Object object;
    //CAccountDBCache clone(accountCache);

    CKeyID senderKeyId;
    accountCache.GetKeyId(txUid, senderKeyId);

    object.push_back(Pair("hash", GetHash().GetHex()));
    object.push_back(Pair("tx_type", GetTxType(nTxType)));
    object.push_back(Pair("version", nVersion));
    object.push_back(Pair("contract", contract));
    object.push_back(Pair("action", wasm::name(action).to_string()));
    object.push_back(Pair("data", HexStr(data)));
    //object.push_back(Pair("regId", txUid.get<CRegID>().ToString()));
    //object.push_back(Pair("addr", senderKeyId.ToAddress()));
    //object.push_back(Pair("script", "script_content"));
    object.push_back(Pair("sender", senderKeyId.GetHex()));

    object.push_back(Pair("fees", llFees));
    object.push_back(Pair("valid_height", valid_height));
    return object;
}

