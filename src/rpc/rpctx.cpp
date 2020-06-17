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
#include "commons/util/util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "config/configuration.h"
#include "miner/miner.h"
#include "main.h"
#include "tx/coinstaketx.h"
#include "tx/accountregtx.h"
#include "tx/dextx.h"
#include "tx/txserializer.h"
#include "config/scoin.h"
#include <boost/assign/list_of.hpp>
#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_reader.h"
#include "vm/wasm/types/name.hpp"

#define revert(height) ((height<<24) | (height << 8 & 0xff0000) |  (height>>8 & 0xff00) | (height >> 24))

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

namespace RPC_PARAM {
    vector<uint8_t> GetSignature(const Value &jsonValue) {
        vector<uint8_t> signature;

        string binStr, errStr;
        if (!ParseHex(jsonValue.get_str(), binStr, errStr))
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("invalid signature! %s", errStr));
        if (binStr.size() == 0 || binStr.size() > 100) {
            throw JSONRPCError(RPC_INVALID_PARAMS, strprintf("Invalid signature size=%d, hex=%s", binStr.size(), jsonValue.get_str()));
        }
        return vector<uint8_t>(binStr.begin(), binStr.end());
    }
}


// TxSignatureMap {keyid -> {account, pSignature} }
typedef map<CKeyID, pair<CAccount, vector<uint8_t>*> > TxSignatureMap;

TxSignatureMap GetTxSignatureMap(CBaseTx &tx, CAccountDBCache &accountCache) {

    if (tx.IsRelayForbidden()) {
        return {};
    }

    TxSignatureMap txSignatureMap;
    auto AddTxSignature = [&](const CUserID &uid, vector<uint8_t>* pSignature) {
        if (!uid.IsEmpty()) {
            auto account =  RPC_PARAM::GetUserAccount(accountCache, uid);
            txSignatureMap.emplace(account.keyid, make_pair(account, pSignature));
        }
    };

    AddTxSignature(tx.txUid, &tx.signature);

    if(tx.nTxType == UNIVERSAL_TX) {
        CUniversalTx &universalTx = dynamic_cast<CUniversalTx&>(tx);
        for (auto &item : universalTx.signatures) {
            AddTxSignature(CRegID(item.account), &item.signature);
        }
    } else if (tx.nTxType == DEX_OPERATOR_ORDER_TX) {
        dex::CDEXOperatorOrderTx &opOrderTx = dynamic_cast<dex::CDEXOperatorOrderTx&>(tx);
        if (opOrderTx.has_operator_config)
            AddTxSignature(opOrderTx.operator_uid, &opOrderTx.operator_signature);
    }
    return txSignatureMap;
}

Value gettxdetail(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettxdetail \"txid\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.\"txid\":    (string, required) The hash of transaction.\n"
            "\nResult an object of the transaction detail\n"
            "\nResult:\n"
            "\n\"txid\"\n"
            "\nExamples:\n"
            + HelpExampleCli("gettxdetail","\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gettxdetail","\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\""));

    return GetTxDetailJSON(uint256S(params[0].get_str()));
}

/* Deprecated for common usages but still required for cold mining account registration */
Value submitaccountregistertx(const Array& params, bool fHelp) {
    if (fHelp || params.size() == 0)
        throw runtime_error("submitaccountregistertx \"addr\" [\"fee\"]\n"
            "\nregister account to acquire its regid\n"
            "\nArguments:\n"
            "1.\"addr\":    (string, required)\n"
            "2.\"fee\":     (numeric, optional)\n"
            "\nResult:\n"
            "\"txid\":      (string) The transaction id.\n"
            "\nExamples:\n"
            + HelpExampleCli("submitaccountregistertx", "\"wTtCsc5X9S5XAy1oDuFiEAfEwf8bZHur1W\" 10000")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("submitaccountregistertx", "\"wTtCsc5X9S5XAy1oDuFiEAfEwf8bZHur1W\", 10000"));

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    ComboMoney fee       = RPC_PARAM::GetFee(params, 1, ACCOUNT_REGISTER_TX);
    int32_t validHeight  = chainActive.Height();

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee.GetAmountInSawi());

    if (account.HasOwnerPubKey())
        throw JSONRPCError(RPC_WALLET_ERROR, "Account was already registered");

    CPubKey pubkey;
    if (!pWalletMain->GetPubKey(account.keyid, pubkey))
        throw JSONRPCError(RPC_WALLET_ERROR, "Key not found in local wallet");

    CUserID minerUid = CNullID();
    CPubKey minerPubKey;
    if (pWalletMain->GetPubKey(account.keyid, minerPubKey, true) && minerPubKey.IsFullyValid()) {
        minerUid = minerPubKey;
    }

    CAccountRegisterTx tx;
    tx.txUid        = pubkey;
    tx.miner_uid    = minerUid;
    tx.llFees       = fee.GetAmountInSawi();
    tx.valid_height = validHeight;

    return SubmitTx(account.keyid, tx);
}

