// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2017-2019 The WaykiChain Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sigcache.h"

void CSignatureCache::ComputeEntry(uint256& entry, const uint256& sigHash,
                                   const std::vector<unsigned char>& vchSig,
                                   const CPubKey& pubKey) {
    CSHA256()
        .Write(sigHash.begin(), 32)
        .Write(&pubKey[0], pubKey.size())
        .Write(&vchSig[0], vchSig.size())
        .Finalize(entry.begin());
}

bool CSignatureCache::Get(const uint256& sigHash, const std::vector<unsigned char>& vchSig,
                          const CPubKey& pubKey) {
    uint256 entry;
    ComputeEntry(entry, sigHash, vchSig, pubKey);
    std::unique_lock<std::mutex> lock(mtx);
    return setValid.count(entry);
}

void CSignatureCache::Set(const uint256& sigHash, const std::vector<unsigned char>& vchSig,
                          const CPubKey& pubKey) {
    static int64_t nMaxCacheSize = SysCfg().GetArg("-maxsigcachesize", 50000);
    if (nMaxCacheSize <= 0) return;

    uint256 entry;
    ComputeEntry(entry, sigHash, vchSig, pubKey);

    std::unique_lock<std::mutex> lock(mtx);

    while (static_cast<int64_t>(setValid.size()) > nMaxCacheSize) {
        // Evict a random entry. Random because that helps
        // foil would-be DoS attackers who might try to pre-generate
        // and re-use a set of valid signatures just-slightly-greater
        // than our cache size.
        UnorderedHashSet::size_type s       = GetRand(setValid.bucket_count());
        UnorderedHashSet::local_iterator it = setValid.begin(s);
        if (it != setValid.end(s)) {
            setValid.erase(*it);
        }
    }

    setValid.insert(entry);
}
