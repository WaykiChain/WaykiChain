// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef ENTITIES_KEY_H
#define ENTITIES_KEY_H

#include <boost/variant.hpp>
#include <stdexcept>
#include <vector>
#include "commons/allocators.h"
#include "commons/serialize.h"
#include "commons/uint256.h"
#include "commons/util.h"
#include "crypto/hash.h"

#include <secp256k1.h>
#include <secp256k1_recovery.h>

class CRegID;

/** A reference to a CKey: the Hash160 of its serialized public key */
class CKeyID : public uint160 {
public:
    CKeyID() : uint160() {}
    CKeyID(const uint160 &in) : uint160(in) {}
    CKeyID(const string &strAddress);

    bool IsEmpty() const { return IsNull(); }

    string ToString() const { return HexStr(begin(), end()); }
    string ToAddress() const;
};

/** An encapsulated public key. */
class CPubKey {
public:
    static constexpr uint32_t PUBLIC_KEY_SIZE            = 65;
    static constexpr uint32_t COMPRESSED_PUBLIC_KEY_SIZE = 33;
    static constexpr uint32_t SIGNATURE_SIZE             = 72;
    static constexpr uint32_t COMPACT_SIGNATURE_SIZE     = 65;

private:
    // Just store the serialized data.
    // Its length can very cheaply be computed from the first byte.
    uint8_t vch[65];

    // Compute the length of a pubkey with a given first byte.
    uint32_t static GetLen(uint8_t chHeader) {
        if (chHeader == 2 || chHeader == 3) return 33;
        //		assert(0); //only sorpurt 33
        //		if (chHeader == 4 || chHeader == 6 || chHeader == 7)
        //			return 65;
        return 0;
    }

    // Set this key data to be invalid
    void Invalidate() { vch[0] = 0xFF; }

public:
    string ToString() const;

    // Construct an invalid public key.
    CPubKey() { Invalidate(); }

    // Initialize a public key using begin/end iterators to byte data.
    template <typename T>
    void Set(const T pbegin, const T pend) {
        int len = pend == pbegin ? 0 : GetLen(pbegin[0]);
        if (len && len == (pend - pbegin))
            memcpy(vch, (uint8_t *)&pbegin[0], len);
        else
            Invalidate();
    }

    // Construct a public key using begin/end iterators to byte data.
    template <typename T>
    CPubKey(const T pbegin, const T pend) {
        Set(pbegin, pend);
    }

    // Construct a public key from a byte vector.
    CPubKey(const vector<uint8_t> &vch) { Set(vch.begin(), vch.end()); }
    CPubKey(const string &str) { Set(str.begin(), str.end()); }

    // Simple read-only vector-like interface to the pubkey data.
    uint32_t size() const {
        uint32_t len = GetLen(vch[0]);
        if (len != 33)  // only use 33 for Coin sys
            return 0;
        return len;
    }
    const uint8_t *begin() const { return vch; }
    const uint8_t *end() const { return vch + size(); }
    const uint8_t &operator[](uint32_t pos) const { return vch[pos]; }

