// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txutxodb.h"
#include "config/chainparams.h"

//////////////////////////////////
//// UTXO Cache
/////////////////////////////////
bool CTxUTXODBCache::SetUtxoTx(const pair<TxID, CFixedUInt16> &utxoKey) {
    return tx_utxo_cache.SetData(utxoKey, 1);
}

bool CTxUTXODBCache::GetUtxoTx(const pair<TxID, CFixedUInt16> &utxoKey) {
    uint8_t data;
    bool result = tx_utxo_cache.GetData(utxoKey, data);
    if (!result)
        return false;
    
    return true;
}
bool CTxUTXODBCache::DelUtoxTx(const pair<TxID, CFixedUInt16> &utxoKey) {
    return tx_utxo_cache.EraseData(utxoKey);
}

//////////////////////////////////
//// Password Proof Cache
/////////////////////////////////
bool CTxUTXODBCache::SetUtxoPasswordProof(const tuple<TxID, CFixedUInt16, CRegIDKey> &proofKey, uint256 &proof) {
    return tx_utxo_password_proof_cache.SetData(proofKey, proof);
}

bool CTxUTXODBCache::GetUtxoPasswordProof(const tuple<TxID, CFixedUInt16, CRegIDKey> &proofKey, uint256 &proof) {
    return tx_utxo_password_proof_cache.GetData(proofKey, proof);
}

bool CTxUTXODBCache::DelUtoxPasswordProof(const tuple<TxID, CFixedUInt16, CRegIDKey> &proofKey) {
    return tx_utxo_password_proof_cache.EraseData(proofKey);
}