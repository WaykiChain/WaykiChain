// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "commons/util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "config/configuration.h"
#include "miner/miner.h"
#include "main.h"
#include "vm/luavm/vmrunenv.h"

#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"

#include "boost/tuple/tuple.hpp"
#define revert(height) ((height<<24) | (height << 8 & 0xff0000) |  (height>>8 & 0xff00) | (height >> 24))

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

Value gettxdetail(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettxdetail \"txid\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.txid   (string,required) The hash of transaction.\n"
            "\nResult an object of the transaction detail\n"
            "\nResult:\n"
            "\n\"txid\"\n"
            "\nExamples:\n"
            + HelpExampleCli("gettxdetail","\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gettxdetail","\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\""));
    return GetTxDetailJSON(uint256S(params[0].get_str()));
}

//create a register account tx
Value registeraccounttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() == 0)
        throw runtime_error("registeraccounttx \"addr\" (\"fee\")\n"
            "\nregister local account public key to get its RegId\n"
            "\nArguments:\n"
            "1.addr: (string, required)\n"
            "2.fee: (numeric, optional) pay tx fees to miner\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 "));

    string addr = params[0].get_str();
    uint64_t fee = 0;
    uint64_t nDefaultFee = SysCfg().GetTxFee();
    if (params.size() > 1) {
        fee = params[1].get_uint64();
        if (fee < nDefaultFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                               strprintf("Input fee smaller than mintxfee: %ld sawi", nDefaultFee));
        }
    } else {
        fee = nDefaultFee;
    }

    CKeyID keyId;
    if (!GetKeyId(addr, keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address invalid");

    CAccountRegisterTx rtx;
    assert(pWalletMain != nullptr);
    {
        EnsureWalletIsUnlocked();

        CAccount account;
        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account))
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");


        if (account.HaveOwnerPubKey())
            throw JSONRPCError(RPC_WALLET_ERROR, "Account was already registered");

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        CPubKey pubkey;
        if (!pWalletMain->GetPubKey(keyId, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Key not found in local wallet");

        CPubKey minerPubKey;
        if (pWalletMain->GetPubKey(keyId, minerPubKey, true)) {
            rtx.minerUid = minerPubKey;
        } else {
            CNullID nullId;
            rtx.minerUid = nullId;
        }
        rtx.txUid        = pubkey;
        rtx.llFees       = fee;
        rtx.valid_height = chainActive.Height();

        if (!pWalletMain->Sign(keyId, rtx.ComputeSignatureHash(), rtx.signature))
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: Sign failed.");
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) &rtx);
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}

Value callcontracttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "callcontracttx \"sender addr\" \"app regid\" \"arguments\" \"amount\" \"fee\" (\"height\")\n"
            "\ncreate contract invocation transaction\n"
            "\nArguments:\n"
            "1.\"sender addr\": (string, required) tx sender's base58 addr\n"
            "2.\"app regid\":   (string, required) contract RegId\n"
            "3.\"arguments\":   (string, required) contract arguments (Hex encode required)\n"
            "4.\"amount\":      (numeric, required) amount of WICC to be sent to the contract account\n"
            "5.\"fee\":         (numeric, required) pay to miner\n"
            "6.\"height\":      (numberic, optional) valid height\n"
            "\nResult:\n"
            "\"txid\":          (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 100"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(int_type)(int_type)(int_type));

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    }

    string arguments = ParseHexStr(params[2].get_str());
    if (arguments.size() >= MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size out of range");
    }

    int64_t amount = AmountToRawValue(params[3]);
    int64_t fee    = RPC_PARAM::GetWiccFee(params, 4, LCONTRACT_INVOKE_TX);
    int32_t height = (params.size() > 5) ? params[5].get_int() : chainActive.Height();

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) && sendRegId.IsMature(chainActive.Height()))
            ? CUserID(sendRegId)
            : CUserID(sendPubKey);

    CRegID recvRegId;
    if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    }

    if (!pCdMan->pContractCache->HaveContract(recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }

    CLuaContractInvokeTx tx;
    tx.nTxType      = LCONTRACT_INVOKE_TX;
    tx.txUid        = sendUserId;
    tx.app_uid      = recvRegId;
    tx.coin_amount  = amount;
    tx.llFees       = fee;
    tx.arguments    = arguments;
    tx.valid_height = height;

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx*)&tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}