    // Comparator implementation.
    friend bool operator==(const CPubKey &a, const CPubKey &b) {
        return a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) == 0;
    }
    friend bool operator!=(const CPubKey &a, const CPubKey &b) { return !(a == b); }
    friend bool operator<(const CPubKey &a, const CPubKey &b) {
        return a.vch[0] < b.vch[0] || (a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) < 0);
    }

    // Implement serialization, as if this was a byte vector.
    uint32_t GetSerializeSize(int nType, int nVersion) const { return size() + 1; }

    template <typename Stream>
    void WriteCompactSize(Stream &os, uint64_t nSize) const {
        if (nSize < 253) {
            uint8_t chSize = nSize;
            WRITEDATA(os, chSize);
        } else if (nSize <= numeric_limits<uint16_t>::max()) {
            uint8_t chSize       = 253;
            uint16_t xSize = nSize;
            WRITEDATA(os, chSize);
            WRITEDATA(os, xSize);
        } else if (nSize <= numeric_limits<uint32_t>::max()) {
            uint8_t chSize = 254;
            uint32_t xSize = nSize;
            WRITEDATA(os, chSize);
            WRITEDATA(os, xSize);
        } else {
            uint8_t chSize = 255;
            uint64_t xSize = nSize;
            WRITEDATA(os, chSize);
            WRITEDATA(os, xSize);
        }
        return;
    }

    template <typename Stream>
    uint64_t ReadCompactSize(Stream &is) {
        uint8_t chSize;
        READDATA(is, chSize);
        uint64_t nSizeRet = 0;
        if (chSize < 253) {
            nSizeRet = chSize;
        } else if (chSize == 253) {
            uint16_t xSize;
            READDATA(is, xSize);
            nSizeRet = xSize;
            if (nSizeRet < 253) throw ios_base::failure("non-canonical ReadCompactSize()");
        } else if (chSize == 254) {
            uint32_t xSize;
            READDATA(is, xSize);
            nSizeRet = xSize;
            if (nSizeRet < 0x10000u) throw ios_base::failure("non-canonical ReadCompactSize()");
        } else {
            uint64_t xSize;
            READDATA(is, xSize);
            nSizeRet = xSize;
            if (nSizeRet < 0x100000000LLu) throw ios_base::failure("non-canonical ReadCompactSize()");
        }
        if (nSizeRet > 0x02000000LLu) throw ios_base::failure("ReadCompactSize() : size too large");
        return nSizeRet;
    }

    template <typename Stream>
    void Serialize(Stream &s, int nType, int nVersion) const {
        uint32_t len = size();
        WriteCompactSize(s, len);
        s.write((char *)vch, len);
    }

    template <typename Stream>
    void Unserialize(Stream &s, int nType, int nVersion) {
        uint32_t len = ReadCompactSize(s);
        if (len == 33) {
            s.read((char *)vch, 33);
        } else if (len == 0) {
            Invalidate();
        } else {
            ERRORMSG("Unserializable: len=%d", len);
        }
    }

    // Get the KeyID of this public key (hash of its serialization)
    CKeyID GetKeyId() const;

    // Get the 256-bit hash of this public key.
    uint256 GetHash() const;
    // Check syntactic correctness.
    //
    // Note that this is consensus critical as CheckSig() calls it!
    bool IsValid() const {
        //		return size() > 0;
        return size() == 33;  // force use Compressed key
    }

    // fully validate whether this is a valid public key (more expensive than IsValid())
    bool IsFullyValid() const;

    // Check whether this is a compressed public key.
    bool IsCompressed() const { return size() == 33; }

    // Verify a DER signature (~72 bytes).
    // If this public key is not fully valid, the return value will be false.
    bool Verify(const uint256 &hash, const vector<uint8_t> &vchSig) const;

    // Recover a public key from a compact signature.
    bool RecoverCompact(const uint256 &hash, const vector<uint8_t> &vchSig);

    // Turn this public key into an uncompressed public key.
    bool Decompress();

    // Derive BIP32 child pubkey.
    bool Derive(CPubKey &pubkeyChild, uint8_t ccChild[32], uint32_t nChild, const uint8_t cc[32]) const;
};

// secure_allocator is defined in allocators.h
// CPrivKey is a serialized private key, with all parameters included (279 bytes)
typedef vector<uint8_t, secure_allocator<uint8_t> > CPrivKey;

/** An encapsulated private key. */
class CKey {
public:
    /**
     * secp256k1:
     */
    static const uint32_t PRIVATE_KEY_SIZE            = 279;
    static const uint32_t COMPRESSED_PRIVATE_KEY_SIZE = 214;
    /**
     * see www.keylength.com
     * script supports up to 75 for single byte push
     */
    static_assert(PRIVATE_KEY_SIZE >= COMPRESSED_PRIVATE_KEY_SIZE,
                  "COMPRESSED_PRIVATE_KEY_SIZE is larger than PRIVATE_KEY_SIZE");

private:
    // Whether this private key is valid. We check for correctness when modifying the key
    // data, so fValid should always correspond to the actual state.
    bool fValid;

    // Whether the public key corresponding to this private key is (to be) compressed.
    bool fCompressed;

    // The actual byte data
    uint8_t vch[32];

    // Check whether the 32-byte array pointed to be vch is valid keydata.
    bool static Check(const uint8_t *vch);

public:
    IMPLEMENT_SERIALIZE(uint32_t len = 0; while (len < sizeof(vch)) { READWRITE(vch[len++]); } READWRITE(fCompressed);
                        READWRITE(fValid);)

    string ToString() const {
        if (fValid) return HexStr(begin(), end());
        return "";
    }

    // Construct an invalid private key.
    CKey() : fValid(false) {
        LockObject(vch);
        fCompressed = false;
    }

    bool Clear() {
        fValid = false;
        memset(vch, 0, sizeof(vch));
        return true;
    }

    // Copy constructor. This is necessary because of memlocking.
    CKey(const CKey &secret) : fValid(secret.fValid), fCompressed(secret.fCompressed) {
        LockObject(vch);
        memcpy(vch, secret.vch, sizeof(vch));
    }

    // Destructor (again necessary because of memlocking).
    ~CKey() { UnlockObject(vch); }

    friend bool operator==(const CKey &a, const CKey &b) {
        return a.fCompressed == b.fCompressed && a.size() == b.size() && memcmp(&a.vch[0], &b.vch[0], a.size()) == 0;
    }

