// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdb.h"
#include "entities/key.h"
#include "commons/uint256.h"
#include "commons/util/util.h"
#include "main.h"

#include <stdint.h>

using namespace std;


/********************** CBlockIndexDB ********************************/
bool CBlockIndexDB::WriteBlockIndex(const CDiskBlockIndex &blockIndex) {
    return Write(dbk::GenDbKey(dbk::BLOCK_INDEX, blockIndex.GetBlockHash()), blockIndex);
}
bool CBlockIndexDB::EraseBlockIndex(const uint256 &blockHash) {
    return Erase(dbk::GenDbKey(dbk::BLOCK_INDEX, blockHash));
}

bool CBlockIndexDB::LoadBlockIndexes() {
    leveldb::Iterator *pCursor = NewIterator();
    const std::string &prefix = dbk::GetKeyPrefix(dbk::BLOCK_INDEX);

    pCursor->Seek(prefix);

    // Load mapBlockIndex
    while (pCursor->Valid()) {
        boost::this_thread::interruption_point();
        try {
            leveldb::Slice slKey = pCursor->key();
            if (slKey.starts_with(prefix)) {
                leveldb::Slice slValue = pCursor->value();
                CDataStream ssValue(slValue.data(), slValue.data() + slValue.size(), SER_DISK, CLIENT_VERSION);
                CDiskBlockIndex diskIndex;
                ssValue >> diskIndex;

                // Construct block index object
                CBlockIndex *pIndexNew    = InsertBlockIndex(diskIndex.GetBlockHash());
                pIndexNew->pprev          = InsertBlockIndex(diskIndex.hashPrev);
                pIndexNew->height         = diskIndex.height;
                pIndexNew->nFile          = diskIndex.nFile;
                pIndexNew->nDataPos       = diskIndex.nDataPos;
                pIndexNew->nUndoPos       = diskIndex.nUndoPos;
                pIndexNew->nVersion       = diskIndex.nVersion;
                pIndexNew->merkleRootHash = diskIndex.merkleRootHash;
                pIndexNew->hashPos        = diskIndex.hashPos;
                pIndexNew->nTime          = diskIndex.nTime;
                pIndexNew->nBits          = diskIndex.nBits;
                pIndexNew->nNonce         = diskIndex.nNonce;
                pIndexNew->nStatus        = diskIndex.nStatus;
                pIndexNew->nTx            = diskIndex.nTx;
                pIndexNew->nFuelFee          = diskIndex.nFuelFee;
                pIndexNew->nFuelRate      = diskIndex.nFuelRate;
                pIndexNew->vSignature     = diskIndex.vSignature;
                pIndexNew->miner          = diskIndex.miner;

                if (!pIndexNew->CheckIndex())
                    return ERRORMSG("LoadBlockIndex() : CheckIndex failed: %s", pIndexNew->ToString());

                pCursor->Next();
            } else {
                break;  // if shutdown requested or finished loading block index
            }
        } catch (std::exception &e) {
            return ERRORMSG("Deserialize or I/O error - %s", e.what());
        }
    }
    delete pCursor;

    return true;
}

bool CBlockIndexDB::WriteBlockFileInfo(int32_t nFile, const CBlockFileInfo &info) {
    return Write(dbk::GenDbKey(dbk::BLOCKFILE_NUM_INFO, nFile), info);
}
bool CBlockIndexDB::ReadBlockFileInfo(int32_t nFile, CBlockFileInfo &info) {
    return Read(dbk::GenDbKey(dbk::BLOCKFILE_NUM_INFO, nFile), info);
}

CBlockIndex *InsertBlockIndex(uint256 hash) {
    if (hash.IsNull())
        return nullptr;

    // Return existing
    map<uint256, CBlockIndex *>::iterator mi = mapBlockIndex.find(hash);
    if (mi != mapBlockIndex.end())
        return (*mi).second;

    // Create new
    CBlockIndex *pIndexNew = new CBlockIndex();
    if (!pIndexNew)
        throw runtime_error("new CBlockIndex failed");
    mi                    = mapBlockIndex.insert(make_pair(hash, pIndexNew)).first;
    pIndexNew->pBlockHash = &((*mi).first);

    return pIndexNew;
}


/************************* CBlockDBCache ****************************/
uint32_t CBlockDBCache::GetCacheSize() const {
    return
        tx_diskpos_cache.GetCacheSize() +
        flag_cache.GetCacheSize() +
        best_block_hash_cache.GetCacheSize() +
        last_block_file_cache.GetCacheSize() +
        reindex_cache.GetCacheSize() +
        finality_block_cache.GetCacheSize();
}

bool CBlockDBCache::Flush() {
    tx_diskpos_cache.Flush();
    flag_cache.Flush();
    best_block_hash_cache.Flush();
    last_block_file_cache.Flush();
    reindex_cache.Flush();
    finality_block_cache.Flush();
    return true;
}

uint256 CBlockDBCache::GetBestBlockHash() const {
    uint256 blockHash;
    best_block_hash_cache.GetData(blockHash);
    return blockHash;
}

bool CBlockDBCache::SetBestBlock(const uint256 &blockHashIn) {
    return best_block_hash_cache.SetData(blockHashIn);
}

bool CBlockDBCache::WriteLastBlockFile(int32_t nFile) {
    return last_block_file_cache.SetData(nFile);
}
bool CBlockDBCache::ReadLastBlockFile(int32_t &nFile) {
    return last_block_file_cache.GetData(nFile);
}

bool CBlockDBCache::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return tx_diskpos_cache.GetData(txid, pos);
}

bool CBlockDBCache::SetTxIndex(const uint256 &txid, const CDiskTxPos &pos) {
    return tx_diskpos_cache.SetData(txid, pos);
}

bool CBlockDBCache::WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list) {
    for (auto it : list) {
        LogPrint(BCLog::DEBUG, "%-30s txid:%s dispos: nFile=%d, nPos=%d nTxOffset=%d\n",
                it.first.GetHex(), it.second.nFile, it.second.nPos, it.second.nTxOffset);

        if (!tx_diskpos_cache.SetData(it.first, it.second))
            return false;
    }
    return true;
}

bool CBlockDBCache::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return reindex_cache.SetData(true);
    else
        return reindex_cache.EraseData();
}
bool CBlockDBCache::ReadReindexing(bool &fReindexing) {
    return reindex_cache.GetData(fReindexing);
}

bool CBlockDBCache::WriteFlag(const string &name, bool fValue) {
    return flag_cache.SetData(name, fValue);
}
bool CBlockDBCache::ReadFlag(const string &name, bool &fValue) {
    return flag_cache.GetData(name, fValue);
}
bool CBlockDBCache::WriteGlobalFinBlock(const int32_t height, const uint256 hash) {
    finality_block_cache.SetData(std::make_pair(height, hash));
    return true;
}
bool CBlockDBCache::ReadGlobalFinBlock(std::pair<int32_t,uint256>& block) {
    return finality_block_cache.GetData(block);
}