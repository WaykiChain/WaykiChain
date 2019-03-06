// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2014-2015 The WaykiChain developers
// Copyright (c) 2016 The Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef COIN_KEY_H
#define COIN_KEY_H

#include <boost/variant.hpp>
#include <stdexcept>
#include <vector>
#include "allocators.h"
#include "uint256.h"
#include "util.h"

/** A reference to a CKey: the Hash160 of its serialized public key */
class CKeyID: public uint160 {
public:
	CKeyID() : uint160() {}
	CKeyID(const uint160 &in) : uint160(in) {}
	CKeyID(const string &strAddress);

	bool IsEmpty()const { return IsNull(); }

	string ToString() const { return HexStr(begin(),end()); }
	string ToAddress() const;

};

/** A reference to a CScript: the Hash160 of its serialization (see script.h) */
class CScriptID: public uint160 {
public:
	CScriptID() : uint160() {}
	CScriptID(const uint160 &in) :
			uint160(in) {
	}
};



/** An encapsulated public key. */
class CPubKey {
private:
	// Just store the serialized data.
	// Its length can very cheaply be computed from the first byte.
	unsigned char vch[65];

	// Compute the length of a pubkey with a given first byte.
	unsigned int static GetLen(unsigned char chHeader) {
		if (chHeader == 2 || chHeader == 3)
			return 33;
//		assert(0); //only sorpurt 33
//		if (chHeader == 4 || chHeader == 6 || chHeader == 7)
//			return 65;
		return 0;
	}

	// Set this key data to be invalid
	void Invalidate() {
		vch[0] = 0xFF;
	}

public:
	string ToString() const;


	// Construct an invalid public key.
	CPubKey() {
		Invalidate();
	}

	// Initialize a public key using begin/end iterators to byte data.
	template<typename T>
	void Set(const T pbegin, const T pend) {
		int len = pend == pbegin ? 0 : GetLen(pbegin[0]);
		if (len && len == (pend - pbegin))
			memcpy(vch, (unsigned char*) &pbegin[0], len);
		else
			Invalidate();
	}

	// Construct a public key using begin/end iterators to byte data.
	template<typename T>
	CPubKey(const T pbegin, const T pend) {
		Set(pbegin, pend);
	}

	// Construct a public key from a byte vector.
	CPubKey(const vector<unsigned char> &vch) {
		Set(vch.begin(), vch.end());
	}

	// Simple read-only vector-like interface to the pubkey data.
	unsigned int size() const {
		unsigned int len = GetLen(vch[0]);
		 if(len != 33) //only use 33 for Coin sys
			 return 0;
		return len;
	}
	const unsigned char *begin() const {
		return vch;
	}
	const unsigned char *end() const {
		return vch + size();
	}
	const unsigned char &operator[](unsigned int pos) const {
		return vch[pos];
	}

