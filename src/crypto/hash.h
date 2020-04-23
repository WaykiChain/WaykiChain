// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_HASH_H
#define COIN_HASH_H

#include "commons/serialize.h"
#include "commons/uint256.h"
#include "commons/util/util.h"
#include "config/version.h"

#include <openssl/ripemd.h>
#include <openssl/sha.h>

#include <vector>

using namespace std;

template <typename T1>
inline uint256 Hash(const T1 pbegin, const T1 pend) {
    static uint8_t pblank[1];
    uint256 hash1;
    SHA256((pbegin == pend ? pblank : (uint8_t *)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]),
           (uint8_t *)&hash1);
    uint256 hash2;
    SHA256((uint8_t *)&hash1, sizeof(hash1), (uint8_t *)&hash2);
    return hash2;
}

class CHashWriter {
private:
    SHA256_CTX ctx;

public:
    int32_t nType;
    int32_t nVersion;

    void Init() { SHA256_Init(&ctx); }

    CHashWriter(int32_t nTypeIn, int32_t nVersionIn) : nType(nTypeIn), nVersion(nVersionIn) { Init(); }

    CHashWriter &write(const char *pch, size_t size) {
        SHA256_Update(&ctx, pch, size);
        return (*this);
    }

    // invalidates the object
    uint256 GetHash() {
        uint256 hash1;
        SHA256_Final((uint8_t *)&hash1, &ctx);
        uint256 hash2;
        SHA256((uint8_t *)&hash1, sizeof(hash1), (uint8_t *)&hash2);
        return hash2;
    }

    template <typename T>
    CHashWriter &operator<<(const T &obj) {
        // Serialize to this stream
        ::Serialize(*this, obj, nType, nVersion);
        return (*this);
    }
};

template <typename T1, typename T2>
inline uint256 Hash(const T1 p1begin, const T1 p1end, const T2 p2begin, const T2 p2end) {
    static uint8_t pblank[1];
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (p1begin == p1end ? pblank : (uint8_t *)&p1begin[0]),
                  (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (p2begin == p2end ? pblank : (uint8_t *)&p2begin[0]),
                  (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Final((uint8_t *)&hash1, &ctx);
    uint256 hash2;
    SHA256((uint8_t *)&hash1, sizeof(hash1), (uint8_t *)&hash2);
    return hash2;
}

template <typename T1, typename T2, typename T3>
inline uint256 Hash(const T1 p1begin, const T1 p1end, const T2 p2begin, const T2 p2end, const T3 p3begin,
                    const T3 p3end) {
    static uint8_t pblank[1];
    uint256 hash1;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, (p1begin == p1end ? pblank : (uint8_t *)&p1begin[0]),
                  (p1end - p1begin) * sizeof(p1begin[0]));
    SHA256_Update(&ctx, (p2begin == p2end ? pblank : (uint8_t *)&p2begin[0]),
                  (p2end - p2begin) * sizeof(p2begin[0]));
    SHA256_Update(&ctx, (p3begin == p3end ? pblank : (uint8_t *)&p3begin[0]),
                  (p3end - p3begin) * sizeof(p3begin[0]));
    SHA256_Final((uint8_t *)&hash1, &ctx);
    uint256 hash2;
    SHA256((uint8_t *)&hash1, sizeof(hash1), (uint8_t *)&hash2);
    return hash2;
}

template <typename C>
inline uint256 Hash(const basic_string<C>& str) {
    return Hash(str.begin(), str.end());
}

template <typename T>
uint256 SerializeHash(const T &obj, int32_t nType = SER_GETHASH, int32_t nVersion = PROTOCOL_VERSION) {
    CHashWriter ss(nType, nVersion);
    ss << obj;
    return ss.GetHash();
}

template <typename T1>
inline uint160 Hash160(const T1 pbegin, const T1 pend) {
    static uint8_t pblank[1];
    uint256 hash1;
    SHA256((pbegin == pend ? pblank : (uint8_t *)&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]),
           (uint8_t *)&hash1);
    uint160 hash2;
    RIPEMD160((uint8_t *)&hash1, sizeof(hash1), (uint8_t *)&hash2);
    return hash2;
}

inline uint160 Hash160(const vector<uint8_t> &vch) { return Hash160(vch.begin(), vch.end()); }

inline uint160 Hash160(std::string str) {
    std::vector<uint8_t> vec(str.begin(), str.end());
    return Hash160(vec);
}

// inline bool Hash256(std::string str, uint256 &hash) {
//     uint8_t rnd[8];
//     GetRandBytes(rnd, sizeof(rnd));
//     SHA256_CTX ctx;
//     SHA256_Init(&ctx);
//     SHA256_Update(&ctx, str.data(), str.size());
//     SHA256_Update(&ctx, rnd, sizeof(rnd));
//     SHA256_Final(hash.begin(), &ctx);

//     return true;
// }

uint32_t MurmurHash3(uint32_t nHashSeed, const vector<uint8_t> &vDataToHash);

typedef struct {
    SHA512_CTX ctxInner;
    SHA512_CTX ctxOuter;
} HMAC_SHA512_CTX;

int32_t HMAC_SHA512_Init(HMAC_SHA512_CTX *pctx, const void *pkey, size_t len);
int32_t HMAC_SHA512_Update(HMAC_SHA512_CTX *pctx, const void *pdata, size_t len);
int32_t HMAC_SHA512_Final(uint8_t *pmd, HMAC_SHA512_CTX *pctx);

#endif