// register a contract app tx
Value deploycontracttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error("deploycontracttx \"addr\" \"filepath\" \"fee\" (\"height\") (\"appdesc\")\n"
            "\ncreate a transaction of registering a contract app\n"
            "\nArguments:\n"
            "1.\"addr\":        (string, required) contract owner address from this wallet\n"
            "2.\"filepath\":    (string, required) the file path of the app script\n"
            "3.\"fee\":         (numeric, required) pay to miner (the larger the size of script, the bigger fees are required)\n"
            "4.\"height\":      (numeric, optional) valid height, when not specified, the tip block height in chainActive will be used\n"
            "5.\"appdesc\":     (string, optional) new app description\n"
            "\nResult:\n"
            "\"txid\":          (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("deploycontracttx",
                "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/myapp.lua\" 11000000 10000 \"appdesc\"") +
                "\nAs json rpc call\n"
            + HelpExampleRpc("deploycontracttx",
                "WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH, \"/tmp/lua/myapp.lua\", 11000000, 10000, \"appdesc\""));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(int_type)(str_type));

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, LUA_CONTRACT_LOCATION_PREFIX.size(), LUA_CONTRACT_LOCATION_PREFIX.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    std::tuple<bool, string> result = CVmlua::CheckScriptSyntax(luaScriptFilePath.c_str());
    bool bOK = std::get<0>(result);
    if (!bOK)
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::get<1>(result));

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw runtime_error("deploycontracttx open script file (" + luaScriptFilePath + ") error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    if (lSize <= 0 || lSize > MAX_CONTRACT_CODE_SIZE) { // contract script file size must be <= 64 KB)
        fclose(file);
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            (lSize == -1) ? "File size is unknown"
                          : ((lSize == 0) ? "File is empty" : "File size exceeds 64 KB limit"));
    }

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == nullptr) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }
    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);
        fclose(file);
        throw runtime_error("read script file error");
    } else {
        fclose(file);
    }

    CLuaContract luaContract;
    luaContract.code.assign(buffer, lSize);

    if (buffer)
        free(buffer);

    if (params.size() > 4) {
        luaContract.memo = params[4].get_str();
        if (luaContract.memo.size() > MAX_CONTRACT_MEMO_SIZE) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Contract description is too large");
        }
    }

    uint64_t fee = params[2].get_uint64();
    int32_t height   = params.size() > 3 ? params[3].get_int() : chainActive.Height();

    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fee is smaller than nMinTxFee");
    }
    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }

    assert(pWalletMain != nullptr);
    CLuaContractDeployTx tx;
    {
        EnsureWalletIsUnlocked();

        CAccount account;
        if (!pCdMan->pAccountCache->GetAccount(keyId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Invalid send address");
        }

        if (!account.HaveOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        CRegID regId;
        pCdMan->pAccountCache->GetRegId(keyId, regId);

        tx.txUid          = regId;
        tx.contract       = luaContract;
        tx.llFees         = fee;
        tx.nRunStep       = tx.contract.GetContractSize();
        if (0 == height) {
            height = chainActive.Height();
        }
        tx.valid_height = height;

        if (!pWalletMain->Sign(keyId, tx.ComputeSignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        std::tuple<bool, string> ret;
        ret = pWalletMain->CommitTx((CBaseTx*)&tx);
        if (!std::get<0>(ret)) {
            throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
        }
        Object obj;
        obj.push_back(Pair("txid", std::get<1>(ret)));
        return obj;
    }
}

//vote a delegate transaction
Value votedelegatetx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
            "votedelegatetx \"sendaddr\" \"votes\" \"fee\" [\"height\"] \n"
            "\ncreate a delegate vote transaction\n"
            "\nArguments:\n"
            "1.\"sendaddr\": (string required) The address from which votes are sent to other "
            "delegate addresses\n"
            "2. \"votes\"    (string, required) A json array of votes to delegate candidates\n"
            " [\n"
            "   {\n"
            "      \"delegate\":\"address\", (string, required) The delegate address where votes "
            "are received\n"
            "      \"votes\": n (numeric, required) votes, increase votes when positive or reduce "
            "votes when negative\n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric required) pay fee to miner\n"
            "4.\"height\": (numeric optional) valid height. When not supplied, the tip block "
            "height in chainActive will be used.\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("votedelegatetx",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 10000 ") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("votedelegatetx",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 10000 "));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fee    = params[2].get_uint64();  // real type
    int32_t height     = 0;
    if (params.size() > 3) {
        height = params[3].get_int();
    }
    Array arrVotes = params[1].get_array();

    CKeyID keyId;
    if (!GetKeyId(sendAddr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }
    CDelegateVoteTx delegateVoteTx;
    assert(pWalletMain != nullptr);
    {
        EnsureWalletIsUnlocked();
        CAccount account;

        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
        }

        if (!account.HaveOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        delegateVoteTx.llFees = fee;
        if (0 != height) {
            delegateVoteTx.valid_height = height;
        } else {
            delegateVoteTx.valid_height = chainActive.Height();
        }
        delegateVoteTx.txUid = account.regid;

        for (auto objVote : arrVotes) {
            const Value& delegateAddr  = find_value(objVote.get_obj(), "delegate");
            const Value& delegateVotes = find_value(objVote.get_obj(), "votes");
            if (delegateAddr.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Vote fund address error or fund value error");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddr.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address error");
            }
            CAccount delegateAcct;
            if (!pCdMan->pAccountCache->GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address is not exist");
            }
            if (!delegateAcct.HaveOwnerPubKey()) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Delegate address is unregistered");
            }

            VoteType voteType = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN : VoteType::MINUS_BCOIN;
            CUserID candidateUid = CUserID(delegateAcct.regid);
            uint64_t bcoins = (uint64_t)abs(delegateVotes.get_int64());
            CCandidateVote candidateVote(voteType, candidateUid, bcoins);

            delegateVoteTx.candidateVotes.push_back(candidateVote);
        }

        if (!pWalletMain->Sign(keyId, delegateVoteTx.ComputeSignatureHash(), delegateVoteTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx*)&delegateVoteTx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object objRet;
    objRet.push_back(Pair("txid", std::get<1>(ret)));
    return objRet;
}

// create a vote delegate raw transaction
Value genvotedelegateraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() <  3  || params.size() > 4) {
        throw runtime_error(
            "genvotedelegateraw \"addr\" \"candidate votes\" \"fees\" [height]\n"
            "\ncreate a vote delegate raw transaction\n"
            "\nArguments:\n"
            "1.\"addr\":                    (string required) from address that votes delegate(s)\n"
            "2. \"candidate votes\"         (string, required) A json array of json oper vote to delegates\n"
            " [\n"
            " {\n"
            "    \"delegate\":\"address\",  (string, required) The transaction id\n"
            "    \"votes\":n                (numeric, required) votes\n"
            " }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fees\":                    (numeric required) pay to miner\n"
            "4.\"height\":                  (numeric optional) valid height, If not provide, use the active chain's block height"
            "\nResult:\n"
            "\"rawtx\":                     (string) raw transaction string\n"
            "\nExamples:\n" +
            HelpExampleCli("genvotedelegateraw",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 10000") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("genvotedelegateraw",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 10000"));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fees   = params[2].get_uint64();
    int32_t height = chainActive.Height();
    if (params.size() > 3) {
        height = params[3].get_int();
        if (height <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
        }
    }
    Array arrVotes = params[1].get_array();

    CKeyID keyId;
    if (!GetKeyId(sendAddr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }
    CDelegateVoteTx delegateVoteTx;
    assert(pWalletMain != nullptr);
    {
        EnsureWalletIsUnlocked();
        CAccount account;

        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
        }

        if (!account.HaveOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fees) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        delegateVoteTx.llFees       = fees;
        delegateVoteTx.valid_height = height;
        delegateVoteTx.txUid        = account.regid;

        for (auto objVote : arrVotes) {

            const Value& delegateAddress = find_value(objVote.get_obj(), "delegate");
            const Value& delegateVotes   = find_value(objVote.get_obj(), "votes");
            if (delegateAddress.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Voted delegator's address type "
                                   "error or vote value error");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddress.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Voted delegator's address error");
            }
            CAccount delegateAcct;
            if (!pCdMan->pAccountCache->GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Voted delegator's address is unregistered");
            }

            VoteType voteType    = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN : VoteType::MINUS_BCOIN;
            CUserID candidateUid = CUserID(delegateAcct.owner_pubkey);
            uint64_t bcoins      = (uint64_t)abs(delegateVotes.get_int64());
            CCandidateVote candidateVote(voteType, candidateUid, bcoins);

            delegateVoteTx.candidateVotes.push_back(candidateVote);
        }

        if (!pWalletMain->Sign(keyId, delegateVoteTx.ComputeSignatureHash(), delegateVoteTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = delegateVoteTx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value listaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "listaddr\n"
            "\nreturn Array containing address,balance,haveminerkey,regid information.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listaddr", "")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listaddr", ""));
    }

    Array retArray;
    assert(pWalletMain != nullptr);
    {
        set<CKeyID> setKeyId;
        pWalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0) {
            return retArray;
        }
        CAccountDBCache accView(*pCdMan->pAccountCache);

        for (const auto &keyId : setKeyId) {
            CUserID userId(keyId);
            CAccount account;
            pCdMan->pAccountCache->GetAccount(userId, account);
            CKeyCombi keyCombi;
            pWalletMain->GetKeyCombi(keyId, keyCombi);

            Object obj;
            obj.push_back(Pair("addr",  keyId.ToAddress()));
            obj.push_back(Pair("regid", account.regid.ToString()));

            Object tokenMapObj;
            for (auto tokenPair : account.tokens) {
                Object tokenObj;
                const CAccountToken& token = tokenPair.second;
                tokenObj.push_back(Pair("free_amount",      token.free_amount));
                tokenObj.push_back(Pair("staked_amount",    token.staked_amount));
                tokenObj.push_back(Pair("frozen_amount",    token.frozen_amount));

                tokenMapObj.push_back(Pair(tokenPair.first, tokenObj));
            }

            obj.push_back(Pair("tokens",        tokenMapObj));
            obj.push_back(Pair("hasminerkey",   keyCombi.HaveMinerKey()));

            retArray.push_back(obj);
        }
    }

    return retArray;
}

Value listtx(const Array& params, bool fHelp) {
if (fHelp || params.size() > 2) {
        throw runtime_error("listtx\n"
                "\nget all confirmed transactions and all unconfirmed transactions from wallet.\n"
                "\nArguments:\n"
                "1. count          (numeric, optional, default=10) The number of transactions to return\n"
                "2. from           (numeric, optional, default=0) The number of transactions to skip\n"
                "\nExamples:\n"
                "\nResult:\n"
                "\nExamples:\n"
                "\nList the most recent 10 transactions in the system\n"
                + HelpExampleCli("listtx", "") +
                "\nList transactions 100 to 120\n"
                + HelpExampleCli("listtx", "20 100")
            );
    }

    Object retObj;
    int32_t nDefCount = 10;
    int32_t nFrom = 0;
    if(params.size() > 0) {
        nDefCount = params[0].get_int();
    }
    if(params.size() > 1) {
        nFrom = params[1].get_int();
    }
    assert(pWalletMain != nullptr);

    Array confirmedTxArray;
    int32_t nCount = 0;
    map<int32_t, uint256, std::greater<int32_t> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != nullptr)
            blockInfoMap.insert(make_pair(pIndex->height, wtx.first));
    }
    bool bUpLimited = false;
    for (auto const &blockInfo : blockInfoMap) {
        CAccountTx accountTx = pWalletMain->mapInBlockTx[blockInfo.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (nFrom-- > 0)
                continue;
            if (++nCount > nDefCount) {
                bUpLimited = true;
                break;
            }
            confirmedTxArray.push_back(item.first.GetHex());
        }
        if (bUpLimited) {
            break;
        }
    }
    retObj.push_back(Pair("confirmed_tx", confirmedTxArray));

    Array unconfirmedTxArray;
    for (auto const &tx : pWalletMain->unconfirmedTx) {
        unconfirmedTxArray.push_back(tx.first.GetHex());
    }
    retObj.push_back(Pair("unconfirmed_tx", unconfirmedTxArray));

    return retObj;
}

