// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txutxodb.h"
#include "config/chainparams.h"

//////////////////////////////////
//// UTXO Cache
/////////////////////////////////
bool CTxUTXODBCache::SetUtxoTx(const pair<TxID, uint16_t> &utxoKey) {
    return txUtxoCache.SetData(utxoKey, 1);
}

bool CTxUTXODBCache::GetUtxoTx(const pair<TxID, uint16_t> &utxoKey) {
    uint8_t data;
    bool result = txUtxoCache.GetData(utxoKey, data);
    if (!result)
        return false;
    
    return true;
}
bool CTxUTXODBCache::DelUtoxTx(const pair<TxID, uint16_t> &utxoKey) {
    return txUtxoCache.EraseData(utxoKey);
}

//////////////////////////////////
//// Password Proof Cache
/////////////////////////////////
bool CTxUTXODBCache::SetUtxoPasswordProof(const tuple<TxID, uint16_t, CRegIDKey> &proofKey, uint256 &proof) {
    return txUtxoPasswordProofCache.SetData(proofKey, proof);
}

bool CTxUTXODBCache::GetUtxoPasswordProof(const tuple<TxID, uint16_t, CRegIDKey> &proofKey, uint256 &proof) {
    bool result = txUtxoPasswordProofCache.GetData(proofKey, proof);
    if (!result)
        return false;
    
    return true;
}
bool CTxUTXODBCache::DelUtoxPasswordProof(const tuple<TxID, uint16_t, CRegIDKey> &proofKey) {
    return txUtxoPasswordProofCache.EraseData(proofKey);
}
