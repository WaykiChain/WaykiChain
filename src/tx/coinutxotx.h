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


////////////////////////////////////////
/// class CCoinUtxoTransferTx
////////////////////////////////////////
/**
 * Coin UTXO Trx that supports following features:
 *    1. UTXO - modify only the balance of the originator account but not the destination account
 *    2. Lock duration - None can collect the amount within the period
 *    3. HTLC (collect secret hash, collect address (optional), expiry block-height)
 *      3.1 Hash Lock, must match with the hash in order to collect the coins within
 *      3.2 Time Lock, after which it can be recollected by the originator
 *
 */
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

        READWRITE(vins);
        READWRITE(vouts);

        READWRITE(memo);
        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees) 
           << vins << vouts << memo;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCoinUtxoTransferTx>(*this); }

    virtual Object ToJson(const CAccountDBCache &accountCache) const {
        Object obj = CBaseTx::ToJson(accountCache);

        obj.push_back(Pair("vins", db_util::ToString(vins)));
        obj.push_back(Pair("vouts", db_util::ToString(vouts)));
        obj.push_back(Pair("memo", memo));

        return obj;
    }

    string ToString() const {
        return strprintf(
                "txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, "
                "valid_height=%d, vins=[%s], vouts=[%s], memo=%s",
                GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees,
                valid_height, db_util::ToString(vins), db_util::ToString(vouts), HexStr(memo));
    }
    virtual string ToString(CAccountDBCache &accountCache);

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

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCoinUtxoTransferTx>(*this); }
    
    virtual Object ToJson(const CAccountDBCache &accountCache) const {
        Object obj = CBaseTx::ToJson(accountCache);

        obj.push_back(Pair("utxo_txid", utxo_txid.ToString()));
        obj.push_back(Pair("utxo_vout_index", utxo_vout_index));
        obj.push_back(Pair("password_proof", password_proof.ToString()));

        return obj;
    }

    string ToString() const {
        return strprintf(
                "txType=%s, hash=%s, ver=%d, txUid=%s, fee_symbol=%s, llFees=%llu, "
                "valid_height=%d, utxo_txid=[%s], utxo_vout_index=[%d], password_proof=%s",
                GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), fee_symbol, llFees,
                valid_height, utxo_txid.ToString(), utxo_vout_index, password_proof.ToString());
    }
    virtual string ToString(CAccountDBCache &accountCache);

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

};

#endif // TX_COIN_UTXO_H