Value getaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getaccountinfo \"addr\"\n"
            "\nget account information\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required) account base58 address"
            "Returns account details.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\""));
    }
    RPCTypeCheck(params, list_of(str_type));
    CKeyID keyId;
    CUserID userId;
    string addr = params[0].get_str();
    if (!GetKeyId(addr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    userId = keyId;
    Object obj;
    bool found = false;

    CAccount account;
    if (pCdMan->pAccountCache->GetAccount(userId, account)) {
        if (!account.owner_pubkey.IsValid()) {
            CPubKey pubKey;
            CPubKey minerPubKey;
            if (pWalletMain->GetPubKey(keyId, pubKey)) {
                pWalletMain->GetPubKey(keyId, minerPubKey, true);
                account.owner_pubkey = pubKey;
                account.keyid        = pubKey.GetKeyId();
                if (pubKey != minerPubKey && !account.miner_pubkey.IsValid()) {
                    account.miner_pubkey = minerPubKey;
                }
            }
        }
        obj = account.ToJsonObj();
        obj.push_back(Pair("position", "inblock"));

        found = true;
    } else {  // unregistered keyId
        CPubKey pubKey;
        CPubKey minerPubKey;
        if (pWalletMain->GetPubKey(keyId, pubKey)) {
            pWalletMain->GetPubKey(keyId, minerPubKey, true);
            account.owner_pubkey = pubKey;
            account.keyid        = pubKey.GetKeyId();
            if (minerPubKey != pubKey) {
                account.miner_pubkey = minerPubKey;
            }
            obj = account.ToJsonObj();
            obj.push_back(Pair("position", "inwallet"));

            found = true;
        }
    }

    if (found) {
        int32_t height                 = chainActive.Height();
        uint64_t slideWindowBlockCount = 0;
        pCdMan->pSysParamCache->GetParam(SysParamType::MEDIAN_PRICE_SLIDE_WINDOW_BLOCKCOUNT, slideWindowBlockCount);
        uint64_t bcoinMedianPrice = pCdMan->pPpCache->GetBcoinMedianPrice(height, slideWindowBlockCount);
        Array cdps;
        vector<CUserCDP> userCdps;
        if (pCdMan->pCdpCache->GetCDPList(account.regid, userCdps)) {
            for (auto& cdp : userCdps) {
                cdps.push_back(cdp.ToJson(bcoinMedianPrice));
            }
        }

        obj.push_back(Pair("cdp_list", cdps));
    }

    return obj;
}

//list unconfirmed transaction of mine
Value listunconfirmedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
         throw runtime_error("listunconfirmedtx \n"
                "\nget the list  of unconfirmedtx.\n"
                "\nArguments:\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listunconfirmedtx", "")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("listunconfirmedtx", ""));
    }

    Object retObj;
    Array unconfirmedTxArray;

    for (auto const& tx : pWalletMain->unconfirmedTx) {
        unconfirmedTxArray.push_back(tx.first.GetHex());
    }

    retObj.push_back(Pair("unconfirmed_tx", unconfirmedTxArray));

    return retObj;
}

