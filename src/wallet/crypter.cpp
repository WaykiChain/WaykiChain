// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "crypter.h"

#include "commons/util.h"
#include <string>
#include <vector>
#include <boost/foreach.hpp>
#include "commons/openssl.hpp"

bool CCrypter::SetKeyFromPassphrase(const SecureString& strKeyData, const vector<unsigned char>& chSalt,
                                    const uint32_t nRounds, const uint32_t nDerivationMethod) {
    if (nRounds < 1 || chSalt.size() != WALLET_CRYPTO_SALT_SIZE)
        return false;

    int32_t i = 0;
    if (nDerivationMethod == 0)
        i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha512(), &chSalt[0], (unsigned char*)&strKeyData[0],
                           strKeyData.size(), nRounds, chKey, chIV);

    if (i != (int32_t)WALLET_CRYPTO_KEY_SIZE) {
        OPENSSL_cleanse(chKey, sizeof(chKey));
        OPENSSL_cleanse(chIV, sizeof(chIV));
        return false;
    }

    fKeySet = true;
    return true;
}

bool CCrypter::SetKey(const CKeyingMaterial& chNewKey, const vector<unsigned char>& chNewIV) {
    if (chNewKey.size() != WALLET_CRYPTO_KEY_SIZE || chNewIV.size() != WALLET_CRYPTO_KEY_SIZE)
        return false;

    memcpy(&chKey[0], &chNewKey[0], sizeof chKey);
    memcpy(&chIV[0], &chNewIV[0], sizeof chIV);

    fKeySet = true;
    return true;
}

bool CCrypter::Encrypt(const CKeyingMaterial& vchPlaintext, vector<unsigned char>& vchCiphertext) {
    if (!fKeySet)
        return false;

    // max ciphertext len for a n bytes of plaintext is
    // n + AES_BLOCK_SIZE - 1 bytes
    int32_t nLen  = vchPlaintext.size();
    int32_t nCLen = nLen + AES_BLOCK_SIZE, nFLen = 0;
    vchCiphertext = vector<unsigned char>(nCLen);

    evp_cipher_ctx ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return false;
    }

    bool fOk = true;

    if (fOk)
        fOk = EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, chKey, chIV);
    if (fOk)
        fOk = EVP_EncryptUpdate(ctx, &vchCiphertext[0], &nCLen, &vchPlaintext[0], nLen);
    if (fOk)
        fOk = EVP_EncryptFinal_ex(ctx, (&vchCiphertext[0]) + nCLen, &nFLen);

    if (!fOk)
        return false;

    vchCiphertext.resize(nCLen + nFLen);
    return true;
}

bool CCrypter::Decrypt(const vector<unsigned char>& vchCiphertext, CKeyingMaterial& vchPlaintext) {
    if (!fKeySet)
        return false;

    // plaintext will always be equal to or lesser than length of ciphertext
    int32_t nLen  = vchCiphertext.size();
    int32_t nPLen = nLen, nFLen = 0;

    vchPlaintext = CKeyingMaterial(nPLen);

    evp_cipher_ctx ctx(EVP_CIPHER_CTX_new());
    if (!ctx) {
        return false;
    }

    bool fOk = true;

    if (fOk)
        fOk = EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, chKey, chIV);
    if (fOk)
        fOk = EVP_DecryptUpdate(ctx, &vchPlaintext[0], &nPLen, &vchCiphertext[0], nLen);
    if (fOk)
        fOk = EVP_DecryptFinal_ex(ctx, (&vchPlaintext[0]) + nPLen, &nFLen);

    if (!fOk)
        return false;

    vchPlaintext.resize(nPLen + nFLen);
    return true;
}

bool EncryptSecret(const CKeyingMaterial& vMasterKey, const CKeyingMaterial& vchPlaintext, const uint256& nIV,
                   vector<unsigned char>& vchCiphertext) {
    CCrypter cKeyCrypter;
    vector<unsigned char> chIV(WALLET_CRYPTO_KEY_SIZE);
    memcpy(&chIV[0], &nIV, WALLET_CRYPTO_KEY_SIZE);
    if (!cKeyCrypter.SetKey(vMasterKey, chIV))
        return false;
    return cKeyCrypter.Encrypt(*((const CKeyingMaterial*)&vchPlaintext), vchCiphertext);
}

bool DecryptSecret(const CKeyingMaterial& vMasterKey, const vector<unsigned char>& vchCiphertext, const uint256& nIV,
                   CKeyingMaterial& vchPlaintext) {
    CCrypter cKeyCrypter;
    vector<unsigned char> chIV(WALLET_CRYPTO_KEY_SIZE);
    memcpy(&chIV[0], &nIV, WALLET_CRYPTO_KEY_SIZE);
    if (!cKeyCrypter.SetKey(vMasterKey, chIV))
        return false;
    return cKeyCrypter.Decrypt(vchCiphertext, *((CKeyingMaterial*)&vchPlaintext));
}

bool CCryptoKeyStore::SetCrypted() {
    LOCK(cs_KeyStore);

    if (fUseCrypto)
        return true;

    if (HaveMainKey())
        return false;

    fUseCrypto = true;
    return true;
}

bool CCryptoKeyStore::Lock() {
    if (!SetCrypted())
        return false;

    {
        LOCK(cs_KeyStore);
        vMasterKey.clear();
    }

    return true;
}