Value submitaccountpermscleartx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 2)
        throw runtime_error("submitaccountpermscleartx \"addr\"  [\"fee\"]\n"
                            "\n clear self perms\n"
                            "\nArguments:\n"
                            "1.\"addr or regid\":  (string, required) the tx submitor's id\n"
                            "2.\"fee\":            (combomoney, optional)\n"
                            "\nResult:\n"
                            "\"txid\":      (string) The transaction id.\n"
                            "\nExamples:\n"
                            + HelpExampleCli("submitaccountpermscleartx",
                                             R"("wTtCsc5X9S5XAy1oDuFiEAfEwf8bZHur1W"  "WICC:1000000:SAWI")")
                            + "\nAs json rpc call\n"
                            + HelpExampleRpc("submitaccountpermscleartx",
                                             R"("wTtCsc5X9S5XAy1oDuFiEAfEwf8bZHur1W", "WICC:1000000:SAWI")"));
    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 1, ACCOUNT_PERMS_CLEAR_TX);
    int32_t validHeight  = chainActive.Height();

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    CAccountPermsClearTx tx(txUid, validHeight, fee.symbol, fee.GetAmountInSawi());

    return SubmitTx(account.keyid,tx);

}

Value submitluacontractdeploytx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error("submitluacontractdeploytx \"addr\" \"filepath\" \"fee\" [\"height\"] [\"contract_memo\"]\n"
            "\ncreate a transaction of registering a contract\n"
            "\nArguments:\n"
            "1.\"addr\":            (string, required) contract owner address from this wallet\n"
            "2.\"filepath\":        (string, required) the file path of the app script\n"
            "3.\"fee\":             (numeric, required) pay to miner (the larger the size of script, the bigger fees are required)\n"
            "4.\"height\":          (numeric, optional) valid height, when not specified, the tip block height in chainActive will be used\n"
            "5.\"contract_memo\":   (string, optional) contract memo\n"
            "\nResult:\n"
            "\"txid\":              (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("submitluacontractdeploytx",
                "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/myapp.lua\" 100000000 10000 \"Hello, WaykiChain!\"") +
                "\nAs json rpc call\n"
            + HelpExampleRpc("submitluacontractdeploytx",
                "WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH, \"/tmp/lua/myapp.lua\", 100000000, 10000, \"Hello, WaykiChain!\""));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(int_type)(str_type));

    EnsureWalletIsUnlocked();

    const CUserID& txUid  = RPC_PARAM::GetUserId(params[0]);
    string contractScript = RPC_PARAM::GetLuaContractScript(params[1]);
    ComboMoney fee           = RPC_PARAM::GetFee(params, 2, LCONTRACT_DEPLOY_TX);
    int32_t validHegiht   = params.size() > 3 ? params[3].get_int() : chainActive.Height();
    string memo           = params.size() > 4 ? params[4].get_str() : "";

    if (memo.size() > MAX_CONTRACT_MEMO_SIZE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Contract memo too large");

    if (!txUid.is<CRegID>())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "RegId not exist or immature");

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee.GetAmountInSawi());

    CLuaContractDeployTx tx;
    tx.txUid        = txUid;
    tx.contract     = CLuaContract(contractScript, memo);
    tx.llFees       = fee.GetAmountInSawi();
    tx.fuel     = tx.contract.GetContractSize();
    tx.valid_height = validHegiht;

    return SubmitTx(account.keyid, tx);
}