static Value TestDisconnectBlock(int32_t number) {
    CBlock block;
    Object obj;

    CValidationState state;
    if ((chainActive.Height() - number) < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "restclient Error: number");
    }
    if (number > 0) {
        do {
            CBlockIndex * pTipIndex = chainActive.Tip();
            LogPrint("DEBUG", "current height:%d\n", pTipIndex->height);
            if (!DisconnectBlockFromTip(state))
                return false;
            chainMostWork.SetTip(pTipIndex->pprev);
            if (!EraseBlockIndexFromSet(pTipIndex))
                return false;
            if (!pCdMan->pBlockTreeDb->EraseBlockIndex(pTipIndex->GetBlockHash()))
                return false;
            mapBlockIndex.erase(pTipIndex->GetBlockHash());
        } while (--number);
    }

    obj.push_back(Pair("tip", strprintf("hash:%s hight:%s",chainActive.Tip()->GetBlockHash().ToString(),chainActive.Height())));
    return obj;
}

Value disconnectblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("disconnectblock \"numbers\"\n"
                "\ndisconnect block\n"
                "\nArguments:\n"
                "1. \"numbers \"  (numeric, required) the block numbers.\n"
                "\nResult:\n"
                "\"disconnect result\"  (bool) \n"
                "\nExamples:\n"
                + HelpExampleCli("disconnectblock", "\"1\"")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("disconnectblock", "\"1\""));
    }
    int32_t number = params[0].get_int();

    Value te = TestDisconnectBlock(number);

    return te;
}

Value listcontracts(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "listcontracts \"show detail\"\n"
            "\nget the list of all contracts\n"
            "\nArguments:\n"
            "1. show detail  (boolean, required) show contract in detail if true.\n"
            "\nReturn an object contains all contracts\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("listcontracts", "true") + "\nAs json rpc call\n" + HelpExampleRpc("listcontracts", "true"));
    }

    bool showDetail = params[0].get_bool();

    map<string, CUniversalContract> contracts;
    if (!pCdMan->pContractCache->GetContracts(contracts)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to acquire contracts from db.");
    }

    Object obj;
    Array contractArray;
    for (const auto &item : contracts) {
        Object contractObject;
        const CUniversalContract &contract = item.second;
        CRegID regid(item.first);
        contractObject.push_back(Pair("contract_regid", regid.ToString()));
        contractObject.push_back(Pair("memo",           HexStr(contract.memo)));

        if (showDetail) {
            contractObject.push_back(Pair("vm_type",    contract.vm_type));
            contractObject.push_back(Pair("upgradable", contract.upgradable));
            contractObject.push_back(Pair("code",       HexStr(contract.code)));
            contractObject.push_back(Pair("abi",        contract.abi));
        }

        contractArray.push_back(contractObject);
    }

    obj.push_back(Pair("count",     contracts.size()));
    obj.push_back(Pair("contracts", contractArray));

    return obj;
}

Value getcontractinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getcontractinfo \"contract regid\"\n"
            "\nget contract information.\n"
            "\nArguments:\n"
            "1. \"contract regid\"    (string, required) the contract regid.\n"
            "\nReturn an object contains contract information\n"
            "\nExamples:\n" +
            HelpExampleCli("getcontractinfo", "1-1") + "\nAs json rpc call\n" +
            HelpExampleRpc("getcontractinfo", "1-1"));

    CRegID regid(params[0].get_str());
    if (regid.IsEmpty() || !pCdMan->pContractCache->HaveContract(regid)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid contract regid.");
    }

    CUniversalContract contract;
    if (!pCdMan->pContractCache->GetContract(regid, contract)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to acquire contract from db.");
    }

    Object obj;
    obj.push_back(Pair("contract_regid",    regid.ToString()));
    obj.push_back(Pair("vm_type",           contract.vm_type));
    obj.push_back(Pair("upgradable",        contract.upgradable));
    obj.push_back(Pair("code",              HexStr(contract.code)));
    obj.push_back(Pair("memo",              HexStr(contract.memo)));
    obj.push_back(Pair("abi",               contract.abi));

    return obj;
}

Value generateblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("generateblock \"addr\"\n"
            "\ncreate a block with the appointed address\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required)\n"
            "\nResult:\n"
            "\nblockhash\n"
            "\nExamples:\n" +
            HelpExampleCli("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\""));
    }
    //get keyId
    CKeyID keyId;

    if (!GetKeyId(params[0].get_str(), keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "in generateblock :address err");

    Object obj;
    // obj.push_back(Pair("blockhash", hash.GetHex()));
    return obj;
}

Value listtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("listtxcache\n"
                "\nget all transactions in cache\n"
                "\nArguments:\n"
                "\nResult:\n"
                "\"txcache\"  (string)\n"
                "\nExamples:\n" + HelpExampleCli("listtxcache", "")+ HelpExampleRpc("listtxcache", ""));
    }
    const map<uint256, UnorderedHashSet> &mapBlockTxHashSet = pCdMan->pTxCache->GetTxHashCache();

    Array retTxHashArray;
    for (auto &item : mapBlockTxHashSet) {
        Object blockObj;
        Array txHashArray;
        blockObj.push_back(Pair("blockhash", item.first.GetHex()));
        for (auto &txid : item.second)
            txHashArray.push_back(txid.GetHex());
        blockObj.push_back(Pair("txcache", txHashArray));
        retTxHashArray.push_back(blockObj);
    }

    return retTxHashArray;
}

