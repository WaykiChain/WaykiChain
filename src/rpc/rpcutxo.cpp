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
using namespace std;

enum CondDirection: uint8_t {
    IN,
    OUT
};

void ParseCond(const Array& arr, vector<shared_ptr<CUtxoCond>>& vCond);
void ParseInput(const Array& arr, vector<CUtxoInput>& vInput);
void ParseOutput(const Array& arr, vector<CUtxoOutput>& vOutput);
void CheckCondDirection(const vector<shared_ptr<CUtxoCond>>& vCond, CondDirection direction);

Value submitpasswordprooftx(const Array& params, bool fHelp) {

    if(fHelp || params.size() < 4 || params.size() > 5) {
        throw runtime_error(
                "submitpasswordprooftx \"addr\" \"utxo_txid\" \"utxo_vout_index\" \"password_proof\" \"symbol:fee:unit\" \n"
                "\nSubmit a password proof.\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"addr\":                (string, required) the addr submit this tx\n"
                "2.\"utxo_txid\":           (string, required) The utxo txid you want to spend\n"
                "3.\"utxo_vout_index\":     (string, required) The index of utxo output \n"
                "4.\"password_proof\":      (symbol:amount:unit, required) password proof\n"
                "5.\"symbol:fee:unit\":     (symbol:amount:unit, optinal) fee paid to miner\n"
                "\nResult:\n"
                "\"txid\"                   (string) The transaction id.\n"
                "\nExamples:\n" +
                HelpExampleCli("submitpasswordprooftx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\" \"23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds\" "
                               "\"5\" \"eowdswd0-eowpds23ewdswwedscde\" \"WICC:10000:sawi\"") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("submitpasswordprooftx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\", \"23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds\", "
                               "\"5\", \"eowdswd0-eowpds23ewdswwedscde\", \"WICC:10000:sawi\"")
                );
    }

    EnsureWalletIsUnlocked();
    CUserID txUid = RPC_PARAM::GetUserId(params[0], true );
    TxID utxoTxid = uint256S(params[1].get_str()) ;
    uint16_t utxoVoutIndex = params[2].get_int() ;
    uint256 passwordProof = uint256S(params[3].get_str()) ;
    ComboMoney fee = RPC_PARAM::GetFee(params, 4 ,TxType::UTXO_PASSWORD_PROOF_TX) ;
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    int32_t validHeight  = chainActive.Height();
    CCoinUtxoPasswordProofTx tx(txUid,validHeight, fee.symbol,fee.GetAmountInSawi(),
            utxoTxid,utxoVoutIndex,passwordProof);

    return SubmitTx(account.keyid, tx);


}

Value submitutxotransfertx(const Array& params, bool fHelp) {

    if(fHelp || params.size() <4 || params.size() > 6) {
        throw runtime_error(
                "submitutxotransfertx \"addr\" \"coin_symbol\" \"vins\" \"vouts\" \"symbol:fee:unit\" \"memo\" \n"
                "\nSubmit a password proof.\n" +
                HelpRequiringPassphrase() +
                "\nArguments:\n"
                "1.\"addr\":              (string, required) the addr submit this tx\n"
                "2.\"coin_symbol\":       (string, required) The utxo transfer coin symbole\n"
                "3.\"vins\":              (string(json), required) The utxo inputs \n"
                "4.\"vouts\":             (string(json), required) the utxo outputs \n"
                "5.\"symbol:fee:unit\":   (symbol:amount:unit, optional) fee paid to miner\n"
                "5.\"memo\":              (string,optinal) tx memo\n"
                "\nResult:\n"
                "\"txid\"                   (string) The transaction id.\n"
                "\nExamples:\n" +
                HelpExampleCli("submitutxotransfertx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\" \"WICC\" \"[{}]\" "
                               " \"[{}]\" \"WICC:10000:sawi\" \"xx\"") +
                "\nAs json rpc call\n" +
                HelpExampleRpc("submitutxotransfertx",
                               "\"wLKf2NqwtHk3BfzK5wMDfbKYN1SC3weyR4\", \"23ewf90203ew000ds0lwsdpoxewdokwesdxcoekdleds\", "
                               "\"5\", \"eowdswd0-eowpds23ewdswwedscde\", \"WICC:10000:sawi\"")
        );
    }

    EnsureWalletIsUnlocked();

    CUserID txUid = RPC_PARAM::GetUserId(params[0], true);
    TokenSymbol coinSymbol = params[1].get_str();
    Array inputArray  = params[2].get_array();
    Array outputArray = params[3].get_array();
    string memo;
    if(params.size() > 4) {
        memo = params[4].get_str();
    }

    ComboMoney fee = RPC_PARAM::GetFee(params,5 ,TxType::UTXO_TRANSFER_TX);
    CAccount account = RPC_PARAM::GetUserAccount(*pCdMan->pAccountCache, txUid);
    RPC_PARAM::CheckAccountBalance(account, fee.symbol, SUB_FREE, fee.GetAmountInSawi());

    std::vector<CUtxoInput> vins;
    std::vector<CUtxoOutput> vouts;
    ParseInput(inputArray, vins);
    ParseOutput(outputArray, vouts);


    int32_t validHeight  = chainActive.Height();
    CCoinUtxoTransferTx utxoTransferTx(txUid,validHeight,fee.symbol,fee.GetAmountInSawi(),
                                  coinSymbol,vins,vouts,memo);

    return SubmitTx(account.keyid, utxoTransferTx);

}


