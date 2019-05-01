// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2019- The WaykiChain Core Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#ifndef CDP_H
#define CDP_H

// #include <memory>
#include <string>
// #include <vector>
// #include <unordered_map>

#include "leveldbwrapper.h"

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "commons/serialize.h"

// #include "key.h"
// #include "chainparams.h"
// #include "crypto/hash.h"
// #include "vote.h"

class CCdp {
private:
    uint64_t bcoins;   // collatorized basecoin amount
    uint64_t scoins;   // minted stablecoin amount

public:
    CCdp() {}
    CCdp(uint64_t bcoinsIn, uint64_t scoinsIn): bcoins(bcoinsIn), scoins(scoinsIn) {}

    uint64_t GetBcoins() const { return bcoins; }
    uint64_t GetScoins() const { return scoins; }

    IMPLEMENT_SERIALIZE(
        READWRITE(bcoins);
        READWRITE(scoins);
    )
};

class CCdpView {

};

class CCdpViewCache: CCdpView {
protected:
    CCdpView *pBase;

public:
    bool Flush();
};

class CCdpViewDB: CCdpView {
private:
    CLevelDBWrapper db;

public:
    CCdpViewDB(size_t nCacheSize, bool fMemory = false, bool fWipe = false) : 
        db(GetDataDir() / "blocks" / "txcache", nCacheSize, fMemory, fWipe) {};
    ~CCdpViewDB() {};

private:
    CCdpViewDB(const CCdpViewDB &);
    void operator=(const CCdpViewDB &);

public:
    virtual bool IsContainBlock(const CBlock &block);
    virtual bool BatchWrite(const map<uint256, UnorderedHashSet> &mapTxHashByBlockHash);
    int64_t GetDbCount() { return db.GetDbCount(); }
};

#endif //CDP_H
