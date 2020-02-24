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
class CCoinUtxoTx: public CBaseTx {
public:
    TokenSymbol coin_symbol;
    std::vector<CUtxoInput> vins;
    std::vector<CUtxoOutput> vouts;
    string memo;                            // memo field, can be empty

public:
    CCoinUtxoTx()
        : CBaseTx(UTXO_TRANSFER_TX) {};

    CCoinUtxoTx(const CUserID &txUidIn, const int32_t validHeightIn, const TokenSymbol &feeSymbol, const uint64_t feesIn, 
                const TokenSymbol &coinSymbol, std::vector<CUtxoInput> &vinsIn, std::vector<CUtxoOutput> &voutsIn, string &memoIn)
        : CBaseTx(UTXO_TRANSFER_TX, txUidIn, validHeightIn, feeSymbol, feesIn),
          coin_symbol(coinSymbol), vins(vinsIn), vouts(voutsIn), memo(memoIn) {};

    ~CCoinUtxoTx() {};

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

    virtual std::shared_ptr<CBaseTx> GetNewInstance() const { return std::make_shared<CCoinUtxoTx>(*this); }
    virtual string ToString(CAccountDBCache &accountCache);
    virtual Object ToJson(const CAccountDBCache &accountCache) const;

    string ToString() const { return ""; } // TODO: fix me

    virtual bool CheckTx(CTxExecuteContext &context);
    virtual bool ExecuteTx(CTxExecuteContext &context);

};

#endif // TX_COIN_UTXO_H