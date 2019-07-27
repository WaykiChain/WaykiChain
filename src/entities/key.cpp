// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "entities/key.h"
#include <openssl/bn.h>
#include <openssl/ecdsa.h>
#include <openssl/obj_mac.h>
#include <openssl/rand.h>
#include "commons/base58.h"
#include "commons/common.h"
#include "commons/random.h"
#include "crypto/hash.h"
#include "lax_der_parsing.h"
#include "lax_der_privatekey_parsing.h"

static secp256k1_context *secp256k1_context_verify = nullptr;
static secp256k1_context *secp256k1_context_sign   = nullptr;

// Check that the sig has a low R value and will be less than 71 bytes
bool SigHasLowR(const secp256k1_ecdsa_signature *sig) {
    uint8_t compact_sig[64];
    secp256k1_ecdsa_signature_serialize_compact(secp256k1_context_sign, compact_sig, sig);

    // In DER serialization, all values are interpreted as big-endian, signed integers. The highest
    // bit in the integer indicates its signed-ness; 0 is positive, 1 is negative. When the value is
    // interpreted as a negative integer, it must be converted to a positive value by prepending a
    // 0x00 byte so that the highest bit is 0. We can avoid this prepending by ensuring that our
    // highest bit is always 0, and thus we must check that the first byte is less than 0x80.
    return compact_sig[0] < 0x80;
}

void static BIP32Hash(const uint8_t chainCode[32], uint32_t nChild, uint8_t header, const uint8_t data[32],
                      uint8_t output[64]) {
    uint8_t num[4];
    num[0] = (nChild >> 24) & 0xFF;
    num[1] = (nChild >> 16) & 0xFF;
    num[2] = (nChild >> 8) & 0xFF;
    num[3] = (nChild >> 0) & 0xFF;
    HMAC_SHA512_CTX ctx;
    HMAC_SHA512_Init(&ctx, chainCode, 32);
    HMAC_SHA512_Update(&ctx, &header, 1);
    HMAC_SHA512_Update(&ctx, data, 32);
    HMAC_SHA512_Update(&ctx, num, 4);
    HMAC_SHA512_Final(output, &ctx);
}

bool CKey::Check(const uint8_t *vch) { return secp256k1_ec_seckey_verify(secp256k1_context_sign, vch); }

void CKey::MakeNewKey(bool fCompressedIn) {
    RandAddSeedPerfmon();
    do {
        RAND_bytes(vch, sizeof(vch));
    } while (!Check(vch));
    fValid = true;
    assert(fCompressedIn == true);
    fCompressed = fCompressedIn;
}

CPrivKey CKey::GetPrivKey() const {
    assert(fValid);
    CPrivKey privkey;
    int ret;
    size_t privkeylen;
    privkey.resize(PRIVATE_KEY_SIZE);
    privkeylen = PRIVATE_KEY_SIZE;
    ret        = ec_privkey_export_der(secp256k1_context_sign, privkey.data(), &privkeylen, begin(), fCompressed);
    assert(ret);
    privkey.resize(privkeylen);

    return privkey;
}

