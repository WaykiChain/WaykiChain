// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "commons/json/json_spirit_utils.h"
#include "commons/json/json_spirit_value.h"
#include "commons/json/json_spirit_reader.h"
#include "entities/utxo.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "main.h"
#include "wallet/wallet.h"
using namespace std;


enum UtxoCondDirection: uint8_t {
    IN,
    OUT
};

extern CWallet* pWalletMain;

static void ParseUtxoCond(const Array& arr, vector<shared_ptr<CUtxoCond>>& vCond, const CKeyID& txKeyID);
static void ParseUtxoInput(const Array& arr, vector<CUtxoInput>& vInput,const CKeyID& txKeyID);
static void ParseUtxoOutput(const Array& arr, vector<CUtxoOutput>& vOutput,const CKeyID& txKeyID);
static void CheckUtxoCondDirection(const vector<shared_ptr<CUtxoCond>>& vCond, UtxoCondDirection direction);

Value genutxomultisignature(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
                "genutxomultisignature \"addr\" \"utxo_content_hash\" \n"
                "\nGenerate a UTXO MultiSign Singature.\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"addr\":                (string, required) the addr of signee\n"
                "2.\"utxo_content_hash\":   (string, required) The utxo content hash\n"
                "\nResult:\n"
                "\"signature\"              (string) signature hex.\n"
                "\nExamples:\n" +
                HelpExampleCli("genutxomultisignature",
                               R"("0-5" "23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds" )") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("genutxomultisignature",
                               R"("0-5", "23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds")")
        );
    }

    CUserID uid = RPC_PARAM::GetUserId(params[0]);
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, uid);
    uint256 hash  = uint256S(params[1].get_str());

    UnsignedCharArray signature;

    if (!pWalletMain->HasKey(account.keyid)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "signee address not found in wallet");
    }

    if (!pWalletMain->Sign(account.keyid, hash, signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    Object obj;
    obj.push_back(Pair("signature", HexStr(signature.begin(), signature.end())));
    return obj;

}

Value genutxomultisignaddr( const Array& params, bool fHelp) {
    if(fHelp || params.size() != 3) {
        throw runtime_error(
                "genutxomultisignaddr \"n\" \"m\" \"signee_uids\" \n"
                "\nGenerate a multi sign address.\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"n\":                       (numberic, required) the total signee size \n"
                "2.\"m\":                       (numberic, required) the min signee size \n"
                "3.\"signee_uids\":             (array<string> the signee uid\n"
                "\nResult:\n"
                "\"txid\"                       (string) The multi address hash.\n"
                "\nExamples:\n" +
                HelpExampleCli("genutxomultisignaddr",
                               R"(2 2 "["0-2", "0-3"]")") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("genutxomultisignaddr",R"(2, 2, "["0-2", "0-3"]")")
        );
    }

    uint8_t n = params[0].get_int();
    uint8_t m = params[1].get_int();
    Array uidStringArray = params[2].get_array();

    vector<string> vAddr;
    for(auto v: uidStringArray){
        CUserID  uid = RPC_PARAM::GetUserId(v);
        CAccount acct;
        if (!pCdMan->pAccountCache->GetAccount(uid, acct))
            throw JSONRPCError(RPC_INVALID_PARAMETER, "the uid is not on chain");

        vAddr.push_back(acct.keyid.ToAddress());
    }

    if( m > n) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "m must be not greater than n");
    }

    if(m > 20 || n >20) {
        throw  JSONRPCError(RPC_INVALID_PARAMETER, " m and n must be not greater than 20");
    }

    if( n != vAddr.size()){
        throw JSONRPCError(RPC_INVALID_PARAMETER, "the n must be equal to signee count");
    }


    string redeemScript("");
    CKeyID multiKeyID;
    if( ComputeRedeemScript(m,n,vAddr, redeemScript) && ComputeMultiSignKeyId(redeemScript, multiKeyID)) {
        Object o;
        o.push_back(Pair("multi_addr", multiKeyID.ToAddress()));
        return o;
    } else {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "gen multi addr error");
    }

}

