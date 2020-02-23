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

/**
 * Coin UTXO Trx that supports following features:
 *    1. UTXO - modify only the balance of the originator account but not the destination account
 *    2. Lock duration - None can collect the amount within the period
 *    3. HTLC (collect secret hash, collect address (optional), expiry block-height)
 *      3.1 Hash Lock, must match with the hash in order to collect the coins within
 *      3.2 Time Lock, after which it can be recollected by the originator
 *
 */
class CCoinUTXOTx: public CBaseTx {
public:
    TxID prior_utxo_txid;                   // optional, fund source: this prior utxo and Tx account balance
    string prior_utxo_secret;               // optional, supplied when prior utxo existing whereas secret matches its internal hash
    UTXOEntity utxo;                        // when empty, it collects/withdraws the total amount in prior utxo into its account balance
    string memo;                            // memo field, can be empty

public:
    CCoinUTXOTx()
        : CBaseTx(UTXO_TRANSFER_TX) {};

    CCoinUTXOTx(const CUserID &txUidIn, const int32_t validHeightIn, const TokenSymbol &feeSymbol, const uint64_t feesIn,
                const UTXOEntity &utxoIn, string &memoIn)
        : CBaseTx(UTXO_TRANSFER_TX, txUidIn, validHeightIn, feeSymbol, feesIn),
          utxo(utxoIn), memo(memoIn) {};

    ~CCoinUTXOTx() {};

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(valid_height));
        READWRITE(txUid);
        READWRITE(fee_symbol);
        READWRITE(VARINT(llFees));

        READWRITE(utxo);
        READWRITE(memo);

        READWRITE(signature);
    )

    virtual void SerializeForHash(CHashWriter &hw) const {
        hw << VARINT(nVersion) << uint8_t(nTxType) << VARINT(valid_height) << txUid << fee_symbol << VARINT(llFees)
           << utxo << memo;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCoinUTXOTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    string ToString() const { return ""; } // TODO: fix me

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);


    bool IsEmpty() const {
        return prior_utxo_txid.IsEmpty() && prior_utxo_secret.empty() && utxo.IsEmpty() && memo.empty();
    }

    void SetEmpty() {
        prior_utxo_txid.SetEmpty();
        prior_utxo_secret = "";
        utxo.SetEmpty();
        memo = "";
    }
};

#endif // TX_COIN_UTXO_H