Value submitluacontractcalltx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "submitluacontractcalltx \"sender_addr\" \"contract_regid\" \"arguments\" \"amount\" \"fee\" [\"height\"]\n"
            "\ncreate contract invocation transaction\n"
            "\nArguments:\n"
            "1.\"sender_addr\":     (string, required) tx sender's base58 addr\n"
            "2.\"contract_regid\":  (string, required) contract regid\n"
            "3.\"arguments\":       (string, required) contract arguments (Hex encode required)\n"
            "4.\"amount\":          (numeric, required) amount of WICC to be sent to the contract account\n"
            "5.\"fee\":             (numeric, required) pay to miner\n"
            "6.\"height\":          (numberic, optional) valid height\n"
            "\nResult:\n"
            "\"txid\":              (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("submitluacontractcalltx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"100-1\" \"01020304\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submitluacontractcalltx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"100-1\", \"01020304\", 10000, 10000, 100"));
    }

    throw JSONRPCError(RPC_INVALID_PARAMETER, "Deprecated since R3");

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(int_type)(str_type)(int_type));

    EnsureWalletIsUnlocked();

    const CUserID& txUid  = RPC_PARAM::GetUserId(params[0], true);
    const CUserID& appUid = RPC_PARAM::GetUserId(params[1]);

    CRegID appRegId;
    if (!pCdMan->pAccountCache->GetRegId(appUid, appRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid contract regid");
    }

    if (!pCdMan->pContractCache->HasContract(appRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to acquire contract");
    }

    string arguments = ParseHexStr(params[2].get_str());
    if (arguments.size() >= MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size is out of range");
    }

    int64_t amount      = AmountToRawValue(params[3]);
    ComboMoney fee         = RPC_PARAM::GetFee(params, 4, LCONTRACT_INVOKE_TX);
    int32_t validHegiht = (params.size() > 5) ? params[5].get_int() : chainActive.Height();

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, amount);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee.GetAmountInSawi());

    CLuaContractInvokeTx tx;
    tx.nTxType      = LCONTRACT_INVOKE_TX;
    tx.txUid        = txUid;
    tx.app_uid      = appUid;
    tx.coin_amount  = amount;
    tx.llFees       = fee.GetAmountInSawi();
    tx.arguments    = arguments;
    tx.valid_height = validHegiht;

    return SubmitTx(account.keyid, tx);
}

Value submitdelegatevotetx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
            "submitdelegatevotetx \"sendaddr\" \"votes\" \"fee\" [\"height\"] \n"
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
            "3.\"fee\": (comboMoney string or numberic, required) pay fee to miner, only support WICC\n"
            "4.\"height\": (numeric optional) valid height. When not supplied, the tip block "
            "height in chainActive will be used.\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("submitdelegatevotetx",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 10000") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submitdelegatevotetx",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 10000"));
    }

    EnsureWalletIsUnlocked();

    const CUserID& txUid = RPC_PARAM::GetUserId(params[0], true);
    ComboMoney fee          = RPC_PARAM::GetFee(params, 2, DELEGATE_VOTE_TX);
    int32_t validHegiht  = params.size() > 3 ? params[3].get_int() : chainActive.Height();

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, SYMB::WICC, SUB_FREE, fee.GetAmountInSawi());

    if (fee.symbol != SYMB::WICC)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The fee symbol must be WICC");

    CDelegateVoteTx delegateVoteTx;
    delegateVoteTx.txUid        = txUid;
    delegateVoteTx.llFees       = fee.GetAmountInSawi();
    delegateVoteTx.valid_height = validHegiht;

    Array arrVotes = params[1].get_array();
    for (auto objVote : arrVotes) {
        const Value& delegateAddr  = find_value(objVote.get_obj(), "delegate");
        const Value& delegateVotes = find_value(objVote.get_obj(), "votes");
        if (delegateAddr.type() == null_type || delegateVotes == null_type) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Vote fund address error or fund value error");
        }
        auto delegateUid = RPC_PARAM::ParseUserIdByAddr(delegateAddr);
        CAccount delegateAcct;
        if (!pCdMan->pAccountCache->GetAccount(delegateUid, delegateAcct)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address does not exist");
        }
        if (!delegateAcct.HasOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Delegate address is unregistered");
        }

        VoteType voteType    = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN : VoteType::MINUS_BCOIN;
        CUserID candidateUid = CUserID(delegateAcct.regid);
        uint64_t bcoins      = (uint64_t)abs(delegateVotes.get_int64());

        CCandidateVote candidateVote(voteType, candidateUid, bcoins);
        delegateVoteTx.candidateVotes.push_back(candidateVote);
    }

    return SubmitTx(account.keyid, delegateVoteTx);
}