Value genutxomultiinputcondhash(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 6) {
        throw runtime_error(
                "genutxomultiinputcondhash \"n\" \"m\" \"pre_utxo_txid\" \"pre_utxo_tx_vout_index\" \"signee_uids\",\"spend_txuid\" \n"
                "\n Generate a hash that will be sign in IP2MA cond\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"n\":                       (numberic, required) the total signee size\n"
                "2.\"m\":                       (numberic, required) the min signee size \n"
                "3.\"pre_utxo_txid\":           (string, required) The utxo txid you want to spend\n"
                "4.\"pre_utxo_tx_vout_index\":  (string, required) The index of pre utxo output \n"
                "5.\"signee_uids\":             (array<string> the signee uid\n"
                "6.\"spend_txuid\":             (string, required) the uid that well submit utxotransfertx to spend\n"
                "\nResult:\n"
                "\"txid\"                       (string) The multi input cond hash.\n"
                "\nExamples:\n" +
                HelpExampleCli("genutxomultiinputcondhash",
                               R"(2 2 "23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds" 4 "["0-2","0-3"]" "0-2")"
                ) +
                "\nAs json rpc call\n" +
                HelpExampleRpc("genutxomultiinputcondhash",
                               R"(2, 2, "23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds", 4, "["0-2","0-3"]", "0-2")")
        );
    }


    uint8_t n = params[0].get_int();
    uint8_t m = params[1].get_int();
    uint256 prevUtxoTxId = uint256S(params[2].get_str());
    uint16_t prevUtxoTxVoutIndex = params[3].get_int();
    Array uidStringArray = params[4].get_array();
    CKeyID txKeyID = RPC_PARAM::GetKeyId(params[5]);

    CAccount txAcct;
    if (!pCdMan->pAccountCache->GetAccount(txKeyID, txAcct))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Get Account from txKeyID failed.");

    vector<string> vAddr;
    for(auto v: uidStringArray){
        CKeyID  keyid = RPC_PARAM::GetKeyId(v);
        vAddr.push_back(keyid.ToAddress());
    }


    if( m > n) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "m must be not greater than n");
    }

    if(m > 20 || n >20) {
        throw  JSONRPCError(RPC_INVALID_PARAMETER, " m and n must be not greater than 20");
    }

    if( n != vAddr.size()){
        throw JSONRPCError(RPC_INVALID_PARAMETER, "the n must be equal to signee count");
    }

    string redeemScript("");
    ComputeRedeemScript(m, n, vAddr, redeemScript);

    uint256 multiSignHash;
    if (!ComputeUtxoMultisignHash(prevUtxoTxId, prevUtxoTxVoutIndex, txAcct, redeemScript, multiSignHash))
        throw JSONRPCError(RPC_WALLET_ERROR, "create hash error");

    Object o;
    o.push_back(Pair("hash", multiSignHash.GetHex()));
    return o;

}