	// Comparator implementation.
	friend bool operator==(const CPubKey &a, const CPubKey &b) {
		return a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) == 0;
	}
	friend bool operator!=(const CPubKey &a, const CPubKey &b) {
		return !(a == b);
	}
	friend bool operator<(const CPubKey &a, const CPubKey &b) {
		return a.vch[0] < b.vch[0] || (a.vch[0] == b.vch[0] && memcmp(a.vch, b.vch, a.size()) < 0);
	}

	// Implement serialization, as if this was a byte vector.
	unsigned int GetSerializeSize(int nType, int nVersion) const {
		return size() + 1;
	}

	template<typename Stream>
	void WriteCompactSize(Stream& os, uint64_t nSize) const {
		if (nSize < 253) {
			unsigned char chSize = nSize;
			WRITEDATA(os, chSize);
		} else if (nSize <= numeric_limits<unsigned short>::max()) {
			unsigned char chSize = 253;
			unsigned short xSize = nSize;
			WRITEDATA(os, chSize);
			WRITEDATA(os, xSize);
		} else if (nSize <= numeric_limits<unsigned int>::max()) {
			unsigned char chSize = 254;
			unsigned int xSize = nSize;
			WRITEDATA(os, chSize);
			WRITEDATA(os, xSize);
		} else {
			unsigned char chSize = 255;
			uint64_t xSize = nSize;
			WRITEDATA(os, chSize);
			WRITEDATA(os, xSize);
		}
		return;
	}

	template<typename Stream>
	uint64_t ReadCompactSize(Stream& is) {
		unsigned char chSize;
		READDATA(is, chSize);
		uint64_t nSizeRet = 0;
		if (chSize < 253) {
			nSizeRet = chSize;
		} else if (chSize == 253) {
			unsigned short xSize;
			READDATA(is, xSize);
			nSizeRet = xSize;
			if (nSizeRet < 253)
				throw ios_base::failure("non-canonical ReadCompactSize()");
		} else if (chSize == 254) {
			unsigned int xSize;
			READDATA(is, xSize);
			nSizeRet = xSize;
			if (nSizeRet < 0x10000u)
				throw ios_base::failure("non-canonical ReadCompactSize()");
		} else {
			uint64_t xSize;
			READDATA(is, xSize);
			nSizeRet = xSize;
			if (nSizeRet < 0x100000000LLu)
				throw ios_base::failure("non-canonical ReadCompactSize()");
		}
		if (nSizeRet > 0x02000000LLu)
			throw ios_base::failure("ReadCompactSize() : size too large");
		return nSizeRet;
	}

	template<typename Stream> void Serialize(Stream &s, int nType, int nVersion) const {
		unsigned int len = size();
		WriteCompactSize(s, len);
		s.write((char*) vch, len);
	}

	template<typename Stream> void Unserialize(Stream &s, int nType, int nVersion) {

		unsigned int len = ReadCompactSize(s);
		if ( len == 33 ) {
			s.read((char*) vch, 33);
		} else if (len == 0) {
			Invalidate();
		} else {
			ERRORMSG("Unserializable: len=%d", len);
		}
	}

	// Get the KeyID of this public key (hash of its serialization)
	CKeyID GetKeyID() const;

	// Get the 256-bit hash of this public key.
	uint256 GetHash() const;
	// Check syntactic correctness.
	//
	// Note that this is consensus critical as CheckSig() calls it!
	bool IsValid() const {
//		return size() > 0;
		return size() == 33;//force use Compressed key
	}

	// fully validate whether this is a valid public key (more expensive than IsValid())
	bool IsFullyValid() const;

	// Check whether this is a compressed public key.
	bool IsCompressed() const {
		return size() == 33;
	}

	// Verify a DER signature (~72 bytes).
	// If this public key is not fully valid, the return value will be false.
	bool Verify(const uint256 &hash, const vector<unsigned char>& vchSig) const;

	// Verify a compact signature (~65 bytes).
	// See CKey::SignCompact.
	bool VerifyCompact(const uint256 &hash, const vector<unsigned char>& vchSig) const;

	// Recover a public key from a compact signature.
	bool RecoverCompact(const uint256 &hash, const vector<unsigned char>& vchSig);

	// Turn this public key into an uncompressed public key.
	bool Decompress();

	// Derive BIP32 child pubkey.
	bool Derive(CPubKey& pubkeyChild, unsigned char ccChild[32], unsigned int nChild, const unsigned char cc[32]) const;
};

// secure_allocator is defined in allocators.h
// CPrivKey is a serialized private key, with all parameters included (279 bytes)
typedef vector<unsigned char, secure_allocator<unsigned char> > CPrivKey;

/** An encapsulated private key. */
class CKey {
private:
	// Whether this private key is valid. We check for correctness when modifying the key
	// data, so fValid should always correspond to the actual state.
	bool fValid;

	// Whether the public key corresponding to this private key is (to be) compressed.
	bool fCompressed;

	// The actual byte data
	unsigned char vch[32];

	// Check whether the 32-byte array pointed to be vch is valid keydata.
	bool static Check(const unsigned char *vch);
public:

	IMPLEMENT_SERIALIZE
	(
			unsigned int len = 0;
			while(len < sizeof(vch))
			{
				READWRITE(vch[len++]);
			}
			READWRITE(fCompressed);
			READWRITE(fValid);
	)

    string ToString()const
	{
		if(fValid)
		return HexStr(begin(),end());
		return "";
	}

	// Construct an invalid private key.
	CKey(): fValid(false)
	{
		LockObject(vch);
		fCompressed = false;
	}

	bool Clear()
	{
		fValid = false;
		memset(vch,0,sizeof(vch));
		return true;
	}

	// Copy constructor. This is necessary because of memlocking.
	CKey(const CKey &secret): fValid(secret.fValid), fCompressed(secret.fCompressed)
	{
		LockObject(vch);
		memcpy(vch, secret.vch, sizeof(vch));
	}

	// Destructor (again necessary because of memlocking).
	~CKey()
	{
		UnlockObject(vch);
	}