Value reloadtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("reloadtxcache \n"
            "\nreload transactions catch\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("reloadtxcache", "")
            + HelpExampleRpc("reloadtxcache", ""));
    }
    pCdMan->pTxCache->Clear();
    CBlockIndex *pIndex = chainActive.Tip();
    if ((chainActive.Height() - SysCfg().GetTxCacheHeight()) >= 0) {
        pIndex = chainActive[(chainActive.Height() - SysCfg().GetTxCacheHeight())];
    } else {
        pIndex = chainActive.Genesis();
    }
    CBlock block;
    do {
        if (!ReadBlockFromDisk(pIndex, block))
            return ERRORMSG("reloadtxcache() : *** ReadBlockFromDisk failed at %d, hash=%s",
                pIndex->height, pIndex->GetBlockHash().ToString());

        pCdMan->pTxCache->AddBlockToCache(block);
        pIndex = chainActive.Next(pIndex);
    } while (nullptr != pIndex);

    Object obj;
    obj.push_back(Pair("info", "reload tx cache succeed"));
    return obj;
}

Value getcontractdata(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error(
            "getcontractdata \"contract regid\" \"key\" [hexadecimal]\n"
            "\nget contract data with key\n"
            "\nArguments:\n"
            "1.\"contract regid\":      (string, required) contract regid\n"
            "2.\"key\":                 (string, required)\n"
            "3.\"hexadecimal format\":  (boolean, optional) in hexadecimal if true, otherwise in plaintext, default to "
            "false\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getcontractdata", "\"1304166-1\" \"key\" true") + "\nAs json rpc call\n" +
            HelpExampleRpc("getcontractdata", "\"1304166-1\", \"key\", true"));
    }

    CRegID regId(params[0].get_str());
    if (regId.IsEmpty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid contract regid");
    }

    bool hexadecimal = params.size() > 2 ? params[2].get_bool() : false;
    string key;
    if (hexadecimal) {
        vector<uint8_t> hexKey = ParseHex(params[1].get_str());
        key                          = string(hexKey.begin(), hexKey.end());
    } else {
        key = params[1].get_str();
    }
    string value;
    if (!pCdMan->pContractCache->GetContractData(regId, key, value)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to acquire contract data");
    }

    Object obj;
    obj.push_back(Pair("contract_regid",    regId.ToString()));
    obj.push_back(Pair("key",               hexadecimal ? HexStr(key) : key));
    obj.push_back(Pair("value",             hexadecimal ? HexStr(value) : value));

    return obj;
}

Value saveblocktofile(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "saveblocktofile \"blockhash\" \"filepath\"\n"
            "\n save the given block info to the given file\n"
            "\nArguments:\n"
            "1.\"blockhash\": (string, required)\n"
            "2.\"filepath\": (string, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("saveblocktofile",
                           "\"c78d162b40625cc8b088fa88302e0e4f08aba0d1c92612e9dd14e77108cbc11a\" \"block.log\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("saveblocktofile",
                           "\"c78d162b40625cc8b088fa88302e0e4f08aba0d1c92612e9dd14e77108cbc11a\", \"block.log\""));
    }
    string strblockhash = params[0].get_str();
    uint256 blockHash(uint256S(params[0].get_str()));
    if(0 == mapBlockIndex.count(blockHash)) {
        throw JSONRPCError(RPC_MISC_ERROR, "block hash is not exist!");
    }
    CBlockIndex *pIndex = mapBlockIndex[blockHash];
    CBlock blockInfo;
    if (!pIndex || !ReadBlockFromDisk(pIndex, blockInfo))
        throw runtime_error(_("Failed to read block"));
    assert(strblockhash == blockInfo.GetHash().ToString());
    string file = params[1].get_str();
    try {
        FILE* fp = fopen(file.c_str(), "wb+");
        CAutoFile fileout = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
        if (!fileout)
            throw JSONRPCError(RPC_MISC_ERROR, "open file:" + strblockhash + "failed!");
        if(chainActive.Contains(pIndex))
            fileout << pIndex->height;
        fileout << blockInfo;
        fflush(fileout);
    } catch (std::exception &e) {
        throw JSONRPCError(RPC_MISC_ERROR, "save block to file error");
    }
    return "save succeed";
}

Value genregisteraccountraw(const Array& params, bool fHelp) {
    if (fHelp || (params.size() < 3 || params.size() > 4)) {
        throw runtime_error(
            "genregisteraccountraw \"fee\" \"height\" \"publickey\" (\"minerpublickey\") \n"
            "\ncreate a register account transaction\n"
            "\nArguments:\n"
            "1.fee: (numeric, required) pay to miner\n"
            "2.height: (numeric, required)\n"
            "3.publickey: (string, required)\n"
            "4.minerpublickey: (string, optional)\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli(
                "genregisteraccountraw",
                "10000  3300 "
                "\"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\" "
                "\"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc(
                "genregisteraccountraw",
                " 10000, 3300, "
                "\"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\", "
                "\"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\""));
    }
    CUserID userId  = CNullID();
    CUserID minerId = CNullID();

    int64_t fee         = AmountToRawValue(params[0]);
    int64_t nDefaultFee = SysCfg().GetTxFee();

    if (fee < nDefaultFee) {
        throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                           strprintf("Input fee smaller than mintxfee: %ld sawi", nDefaultFee));
    }

    int32_t height = params[1].get_int();
    if (height <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
    }

    CPubKey pubKey = CPubKey(ParseHex(params[2].get_str()));
    if (!pubKey.IsCompressed() || !pubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key");
    }
    userId = pubKey;

    if (params.size() > 3) {
        CPubKey minerPubKey = CPubKey(ParseHex(params[3].get_str()));
        if (!minerPubKey.IsCompressed() || !minerPubKey.IsFullyValid()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key");
        }
        minerId = minerPubKey;
    }

    EnsureWalletIsUnlocked();
    std::shared_ptr<CAccountRegisterTx> tx =
        std::make_shared<CAccountRegisterTx>(userId, minerId, fee, height);
    if (!pWalletMain->Sign(pubKey.GetKeyId(), tx->ComputeSignatureHash(), tx->signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
    }
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value sendtxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "sendtxraw \"transaction\" \n"
            "\nsend raw transaction\n"
            "\nArguments:\n"
            "1.\"transaction\": (string, required)\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtxraw", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("sendtxraw", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\""));
    }
    vector<uint8_t> vch(ParseHex(params[0].get_str()));
    if (vch.size() > MAX_RPC_SIG_STR_LEN) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The rawtx str is too long");
    }

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);

    std::shared_ptr<CBaseTx> tx;
    stream >> tx;
    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) tx.get());
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, "sendtxraw error: " + std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}