CPubKey CKey::GetPubKey() const {
    assert(fValid);

    secp256k1_pubkey pubkey;
    size_t clen = CPubKey::PUBLIC_KEY_SIZE;
    CPubKey result;
    int ret = secp256k1_ec_pubkey_create(secp256k1_context_sign, &pubkey, begin());
    assert(ret);
    secp256k1_ec_pubkey_serialize(secp256k1_context_sign, (uint8_t *)result.begin(), &clen, &pubkey,
                                  fCompressed ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    assert(result.size() == clen);
    assert(result.IsValid());
    return result;
}

bool CKey::Sign(const uint256 &hash, vector<uint8_t> &vchSig) const {
    if (!fValid) return false;

    static const int test_case = 0;
    static const bool grind    = true;

    if (!fValid) return false;
    vchSig.resize(CPubKey::SIGNATURE_SIZE);
    size_t nSigLen            = CPubKey::SIGNATURE_SIZE;
    uint8_t extra_entropy[32] = {0};
    WriteLE32(extra_entropy, test_case);
    secp256k1_ecdsa_signature sig;
    uint32_t counter = 0;
    int ret          = secp256k1_ecdsa_sign(secp256k1_context_sign, &sig, hash.begin(), begin(),
                                   secp256k1_nonce_function_rfc6979, (!grind && test_case) ? extra_entropy : nullptr);

    // Grind for low R
    while (ret && !SigHasLowR(&sig) && grind) {
        WriteLE32(extra_entropy, ++counter);
        ret = secp256k1_ecdsa_sign(secp256k1_context_sign, &sig, hash.begin(), begin(),
                                   secp256k1_nonce_function_rfc6979, extra_entropy);
    }
    assert(ret);
    secp256k1_ecdsa_signature_serialize_der(secp256k1_context_sign, vchSig.data(), &nSigLen, &sig);
    vchSig.resize(nSigLen);
    return true;
}

bool CKey::SignCompact(const uint256 &hash, vector<uint8_t> &vchSig) const {
    if (!fValid) return false;

    vchSig.resize(CPubKey::COMPACT_SIGNATURE_SIZE);
    int rec = -1;
    secp256k1_ecdsa_recoverable_signature sig;
    int ret = secp256k1_ecdsa_sign_recoverable(secp256k1_context_sign, &sig, hash.begin(), begin(),
                                               secp256k1_nonce_function_rfc6979, nullptr);
    assert(ret);
    ret = secp256k1_ecdsa_recoverable_signature_serialize_compact(secp256k1_context_sign, &vchSig[1], &rec, &sig);
    assert(ret);
    assert(rec != -1);
    vchSig[0] = 27 + rec + (fCompressed ? 4 : 0);

    return true;
}

bool CKey::Load(CPrivKey &privkey, CPubKey &vchPubKey, bool fSkipCheck = false) {
    if (!ec_privkey_import_der(secp256k1_context_sign, (uint8_t *)begin(), privkey.data(), privkey.size()))
        return false;
    fCompressed = vchPubKey.IsCompressed();
    fValid      = true;

    if (fSkipCheck) return true;

    return VerifyPubKey(vchPubKey);
}

bool CKey::VerifyPubKey(const CPubKey &pubkey) const {
    if (pubkey.IsCompressed() != fCompressed) {
        return false;
    }
    uint8_t rnd[8];
    std::string str = "WaykiChain key verification\n";
    GetRandBytes(rnd, sizeof(rnd));
    uint256 hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, str.data(), str.size());
    SHA256_Update(&ctx, rnd, sizeof(rnd));
    SHA256_Final(hash.begin(), &ctx);
    // CHash256().Write((uint8_t*)str.data(), str.size()).Write(rnd,
    // sizeof(rnd)).Finalize(hash.begin());
    std::vector<uint8_t> vchSig;
    Sign(hash, vchSig);
    return pubkey.Verify(hash, vchSig);
}

bool CKey::Derive(CKey &keyChild, uint8_t ccChild[32], uint32_t nChild, const uint8_t cc[32]) const {
    assert(IsValid());
    assert(IsCompressed());
    uint8_t out[64];
    if ((nChild >> 31) == 0) {
        CPubKey pubkey = GetPubKey();
        assert(pubkey.begin() + 33 == pubkey.end());
        BIP32Hash(cc, nChild, *pubkey.begin(), pubkey.begin() + 1, out);
    } else {
        assert(begin() + 32 == end());
        BIP32Hash(cc, nChild, 0, begin(), out);
    }
    memcpy(ccChild, out + 32, 32);
    memcpy((uint8_t *)keyChild.begin(), begin(), 32);
    bool ret             = secp256k1_ec_privkey_tweak_add(secp256k1_context_sign, (uint8_t *)keyChild.begin(), out);
    keyChild.fCompressed = true;
    keyChild.fValid      = ret;
    return ret;
}

///////////////////////////////////////////////////////////////////////////////
// class CPubKey

CKeyID CPubKey::GetKeyId() const { return CKeyID(Hash160(vch, vch + size())); }

uint256 CPubKey::GetHash() const { return Hash(vch, vch + size()); }