Value submitucontractdeploytx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error(
            "submitucontractdeploytx \"sender\" \"filepath\" \"symbol:fee:unit\" [\"height\"] [\"contract_memo\"]\n"
            "\ncreate a transaction of registering a universal contract\n"
            "\nArguments:\n"
            "1.\"sender\":          (string, required) contract owner address from this wallet\n"
            "2.\"filepath\":        (string, required) the file path of the app script\n"
            "3.\"symbol:fee:unit\": (symbol:amount:unit, required) fee paid to miner, default is WICC:100000000:sawi\n"
            "4.\"height\":          (numeric, optional) valid height, when not specified, the tip block height in "
            "chainActive will be used\n"
            "5.\"contract_memo\":   (string, optional) contract memo\n"
            "\nResult:\n"
            "\"txid\":              (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("submitucontractdeploytx",
                           "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/myapp.lua\" \"WICC:100000000:sawi\" "
                           "10000 \"Hello, WaykiChain!\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submitucontractdeploytx",
                           "WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH, \"/tmp/lua/myapp.lua\", \"WICC:100000000:sawi\", "
                           "10000, \"Hello, WaykiChain!\""));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(int_type)(str_type));

    EnsureWalletIsUnlocked();

    const CUserID& txUid  = RPC_PARAM::GetUserId(params[0],true);
    string contractScript = RPC_PARAM::GetLuaContractScript(params[1]); // TODO: support universal contract script
    ComboMoney cmFee      = RPC_PARAM::GetFee(params, 2, UCONTRACT_DEPLOY_TX);
    int32_t validHegiht   = params.size() > 3 ? params[3].get_int() : chainActive.Height();
    string memo           = params.size() > 4 ? params[4].get_str() : "";

    if (!txUid.is<CRegID>())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "RegID not exist or immature");

    if (memo.size() > MAX_CONTRACT_MEMO_SIZE)
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Contract memo size too large");

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, cmFee.symbol, SUB_FREE, cmFee.GetAmountInSawi());

    CUniversalContractDeployTx tx;
    tx.txUid        = txUid;
    tx.contract     = CUniversalContract(contractScript, memo);
    tx.fee_symbol   = cmFee.symbol;
    tx.llFees       = cmFee.GetAmountInSawi();
    tx.fuel         = tx.contract.GetContractSize();
    tx.valid_height = validHegiht;

    return SubmitTx(account.keyid, tx);
}

Value submitucontractcalltx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "submitucontractcalltx \"sender_addr\" \"contract_regid\" \"arguments\" \"symbol:coin:unit\" "
            "\"symbol:fee:unit\" [\"height\"]\n"
            "\ncreate contract invocation transaction\n"
            "\nArguments:\n"
            "1.\"sender_addr\":     (string, required) tx sender's base58 addr\n"
            "2.\"contract_regid\":  (string, required) contract regid\n"
            "3.\"arguments\":       (string, required) contract arguments (Hex encode required)\n"
            "4.\"symbol:coin:unit\":(symbol:amount:unit, required) transferred coins\n"
            "5.\"symbol:fee:unit\": (symbol:amount:unit, required) fee paid to miner, default is WICC:10000:sawi\n"
            "6.\"height\":          (numberic, optional) valid height\n"
            "\nResult:\n"
            "\"txid\":              (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("submitucontractcalltx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"100-1\" \"01020304\" \"WICC:10000:sawi\" "
                           "\"WICC:10000:sawi\" 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submitucontractcalltx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"100-1\", \"01020304\", \"WICC:10000:sawi\", "
                           "\"WICC:10000:sawi\", 100"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(str_type)(str_type)(int_type));

    EnsureWalletIsUnlocked();

    const CUserID& txUid  = RPC_PARAM::GetUserId(params[0], true);
    const CUserID& appUid = RPC_PARAM::GetUserId(params[1]);

    CRegID appRegId;
    if (!pCdMan->pAccountCache->GetRegId(appUid, appRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid contract regid");
    }

    if (!pCdMan->pContractCache->HasContract(appRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to acquire contract");
    }

    string arguments = ParseHexStr(params[2].get_str());
    if (arguments.size() >= MAX_CONTRACT_ARGUMENT_SIZE) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size is out of range");
    }

    ComboMoney cmCoin   = RPC_PARAM::GetComboMoney(params[3], SYMB::WICC);
    ComboMoney cmFee    = RPC_PARAM::GetFee(params, 4, UCONTRACT_INVOKE_TX);
    int32_t validHegiht = (params.size() > 5) ? params[5].get_int() : chainActive.Height();

    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, cmCoin.symbol, SUB_FREE, cmCoin.GetAmountInSawi());
    RPC_PARAM::CheckAccountBalance(account, cmFee.symbol, SUB_FREE, cmFee.GetAmountInSawi());

    CUniversalContractInvokeTx tx;
    tx.nTxType      = UCONTRACT_INVOKE_TX;
    tx.txUid        = txUid;
    tx.app_uid      = appUid;
    tx.coin_symbol  = cmCoin.symbol;
    tx.coin_amount  = cmCoin.GetAmountInSawi();
    tx.fee_symbol   = cmFee.symbol;
    tx.llFees       = cmFee.GetAmountInSawi();
    tx.arguments    = arguments;
    tx.valid_height = validHegiht;

    return SubmitTx(account.keyid, tx);
}

