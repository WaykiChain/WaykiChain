
#ifndef OPENSSL_HPP
#define OPENSSL_HPP

#include <openssl/aes.h>
#include <openssl/ec.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/ecdh.h>
#include <openssl/sha.h>
#include <openssl/obj_mac.h>

/** 
 * @file openssl.hpp
 * Provides common utility calls for wrapping openssl c api.
 */

class path;

template <typename SslType>
struct SslWrapper
{
    SslWrapper(SslType* obj):obj(obj) {}

    operator SslType*() { return obj; }
    operator const SslType*() const { return obj; }
    SslType* operator->() { return obj; }
    const SslType* operator->() const { return obj; }

    SslType* obj;
};

#define SSL_TYPE(name, sslType, freeFunc) \
    struct name  : public SslWrapper<sslType> \
    { \
        name(sslType* obj=nullptr) \
            : SslWrapper(obj) {} \
        ~name() \
        { \
            if( obj != nullptr ) \
            freeFunc(obj); \
        } \
    };

SSL_TYPE(ec_group,       EC_GROUP,       EC_GROUP_free)
SSL_TYPE(ec_point,       EC_POINT,       EC_POINT_free)
SSL_TYPE(ecdsa_sig,      ECDSA_SIG,      ECDSA_SIG_free)
SSL_TYPE(bn_ctx,         BN_CTX,         BN_CTX_free)
SSL_TYPE(evp_cipher_ctx, EVP_CIPHER_CTX, EVP_CIPHER_CTX_free )
SSL_TYPE(ec_key,         EC_KEY,         EC_KEY_free)

/** allocates a bignum by default.. */
struct SslBigNum : public SslWrapper<BIGNUM>
{
    SslBigNum() : SslWrapper(BN_new()) {}
    ~SslBigNum() { BN_free(obj); }
};

#endif//OPENSSL_HPP