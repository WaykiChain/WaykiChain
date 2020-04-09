// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "txreceiptdb.h"
#include "config/chainparams.h"

bool CTxReceiptDBCache::SetTxReceipts(const TxID &txid, const vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return true;

    return tx_receipt_cache.SetData(txid, receipts);
}

bool CTxReceiptDBCache::GetTxReceipts(const TxID &txid, vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return false;

    return tx_receipt_cache.GetData(txid, receipts);
}

bool CTxReceiptDBCache::SetBlockReceipts(const uint256 &blockHash, const vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return true;

    return block_receipt_cache.SetData(blockHash, receipts);
}

bool CTxReceiptDBCache::GetBlockReceipts(const uint256 &blockHash, vector<CReceipt> &receipts) {
    if (!SysCfg().IsGenReceipt())
        return false;

    return block_receipt_cache.GetData(blockHash, receipts);
}

void CTxReceiptDBCache::Flush() {
    tx_receipt_cache.Flush();
    block_receipt_cache.Flush();
}
