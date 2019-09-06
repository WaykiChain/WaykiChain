// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_BLOCKDB_H
#define PERSIST_BLOCKDB_H

#include <string>
#include <utility>
#include <vector>
#include "commons/arith_uint256.h"
#include "leveldbwrapper.h"
#include "persistence/block.h"

#include <map>

/** Access to the block database (blocks/index/) */
class CBlockTreeDB : public CLevelDBWrapper {
private:
    CBlockTreeDB(const CBlockTreeDB &);
    void operator=(const CBlockTreeDB &);

public:
    CBlockTreeDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false);
    CBlockTreeDB(const std::string &name, size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool WriteBlockIndex(const CDiskBlockIndex &blockindex);
    bool EraseBlockIndex(const uint256 &blockHash);
    // TODO: need to delete WriteBestInvalidWork
//    bool WriteBestInvalidWork(const uint256 &bnBestInvalidWork);
    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &fileinfo);
    bool WriteBlockFileInfo(int nFile, const CBlockFileInfo &fileinfo);
    bool ReadLastBlockFile(int &nFile);
    bool WriteLastBlockFile(int nFile);
    bool WriteReindexing(bool fReindex);
    bool ReadReindexing(bool &fReindex);
    //  bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    //  bool WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list);
    bool WriteFlag(const string &name, bool fValue);
    bool ReadFlag(const string &name, bool &fValue);
    bool LoadBlockIndexGuts();
};

/** Create a new block index entry for a given block hash */
CBlockIndex * InsertBlockIndex(uint256 hash);

#endif  // PERSIST_BLOCKDB_H