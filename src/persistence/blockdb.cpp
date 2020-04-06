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
                pIndexNew->nFuel          = diskIndex.nFuel;
                pIndexNew->nFuelRate      = diskIndex.nFuelRate;
                pIndexNew->vSignature     = diskIndex.vSignature;
                pIndexNew->miner          = diskIndex.miner ;

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
        txDiskPosCache.GetCacheSize() +
        flagCache.GetCacheSize() +
        bestBlockHashCache.GetCacheSize() +
        lastBlockFileCache.GetCacheSize() +
        reindexCache.GetCacheSize() +
        finalityBlockCache.GetCacheSize() ;
}

bool CBlockDBCache::Flush() {
    txDiskPosCache.Flush();
    flagCache.Flush();
    bestBlockHashCache.Flush();
    lastBlockFileCache.Flush();
    reindexCache.Flush();
    finalityBlockCache.Flush();
    return true;
}

uint256 CBlockDBCache::GetBestBlockHash() const {
    uint256 blockHash;
    bestBlockHashCache.GetData(blockHash);
    return blockHash;
}

bool CBlockDBCache::SetBestBlock(const uint256 &blockHashIn) {
    return bestBlockHashCache.SetData(blockHashIn);
}

bool CBlockDBCache::WriteLastBlockFile(int32_t nFile) {
    return lastBlockFileCache.SetData(nFile);
}
bool CBlockDBCache::ReadLastBlockFile(int32_t &nFile) {
    return lastBlockFileCache.GetData(nFile);
}

bool CBlockDBCache::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) {
    return txDiskPosCache.GetData(txid, pos);
}

bool CBlockDBCache::SetTxIndex(const uint256 &txid, const CDiskTxPos &pos) {
    return txDiskPosCache.SetData(txid, pos);
}

bool CBlockDBCache::WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list) {
    for (auto it : list) {
        LogPrint(BCLog::DEBUG, "%-30s txid:%.7s** dispos: nFile=%d, nPos=%d nTxOffset=%d\n",
                it.first.GetHex(), it.second.nFile, it.second.nPos, it.second.nTxOffset);

        if (!txDiskPosCache.SetData(it.first, it.second))
            return false;
    }
    return true;
}

bool CBlockDBCache::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return reindexCache.SetData(true);
    else
        return reindexCache.EraseData();
}
bool CBlockDBCache::ReadReindexing(bool &fReindexing) {
    return reindexCache.GetData(fReindexing);
}

bool CBlockDBCache::WriteFlag(const string &name, bool fValue) {
    return flagCache.SetData(name, fValue);
}
bool CBlockDBCache::ReadFlag(const string &name, bool &fValue) {
    return flagCache.GetData(name, fValue);
}
bool CBlockDBCache::WriteGlobalFinBlock(const int32_t height, const uint256 hash) {
    finalityBlockCache.SetData(std::make_pair(height, hash)) ;
    return true ;
}
bool CBlockDBCache::ReadGlobalFinBlock(std::pair<int32_t,uint256>& block) {
    return finalityBlockCache.GetData(block) ;
}