Value submitpasswordprooftx(const Array& params, bool fHelp) {

    if(fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
                "submitpasswordprooftx \"addr\" \"utxo_txid\" \"utxo_vout_index\" \"password\" \"pre_utxo_tx_uid\" [\"fee\"] \n"
                "\nSubmit a password proof tx.\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"addr\":                (string, required) the addr submit this tx\n"
                "2.\"prev_utxo_txid\":       (string, required) The utxo txid you want to spend\n"
                "3.\"prev_utxo_vout_index\": (string, required) The index of utxo output \n"
                "4.\"password\":            (symbol:amount:unit, required) password\n"
                "5.\"pre_utxo_tx_uid\"      (string, required) the txUid of prev utxotx that provide prevOutput\n "
                "6.\"symbol:fee:unit\":     (symbol:amount:unit, optinal) fee paid to miner\n"
                "\nResult:\n"
                "\"txid\"                   (string) The transaction id.\n"
                "\nExamples:\n" +
                HelpExampleCli("submitpasswordprooftx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\" \"23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds\" "
                               "5 \"123\" \'0-2\" \"WICC:10000:sawi\"") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("submitpasswordprooftx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\", \"23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds\","
                               " 5, \"123\", \"0-2\", \"WICC:10000:sawi\"")
                );
    }

    EnsureWalletIsUnlocked();
    CUserID txUid = RPC_PARAM::GetUserId(params[0], true );
    TxID prevUtxoTxid = uint256S(params[1].get_str());
    uint16_t prevUtxoVoutIndex = params[2].get_int();
    string password = params[3].get_str();
    CUserID preUtxoTxUid = RPC_PARAM::GetUserId(params[4]);
    CAccount preUtxoTxAccount = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, preUtxoTxUid);
    ComboMoney fee = RPC_PARAM::GetFee(params, 5 ,TxType::UTXO_PASSWORD_PROOF_TX);
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    string text = strprintf("%s%s%s%s%d", password,
                            preUtxoTxAccount.keyid.ToString(), account.keyid.ToString(),
                            prevUtxoTxid.ToString(), prevUtxoVoutIndex);
    uint256 passwordProof = Hash(text);
    int32_t validHeight  = chainActive.Height();
    CCoinUtxoPasswordProofTx tx(txUid, validHeight, fee.symbol, fee.GetAmountInSawi(), prevUtxoTxid, prevUtxoVoutIndex, passwordProof);
    return SubmitTx(account.keyid, tx);

}

Value submitutxotransfertx(const Array& params, bool fHelp) {

    if(fHelp || params.size() <4 || params.size() > 6) {
        throw runtime_error(
                "submitutxotransfertx \"addr\" \"coin_symbol\" \"vins\" \"vouts\" \"symbol:fee:unit\" \"memo\" \n"
                "\nSubmit utxo tx.\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"addr\":              (string, required) the addr submit this tx\n"
                "2.\"coin_symbol\":       (string, required) The utxo transfer coin symbole\n"
                "3.\"vins\":              (string(json), required) The utxo inputs \n"
                "4.\"vouts\":             (string(json), required) the utxo outputs \n"
                "5.\"symbol:fee:unit\":   (symbol:amount:unit, optional) fee paid to miner\n"
                "6.\"memo\":              (string,optinal) tx memo\n"
                "\nResult:\n"
                "\"txid\"                 (string) The transaction id.\n"
                "\nExamples:\n" +
                HelpExampleCli("submitutxotransfertx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\" \"WICC\" \"[]\" "
                               " \"[]\" \"WICC:10000:sawi\" \"xx\"") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("submitutxotransfertx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\", \"WICC\", \"[]\", "
                               "\"[]\", \"WICC:10000:sawi\", \"xx\"")
        );
    }

    EnsureWalletIsUnlocked();

    CUserID txUid = RPC_PARAM::GetUserId(params[0], true);
    TokenSymbol coinSymbol = params[1].get_str();
    Array inputArray  = params[2].get_array();
    Array outputArray = params[3].get_array();

    ComboMoney fee = RPC_PARAM::GetFee(params, 4, TxType::UTXO_TRANSFER_TX);
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());
    string memo;
    if(params.size() > 5)
        memo = params[5].get_str();

    std::vector<CUtxoInput> vins;
    std::vector<CUtxoOutput> vouts;
    ParseUtxoInput(inputArray, vins, account.keyid);
    ParseUtxoOutput(outputArray, vouts, account.keyid);

    for (auto input : vins) {
        auto inCondTypes = unordered_set<UtxoCondType>();
        for (auto cond: input.conds) {
            if (inCondTypes.count(cond.sp_utxo_cond->cond_type) > 0)
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("in cond type (%d) exists error!", cond.sp_utxo_cond->cond_type));
            else
                inCondTypes.emplace(cond.sp_utxo_cond->cond_type);
        }

        if (inCondTypes.count(UtxoCondType::IP2SA) == 1 && inCondTypes.count(UtxoCondType::IP2MA) == 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "can't have both IP2SA & IP2MA error");
    }

    for (auto output : vouts) {
        auto outCondTypes = unordered_set<UtxoCondType>();
        for (auto cond : output.conds) {
            if (outCondTypes.count(cond.sp_utxo_cond->cond_type) > 0)
                throw JSONRPCError(RPC_INVALID_PARAMETER, strprintf("out cond type (%d) exists error!", cond.sp_utxo_cond->cond_type));
            else
                outCondTypes.emplace(cond.sp_utxo_cond->cond_type);
        }
        if (outCondTypes.count(UtxoCondType::OP2SA) == 1 && outCondTypes.count(UtxoCondType::OP2MA) == 1)
            throw JSONRPCError(RPC_INVALID_PARAMETER, "can't have both OP2SA & OP2MA error");
    }

    int32_t validHeight  = chainActive.Height();
    CCoinUtxoTransferTx utxoTransferTx(txUid, validHeight, fee.symbol, fee.GetAmountInSawi(), coinSymbol, vins, vouts, memo);

    return SubmitTx(account.keyid, utxoTransferTx);

}


