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

struct CBlockInflatedReward {
    uint64_t start_height = 0;        // start block height
    uint64_t new_rewards = 0;           // new rewards, availible to mint
    uint64_t total_minted = 0;          // total minted rewards
    uint64_t last_minted = 0;           // last minted rewards
    uint64_t last_minted_height = 0;    // last minted block height

    static const CBlockInflatedReward EMPTY;

    bool IsEmpty() const {
        return start_height == 0 && new_rewards == 0 && total_minted == 0
               && last_minted == 0 && last_minted_height == 0;
    }
    void SetEmpty() { *this = EMPTY; }

    string ToString() const {
        return "{" +
                strprintf( "start_height=%d, ", start_height) +
                strprintf( "new_rewards=%d, ", new_rewards) +
                strprintf( "total_minted=%d, ", total_minted) +
                strprintf( "last_minted=%d, ", last_minted) +
                strprintf( "last_minted_height=%d, ", last_minted_height) +
                "}";
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(start_height);
        READWRITE(new_rewards);
        READWRITE(total_minted);
        READWRITE(last_minted);
        READWRITE(last_minted_height);
    )

};

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
    bool GetBlockIndex(const uint256 &hash, CDiskBlockIndex &blockIndex);
    bool WriteBlockIndex(const CDiskBlockIndex &blockIndex);
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
            tx_diskpos_cache(pDbAccess),
            flag_cache(pDbAccess),
            best_block_hash_cache(pDbAccess),
            last_block_file_cache(pDbAccess),
            reindex_cache(pDbAccess),
            finality_block_cache(pDbAccess),
            block_inflated_reward_cache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::BLOCK);
    };

    CBlockDBCache(CBlockDBCache *pBaseIn):
            tx_diskpos_cache(pBaseIn->tx_diskpos_cache),
            flag_cache(pBaseIn->flag_cache),
            best_block_hash_cache(pBaseIn->best_block_hash_cache),
            last_block_file_cache(pBaseIn->last_block_file_cache),
            reindex_cache(pBaseIn->reindex_cache),
            finality_block_cache(pBaseIn->finality_block_cache),
            block_inflated_reward_cache(pBaseIn->block_inflated_reward_cache){};

public:
    bool Flush();
    uint32_t GetCacheSize() const;

    void SetBaseViewPtr(CBlockDBCache *pBaseIn) {
        tx_diskpos_cache.SetBase(&pBaseIn->tx_diskpos_cache);
        flag_cache.SetBase(&pBaseIn->flag_cache);
        best_block_hash_cache.SetBase(&pBaseIn->best_block_hash_cache);
        last_block_file_cache.SetBase(&pBaseIn->last_block_file_cache);
        reindex_cache.SetBase(&pBaseIn->reindex_cache);
        finality_block_cache.SetBase(&pBaseIn->finality_block_cache);
        block_inflated_reward_cache.SetBase(&pBaseIn->block_inflated_reward_cache);

    };

    void SetDbOpLogMap(CDBOpLogMap *pDbOpLogMapIn) {
        tx_diskpos_cache.SetDbOpLogMap(pDbOpLogMapIn);
        flag_cache.SetDbOpLogMap(pDbOpLogMapIn);
        best_block_hash_cache.SetDbOpLogMap(pDbOpLogMapIn);
        last_block_file_cache.SetDbOpLogMap(pDbOpLogMapIn);
        reindex_cache.SetDbOpLogMap(pDbOpLogMapIn);
        finality_block_cache.SetDbOpLogMap(pDbOpLogMapIn);
        block_inflated_reward_cache.SetDbOpLogMap(pDbOpLogMapIn);
    }

    void RegisterUndoFunc(UndoDataFuncMap &undoDataFuncMap) {
        tx_diskpos_cache.RegisterUndoFunc(undoDataFuncMap);
        flag_cache.RegisterUndoFunc(undoDataFuncMap);
        best_block_hash_cache.RegisterUndoFunc(undoDataFuncMap);
        last_block_file_cache.RegisterUndoFunc(undoDataFuncMap);
        reindex_cache.RegisterUndoFunc(undoDataFuncMap);
        finality_block_cache.RegisterUndoFunc(undoDataFuncMap);
        block_inflated_reward_cache.RegisterUndoFunc(undoDataFuncMap);
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

    bool GetBlockInflatedReward(CBlockInflatedReward &value);
    bool SetBlockInflatedReward(const CBlockInflatedReward &value);
public:
/*  CCompositeKVCache      prefixType               key                     value                 variable               */
/*  ----------------   -------------------------   -----------------------  ------------------   ------------------------ */
    // txId -> DiskTxPos
    CCompositeKVCache< dbk::TXID_DISKINDEX,         uint256,                  CDiskTxPos >          tx_diskpos_cache;
    // flag$name -> bool
    CCompositeKVCache< dbk::FLAG,                   string,                   bool>                 flag_cache;

/*  CSimpleKVCache          prefixType             value           variable           */
/*  -------------------- --------------------   -------------   --------------------- */
    CSimpleKVCache< dbk::BEST_BLOCKHASH,            uint256>      best_block_hash_cache;    // best blockHash
    CSimpleKVCache< dbk::LAST_BLOCKFILE,            int32_t>          last_block_file_cache;
    CSimpleKVCache< dbk::REINDEX,                   bool>         reindex_cache;
    CSimpleKVCache< dbk::FINALITY_BLOCK,            std::pair<int32_t,uint256>> finality_block_cache;
    CSimpleKVCache< dbk::BLOCK_INFLATED_REWARD,     CBlockInflatedReward>       block_inflated_reward_cache;
};

/** Create a new block index entry for a given block hash */
CBlockIndex * InsertBlockIndex(uint256 hash);

#endif  // PERSIST_BLOCKDB_H