Value listaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "listaddr\n"
            "\nreturn Array containing address, balance, haveminerkey, regid information.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("listaddr", "") + "\nAs json rpc call\n" + HelpExampleRpc("listaddr", ""));
    }

    Array retArray;
    assert(pWalletMain != nullptr);
    {
        set<CKeyID> setKeyId;
        pWalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0) {
            return retArray;
        }

        for (const auto &keyid : setKeyId) {
            CUserID userId(keyid);
            CAccount account;
            pCdMan->pAccountCache->GetAccount(userId, account);
            CKeyCombi keyCombi;
            pWalletMain->GetKeyCombi(keyid, keyCombi);

            Object obj;
            obj.push_back(Pair("addr",          keyid.ToAddress()));
            obj.push_back(Pair("regid",         account.regid.ToString()));
            obj.push_back(Pair("regid_mature",  account.regid.IsMature(chainActive.Height())));
            obj.push_back(Pair("received_votes",account.received_votes));

            Object tokenMapObj;
            for (auto tokenPair : account.tokens) {
                Object tokenObj;
                const CAccountToken& token = tokenPair.second;
                tokenObj.push_back(Pair("free_amount",      token.free_amount));
                tokenObj.push_back(Pair("staked_amount",    token.staked_amount));
                tokenObj.push_back(Pair("frozen_amount",    token.frozen_amount));
                tokenObj.push_back(Pair("voted_amount",     token.voted_amount));

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
    if (params.size() > 0) {
        nDefCount = params[0].get_int();
    }

    if (params.size() > 1) {
        nFrom = params[1].get_int();
    }
    assert(pWalletMain != nullptr);

    Array confirmedTxArray;
    int32_t nCount = 0;
    map<int32_t, uint256, std::greater<int32_t> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        auto it = mapBlockIndex.find(wtx.first);
        if (it != mapBlockIndex.end())
            blockInfoMap.insert(make_pair(it->second->height, wtx.first));
    }
    bool bUpLimited = false;
    for (auto const &blockInfo : blockInfoMap) {
        CWalletAccountTxDb acctTxDb = pWalletMain->mapInBlockTx[blockInfo.second];
        for (auto const & item : acctTxDb.account_tx_map) {
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
            "{\n"
            "  \"address\": \"xxxxx\",       (string) the address\n"
            "  \"keyid\": \"xxxxx\",         (string) the keyid referred to the address\n"
            "  \"regid_mature\": true|false,   (bool) whether regid is mature or not\n"
            "  \"regid\": \"xxxxx\",         (string) the regid referred to the address\n"
            "  \"regid_mature\": true|false,   (bool) the regid is mature or not\n"
            "  \"owner_pubkey\": \"xxxxx\",  (string) the public key referred to the address\n"
            "  \"miner_pubkey\": \"xxxxx\",  (string) the miner publick key referred to the address\n"
            "  \"tokens\": {},             (object) tokens object all the address owned\n"
            "  \"received_votes\": xxxxx,  (numeric) received votes in total\n"
            "  \"vote_list\": [],       (array) votes to others\n"
            "  \"position\": \"xxxxx\",      (string) in wallet if the address never involved in transaction, otherwise, in block\n"
            "  \"cdp_list\": [],           (array) cdp list\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\""));
    }

    RPCTypeCheck(params, list_of(str_type));
    CKeyID keyid = RPC_PARAM::GetKeyId(params[0]);
    const auto &addrStr = params[0].get_str();
    CUserID userId = keyid;
    Object obj;
    CAccount account;
    bool on_chain = false;
    bool pubkey_registered = false;
    bool in_wallet = false;

    if (pCdMan->pAccountCache->GetAccount(userId, account)) {
        on_chain = true;
        pubkey_registered = account.owner_pubkey.IsValid();
    }

    CPubKey pubKey_in_wallet;
    if (pWalletMain->GetPubKey(keyid, pubKey_in_wallet)) {
        in_wallet = true;
        if (!pubkey_registered) {
            account.owner_pubkey = pubKey_in_wallet;
        }
        if (!account.miner_pubkey.IsValid()) {
            CPubKey miner_pubKey_in_wallet;
            if (pWalletMain->GetPubKey(keyid, miner_pubKey_in_wallet, true)) {
                account.miner_pubkey = miner_pubKey_in_wallet;
            }
        }
        if (account.keyid.IsEmpty()) {
            account.keyid        = pubKey_in_wallet.GetKeyId();
        }
    } else {
        if (!on_chain) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("account does not exist on chain and in wallet! addr=%s",
                addrStr));
        }
    }

    obj = account.ToJsonObj();
    obj.push_back(Pair("onchain", on_chain));
    obj.push_back(Pair("in_wallet", in_wallet));
    obj.push_back(Pair("pubkey_registered", pubkey_registered));

    if (on_chain && !account.regid.IsEmpty()) {
        Array cdps;
        vector<CUserCDP> userCdps;
        if (pCdMan->pCdpCache->GetCDPList(account.regid, userCdps)) {
            for (auto& cdp : userCdps) {
                uint64_t bcoinMedianPrice = RPC_PARAM::GetPriceByCdp(*pCdMan->pPriceFeedCache, cdp);
                cdps.push_back(cdp.ToJson(bcoinMedianPrice));
            }
        }

        obj.push_back(Pair("cdp_list", cdps));
    }

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


    if (number >= chainActive.Height())
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid number");

    if (number > 0) {
        CValidationState state;
        do {
            CBlockIndex * pTipIndex = chainActive.Tip();
            if (!DisconnectTip(state))
                throw JSONRPCError(RPC_MISC_ERROR, "DisconnectTip err");

            chainMostWork.SetTip(pTipIndex->pprev);
            if (!EraseBlockIndexFromSet(pTipIndex))
                throw JSONRPCError(RPC_MISC_ERROR, "EraseBlockIndexFromSet err");

            if (!pCdMan->pBlockIndexDb->EraseBlockIndex(pTipIndex->GetBlockHash()))
                throw JSONRPCError(RPC_MISC_ERROR, "EraseBlockIndex err");

            mapBlockIndex.erase(pTipIndex->GetBlockHash());
        } while (--number);
    }

    Object obj;
    obj.push_back(
        Pair("tip", strprintf("hash:%s hight:%s", chainActive.Tip()->GetBlockHash().ToString(), chainActive.Height())));

    return obj;
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

    auto dbIt = MakeDbIterator(pCdMan->pContractCache->contractCache);
    Object obj;
    Array contractArray;
    for (dbIt->First(); dbIt->IsValid(); dbIt->Next()) {
        Object contractObject;
        const auto &regidKey = dbIt->GetKey();
        const CUniversalContractStore &contract = dbIt->GetValue();
        contractObject.push_back(Pair("contract_regid", regidKey.ToString()));
        contractObject.push_back(Pair("memo",           contract.memo));

        if (showDetail) {
            contractObject.push_back(Pair("vm_type",    contract.vm_type));
            contractObject.push_back(Pair("upgradable", contract.upgradable));
            contractObject.push_back(Pair("code",       HexStr(contract.code)));
            contractObject.push_back(Pair("abi",        contract.abi));
        }

        contractArray.push_back(contractObject);
    }

    obj.push_back(Pair("count",     contractArray.size()));
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
    if (regid.IsEmpty() || !pCdMan->pContractCache->HasContract(regid)) {
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid contract regid.");
    }

    CUniversalContractStore contractStore;
    if (!pCdMan->pContractCache->GetContract(regid, contractStore)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to acquire contract from db.");
    }

    Object obj = contractStore.ToJson();
    obj.insert(obj.begin(), Pair("contract_regid",    regid.ToString()));

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

    return pCdMan->pTxCache->ToJsonObj();
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
    if (chainActive.Height() - SysCfg().GetTxCacheHeight() >= 0) {
        pIndex = chainActive[(chainActive.Height() - SysCfg().GetTxCacheHeight())];
    } else {
        pIndex = chainActive.Genesis();
    }

    CBlock block;
    do {
        if (!ReadBlockFromDisk(pIndex, block))
            return ERRORMSG("reloadtxcache() : *** ReadBlockFromDisk failed at %d, hash=%s",
                pIndex->height, pIndex->GetBlockHash().ToString());

        pCdMan->pTxCache->AddBlockTx(block);
        pIndex = chainActive.Next(pIndex);
    } while (nullptr != pIndex);

    Object obj;
    obj.push_back(Pair("info", "reload tx cache succeed"));
    return obj;
}