static void TransToStorageBean(const vector<shared_ptr<CUtxoCond>>& vCond, vector<CUtxoCondStorageBean>& vCondStorageBean) {

    for (auto& cond: vCond ) {
        auto bean = CUtxoCondStorageBean(cond);
        vCondStorageBean.push_back(bean);
    }
}

static void ParseUtxoInput(const Array& arr, vector<CUtxoInput>& vInput, const CKeyID& txKeyID) {
    for (auto obj : arr) {
        const Value& preTxidObj  = JSON::GetObjectFieldValue(obj, "prev_utxo_txid");
        TxID prevUtxoTxid = uint256S(preTxidObj.get_str());
        const Value& preVoutIndexObj = JSON::GetObjectFieldValue(obj,"prev_utxo_vout_index");
        uint16_t  preUoutIndex = AmountToRawValue(preVoutIndexObj);
        const Array& condArray = JSON::GetObjectFieldValue(obj, "conds").get_array();
        vector<CUtxoCondStorageBean> vCondStorageBean;
        vector<shared_ptr<CUtxoCond>> vCond;
        ParseUtxoCond(condArray, vCond,txKeyID);
        CheckUtxoCondDirection(vCond, UtxoCondDirection::IN);
        TransToStorageBean(vCond, vCondStorageBean);
        CUtxoInput input = CUtxoInput(prevUtxoTxid,preUoutIndex,vCondStorageBean);
        vInput.push_back(input);

    }
}


static void ParseUtxoOutput(const Array& arr, vector<CUtxoOutput>& vOutput,const CKeyID& txKeyID) {
    for (auto& obj : arr) {
        const Value& coinAmountObj  = JSON::GetObjectFieldValue(obj, "coin_amount");
        uint64_t coinAmount = AmountToRawValue(coinAmountObj);
        const Array& condArray = JSON::GetObjectFieldValue(obj, "conds").get_array();
        vector<CUtxoCondStorageBean> vCondStorageBean;
        vector<shared_ptr<CUtxoCond>> vCond;
        ParseUtxoCond(condArray, vCond,txKeyID);
        CheckUtxoCondDirection(vCond, UtxoCondDirection::OUT);
        TransToStorageBean(vCond, vCondStorageBean);
        CUtxoOutput output = CUtxoOutput(coinAmount, vCondStorageBean);
        vOutput.push_back(output);

    }
}


