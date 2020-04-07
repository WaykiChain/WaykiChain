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
#include "dbaccess.h"
#include "persistence/block.h"

#include <map>

/** Access to the block database (blocks/index/) */
class CBlockIndexDB : public CLevelDBWrapper {
private:
    CBlockIndexDB(const CBlockIndexDB &);
    void operator=(const CBlockIndexDB &);

public:
    CBlockIndexDB(bool fMemory = false, bool fWipe = false) :
        CLevelDBWrapper(GetDataDir() / "blocks" / "index", 2 << 20 /* 2MB */, fMemory, fWipe) {}

    // CBlockIndexDB(const std::string &name, size_t nCacheSize, bool fMemory = false, bool fWipe = false);

public:
    bool WriteBlockIndex(const CDiskBlockIndex &blockindex);
    bool EraseBlockIndex(const uint256 &blockHash);
    bool LoadBlockIndexes();

    bool ReadBlockFileInfo(int32_t nFile, CBlockFileInfo &fileinfo);
    bool WriteBlockFileInfo(int32_t nFile, const CBlockFileInfo &fileinfo);
};


/** Access to the block database (blocks/index/) */
class CBlockDBCache {
public:
    CBlockDBCache() {};

    CBlockDBCache(CDBAccess *pDbAccess):
        txDiskPosCache(pDbAccess),
        flagCache(pDbAccess),
        bestBlockHashCache(pDbAccess),
        lastBlockFileCache(pDbAccess),
        reindexCache(pDbAccess),
        finalityBlockCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::BLOCK);
    };

    CBlockDBCache(CBlockDBCache *pBaseIn):
        txDiskPosCache(pBaseIn->txDiskPosCache),
        flagCache(pBaseIn->flagCache),
        bestBlockHashCache(pBaseIn->bestBlockHashCache),
        lastBlockFileCache(pBaseIn->lastBlockFileCache),
        reindexCache(pBaseIn->reindexCache),
        finalityBlockCache(pBaseIn->finalityBlockCache){};

public:
    bool Flush();
    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(CBlockDBCache *pBaseIn) {
        txDiskPosCache.SetBase(&pBaseIn->txDiskPosCache);
        flagCache.SetBase(&pBaseIn->flagCache);
        bestBlockHashCache.SetBase(&pBaseIn->bestBlockHashCache);
        lastBlockFileCache.SetBase(&pBaseIn->lastBlockFileCache);
        reindexCache.SetBase(&pBaseIn->reindexCache);
        finalityBlockCache.SetBase(&pBaseIn->finalityBlockCache);

    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        txDiskPosCache.SetDbOpLogMap(pDbOpLogMapIn);
        flagCache.SetDbOpLogMap(pDbOpLogMapIn);
        bestBlockHashCache.SetDbOpLogMap(pDbOpLogMapIn);
        lastBlockFileCache.SetDbOpLogMap(pDbOpLogMapIn);
        reindexCache.SetDbOpLogMap(pDbOpLogMapIn);
        finalityBlockCache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        txDiskPosCache.RegisterUndoFunc(undoDataFuncMap);
        flagCache.RegisterUndoFunc(undoDataFuncMap);
        bestBlockHashCache.RegisterUndoFunc(undoDataFuncMap);
        lastBlockFileCache.RegisterUndoFunc(undoDataFuncMap);
        reindexCache.RegisterUndoFunc(undoDataFuncMap);
        finalityBlockCache.RegisterUndoFunc(undoDataFuncMap);
    }

    bool ReadTxIndex(const uint256 &txid, CDiskTxPos &pos);
    bool SetTxIndex(const uint256 &txid, const CDiskTxPos &pos);
    bool WriteTxIndexes(const vector<pair<uint256, CDiskTxPos> > &list);

    bool ReadLastBlockFile(int32_t &nFile);
    bool WriteLastBlockFile(int nFile);

    bool WriteReindexing(bool fReindex);
    bool ReadReindexing(bool &fReindex);

    bool WriteFlag(const string &name, bool fValue);
    bool ReadFlag(const string &name, bool &fValue);

    bool WriteGlobalFinBlock(const int32_t height, const uint256 hash);
    bool ReadGlobalFinBlock(std::pair<int32_t,uint256>& block);

    uint256 GetBestBlockHash() const;
    bool SetBestBlock(const uint256 &blockHash);

public:
/*  CCompositeKVCache      prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    // txId -> DiskTxPos
    CCompositeKVCache< dbk::TXID_DISKINDEX,         uint256,                  CDiskTxPos >          txDiskPosCache;
    // flag$name -> bool
    CCompositeKVCache< dbk::FLAG,                   string,                   bool>                 flagCache;


/*  CSimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    CSimpleKVCache< dbk::BEST_BLOCKHASH,            uint256>      bestBlockHashCache;    // best blockHash
    CSimpleKVCache< dbk::LAST_BLOCKFILE,            int32_t>          lastBlockFileCache;
    CSimpleKVCache< dbk::REINDEX,                   bool>         reindexCache;
    CSimpleKVCache< dbk::FINALITY_BLOCK,            std::pair<int32_t,uint256>> finalityBlockCache ;
};

/** Create a new block index entry for a given block hash */
CBlockIndex * InsertBlockIndex(uint256 hash);

#endif  // PERSIST_BLOCKDB_H