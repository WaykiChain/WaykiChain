// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "blockdb.h"
#include "entities/key.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include "main.h"

#include <stdint.h>

using namespace std;

//void static BatchWriteHashBestChain(CLevelDBBatch &batch, const uint256 &hash) {
//	batch.Write('B', hash);
//}

CBlockTreeDB::CBlockTreeDB(size_t nCacheSize, bool fMemory, bool fWipe) :
    CLevelDBWrapper(GetDataDir() / "blocks" / "index", nCacheSize, fMemory, fWipe) {}

bool CBlockTreeDB::WriteBlockIndex(const CDiskBlockIndex &blockindex) {
    return Write(dbk::GenDbKey(dbk::BLOCK_INDEX, blockindex.GetBlockHash()), blockindex);
}

bool CBlockTreeDB::EraseBlockIndex(const uint256 &blockHash) {
    return Erase(dbk::GenDbKey(dbk::BLOCK_INDEX, blockHash));
}

// TODO: need to delete WriteBestInvalidWork
// bool CBlockTreeDB::WriteBestInvalidWork(const uint256 &bnBestInvalidWork) {
//     // Obsolete; only written for backward compatibility.
//     return Write('I', bnBestInvalidWork);
// }

bool CBlockTreeDB::WriteBlockFileInfo(int nFile, const CBlockFileInfo &info) {
    return Write(dbk::GenDbKey(dbk::BLOCKFILE_NUM_INFO, nFile), info);
}

bool CBlockTreeDB::ReadBlockFileInfo(int nFile, CBlockFileInfo &info) {
    return Read(dbk::GenDbKey(dbk::BLOCKFILE_NUM_INFO, nFile), info);
}

bool CBlockTreeDB::WriteLastBlockFile(int nFile) {
    return Write(dbk::GetKeyPrefix(dbk::LAST_BLOCKFILE), nFile);
}

bool CBlockTreeDB::WriteReindexing(bool fReindexing) {
    if (fReindexing)
        return Write(dbk::GetKeyPrefix(dbk::REINDEX), '1');
    else
        return Erase(dbk::GetKeyPrefix(dbk::REINDEX));
}

bool CBlockTreeDB::ReadReindexing(bool &fReindexing) {
    fReindexing = Exists(dbk::GetKeyPrefix(dbk::REINDEX));
    return true;
}

bool CBlockTreeDB::ReadLastBlockFile(int &nFile) {
    return Read(dbk::GetKeyPrefix(dbk::LAST_BLOCKFILE), nFile);
}

// bool CBlockTreeDB::ReadTxIndex(const uint256 &txid, CDiskTxPos &pos) { return Read(make_pair('t', txid), pos); }

// bool CBlockTreeDB::WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &vect) {
//     CLevelDBBatch batch;
//     for (vector<pair<uint256, CDiskTxPos> >::const_iterator it = vect.begin(); it != vect.end(); it++) {
//         LogPrint("txindex", "txid:%s dispos: nFile=%d, nPos=%d nTxOffset=%d\n", it->first.GetHex(), it->second.nFile,
//                  it->second.nPos, it->second.nTxOffset);
//         batch.Write(make_pair('t', it->first), it->second);
//     }
//     return WriteBatch(batch);
// }

bool CBlockTreeDB::WriteFlag(const string &name, bool fValue) {
    return Write(dbk::GenDbKey(dbk::FLAG, name), fValue ? '1' : '0');
}

bool CBlockTreeDB::ReadFlag(const string &name, bool &fValue) {
    char ch;
    if (!Read(dbk::GenDbKey(dbk::FLAG, name), ch))
        return false;
    fValue = ch == '1';
    return true;
}

bool CBlockTreeDB::LoadBlockIndexGuts() {
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
                CDiskBlockIndex diskindex;
                ssValue >> diskindex;

                // Construct block index object
                CBlockIndex *pIndexNew    = InsertBlockIndex(diskindex.GetBlockHash());
                pIndexNew->pprev          = InsertBlockIndex(diskindex.hashPrev);
                pIndexNew->height        = diskindex.height;
                pIndexNew->nFile          = diskindex.nFile;
                pIndexNew->nDataPos       = diskindex.nDataPos;
                pIndexNew->nUndoPos       = diskindex.nUndoPos;
                pIndexNew->nVersion       = diskindex.nVersion;
                pIndexNew->merkleRootHash = diskindex.merkleRootHash;
                pIndexNew->hashPos        = diskindex.hashPos;
                pIndexNew->nTime          = diskindex.nTime;
                pIndexNew->nBits          = diskindex.nBits;
                pIndexNew->nNonce         = diskindex.nNonce;
                pIndexNew->nStatus        = diskindex.nStatus;
                pIndexNew->nTx            = diskindex.nTx;
                pIndexNew->nFuel          = diskindex.nFuel;
                pIndexNew->nFuelRate      = diskindex.nFuelRate;
                pIndexNew->vSignature     = diskindex.vSignature;
                // TODO: Fees
                // pIndexNew->dFeePerKb      = diskindex.dFeePerKb;

                if (!pIndexNew->CheckIndex())
                    return ERRORMSG("LoadBlockIndex() : CheckIndex failed: %s", pIndexNew->ToString());

                pCursor->Next();
            } else {
                break;  // if shutdown requested or finished loading block index
            }
        } catch (std::exception &e) {
            return ERRORMSG("%s : Deserialize or I/O error - %s", __func__, e.what());
        }
    }
    delete pCursor;

    return true;
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
        throw runtime_error("LoadBlockIndex() : new CBlockIndex failed");
    mi                    = mapBlockIndex.insert(make_pair(hash, pIndexNew)).first;
    pIndexNew->pBlockHash = &((*mi).first);

    return pIndexNew;
}