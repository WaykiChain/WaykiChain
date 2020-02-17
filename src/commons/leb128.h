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
template<typename I, typename UnsignedInt = I>
class CFixedLeb128 {
public:
    I value;
    static_assert(sizeof(I) == sizeof(UnsignedInt), "sizeof(I) == sizeof(UnsignedInt)");
    static const uint32_t SIZE = (sizeof(I) * 8 + 6) / 7;

    template<typename Stream>
    static inline unsigned int SerReadWrite(Stream& s, const I& v, int nType, int nVersion, CSerActionGetSerializeSize ser_action) {
        return SIZE; // hex lenth
    }

    template<typename Stream>
    static inline unsigned int SerReadWrite(Stream& s, const I& v, int nType, int nVersion, CSerActionSerialize ser_action) {
        SerializeValue(s, v);
        return 0;
    }

    template<typename Stream>
    static inline unsigned int SerReadWrite(Stream& s, I& v, int nType, int nVersion, CSerActionUnserialize ser_action) {
        UnserializeValue(s, v);
        return 0;
    }

    template<typename Stream>
    static inline void SerializeValue(Stream& s, const I& v)
    {
        uint8_t bytes[SIZE] = {0};
        UnsignedInt uv = v;
        for(uint32_t i = 0; i < SIZE; i++) {
            // code the int from high to low order
            bytes[SIZE - i - 1] = (uint8_t)(uv & 0x7F);
            uv = uv >> 7;
        }
        s.write((char*)bytes, SIZE);
    }

    template<typename Stream>
    static inline void UnserializeValue(Stream& s, I& v)
    {
        uint8_t bytes[SIZE] = {0};
        s.read((char*)bytes, SIZE);
        v = 0;
        for(uint32_t i = 0; i < SIZE; i++) {
            // the bytes store the int from high to low order
            v = (v << 7) | bytes[i];
        }
    }
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
        SerializeValue(s, value);
    }

    template<typename Stream>
    void Unserialize(Stream& s, int nType, int nVersion)
    {
        UnserializeValue(s, value);
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

using CFixedUInt64 = CFixedLeb128<uint64_t>;
using CFixedUInt32 = CFixedLeb128<uint32_t>;
using CFixedUInt16 = CFixedLeb128<uint16_t>;

#define READWRITE_FIXED_UINT32(n) READWRITE_FUNC(n, CFixedUInt32::SerReadWrite)
#define READWRITE_FIXED_UINT16(n) READWRITE_FUNC(n, CFixedUInt16::SerReadWrite)

#endif  // COMMONS_HEXINT_H