void TransToStorageBean(const vector<shared_ptr<CUtxoCond>>& vCond, vector<CUtxoCondStorageBean>& vCondStorageBean) {

    for (auto& cond: vCond ) {
        auto bean = CUtxoCondStorageBean(cond);
        vCondStorageBean.push_back(bean);
    }
}

void ParseInput(const Array& arr, vector<CUtxoInput>& vInput){


    for (auto obj : arr) {

        const Value& preTxidObj  = JSON::GetObjectFieldValue(obj, "prev_utxo_txid");
        TxID prevUtxoTxid = uint256S(preTxidObj.get_str()) ;
        const Value& preVoutIndexObj = JSON::GetObjectFieldValue(obj,"prev_utxo_vout_index");
        uint16_t  preUoutIndex = AmountToRawValue(preVoutIndexObj) ;
        const Array& condArray = JSON::GetObjectFieldValue(obj, "conds").get_array();
        vector<CUtxoCondStorageBean> vCondStorageBean ;
        vector<shared_ptr<CUtxoCond>> vCond ;
        ParseCond(condArray, vCond) ;
        CheckCondDirection(vCond, CondDirection::IN);
        TransToStorageBean(vCond, vCondStorageBean) ;
        CUtxoInput input = CUtxoInput(prevUtxoTxid,preUoutIndex,vCondStorageBean);
        vInput.push_back(input);

    }

}


void ParseOutput(const Array& arr, vector<CUtxoOutput>& vOutput) {

    for (auto& obj : arr) {

        const Value& coinAmountObj  = JSON::GetObjectFieldValue(obj, "coin_amount");
        uint64_t coinAmount = AmountToRawValue(coinAmountObj);
        const Array& condArray = JSON::GetObjectFieldValue(obj, "conds").get_array();
        vector<CUtxoCondStorageBean> vCondStorageBean;
        vector<shared_ptr<CUtxoCond>> vCond;
        ParseCond(condArray, vCond);
        CheckCondDirection(vCond, CondDirection::OUT);
        TransToStorageBean(vCond, vCondStorageBean);
        CUtxoOutput output = CUtxoOutput(coinAmount, vCondStorageBean);
        vOutput.push_back(output);

    }
}


void CheckCondDirection(const vector<shared_ptr<CUtxoCond>>& vCond, CondDirection direction) {


    for (auto& cond:vCond) {
        switch (cond->cond_type) {
            case IP2MA:
            case IP2PH:
            case IP2SA:
                if(direction != CondDirection::IN){
                    throw JSONRPCError(RPC_INVALID_PARAMETER,"cond direction is error");
                }
                break;
            case OP2MA:
            case OP2PH:
            case OP2SA:
            case OCLAIM_LOCK:
            case ORECLAIM_LOCK:
                if(direction != CondDirection::OUT) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER,"cond direction is error");
                }
                break;
            case NULL_UTXOCOND_TYPE:
            default:
                throw JSONRPCError(RPC_INVALID_PARAMETER, "cond type is error") ;

        }

    }

}
void ParseCond(const Array& arr, vector<shared_ptr<CUtxoCond>>& vCond) {

    for (auto& obj : arr) {
        const Value& typeObj = JSON::GetObjectFieldValue(obj, "cond_type");
        uint8_t typeInt = AmountToRawValue(typeObj) ;
        UtxoCondType  type = UtxoCondType(typeInt);
        switch(type){
            case UtxoCondType::IP2SA: {
                vCond.push_back(make_shared<CSingleAddressCondIn>());
                break;
            }
            case UtxoCondType::IP2PH: {
                const Value& passwordObj = JSON::GetObjectFieldValue(obj,"password");
                string password = passwordObj.get_str() ;
                vCond.push_back(make_shared<CPasswordHashLockCondIn>(password));
                break ;

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
                vector<UnsignedCharArray> vSign ;
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
                const Value& passwordObj = JSON::GetObjectFieldValue(obj,"password_hash");
                string passwordHashString = passwordObj.get_str();
                const Value& pwdProofRequiredObj = JSON::GetObjectFieldValue(obj, "password_proof_required");
                bool password_proof_required = pwdProofRequiredObj.get_bool();
                uint256 passwordHash = uint256S(passwordHashString);
                vCond.push_back(make_shared<CPasswordHashLockCondOut>(password_proof_required,passwordHash));
                break;
            }
            case UtxoCondType::OP2MA: {
                const Value& uidV = JSON::GetObjectFieldValue(obj,"uid") ;
                CUserID uid = RPC_PARAM::GetUserId(uidV) ;
                vCond.push_back(make_shared<CMultiSignAddressCondOut>(uid)) ;
                break;
            }

            case UtxoCondType::OCLAIM_LOCK: {
                uint64_t height = AmountToRawValue(JSON::GetObjectFieldValue(obj, "height")) ;
                vCond.push_back(make_shared<CClaimLockCondOut>(height)) ;
                break;
            }

            case UtxoCondType::ORECLAIM_LOCK: {

                uint64_t height = AmountToRawValue(JSON::GetObjectFieldValue(obj, "height")) ;
                vCond.push_back(make_shared<CReClaimLockCondOut>(height)) ;
                break;
            }

            case UtxoCondType ::NULL_UTXOCOND_TYPE:
            default:
                throw JSONRPCError(RPC_INVALID_PARAMETER, "condition type error") ;

        }
    }

}

