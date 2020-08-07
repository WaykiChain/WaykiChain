// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/messagequeue.h"
#include "init.h"
#include "main.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "tx/tx.h"
#include "tx/cointransfertx.h"
#include "tx/universaltx.h"
#include "wallet/wallet.h"
#include "wasm/abi_serializer.hpp"
#include "wasm/modules/wasm_native_dispatch.hpp"
#include <boost/thread/thread.hpp>

using namespace json_spirit;
using namespace std;

class TpsTester {
public:
    typedef MsgQueue<std::shared_ptr<CBaseTx>> GenTxQueue;
public:
    int64_t period;
    int64_t batchSize;
    unique_ptr<GenTxQueue> generationQueue;
    boost::thread_group* generateThreads;
public:
    TpsTester() {

    }

    ~TpsTester() {
        Stop();
    }

    void Stop() {
        if (generateThreads != nullptr) {
            if(generationQueue) {
                generationQueue->Clear();
            }
            generateThreads->interrupt_all();
            generateThreads->join_all();
            delete generateThreads;
            generateThreads = nullptr;
            generationQueue = nullptr;
        }
    }

    void SendTx() {
        RenameThread("coin-tpssend");
        SetThreadPriority(THREAD_PRIORITY_NORMAL);

        CValidationState state;
        std::shared_ptr<CBaseTx> tx;

        while (true) {
            // add interruption point
            boost::this_thread::interruption_point();

            if (generationQueue->Pop(&tx)) {
                LOCK(cs_main);
                if (!::AcceptToMemoryPool(mempool, state, tx.get(), true)) {
                    LogPrint(BCLog::ERROR, "TpsTester::SendTx, accept to mempool failed: %s\n", state.GetRejectReason());
                    throw boost::thread_interrupted();
                }
            }
        }
    }

    template<typename F>
    void StartCommonGeneration(const int64_t periodIn, const int64_t batchSizeIn, F generateFunc) {

        Stop();

        period = periodIn;
        batchSize = batchSizeIn;
        if (period == 0 || batchSize == 0)
            return;

        // reset message queue according to <period, batchSize>
        // For example, generate 50(batchSize) transactions in 20(period), then
        // we need to prepare 1000 * 10 / 20 * 50 = 25,000 transactions in 10 second.
        // Actually, set the message queue's size to 50,000(double or up to 60,000).
        GenTxQueue::SizeType size       = 1000 * 10 * batchSize * 2 / period;
        GenTxQueue::SizeType actualSize = size > MSG_QUEUE_MAX_LEN ? MSG_QUEUE_MAX_LEN : size;

        generationQueue = std::make_unique<GenTxQueue>(actualSize);

        generateThreads = new boost::thread_group();
        generateThreads->create_thread(generateFunc);
        generateThreads->create_thread(boost::bind(&TpsTester::SendTx, this));
    }


};

TpsTester g_tpsTester = TpsTester();

static void CommonTxGenerator(TpsTester *tpsTester) {
    RenameThread("coin-gentxcomm");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key))
            throw boost::thread_interrupted();
    }
    shared_ptr<CCacheWrapper> spCw;
    uint64_t llFees         = 0;
    {
        LOCK(cs_main);
        spCw = make_shared<CCacheWrapper>(pCdMan);
        GetTxMinFee(*spCw, BCOIN_TRANSFER_TX, chainActive.Height(), SYMB::WICC, llFees);
    }

    CRegID srcRegId("0-1");
    CRegID desRegId("0-1");
    static uint64_t llValue = 10000;  // use static variable to keep autoincrement

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();

        for (int64_t i = 0; i < tpsTester->batchSize; ++i) {
            std::shared_ptr<CBaseCoinTransferTx> tx = make_shared<CBaseCoinTransferTx>();
            tx->txUid        = srcRegId;
            tx->toUid        = desRegId;
            tx->coin_amount  = llValue++;
            tx->llFees       = llFees;
            tx->valid_height = validHeight;

            // sign transaction
            key.Sign(tx->GetHash(), tx->signature);

            tpsTester->generationQueue->Push(tx);
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint(BCLog::DEBUG, "batch generate transaction(s): %ld, elapse time: %ld ms.\n",
                tpsTester->batchSize, elapseTime);

        if (elapseTime < tpsTester->period) {
            MilliSleep(tpsTester->period - elapseTime);
        } else {
            LogPrint(BCLog::DEBUG, "need to slow down for overloading.\n");
        }
    }
}

void StartCommonGeneration(const int64_t period, const int64_t batchSize) {
    g_tpsTester.StartCommonGeneration(period, batchSize, boost::bind(&CommonTxGenerator, &g_tpsTester));
}

