// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_UINT256_H
#define COIN_UINT256_H

#include <cassert>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

extern const signed char p_util_hexdigit[256];  // defined in util.cpp

inline signed char HexDigit(char c) { return p_util_hexdigit[(unsigned char)c]; }

/** Template base class for fixed-sized opaque blobs. */
template <unsigned int BITS>
class base_blob {
public:
    enum { WIDTH = BITS / 8 };
protected:
    uint8_t data[WIDTH];

public:
    base_blob() { memset(data, 0, sizeof(data)); }

    explicit base_blob(const std::vector<unsigned char>& vch);

    template<typename T>
    explicit base_blob(const T begin, const T end) {
        if (end - begin == sizeof(data)) {
            uint8_t* p = data; T it = begin;
            for (; it != end; it++, p++)
                *p = *it;
        } else {
            assert(false);
        }
    };

    bool IsNull() const {
        for (int i = 0; i < WIDTH; i++)
            if (data[i] != 0)
                return false;
        return true;
    }

    bool IsEmpty() const { return IsNull(); }

    void SetNull() { memset(data, 0, sizeof(data)); }

    void SetEmpty() { SetNull(); }

    friend inline bool operator==(const base_blob& a, const base_blob& b) {
        return memcmp(a.data, b.data, sizeof(a.data)) == 0;
    }
    friend inline bool operator!=(const base_blob& a, const base_blob& b) {
        return memcmp(a.data, b.data, sizeof(a.data)) != 0;
    }
    friend inline bool operator<(const base_blob& a, const base_blob& b) {
        return memcmp(a.data, b.data, sizeof(a.data)) < 0;
    }

    std::string GetHex() const;
    void SetHex(const char* psz);
    void SetHex(const std::string& str);
    std::string ToString() const;

    unsigned char* begin() { return &data[0]; }

    unsigned char* end() { return &data[WIDTH]; }

    const unsigned char* begin() const { return &data[0]; }

    const unsigned char* end() const { return &data[WIDTH]; }

    unsigned int size() const { return sizeof(data); }

    unsigned int GetSerializeSize(int nType, int nVersion) const { return sizeof(data); }

    template <typename Stream>
    void Serialize(Stream& s, int nType, int nVersion) const {
        s.write((char*)data, sizeof(data));
    }

    template <typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion) {
        s.read((char*)data, sizeof(data));
    }
};

/** 160-bit opaque blob.
 * @note This type is called uint160 for historical reasons only. It is an opaque
 * blob of 160 bits and has no integer operations.
 */
class uint160 : public base_blob<160> {
public:
    uint160() {}
    uint160(const base_blob<160>& b) : base_blob<160>(b) {}
    explicit uint160(const std::vector<unsigned char>& vch) : base_blob<160>(vch) {}
};

/** 256-bit opaque blob.
 * @note This type is called uint256 for historical reasons only. It is an
 * opaque blob of 256 bits and has no integer operations. Use arith_uint256 if
 * those are required.
 */
class uint256 : public base_blob<256> {
public:
    uint256() {}
    uint256(const base_blob<256>& b) : base_blob<256>(b) {}
    explicit uint256(const std::vector<unsigned char>& vch) : base_blob<256>(vch) {}

    template<typename T>
    explicit uint256(const T begin, const T end) : base_blob<256>(begin, end) {}

    /** A cheap hash function that just returns 64 bits from the result, it can be
     * used when the contents are considered uniformly random. It is not appropriate
     * when the value can easily be influenced from outside as e.g. a network adversary could
     * provide values to trigger worst-case behavior.
     * @note The result of this function is not stable between little and big endian.
     */
    uint64_t GetCheapHash() const {
        uint64_t result;
        memcpy((void*)&result, (void*)data, 8);
        return result;
    }

    /** A more secure, salted hash function.
     * @note This hash is not stable between little and big endian.
     */
    uint64_t GetHash(const uint256& salt) const;
};

inline uint160 uint160S(const char* str) {
    uint160 rv;
    rv.SetHex(str);
    return rv;
}

inline uint160 uint160S(const std::string& str) {
    uint160 rv;
    rv.SetHex(str);
    return rv;
}

/* uint256 from const char *.
 * This is a separate function because the constructor uint256(const char*) can result
 * in dangerously catching uint256(0).
 */
inline uint256 uint256S(const char* str) {
    uint256 rv;
    rv.SetHex(str);
    return rv;
}
/* uint256 from std::string.
 * This is a separate function because the constructor uint256(const std::string &str) can result
 * in dangerously catching uint256(0) via std::string(const char*).
 */
inline uint256 uint256S(const std::string& str) { return uint256S(str.c_str()); }

class CUint256Hasher {
public:
    size_t operator()(const uint256& key) const { return key.GetCheapHash(); }
};

typedef std::unordered_set<uint256, CUint256Hasher> UnorderedHashSet;

#endif  // COIN_UINT256_H