bool CPubKey::Verify(const uint256 &hash, const vector<uint8_t> &vchSig) const {
    if (!IsValid()) return false;

    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_signature sig;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size())) {
        return false;
    }
    if (!ecdsa_signature_parse_der_lax(secp256k1_context_verify, &sig, vchSig.data(), vchSig.size())) {
        return false;
    }
    /* libsecp256k1's ECDSA verification requires lower-S signatures, which have
     * not historically been enforced in Bitcoin, so normalize them first. */
    secp256k1_ecdsa_signature_normalize(secp256k1_context_verify, &sig, &sig);
    return secp256k1_ecdsa_verify(secp256k1_context_verify, &sig, hash.begin(), &pubkey);
}

bool CPubKey::RecoverCompact(const uint256 &hash, const vector<uint8_t> &vchSig) {
    if (vchSig.size() != COMPACT_SIGNATURE_SIZE) return false;

    int recid  = (vchSig[0] - 27) & 3;
    bool fComp = ((vchSig[0] - 27) & 4) != 0;
    secp256k1_pubkey pubkey;
    secp256k1_ecdsa_recoverable_signature sig;
    if (!secp256k1_ecdsa_recoverable_signature_parse_compact(secp256k1_context_verify, &sig, &vchSig[1], recid)) {
        return false;
    }
    if (!secp256k1_ecdsa_recover(secp256k1_context_verify, &pubkey, &sig, hash.begin())) {
        return false;
    }
    uint8_t pub[PUBLIC_KEY_SIZE];
    size_t publen = PUBLIC_KEY_SIZE;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey,
                                  fComp ? SECP256K1_EC_COMPRESSED : SECP256K1_EC_UNCOMPRESSED);
    Set(pub, pub + publen);
    return true;
}

bool CPubKey::IsFullyValid() const {
    if (!IsValid()) return false;

    secp256k1_pubkey pubkey;
    return secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size());
}

bool CPubKey::Decompress() {
    if (!IsValid()) return false;

    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size())) {
        return false;
    }
    uint8_t pub[PUBLIC_KEY_SIZE];
    size_t publen = PUBLIC_KEY_SIZE;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, SECP256K1_EC_UNCOMPRESSED);
    Set(pub, pub + publen);
    return true;
}

bool CPubKey::Derive(CPubKey &pubkeyChild, uint8_t ccChild[32], uint32_t nChild, const uint8_t cc[32]) const {
    assert(IsValid());
    assert((nChild >> 31) == 0);
    assert(size() == COMPRESSED_PUBLIC_KEY_SIZE);
    uint8_t out[64];
    BIP32Hash(cc, nChild, *begin(), begin() + 1, out);
    memcpy(ccChild, out + 32, 32);

    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_parse(secp256k1_context_verify, &pubkey, vch, size())) {
        return false;
    }
    if (!secp256k1_ec_pubkey_tweak_add(secp256k1_context_verify, &pubkey, out)) {
        return false;
    }
    uint8_t pub[COMPRESSED_PUBLIC_KEY_SIZE];
    size_t publen = COMPRESSED_PUBLIC_KEY_SIZE;
    secp256k1_ec_pubkey_serialize(secp256k1_context_verify, pub, &publen, &pubkey, SECP256K1_EC_COMPRESSED);
    pubkeyChild.Set(pub, pub + publen);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
// class CExtKey

bool CExtKey::Derive(CExtKey &out, uint32_t nChild) const {
    out.nDepth = nDepth + 1;
    CKeyID id  = key.GetPubKey().GetKeyId();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return key.Derive(out.key, out.vchChainCode, nChild, vchChainCode);
}

void CExtKey::SetMaster(const uint8_t *seed, uint32_t nSeedLen) {
    static const char hashkey[] = {'B', 'i', 't', 'c', 'o', 'i', 'n', ' ', 's', 'e', 'e', 'd'};
    HMAC_SHA512_CTX ctx;
    HMAC_SHA512_Init(&ctx, hashkey, sizeof(hashkey));
    HMAC_SHA512_Update(&ctx, seed, nSeedLen);
    uint8_t out[64];
    LockObject(out);
    HMAC_SHA512_Final(out, &ctx);
    key.Set(&out[0], &out[32], true);
    memcpy(vchChainCode, &out[32], 32);
    UnlockObject(out);
    nDepth = 0;
    nChild = 0;
    memset(vchFingerprint, 0, sizeof(vchFingerprint));
}