Value gencallcontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "gencallcontractraw \"sender addr\" \"app regid\" \"amount\" \"contract\" \"fee\" (\"height\")\n"
            "\ncreate contract invocation raw transaction\n"
            "\nArguments:\n"
            "1.\"sender addr\": (string, required)\n tx sender's base58 addr\n"
            "2.\"app regid\":(string, required) contract RegId\n"
            "3.\"arguments\": (string, required) contract arguments (Hex encode required)\n"
            "4.\"amount\":(numeric, required)\n amount of WICC to be sent to the contract account\n"
            "5.\"fee\": (numeric, required) pay to miner\n"
            "6.\"height\": (numeric, optional)create height,If not provide use the tip block height in "
            "chainActive\n"
            "\nResult:\n"
            "\"rawtx\"  (string) The raw transaction\n"
            "\nExamples:\n" +
            HelpExampleCli("gencallcontractraw",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("gencallcontractraw",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 100"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(str_type)(int_type)(int_type));

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    }

    string arguments = ParseHexStr(params[2].get_str());
    if (arguments.size() >= MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size out of range");
    }

    int64_t amount = AmountToRawValue(params[3]);
    int64_t fee    = AmountToRawValue(params[4]);

    int32_t height = chainActive.Height();
    if (params.size() > 5)
        height = params[5].get_int();

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) && sendRegId.IsMature(chainActive.Height()))
            ? CUserID(sendRegId)
            : CUserID(sendPubKey);

    CRegID recvRegId;
    if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    }

    if (!pCdMan->pContractCache->HaveContract(recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }

    CLuaContractInvokeTx tx;
    tx.nTxType      = LCONTRACT_INVOKE_TX;
    tx.txUid        = sendUserId;
    tx.app_uid      = recvRegId;
    tx.coin_amount  = amount;
    tx.llFees       = fee;
    tx.arguments    = arguments;
    tx.valid_height = height;

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

    return obj;
}

Value genregistercontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error(
            "genregistercontractraw \"addr\" \"filepath\" \"fee\"  \"height\" (\"script description\")\n"
            "\nregister script\n"
            "\nArguments:\n"
            "1.\"addr\":                (string required)\n from address that registers the contract"
            "2.\"filepath\":            (string required), script's file path\n"
            "3.\"fee\":                 (numeric required) pay to miner\n"
            "4.\"height\":              (number optional) valid height\n"
            "5.\"script description\":  (string optional) new script description\n"
            "\nResult:\n"
            "\"txid\":                  (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("genregistercontractraw",
                    "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/hello.lua\" \"10000\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("genregistercontractraw",
                    "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"/tmp/lua/hello.lua\", \"10000\" "));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(int_type)(str_type));

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, LUA_CONTRACT_LOCATION_PREFIX.size(), LUA_CONTRACT_LOCATION_PREFIX.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw runtime_error("genregistercontractraw open App Lua Script file" + luaScriptFilePath + "error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == nullptr) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }

    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);
        fclose(file);
        throw runtime_error("read contract script file error");
    } else {
        fclose(file);
    }
    string code(buffer, lSize);

    if (buffer) {
        free(buffer);
    }

    string memo;
    if (params.size() > 4) {
        memo = params[4].get_str();
    }

    uint64_t fee = params[2].get_uint64();
    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fee smaller than nMinTxFee");
    }
    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Recv address invalid");
    }

    std::shared_ptr<CAccountDBCache> pAccountCache(new CAccountDBCache(pCdMan->pAccountCache));
    CAccount account;
    CUserID userId = keyId;
    if (!pAccountCache->GetAccount(userId, account)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
    }
    if (!account.HaveOwnerPubKey()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
    }

    std::shared_ptr<CLuaContractDeployTx> tx = std::make_shared<CLuaContractDeployTx>();
    CRegID regId;
    pAccountCache->GetRegId(keyId, regId);

    tx->txUid          = regId;
    tx->contract.code  = code;
    tx->contract.memo  = memo;
    tx->llFees         = fee;

    int32_t height = chainActive.Height();
    if (params.size() > 3) {
        height =  params[3].get_int();
        if (height <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
        }
    }
    tx.get()->valid_height = height;

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;

    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

    return obj;
}

Value signtxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "signtxraw \"str\" \"addr\"\n"
            "\nsignature transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) Hex-format string, no longer than 65K in binary bytes\n"
            "2.\"addr\": (string, required) A json array of WICC addresses\n"
            "[\n"
            "  \"address\"  (string) WICC address\n"
            "  ...,\n"
            "]\n"
            "\nExamples:\n" +
            HelpExampleCli("signtxraw",
                           "\"0701ed7f0300030000010000020002000bcd10858c200200\" "
                           "\"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("signtxraw",
                           "\"0701ed7f0300030000010000020002000bcd10858c200200\", "
                           "\"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\""));
    }

    vector<uint8_t> vch(ParseHex(params[0].get_str()));
    if (vch.size() > MAX_RPC_SIG_STR_LEN) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The sig str is too long");
    }

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get()) {
        return Value::null;
    }

    const Array& addresses = params[1].get_array();
    if (pBaseTx.get()->nTxType != BCOIN_TRANSFER_MTX && addresses.size() != 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "To many addresses provided");
    }

    std::set<CKeyID> keyIds;
    CKeyID keyId;
    for (uint32_t i = 0; i < addresses.size(); i++) {
        if (!GetKeyId(addresses[i].get_str(), keyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get keyId");
        }
        keyIds.insert(keyId);
    }

    if (keyIds.empty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No valid address provided");
    }

    Object obj;

    switch (pBaseTx.get()->nTxType) {
        case BLOCK_REWARD_TX:
        case UCOIN_REWARD_TX:
        case UCOIN_BLOCK_REWARD_TX: {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Reward transation is forbidden");
        }
        case BCOIN_TRANSFER_MTX: {
            CMulsigTx *pTx = dynamic_cast<CMulsigTx*>(pBaseTx.get());

            vector<CSignaturePair>& signaturePairs = pTx->signaturePairs;
            for (const auto& keyIdItem : keyIds) {
                CRegID regId;
                if (!pCdMan->pAccountCache->GetRegId(CUserID(keyIdItem), regId)) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Address is unregistered");
                }

                bool valid = false;
                for (auto& signatureItem : signaturePairs) {
                    if (regId == signatureItem.regid) {
                        if (!pWalletMain->Sign(keyIdItem, pTx->ComputeSignatureHash(),
                                               signatureItem.signature)) {
                            throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
                        } else {
                            valid = true;
                        }
                    }
                }

                if (!valid) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Provided address is unmatched");
                }
            }

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        default: {
            if (!pWalletMain->Sign(*keyIds.begin(), pBaseTx->ComputeSignatureHash(), pBaseTx->signature))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
        }
    }
    return obj;
}

