// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef TX_COIN_UTXO_H
#define TX_COIN_UTXO_H

#include <string>
#include <cstdint>

#include "tx.h"
#include "entities/utxo.h"


bool ComputeRedeemScript(const uint8_t m, const uint8_t n, vector<string>& addresses, string &redeemScript);
bool ComputeMultiSignKeyId(const string &redeemScript, CKeyID &keyId);
bool ComputeUtxoMultisignHash(const TxID &prevUtxoTxId, uint16_t prevUtxoTxVoutIndex, const CAccount &txAcct,
                            string &redeemScript, uint256 &hash);

////////////////////////////////////////
/// class CCoinUtxoTransferTx
////////////////////////////////////////
class CCoinUtxoTransferTx: public CBaseTx {
public:
    TokenSymbol coin_symbol;
    std::vector<CUtxoInput> vins;
    std::vector<CUtxoOutput> vouts;
    string memo;                            // memo field, can be empty

public:
    CCoinUtxoTransferTx()
        : CBaseTx(UTXO_TRANSFER_TX) {};

    CCoinUtxoTransferTx(const CUserID &txUidIn, const int32_t validHeightIn, const TokenSymbol &feeSymbol, const uint64_t feesIn,
                const TokenSymbol &coinSymbol, std::vector<CUtxoInput> &vinsIn, std::vector<CUtxoOutput> &voutsIn, string &memoIn)
        : CBaseTx(UTXO_TRANSFER_TX, txUidIn, validHeightIn, feeSymbol, feesIn),
          coin_symbol(coinSymbol), vins(vinsIn), vouts(voutsIn), memo(memoIn) {};

    ~CCoinUtxoTransferTx() {};

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(coin_symbol);
        READWRITE(vins);
        READWRITE(vouts);

        READWRITE(memo);
        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
           << coin_symbol << vins << vouts << memo;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCoinUtxoTransferTx>(*this); }

    virtual Object ToJson(CCacheWrapper &cw) const {
        Object obj = CBaseTx::ToJson(cw);

        Array inputArr;
        for(auto input: vins){
            inputArr.push_back(input.ToJson());
        }

        Array outputArr;
        for(auto output: vouts){
            outputArr.push_back(output.ToJson());
        }

        obj.push_back(Pair("coin_symbol", coin_symbol));
        obj.push_back(Pair("vins", inputArr));
        obj.push_back(Pair("vouts", outputArr));
        obj.push_back(Pair("memo", memo));

        return obj;
    }

    virtual string ToString(CAccountDBCache &accountCache) {
        return strprintf(
                "txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, "
                "valid_height=%d, coin_symbol=%s, vins=[%s], vouts=[%s], memo=%s",
                GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees,
                valid_height, coin_symbol, db_util::ToString(vins), db_util::ToString(vouts), HexStr(memo));
    }

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

};

////////////////////////////////////////
/// class CCoinUtxoPasswordProofTx
////////////////////////////////////////
class CCoinUtxoPasswordProofTx: public CBaseTx {
public:
    TxID        utxo_txid;
    uint16_t    utxo_vout_index = 0;
    uint256     password_proof;

public:
    CCoinUtxoPasswordProofTx()
        : CBaseTx(UTXO_PASSWORD_PROOF_TX) {};

    CCoinUtxoPasswordProofTx(const CUserID &txUidIn, const int32_t validHeightIn, const TokenSymbol &feeSymbol, const uint64_t feesIn,
                TxID &utoxTxid, uint16_t utxoOutIndex, uint256 &passwordProof)
        : CBaseTx(UTXO_PASSWORD_PROOF_TX, txUidIn, validHeightIn, feeSymbol, feesIn),
          utxo_txid(utoxTxid), utxo_vout_index(utxoOutIndex), password_proof(passwordProof) {};

    ~CCoinUtxoPasswordProofTx() {};

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(utxo_txid);
        READWRITE(VARINT(utxo_vout_index));
        READWRITE(password_proof);

        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
           << utxo_txid << utxo_vout_index << password_proof;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCoinUtxoPasswordProofTx>(*this); }

    virtual Object ToJson(CCacheWrapper &cw) const {
        Object obj = CBaseTx::ToJson(cw);

        obj.push_back(Pair("utxo_txid", utxo_txid.ToString()));
        obj.push_back(Pair("utxo_vout_index", utxo_vout_index));
        obj.push_back(Pair("password_proof", password_proof.ToString()));

        return obj;
    }

    virtual string ToString(CAccountDBCache &accountCache) {
        return strprintf(
                "txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, "
                "valid_height=%d, utxo_txid=[%s], utxo_vout_index=[%d], password_proof=%s",
                GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees,
                valid_height, utxo_txid.ToString(), utxo_vout_index, password_proof.ToString());
    }

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

};

#endif // TX_COIN_UTXO_H