Value getcontractdata(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error(
            "getcontractdata \"contract_regid\" \"key\" [hexadecimal]\n"
            "\nget contract data with key\n"
            "\nArguments:\n"
            "1.\"contract_regid\":      (string, required) contract regid\n"
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

Value submittxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 1 || params.size() > 2) {
        throw runtime_error(
            "submittxraw \"rawtx\" [\"signatures\"]\n"
            "\nsubmit raw transaction (hex format)\n"
            "\nArguments:\n"
            "1. \"rawtx\":   (string, required) The raw signed transaction\n"
            "2. \"signatures\"    (array, optional) A json array of signature info\n"
            " [\n"
            "   {\n"
            "      \"addr\": \"address\" (string, required) address of the signature\n"
            "      \"signature\": \"hex str\" (string, required) signature, hex format\n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "\nExamples:\n" +
            HelpExampleCli("submittxraw",
                           "\"0b01848908020001145e3550cfae2422dce90a778b0954409b1c6ccc3a045749434382dbea93000457494343c"
                           "d10004630440220458e2239348a9442d05503137ec84b84d69c7141b3618a88c50c16f76d9655ad02206dd20806"
                           "87cffad42f7293522568fc36850d4e3b81fa9ad860d1490cf0225cf8\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("submittxraw",
                           "\"0b01848908020001145e3550cfae2422dce90a778b0954409b1c6ccc3a045749434382dbea93000457494343c"
                           "d10004630440220458e2239348a9442d05503137ec84b84d69c7141b3618a88c50c16f76d9655ad02206dd20806"
                           "87cffad42f7293522568fc36850d4e3b81fa9ad860d1490cf0225cf8\""));
    }

    vector<uint8_t> vch(ParseHex(params[0].get_str()));
    if (vch.size() > MAX_RPC_SIG_STR_LEN) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The rawtx is too long.");
    }

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);

    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;

    if (params.size() > 1) {
        Array sigArray = params[1].get_array();
        // TxSignatureMap {keyid -> {account, pSignature} }
        TxSignatureMap txSignatureMap = GetTxSignatureMap(*pBaseTx, *pCdMan->pAccountCache);
        set<CKeyID> signedUser;

        for (auto sigItemObj : sigArray) {
            const Value& addrObj      = JSON::GetObjectFieldValue(sigItemObj, "addr");
            auto uid            = RPC_PARAM::ParseUserIdByAddr(addrObj);
            const Value& signatureObj     = JSON::GetObjectFieldValue(sigItemObj, "signature");
            auto signature           = RPC_PARAM::GetSignature(signatureObj);
            auto keyid = RPC_PARAM::GetUserKeyId(uid);
            auto it = txSignatureMap.find(keyid);
            if (it == txSignatureMap.end()) {
                throw JSONRPCError(
                    RPC_INVALID_PARAMETER,
                    strprintf("The signature of the user=%s is not required in tx",
                              addrObj.get_str()));
            }
            auto ret = signedUser.insert(keyid);
            if (!ret.second) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("duplicated signature of user=%s",
                    addrObj.get_str()));
            }
            auto &signatureDest = *it->second.second;
            signatureDest = signature;
        }
    }

    string retMsg;
    if (!pWalletMain->CommitTx((CBaseTx *) pBaseTx.get(), retMsg))
        throw JSONRPCError(RPC_WALLET_ERROR, "Submittxraw error: " + retMsg);

    Object obj;
    obj.push_back( Pair("txid", pBaseTx->GetHash().ToString()) );
    if (pBaseTx->nTxType == UNIVERSAL_TX) {
        Value  detailObj;
        if (json_spirit::read(retMsg, detailObj)) {
            obj.push_back( Pair("detail", detailObj) );
        } else {
            obj.push_back( Pair("detail_msg", retMsg) );
        }
    }
    return obj;
}