static void CheckUtxoCondDirection(const vector<shared_ptr<CUtxoCond>>& vCond, UtxoCondDirection direction) {


    for (auto& cond:vCond) {
        switch (cond->cond_type) {
            case IP2MA:
            case IP2PH:
            case IP2SA:
                if(direction != UtxoCondDirection::IN){
                    throw JSONRPCError(RPC_INVALID_PARAMETER,"cond direction is error");
                }
                break;
            case OP2MA:
            case OP2PH:
            case OP2SA:
            case OCLAIM_LOCK:
            case ORECLAIM_LOCK:
                if(direction != UtxoCondDirection::OUT) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER,"cond direction is error");
                }
                break;
            case NULL_UTXOCOND_TYPE:
            default:
                throw JSONRPCError(RPC_INVALID_PARAMETER, "cond type is error");

        }

    }

}
static void ParseUtxoCond(const Array& arr, vector<shared_ptr<CUtxoCond>>& vCond,const CKeyID& txKeyID) {

    for (auto& obj : arr) {
        const Value& typeObj = JSON::GetObjectFieldValue(obj, "cond_type");
        uint8_t typeInt = AmountToRawValue(typeObj);
        UtxoCondType  type = UtxoCondType(typeInt);
        switch(type){
            case UtxoCondType::IP2SA: {
                vCond.push_back(make_shared<CSingleAddressCondIn>());
                break;
            }
            case UtxoCondType::IP2PH: {
                const Value& passwordObj = JSON::GetObjectFieldValue(obj,"password");
                string password = passwordObj.get_str();
                vCond.push_back(make_shared<CPasswordHashLockCondIn>(password));
                break;

            }
            case UtxoCondType::IP2MA: {

                const Value& mObj = JSON::GetObjectFieldValue(obj,"m");
                uint8_t m = AmountToRawValue(mObj);
                const Value& nObj = JSON::GetObjectFieldValue(obj,"n");
                uint8_t n = AmountToRawValue(nObj);

                const Array& uidArray = JSON::GetObjectFieldValue(obj, "uids").get_array();
                vector<CUserID> vUid;
                for(auto u: uidArray){
                    CUserID uid = RPC_PARAM::GetUserId(u);
                    vUid.push_back(uid);
                }

                const Array& signArray = JSON::GetObjectFieldValue(obj, "signatures").get_array();
                vector<UnsignedCharArray> vSign;
                for (auto s: signArray) {
                    UnsignedCharArray unsignedCharArray = ParseHex(s.get_str());
                    vSign.push_back(unsignedCharArray);
                }

                vCond.push_back(make_shared<CMultiSignAddressCondIn>(m,n,vUid,vSign));
                break;

            }

            case UtxoCondType::OP2SA: {
                const Value& uidV = JSON::GetObjectFieldValue(obj,"uid");
                CUserID uid = RPC_PARAM::GetUserId(uidV);
                vCond.push_back(make_shared<CSingleAddressCondOut>(uid));
                break;
            }
            case UtxoCondType::OP2PH: {
                const Value& passwordObj = JSON::GetObjectFieldValue(obj,"password");
                string password = passwordObj.get_str();
                string text = strprintf("%s%s", txKeyID.ToString(), password);
                uint256 passwordHash = Hash(text);
                const Value& pwdProofRequiredObj = JSON::GetObjectFieldValue(obj, "password_proof_required");
                bool password_proof_required = pwdProofRequiredObj.get_bool();
                vCond.push_back(make_shared<CPasswordHashLockCondOut>(password_proof_required,passwordHash));
                break;
            }
            case UtxoCondType::OP2MA: {


                const Value& addrV = JSON::GetObjectFieldValue(obj,"multisign_address");
                CKeyID multiSignKeyId(addrV.get_str());
                if (multiSignKeyId.IsNull()) {
                    throw  JSONRPCError(RPC_INVALID_PARAMETER,
                            strprintf("the multi sign address (%s) is illegal",addrV.get_str()));
                }
                vCond.push_back(make_shared<CMultiSignAddressCondOut>(multiSignKeyId));
                break;
            }

            case UtxoCondType::OCLAIM_LOCK: {
                uint64_t height = AmountToRawValue(JSON::GetObjectFieldValue(obj, "height"));
                vCond.push_back(make_shared<CClaimLockCondOut>(height));
                break;
            }

            case UtxoCondType::ORECLAIM_LOCK: {

                uint64_t height = AmountToRawValue(JSON::GetObjectFieldValue(obj, "height"));
                vCond.push_back(make_shared<CReClaimLockCondOut>(height));
                break;
            }

            case UtxoCondType ::NULL_UTXOCOND_TYPE:
            default:
                throw JSONRPCError(RPC_INVALID_PARAMETER, "condition type error");

        }
    }

}
