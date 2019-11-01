// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COMMONS_HEXINT_H
#define COMMONS_HEXINT_H

#include <cstdlib>
#include <string>
#include <commons/serialize.h>


// fixed size of LEB128(Little Endian Base 128)
// @see LEB128: https://en.wikipedia.org/wiki/LEB128
template<typename I>
class CFixedLeb128 {
public:
    I value;
    static const uint32_t SIZE = (sizeof(I) * 8 + 6) / 7;
    typedef typename std::make_unsigned<I>::type UnsignedInt; // supports signed int type
public:
    CFixedLeb128(): value(0) {}
    explicit CFixedLeb128(const I &valueIn): value(valueIn) {}

    unsigned int GetSerializeSize(int nType, int nVersion) const
    {
        return SIZE; // hex lenth
    }

    template<typename Stream>
    void Serialize(Stream& s, int nType, int nVersion) const
    {
        uint8_t bytes[SIZE] = {0};
        UnsignedInt v = value;
        for(uint32_t i = 0; i < SIZE; i++) {
            // code the int from high to low order
            bytes[SIZE - i - 1] = (uint8_t)(v & 0x7F);
            v = v >> 7;
        }
        s.write((char*)bytes, SIZE);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        uint8_t bytes[SIZE] = {0};
        s.read((char*)bytes, SIZE);
        value = 0;
        for(uint32_t i = 0; i < SIZE; i++) {
            // the bytes store the int from high to low order
            value = (value << 7) | bytes[i];
        }
    }

    bool IsEmpty() const { return value == 0; }
    void SetEmpty() { value = 0; }

    bool operator==(I i) { return value == i; }
    bool operator<(I i) { return value < i; }

    friend bool operator==(const CFixedLeb128 &a, const CFixedLeb128 &b) { return a.value == b.value; }
    friend bool operator!=(const CFixedLeb128 &a, const CFixedLeb128 &b) { return a.value != b.value; }
    friend bool operator<(const CFixedLeb128 &a, const CFixedLeb128 &b) { return a.value < b.value; }
    friend bool operator>(const CFixedLeb128 &a, const CFixedLeb128 &b) { return a.value > b.value; }
};

using CFixedUInt32 = CFixedLeb128<uint32_t>;

#endif  // COMMONS_HEXINT_H