Value startcommontpstest(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "startcommontpstest \"period\" \"batch_size\"\n"
            "\nStart generation blocks with batch_size transactions in period ms.\n"
            "\nArguments:\n"
            "1.\"period\" (numeric, required) 0~1000\n"
            "2.\"batch_size\" (numeric, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("startcommontpstest", "20 20") + "\nAs json rpc call\n" +
            HelpExampleRpc("startcommontpstest", "20, 20"));
    }

    Object obj;
    if (SysCfg().NetworkID() != REGTEST_NET) {
        obj.push_back(Pair("msg", "regtest only."));
        return obj;
    }

    int64_t period = params[0].get_int64();
    if (period < 0 || period > 1000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "period should range between 0 to 1000");
    }

    int64_t batchSize = params[1].get_int64();
    if (batchSize < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "batch size should be bigger than 0");
    }

    StartCommonGeneration(period, batchSize);

    obj.push_back(Pair("msg", "success"));
    return obj;
}

void static ContractTxGenerator(TpsTester *tpsTester, const string& regid) {
    RenameThread("coin-gentxlua");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key))
            throw boost::thread_interrupted();
    }
    shared_ptr<CCacheWrapper> spCw;
    uint64_t llFees         = 0;
    {
        LOCK(cs_main);
        spCw = make_shared<CCacheWrapper>(pCdMan);
        GetTxMinFee(*spCw, BCOIN_TRANSFER_TX, chainActive.Height(), SYMB::WICC, llFees);
    }
    CRegID txUid("0-1");
    CRegID appUid(regid);
    static uint64_t llValue = 10000;  // use static variable to keep autoincrement

    // hex(whmD4M8Q8qbEx6R5gULbcb5ZkedbcRDGY1) =
    // 77686d44344d3851387162457836523567554c626362355a6b656462635244475931
    string arguments = ParseHexStr("77686d44344d3851387162457836523567554c626362355a6b656462635244475931");

    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();

        for (int64_t i = 0; i < tpsTester->batchSize; ++i) {
            shared_ptr<CLuaContractInvokeTx> tx = make_shared<CLuaContractInvokeTx>();
            tx->txUid        = txUid;
            tx->app_uid      = appUid;
            tx->coin_amount  = llValue++;
            tx->llFees       = llFees;
            tx->arguments    = arguments;
            tx->valid_height = validHeight;

            // sign transaction
            key.Sign(tx->GetHash(), tx->signature);

            tpsTester->generationQueue->Push(tx);
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint(BCLog::DEBUG, "batch generate transaction(s): %ld, elapse time: %ld ms.\n", tpsTester->batchSize, elapseTime);

        if (elapseTime < tpsTester->period) {
            MilliSleep(tpsTester->period - elapseTime);
        } else {
            LogPrint(BCLog::DEBUG, "need to slow down for overloading.\n");
        }
    }
}

void StartContractGeneration(const string& regid, const int64_t period, const int64_t batchSize) {
    g_tpsTester.StartCommonGeneration(period, batchSize, boost::bind(&ContractTxGenerator, &g_tpsTester, regid));
}

Value startcontracttpstest(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 3) {
        throw runtime_error(
            "startcontracttpstest \"regid\" \"period\" \"batch_size\"\n"
            "\nStart generation blocks with batch_size contract transactions in period ms.\n"
            "\nArguments:\n"
            "1.\"regid\" (string, required) contract regid\n"
            "2.\"period\" (numeric, required) 0~1000\n"
            "3.\"batch_size\" (numeric, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("startcontracttpstest", "\"3-1\" 20 20") + "\nAs json rpc call\n" +
            HelpExampleRpc("startcontracttpstest", "\"3-1\", 20, 20"));
    }

    Object obj;
    if (SysCfg().NetworkID() != REGTEST_NET) {
        obj.push_back(Pair("msg", "regtest only."));
        return obj;
    }

    string regid = params[0].get_str();
    if (regid.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "regid should not be empty");
    }

    int64_t period = params[1].get_int64();
    if (period < 0 || period > 1000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "period should range between 0 to 1000");
    }

    int64_t batchSize = params[2].get_int64();
    if (batchSize < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "batch size should be bigger than 0");
    }

    StartContractGeneration(regid, period, batchSize);

    obj.push_back(Pair("msg", "success"));
    return obj;
}