Value droptxfrommempool(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "droptxfrommempool \"txid\"\n"
            "\ndrop tx from mem pool\n"
            "\nArguments:\n"
            "1.\"txid\":   (string, required) the txid of dropping tx\n"
            "\nExamples:\n" +
            HelpExampleCli("droptxfrommempool",
                           "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("droptxfrommempool",
                           "\"c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\""));
    }

    const string &txidStr = params[0].get_str();

    uint256 txid = uint256S(txidStr);
    if (txid.IsEmpty())
        throw JSONRPCError(RPC_INVALID_PARAMS, "Invalid txid=" + txidStr);

    mempool.Remove(txid);
    mempool.ReScanMemPoolTx();
    return Object();
}

static vector<uint8_t> SignTxHash(const CKeyID &keyId, const uint256 &hash) {
    vector<uint8_t> signature;
    if (!pWalletMain->Sign(keyId, hash, signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Sign tx hash failed! addr=%s",
            keyId.ToAddress()));
    }
    return signature;
}

Value signtxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "signtxraw \"str\" \"addr\"\n"
            "\nsignature transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) Hex-format string, no longer than 65K in binary bytes\n"
            "2.\"sginaturor_addr\": (string, required) A json array of WICC addresses\n"
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

    if (pBaseTx->IsRelayForbidden()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("Unsupport to sign tx=%s", pBaseTx->GetTxTypeName()));
    }

    const Array& addresses = params[1].get_array();

    if (addresses.empty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "address list is empty()");
    }

    // TxSignatureMap {keyid -> {account, pSignature} }
    TxSignatureMap txSignatureMap = GetTxSignatureMap(*pBaseTx, *pCdMan->pAccountCache);
    set<CKeyID> signedUser;
    auto txid = pBaseTx->GetHash();

    Array signatureArray;
    for (const auto &addrObj : addresses) {
        auto uid            = RPC_PARAM::ParseUserIdByAddr(addrObj);
        auto keyid = RPC_PARAM::GetUserKeyId(uid);

        auto it = txSignatureMap.find(keyid);
        if (it == txSignatureMap.end()) {
            throw JSONRPCError(
                RPC_INVALID_PARAMETER,
                strprintf("The signature of the user=%s is not required in tx",
                            addrObj.get_str()));
        }
        auto &account = it->second.first;

        auto ret = signedUser.insert(keyid);
        if (!ret.second) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("duplicated signature of user, addr=%s, regid=%s",
                account.keyid.ToAddress(), account.regid.ToString()));
        }

        auto &signatureDest = *it->second.second;
        signatureDest = SignTxHash(keyid, txid);

        Object itemObj;
        itemObj.push_back(Pair("addr", account.keyid.ToAddress()));
        itemObj.push_back(Pair("regid", account.regid.ToString()));
        itemObj.push_back(Pair("signature", HexStr(signatureDest)));
        signatureArray.push_back(itemObj);
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    ds << pBaseTx;

    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    obj.push_back(Pair("signed_count", (int64_t)signatureArray.size()));
    obj.push_back(Pair("signed_list", signatureArray));
    return obj;
}