CExtPubKey CExtKey::Neuter() const {
    CExtPubKey ret;
    ret.nDepth = nDepth;
    memcpy(&ret.vchFingerprint[0], &vchFingerprint[0], 4);
    ret.nChild = nChild;
    ret.pubkey = key.GetPubKey();
    memcpy(&ret.vchChainCode[0], &vchChainCode[0], 32);
    return ret;
}

void CExtKey::Encode(uint8_t code[74]) const {
    code[0] = nDepth;
    memcpy(code + 1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF;
    code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >> 8) & 0xFF;
    code[8] = (nChild >> 0) & 0xFF;
    memcpy(code + 9, vchChainCode, 32);
    code[41] = 0;
    assert(key.size() == 32);
    memcpy(code + 42, key.begin(), 32);
}

void CExtKey::Decode(const uint8_t code[74]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code + 1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code + 9, 32);
    key.Set(code + 42, code + 74, true);
}

void CExtPubKey::Encode(uint8_t code[74]) const {
    code[0] = nDepth;
    memcpy(code + 1, vchFingerprint, 4);
    code[5] = (nChild >> 24) & 0xFF;
    code[6] = (nChild >> 16) & 0xFF;
    code[7] = (nChild >> 8) & 0xFF;
    code[8] = (nChild >> 0) & 0xFF;
    memcpy(code + 9, vchChainCode, 32);
    assert(pubkey.size() == 33);
    memcpy(code + 41, pubkey.begin(), 33);
}

void CExtPubKey::Decode(const uint8_t code[74]) {
    nDepth = code[0];
    memcpy(vchFingerprint, code + 1, 4);
    nChild = (code[5] << 24) | (code[6] << 16) | (code[7] << 8) | code[8];
    memcpy(vchChainCode, code + 9, 32);
    pubkey.Set(code + 41, code + 74);
}

bool CExtPubKey::Derive(CExtPubKey &out, uint32_t nChild) const {
    out.nDepth = nDepth + 1;
    CKeyID id  = pubkey.GetKeyId();
    memcpy(&out.vchFingerprint[0], &id, 4);
    out.nChild = nChild;
    return pubkey.Derive(out.pubkey, out.vchChainCode, nChild, vchChainCode);
}

string CPubKey::ToString() const { return HexStr(begin(), end()); }

string CKeyID::ToAddress() const {
    if (IsNull()) {
        return "";
    } else {
        return CCoinAddress(*this).ToString();
    }
}

CKeyID::CKeyID(const string &strAddress) : uint160() {
    if (strAddress.length() == 40) {
        *this = uint160S(strAddress);
    } else {
        CCoinAddress addr(strAddress);
        addr.GetKeyId(*this);
    }
}

/* static */ int ECCVerifyHandle::refcount = 0;

ECCVerifyHandle::ECCVerifyHandle() {
    if (refcount == 0) {
        assert(secp256k1_context_verify == nullptr);
        secp256k1_context_verify = secp256k1_context_create(SECP256K1_CONTEXT_VERIFY);
        assert(secp256k1_context_verify != nullptr);
    }
    refcount++;
}

ECCVerifyHandle::~ECCVerifyHandle() {
    refcount--;
    if (refcount == 0) {
        assert(secp256k1_context_verify != nullptr);
        secp256k1_context_destroy(secp256k1_context_verify);
        secp256k1_context_verify = nullptr;
    }
}

bool ECC_InitSanityCheck() {
    CKey key;
    key.MakeNewKey(true);
    CPubKey pubkey = key.GetPubKey();
    return key.VerifyPubKey(pubkey);
}

void ECC_Start() {
    assert(secp256k1_context_sign == nullptr);

    secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    assert(ctx != nullptr);

    {
        // Pass in a random blinding seed to the secp256k1 context.
        std::vector<uint8_t, secure_allocator<uint8_t>> vseed(32);
        GetRandBytes(vseed.data(), 32);
        bool ret = secp256k1_context_randomize(ctx, vseed.data());
        assert(ret);
    }

    secp256k1_context_sign = ctx;
}

void ECC_Stop() {
    secp256k1_context *ctx = secp256k1_context_sign;
    secp256k1_context_sign = nullptr;

    if (ctx) {
        secp256k1_context_destroy(ctx);
    }
}