void static WasmTxGenerator(TpsTester *tpsTester, wasm::inline_transaction &inlineTx) {
    RenameThread("coin-gentxwasm");
    SetThreadPriority(THREAD_PRIORITY_NORMAL);

    CCoinSecret vchSecret;
    vchSecret.SetString("Y6J4aK6Wcs4A3Ex4HXdfjJ6ZsHpNZfjaS4B9w7xqEnmFEYMqQd13");
    CKey key = vchSecret.GetKey();

    // remove key from wallet first.
    {
        LOCK2(cs_main, pWalletMain->cs_wallet);
        if (!pWalletMain->RemoveKey(key))
            throw boost::thread_interrupted();
    }
    shared_ptr<CCacheWrapper> spCw;
    uint64_t minFees         = 0;
    {
        LOCK(cs_main);
        spCw = make_shared<CCacheWrapper>(pCdMan);
        GetTxMinFee(*spCw, BCOIN_TRANSFER_TX, chainActive.Height(), SYMB::WICC, minFees);
    }
    CRegID txUid("0-1");
    auto payer      = wasm::regid(txUid.GetIntValue());

    inlineTx.authorization = std::vector<permission>{{payer.value, wasmio_owner}};

    int32_t lastHeight = 0;
    uint64_t fees = minFees;
    while (true) {
        // add interruption point
        boost::this_thread::interruption_point();

        int64_t nStart      = GetTimeMillis();
        int32_t validHeight = chainActive.Height();
        if (lastHeight != validHeight) {
            fees = minFees;
            lastHeight = validHeight;
        }

        for (int64_t i = 0; i < tpsTester->batchSize; ++i) {
            shared_ptr<CUniversalTx> tx = std::make_shared<CUniversalTx>();
            tx->txUid        = txUid;
            tx->fee_symbol   = SYMB::WICC;
            tx->llFees       = fees;
            tx->valid_height = validHeight;

            tx->inline_transactions.push_back(inlineTx);
            // sign transaction
            key.Sign(tx->GetHash(), tx->signature);

            tpsTester->generationQueue->Push(tx);
            fees++;
        }

        int64_t elapseTime = GetTimeMillis() - nStart;
        LogPrint(BCLog::DEBUG, "batch generate transaction(s): %ld, elapse time: %ld ms.\n", tpsTester->batchSize, elapseTime);

        if (elapseTime < tpsTester->period) {
            MilliSleep(tpsTester->period - elapseTime);
        } else {
            LogPrint(BCLog::DEBUG, "need to slow down for overloading.\n");
        }
    }
}

namespace RPC_PARAM {
    int64_t ParseInt64(const Value &jsonValue, const string &name) {
        ComboMoney money;
        Value_type valueType = jsonValue.type();
        if (valueType == json_spirit::Value_type::int_type ) {
            return jsonValue.get_int64();
        } else if (valueType == json_spirit::Value_type::str_type) {
            return atoi64(jsonValue.get_str());
        } else {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("%s should be numeric or string, but got %s type",
                name, JSON::GetValueTypeName(valueType)));
        }
    }
}

Value startwasmtpstest(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 5) {
        throw runtime_error(
            "startwasmtpstest \"regid\" \"period\" \"batch_size\"\n"
            "\nStart generate wasm contract tx for tps with batch_size tx in period ms.\n"
            "\nArguments:\n"
            "1.\"regid\" (string, required) contract regid\n"
            "2.\"action\" (string, required) contract action\n"
            "3.\"data\" (object, required) contract data, can be object or array\n"
            "4.\"period\" (numeric, required) 0~1000\n"
            "5.\"batch_size\" (numeric, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("startwasmtpstest", "\"0-1\" \"0-800\" \"transfer\" '["
                "\"0-1\", \"8-2\", \"1.00000000 WICC\", \"\"]' 20 20") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("startwasmtpstest", "\"0-1\", \"0-800\", \"transfer\", ["
                "\"0-1\", \"8-2\", \"1.00000000 WICC\", \"\"], 20, 20"));
    }
    auto regid  = RPC_PARAM::ParseRegId(params[0], "contract");
    auto action = wasm::name(params[1].get_str());
    auto argsIn = RPC_PARAM::GetWasmContractArgs(params[2]);
    int64_t period = RPC_PARAM::ParseInt64(params[3], "period");
    int64_t batchSize = RPC_PARAM::ParseInt64(params[4], "batch_size");

    if (SysCfg().NetworkID() != REGTEST_NET) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "work in regtest only");
    }

    //get abi
    std::vector<char> abi;
    {
        LOCK(cs_main);
        if (!get_native_contract_abi(regid.GetIntValue(), abi)) {
            CUniversalContractStore contract_store = RPC_PARAM::GetWasmContract(*pCdMan->pContractCache, regid);
            abi = std::vector<char>(contract_store.abi.begin(), contract_store.abi.end());
        }
    }


    std::vector<char> action_data = wasm::abi_serializer::pack(abi, action.to_string(), argsIn, max_serialization_time);
    CHAIN_ASSERT( action_data.size() < MAX_CONTRACT_ARGUMENT_SIZE,
                    wasm_chain::inline_transaction_data_size_exceeds_exception,
                    "inline transaction args is out of size(%u vs %u)",
                    action_data.size(), MAX_CONTRACT_ARGUMENT_SIZE)


    if (period < 0 || period > 1000) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "period should range between 0 to 1000");
    }

    if (batchSize < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "batch size should be bigger than 0");
    }

    wasm::inline_transaction inlineTx = { regid.GetIntValue(), action.value, {}, action_data };

    g_tpsTester.StartCommonGeneration(period, batchSize, boost::bind(&WasmTxGenerator, &g_tpsTester, inlineTx));

    Object obj;
    obj.push_back(Pair("msg", "success"));
    return obj;
}