Value decodetxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "decodetxraw \"hexstring\"\n"
            "\ndecode transaction\n"
            "\nArguments:\n"
            "1.\"rawtx\": (string, required) hexstring\n"
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
    auto pCw = std::make_shared<CCacheWrapper>(pCdMan);
    obj = pBaseTx->ToJson(*pCw);
    return obj;
}

Value getcontractaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error(
            "getcontractaccountinfo \"contract_regid\" \"account address or regid\""
            "\nget contract account info\n"
            "\nArguments:\n"
            "1.\"contract_regid\":              (string, required) contract regid\n"
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

        for (const auto &keyid : setKeyId) {

            string key = keyid.ToAddress();

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
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "validateaddr \"address\"\n"
            "\ncheck whether address is valid or not\n"
            "\nArguments:\n"
            "1.\"address\"      (string, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\"") + "\nAs json rpc call\n" +
            HelpExampleRpc("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\""));
    }

    Object obj;
    CKeyID keyid = RPC_PARAM::GetKeyId(params[0]);
    obj.push_back(Pair("is_valid", true));
    obj.push_back(Pair("addr", keyid.ToAddress()));

    return obj;
}

Value gettotalcoins(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "gettotalcoins \n"
            "\nget the total number of circulating coins excluding those locked for votes\n"
            "\nand the total number of registered addresses\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("gettotalcoins", "") + "\nAs json rpc call\n" + HelpExampleRpc("gettotalcoins", ""));
    }

    Object stats = pCdMan->pAccountCache->GetAccountDBStats();
    return stats;
}

Value listdelegates(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1) {
        throw runtime_error(
            "listdelegates \n"
            "\nreturns the specified number delegates by reversed order voting number.\n"
            "\nArguments:\n"
            "1. count:           (number, optional) the count of the delegates, default to all delegates.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("listdelegates", "11") + "\nAs json rpc call\n" + HelpExampleRpc("listdelegates", "11"));
    }


    int32_t defaultDelegateNum = pCdMan->pDelegateCache->GetActivedDelegateNum();

    int32_t delegateNum = (params.size() == 1) ? params[0].get_int() : defaultDelegateNum;
    if (delegateNum < 1 || delegateNum > defaultDelegateNum) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("Delegate number not between 1 and %u", defaultDelegateNum));
    }

    VoteDelegateVector delegates;
    if (!pCdMan->pDelegateCache->GetActiveDelegates(delegates)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "get active delegates failed");
    }

    Object obj;
    Array delegateArray;

    CAccount account;
    int i = 0;
    for (const auto& delegate : delegates) {

        if (!pCdMan->pAccountCache->GetAccount(delegate.regid, account)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get account info");
        }
        Object accountObj = account.ToJsonObj();
        accountObj.push_back(Pair("active_votes", delegate.votes));
        delegateArray.push_back(accountObj);
        if(++i == delegateNum)
            break;
    }

    obj.push_back(Pair("delegates", delegateArray));

    return obj;
}