	friend bool operator==(const CKey &a, const CKey &b) {
		return a.fCompressed == b.fCompressed && a.size() == b.size() && memcmp(&a.vch[0], &b.vch[0], a.size()) == 0;
	}

	// Initialize using begin and end iterators to byte data.
	template<typename T>
	void Set(const T pbegin, const T pend, bool fCompressedIn) {
		if (pend - pbegin != 32) {
			fValid = false;
			return;
		}
		if (Check(&pbegin[0])) {
			memcpy(vch, (unsigned char*) &pbegin[0], 32);
			fValid = true;
			fCompressed = fCompressedIn;
		} else {
			fValid = false;
		}
	}

	// Simple read-only vector-like interface.
	unsigned int size() const {
		return (fValid ? 32 : 0);
	}
	const unsigned char *begin() const {
		return vch;
	}
	const unsigned char *end() const {
		return vch + size();
	}

	// Check whether this private key is valid.
	bool IsValid() const {
		return fValid;
	}

	// Check whether the public key corresponding to this private key is (to be) compressed.
	bool IsCompressed() const {
		return fCompressed;
	}

	// Initialize from a CPrivKey (serialized OpenSSL private key data).
	bool SetPrivKey(const CPrivKey &vchPrivKey, bool fCompressed);

	// Generate a new private key using a cryptographic PRNG.
	void MakeNewKey(bool fCompressed = true);

	// Convert the private key to a CPrivKey (serialized OpenSSL private key data).
	// This is expensive.
	CPrivKey GetPrivKey() const;

	// Compute the public key from a private key.
	// This is expensive.
	CPubKey GetPubKey() const;

	// Create a DER-serialized signature.
	bool Sign(const uint256 &hash, vector<unsigned char>& vchSig) const;

	// Create a compact signature (65 bytes), which allows reconstructing the used public key.
	// The format is one header byte, followed by two times 32 bytes for the serialized r and s values.
	// The header byte: 0x1B = first key with even y, 0x1C = first key with odd y,
	//                  0x1D = second key with even y, 0x1E = second key with odd y,
	//                  add 0x04 for compressed keys.
	bool SignCompact(const uint256 &hash, vector<unsigned char>& vchSig) const;

	// Derive BIP32 child key.
	bool Derive(CKey& keyChild, unsigned char ccChild[32], unsigned int nChild, const unsigned char cc[32]) const;

	// Load private key and check that public key matches.
	bool Load(CPrivKey &privkey, CPubKey &vchPubKey, bool fSkipCheck);
};

struct CExtPubKey {
	unsigned char nDepth;
	unsigned char vchFingerprint[4];
	unsigned int nChild;
	unsigned char vchChainCode[32];
	CPubKey pubkey;

	friend bool operator==(const CExtPubKey &a, const CExtPubKey &b) {
		return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0
				&& a.nChild == b.nChild && memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) == 0
				&& a.pubkey == b.pubkey;
	}

	void Encode(unsigned char code[74]) const;
	void Decode(const unsigned char code[74]);
	bool Derive(CExtPubKey &out, unsigned int nChild) const;
};

struct CExtKey {
	unsigned char nDepth;
	unsigned char vchFingerprint[4];
	unsigned int nChild;
	unsigned char vchChainCode[32];
	CKey key;

	friend bool operator==(const CExtKey &a, const CExtKey &b) {
		return a.nDepth == b.nDepth && memcmp(&a.vchFingerprint[0], &b.vchFingerprint[0], 4) == 0
				&& a.nChild == b.nChild && memcmp(&a.vchChainCode[0], &b.vchChainCode[0], 32) == 0 && a.key == b.key;
	}

	void Encode(unsigned char code[74]) const;
	void Decode(const unsigned char code[74]);
	bool Derive(CExtKey &out, unsigned int nChild) const;
	CExtPubKey Neuter() const;
	void SetMaster(const unsigned char *seed, unsigned int nSeedLen);
};

class CNoDestination {
public:
    friend bool operator==(const CNoDestination &a, const CNoDestination &b) { return true; }
    friend bool operator<(const CNoDestination &a, const CNoDestination &b) { return true; }
};

/** A txout script template with a specific destination. It is either:
 *  * CNoDestination: no destination set
 *  * CKeyID: TX_PUBKEYHASH destination
 *  * CScriptID: TX_SCRIPTHASH destination
 *  A CTxDestination is the internal data type encoded in a CCoinAddress
 */
typedef boost::variant<CNoDestination, CKeyID> CTxDestination;






#endif
