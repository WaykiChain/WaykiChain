// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2017-2019 The WaykiChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_SIGCACHE_H
#define COIN_SIGCACHE_H

#include <mutex>
#include <vector>

#include "config/chainparams.h"
#include "crypto/sha256.h"
#include "entities/key.h"
#include "commons/random.h"
#include "commons/uint256.h"
#include "commons/util.h"

/**
 * Valid signature cache, to avoid doing expensive ECDSA signature checking
 * twice for every transaction (once when accepted into memory pool, and
 * again when accepted into the block chain)
 */
class CSignatureCache {
private:
    //! Entries are SHA256(signature hash || public key || signature):
    UnorderedHashSet setValid;
    std::mutex mtx;

public:
    CSignatureCache() {}
    ~CSignatureCache() {}
    bool Get(const uint256& sigHash, const std::vector<unsigned char>& vchSig,
             const CPubKey& pubKey);
    void Set(const uint256& sigHash, const std::vector<unsigned char>& vchSig,
             const CPubKey& pubKey);

private:
    void ComputeEntry(uint256& entry, const uint256& sigHash,
                      const std::vector<unsigned char>& vchSig, const CPubKey& pubKey);
};

#endif  // COIN_SIGCACHE_H