Value decodemulsigscript(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "decodemulsigscript \"hex\"\n"
            "\nDecode a hex-encoded script.\n"
            "\nArguments:\n"
            "1. \"hex\"     (string) the hex encoded mulsig script\n"
            "\nResult:\n"
            "{\n"
            "  \"type\":\"type\", (string) The transaction type\n"
            "  \"reqSigs\": n,    (numeric) The required signatures\n"
            "  \"addr\",\"address\" (string) mulsig script address\n"
            "  \"addresses\": [   (json array of string)\n"
            "     \"address\"     (string) bitcoin address\n"
            "     ,...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("decodemulsigscript", "\"hexstring\"") +
            HelpExampleRpc("decodemulsigscript", "\"hexstring\""));

    RPCTypeCheck(params, list_of(str_type));

    vector<uint8_t> multiScript = ParseHex(params[0].get_str());
    if (multiScript.empty() || multiScript.size() > MAX_MULSIG_SCRIPT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script size");
    }

    CDataStream ds(multiScript, SER_DISK, CLIENT_VERSION);
    CMulsigScript script;
    try {
        ds >> script;
    } catch (std::exception& e) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script content");
    }

    CKeyID scriptId           = script.GetID();
    int8_t required           = (int8_t)script.GetRequired();
    std::set<CPubKey> pubKeys = script.GetPubKeys();

    Array addressArray;
    for (const auto& pubKey : pubKeys) {
        addressArray.push_back(pubKey.GetKeyId().ToAddress());
    }

    Object obj;
    obj.push_back(Pair("type", "mulsig"));
    obj.push_back(Pair("req_sigs", required));
    obj.push_back(Pair("addr", scriptId.ToAddress()));
    obj.push_back(Pair("addresses", addressArray));

    return obj;
}

Value decodetxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "decodetxraw \"hexstring\"\n"
            "\ndecode transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) hexstring\n"
            "\nExamples:\n" +
            HelpExampleCli("decodetxraw",
                           "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f2"
                           "2af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea46"
                           "7d02d2d2c3db62a1addaa8009ccd\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("decodetxraw",
                           "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f2"
                           "2af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea46"
                           "7d02d2d2c3db62a1addaa8009ccd\""));
    }
    Object obj;
    vector<uint8_t> vch(ParseHex(params[0].get_str()));
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get()) {
        return obj;
    }
    obj = pBaseTx->ToJson(*pCdMan->pAccountCache);
    return obj;
}

Value getalltxinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 0 && params.size() != 1)) {
        throw runtime_error("getalltxinfo \n"
            "\nget all transaction info\n"
            "\nArguments:\n"
            "1.\"nlimitCount\": (numeric, optional, default=0) 0 return all tx, else return number of nlimitCount txs \n"
            "\nResult:\n"
            "\nExamples:\n" + HelpExampleCli("getalltxinfo", "") + "\nAs json rpc call\n"
            + HelpExampleRpc("getalltxinfo", ""));
    }

    Object retObj;
    int32_t nLimitCount(0);
    if(params.size() == 1)
        nLimitCount = params[0].get_int();
    assert(pWalletMain != nullptr);
    if (nLimitCount <= 0) {
        Array confirmedTx;
        for (auto const &wtx : pWalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                confirmedTx.push_back(objtx);
            }
        }
        retObj.push_back(Pair("confirmed", confirmedTx));

        Array unconfirmedTx;
        for (auto const &wtx : pWalletMain->unconfirmedTx) {
            Object objtx = GetTxDetailJSON(wtx.first);
            unconfirmedTx.push_back(objtx);
        }
        retObj.push_back(Pair("unconfirmed", unconfirmedTx));
    } else {
        Array confirmedTx;
        multimap<int32_t, Object, std::greater<int32_t> > mapTx;
        for (auto const &wtx : pWalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                int32_t nConfHeight = find_value(objtx, "confirmedheight").get_int();
                mapTx.insert(pair<int32_t, Object>(nConfHeight, objtx));
            }
        }
        int32_t nSize(0);
        for(auto & txItem : mapTx) {
            if(++nSize > nLimitCount)
                break;
            confirmedTx.push_back(txItem.second);
        }
        retObj.push_back(Pair("confirmed", confirmedTx));
    }

    return retObj;
}