    // Initialize using begin and end iterators to byte data.
    template <typename T>
    void Set(const T pbegin, const T pend, bool fCompressedIn) {
        if (pend - pbegin != 32) {
            fValid = false;
            return;
        }
        if (Check(&pbegin[0])) {
            memcpy(vch, (uint8_t *)&pbegin[0], 32);
            fValid      = true;
            fCompressed = fCompressedIn;
        } else {
            fValid = false;
        }
    }

    // Simple read-only vector-like interface.
    uint32_t size() const { return (fValid ? 32 : 0); }
    const uint8_t *begin() const { return vch; }
    const uint8_t *end() const { return vch + size(); }

    // Check whether this private key is valid.
    bool IsValid() const { return fValid; }

    // Check whether the public key corresponding to this private key is (to be) compressed.
    bool IsCompressed() const { return fCompressed; }

    // Generate a new private key using a cryptographic PRNG.
    void MakeNewKey(bool fCompressed = true);

    // Convert the private key to a CPrivKey (serialized OpenSSL private key data).
    // This is expensive.
    CPrivKey GetPrivKey() const;

    // Compute the public key from a private key.
    // This is expensive.
    CPubKey GetPubKey() const;

    // Create a DER-serialized signature.
    bool Sign(const uint256 &hash, vector<uint8_t> &vchSig) const;

    // Create a compact signature (65 bytes), which allows reconstructing the used public key.
    // The format is one header byte, followed by two times 32 bytes for the serialized r and s
    // values. The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
    //                  0x1D = second key with even y, 0x1E = second key with odd y,
    //                  add 0x04 for compressed keys.
    bool SignCompact(const uint256 &hash, vector<uint8_t> &vchSig) const;

    // Derive BIP32 child key.
    bool Derive(CKey &keyChild, uint8_t ccChild[32], uint32_t nChild, const uint8_t cc[32]) const;

    // Load private key and check that public key matches.
    bool Load(CPrivKey &privkey, CPubKey &vchPubKey, bool fSkipCheck);

    /**
     * Verify thoroughly whether a private key and a public key match.
     * This is done using a different mechanism than just regenerating it.
     */
    bool VerifyPubKey(const CPubKey &vchPubKey) const;
};

struct CExtPubKey {
    uint8_t nDepth;
    uint8_t vchFingerprint[4];
    uint32_t nChild;
    uint8_t vchChainCode[32];
    CPubKey pubkey;

    friend bool operator==(const CExtPubKey &a, const CExtPubKey &b) {
        return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0 &&
               a.nChild == b.nChild && memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) == 0 && a.pubkey == b.pubkey;
    }

    void Encode(uint8_t code[74]) const;
    void Decode(const uint8_t code[74]);
    bool Derive(CExtPubKey &out, uint32_t nChild) const;
};

struct CExtKey {
    uint8_t nDepth;
    uint8_t vchFingerprint[4];
    uint32_t nChild;
    uint8_t vchChainCode[32];
    CKey key;

    friend bool operator==(const CExtKey &a, const CExtKey &b) {
        return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0 &&
               a.nChild == b.nChild && memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) == 0 && a.key == b.key;
    }

    void Encode(uint8_t code[74]) const;
    void Decode(const uint8_t code[74]);
    bool Derive(CExtKey &out, uint32_t nChild) const;
    CExtPubKey Neuter() const;
    void SetMaster(const uint8_t *seed, uint32_t nSeedLen);
};

class CMulsigScript {
private:
    int nRequired;
    mutable std::set<CPubKey> pubKeys;

public:
    CMulsigScript() {}

    void SetMultisig(const int nRequiredIn, const std::set<CPubKey> &pubKeysIn) {
        nRequired = nRequiredIn;
        pubKeys   = pubKeysIn;
    }

    int GetRequired() const { return nRequired; }
    std::set<CPubKey> &GetPubKeys() { return pubKeys; }

    CKeyID GetID() const {
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << *this;
        return CKeyID(Hash160(ds.begin(), ds.end()));
    }

    string ToString() const {
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << *this;

        return ds.str();
    }

    IMPLEMENT_SERIALIZE(READWRITE(VARINT(nRequired)); READWRITE(pubKeys);)
};

/** Users of this module must hold an ECCVerifyHandle. The constructor and
 *  destructor of these are not allowed to run in parallel, though. */
class ECCVerifyHandle {
    static int refcount;

public:
    ECCVerifyHandle();
    ~ECCVerifyHandle();
};

/** Initialize the elliptic curve support. May not be called twice without calling ECC_Stop first.
 */
void ECC_Start();

/** Deinitialize the elliptic curve support. No-op if ECC_Start wasn't called first. */
void ECC_Stop();

/** Check that required EC support is available at runtime. */
bool ECC_InitSanityCheck();

#endif  // ENTITIES_KEY_H