bool CCryptoKeyStore::Unlock(const CKeyingMaterial& vMasterKeyIn) {
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return false;

        CryptedKeyMap::const_iterator mi = mapCryptedKeys.begin();
        for (; mi != mapCryptedKeys.end(); ++mi) {
            const CPubKey& vchPubKey                      = (*mi).second.first;
            const vector<unsigned char>& vchCryptedSecret = (*mi).second.second;
            CKeyingMaterial vchSecret;
            if (!DecryptSecret(vMasterKeyIn, vchCryptedSecret, vchPubKey.GetHash(), vchSecret))
                return false;
            if (vchSecret.size() != 32)
                return false;
            CKey key;
            key.Set(vchSecret.begin(), vchSecret.end(), vchPubKey.IsCompressed());
            if (key.GetPubKey() == vchPubKey)
                break;
            return false;
        }
        vMasterKey = vMasterKeyIn;
    }

    return true;
}

bool CCryptoKeyStore::AddKeyCombi(const CKeyID& keyId, const CKeyCombi& keyCombi) {
    {
        LOCK(cs_KeyStore);

        if (!IsEncrypted())
            return CBasicKeyStore::AddKeyCombi(keyId, keyCombi);

        if (IsLocked())
            return false;

        CKey mainKey;
        keyCombi.GetCKey(mainKey, false);
        CKeyCombi newkeyCombi = keyCombi;
        newkeyCombi.CleanMainKey();
        CBasicKeyStore::AddKeyCombi(keyId, keyCombi);

        vector<unsigned char> vchCryptedSecret;
        CKeyingMaterial vchSecret(mainKey.begin(), mainKey.end());
        CPubKey pubKey;
        pubKey = mainKey.GetPubKey();
        if (!EncryptSecret(vMasterKey, vchSecret, pubKey.GetHash(), vchCryptedSecret))
            return false;

        if (!AddCryptedKey(pubKey, vchCryptedSecret))
            return false;
    }
    return true;
}

bool CCryptoKeyStore::AddCryptedKey(const CPubKey& vchPubKey, const vector<unsigned char>& vchCryptedSecret) {
    {
        LOCK(cs_KeyStore);
        if (!SetCrypted())
            return false;
        mapCryptedKeys[vchPubKey.GetKeyId()] = make_pair(vchPubKey, vchCryptedSecret);
    }
    return true;
}

bool CCryptoKeyStore::GetKey(const CKeyID& address, CKey& keyOut, bool IsMiner) const {
    {
        LOCK(cs_KeyStore);

        if (IsMiner) {
            return CBasicKeyStore::GetKey(address, keyOut, IsMiner);
        } else {
            if (!IsEncrypted())
                return CBasicKeyStore::GetKey(address, keyOut);

            CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
            if (mi != mapCryptedKeys.end()) {
                const CPubKey& vchPubKey                      = (*mi).second.first;
                const vector<unsigned char>& vchCryptedSecret = (*mi).second.second;
                CKeyingMaterial vchSecret;
                if (!DecryptSecret(vMasterKey, vchCryptedSecret, vchPubKey.GetHash(), vchSecret))
                    return false;
                if (vchSecret.size() != 32)
                    return false;
                keyOut.Set(vchSecret.begin(), vchSecret.end(), vchPubKey.IsCompressed());
                return true;
            }
        }
    }
    return false;
}

bool CCryptoKeyStore::GetPubKey(const CKeyID& address, CPubKey& vchPubKeyOut, bool IsMiner) const {
    {
        LOCK(cs_KeyStore);
        if (IsMiner) {
            return CKeyStore::GetPubKey(address, vchPubKeyOut, IsMiner);
        } else {
            if (!IsEncrypted())
                return CKeyStore::GetPubKey(address, vchPubKeyOut, IsMiner);

            CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(address);
            if (mi != mapCryptedKeys.end()) {
                vchPubKeyOut = (*mi).second.first;
                return true;
            }
        }
    }
    return false;
}

bool CCryptoKeyStore::GetKeyCombi(const CKeyID& address, CKeyCombi& keyCombiOut) const {
    CBasicKeyStore::GetKeyCombi(address, keyCombiOut);
    if (!IsEncrypted())
        return true;
    CKey keyOut;
    if (!IsLocked()) {
        if (!GetKey(address, keyOut))
            return false;
        keyCombiOut.SetMainKey(keyOut);
    }
    return true;
}

bool CCryptoKeyStore::EncryptKeys(CKeyingMaterial& vMasterKeyIn) {
    {
        LOCK(cs_KeyStore);
        if (!mapCryptedKeys.empty() || IsEncrypted())
            return false;
        fUseCrypto = true;
        for (auto& mKey : mapKeys) {
            CKey mainKey;
            mKey.second.GetCKey(mainKey, false);
            CPubKey vchPubKey = mainKey.GetPubKey();
            CKeyingMaterial vchSecret(mainKey.begin(), mainKey.end());
            vector<unsigned char> vchCryptedSecret;
            if (!EncryptSecret(vMasterKeyIn, vchSecret, vchPubKey.GetHash(), vchCryptedSecret))
                return false;
            if (!AddCryptedKey(vchPubKey, vchCryptedSecret))
                return false;
            mKey.second.CleanMainKey();
        }
    }
    return true;
}