Value getcontractaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error(
            "getcontractaccountinfo \"contract regid\" \"account address or regid\""
            "\nget contract account info\n"
            "\nArguments:\n"
            "1.\"contract regid\":              (string, required) contract regid\n"
            "2.\"account address or regid\":    (string, required) contract account address or its regid\n"
            "3.\"minconf\"                      (numeric, optional, default=1) Only include contract transactions "
            "confirmed\n"
            "\nExamples:\n" +
            HelpExampleCli("getcontractaccountinfo", "\"452974-3\" \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("getcontractaccountinfo", "\"452974-3\", \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\""));
    }

    string strAppRegId = params[0].get_str();
    if (!CRegID::IsSimpleRegIdStr(strAppRegId))
        throw runtime_error("getcontractaccountinfo: invalid contract regid: " + strAppRegId);

    CRegID appRegId(strAppRegId);
    string acctKey;
    if (CRegID::IsSimpleRegIdStr(params[1].get_str())) {
        CRegID acctRegId(params[1].get_str());
        CUserID acctUserId(acctRegId);
        acctKey = RegIDToAddress(acctUserId);
    } else { //in wicc address format
        acctKey = params[1].get_str();
    }

    std::shared_ptr<CAppUserAccount> appUserAccount = std::make_shared<CAppUserAccount>();
    if (params.size() == 3 && params[2].get_int() == 0) {
        if (!mempool.cw->contractCache.GetContractAccount(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    } else {
        if (!pCdMan->pContractCache->GetContractAccount(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    }
    appUserAccount.get()->AutoMergeFreezeToFree(chainActive.Height());

    return Value(appUserAccount.get()->ToJson());
}

Value listcontractassets(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("listcontractassets regid\n"
            "\nreturn Array containing address, asset information.\n"
            "\nArguments: regid: Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listcontractassets", "1-1")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listcontractassets", "1-1"));
    }

    if (!CRegID::IsSimpleRegIdStr(params[0].get_str()))
        throw runtime_error("in listcontractassets :regid is invalid!\n");

    CRegID script(params[0].get_str());

    Array retArray;
    assert(pWalletMain != nullptr);
    {
        set<CKeyID> setKeyId;
        pWalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0)
            return retArray;

        CContractDBCache contractScriptTemp(*pCdMan->pContractCache);

        for (const auto &keyId : setKeyId) {

            string key = keyId.ToAddress();

            std::shared_ptr<CAppUserAccount> tem = std::make_shared<CAppUserAccount>();
            if (!contractScriptTemp.GetContractAccount(script, key, *tem.get())) {
                tem = std::make_shared<CAppUserAccount>(key);
            }
            tem.get()->AutoMergeFreezeToFree(chainActive.Height());

            Object obj;
            obj.push_back(Pair("addr", key));
            obj.push_back(Pair("asset", (double) tem.get()->GetBcoins() / (double) COIN));
            retArray.push_back(obj);
        }
    }

    return retArray;
}


Value gethash(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("gethash  \"str\"\n"
            "\nget the hash of given str\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) \n"
            "\nresult an object \n"
            "\nExamples:\n"
            + HelpExampleCli("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\""));
    }

    string str = params[0].get_str();
    vector<uint8_t> vTemp;
    vTemp.assign(str.c_str(), str.c_str() + str.length());
    uint256 strhash = Hash(vTemp.begin(), vTemp.end());
    Object obj;
    obj.push_back(Pair("txid", strhash.ToString()));
    return obj;

}

Value validateaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddr \"wicc_address\"\n"
            "\ncheck whether address is valid or not\n"
            "\nArguments:\n"
            "1.\"wicc_address\"     (string, required) WICC address\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\"") +
            HelpExampleRpc("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\""));
    Object obj;

    string addr = params[0].get_str();
    CKeyID keyid;
    if (!GetKeyId(addr, keyid)) {
        obj.push_back(Pair("is_valid", false));
    } else {
        obj.push_back(Pair("is_valid", true));
    }

    return obj;
}

Value gettotalcoins(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0) {
        throw runtime_error(
            "gettotalcoins \n"
            "\nget the total number of circulating coins excluding those locked for votes\n"
            "\nand the toal number of registered addresses\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalcoins", "")
            + HelpExampleRpc("gettotalcoins", ""));
    }

    Object obj;
    {
        uint64_t totalCoins(0);
        uint64_t totalRegIds(0);
        std::tie(totalCoins, totalRegIds) = pCdMan->pAccountCache->TraverseAccount();
        // auto [totalCoins, totalRegIds] = pCdMan->pAccountCache->TraverseAccount(); //C++17
        obj.push_back( Pair("total_coins", ValueFromAmount(totalCoins)) );
        obj.push_back( Pair("total_regids", totalRegIds) );
    }
    return obj;
}

Value gettotalassets(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 1) {
        throw runtime_error("gettotalassets \n"
            "\nget all assets belonging to a contract\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalassets", "11-1")
            + HelpExampleRpc("gettotalassets", "11-1"));
    }
    CRegID regId(params[0].get_str());
    if (regId.IsEmpty() == true)
        throw runtime_error("contract regid invalid!\n");

    if (!pCdMan->pContractCache->HaveContract(regId))
        throw runtime_error("contract regid not exist!\n");

    Object obj;
    {
        map<string, string> mapAcc;
        bool bRet = pCdMan->pContractCache->GetContractAccounts(regId, mapAcc);
        if (bRet) {
            uint64_t totalassets = 0;
            for (auto & it : mapAcc) {
                CAppUserAccount appAccOut;
                CDataStream ds(it.second, SER_DISK, CLIENT_VERSION);
                ds >> appAccOut;

                totalassets += appAccOut.GetBcoins();
                totalassets += appAccOut.GetAllFreezedValues();
            }

            obj.push_back(Pair("total_assets", ValueFromAmount(totalassets)));
        } else
            throw runtime_error("failed to find contract account!\n");
    }
    return obj;
}

Value listdelegates(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1) {
        throw runtime_error(
                "listdelegates \n"
                "\nreturns the specified number delegates by reversed order voting number.\n"
                "\nArguments:\n"
                "1. number           (number, optional) the number of the delegates, default to all delegates.\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listdelegates", "11")
                + HelpExampleRpc("listdelegates", "11"));
    }

    int32_t delegateNum = (params.size() == 1) ? params[0].get_int() : IniCfg().GetTotalDelegateNum();
    if (delegateNum < 1 || delegateNum > 11) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("Delegate number not between 1 and %u", IniCfg().GetTotalDelegateNum()));
    }

    vector<CRegID> delegatesList;
    if (!pCdMan->pDelegateCache->GetTopDelegateList(delegatesList)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get delegates list");
    }

    delegatesList.resize(std::min(delegateNum, (int32_t)delegatesList.size()));

    Object obj;
    Array delegateArray;

    CAccount account;
    for (const auto& delegate : delegatesList) {
        if (!pCdMan->pAccountCache->GetAccount(delegate, account)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get account info");
        }
        delegateArray.push_back(account.ToJsonObj());
    }

    obj.push_back(Pair("delegates", delegateArray));

    return obj;
}
