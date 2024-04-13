#include "SecureBoot.h"
#include <string.h>

SecureBoot::SecureBoot()
{
    mbedtls_memory_buffer_alloc_init(mbedtlsBuff, sizeof(mbedtlsBuff));
}

SecureBoot::~SecureBoot()
{
    mbedtls_memory_buffer_alloc_free();
}

SecureBoot::retStatus SecureBoot::calculateHash(const unsigned char *data, size_t data_len, unsigned char *hash)
{
    mbedtls_sha256_context sha256_ctx;

    mbedtls_sha256_init(&sha256_ctx);

    if (mbedtls_sha256_starts(&sha256_ctx, 0) != 0)
    {
        return retStatus::hashCalculationError;
    }
    if (mbedtls_sha256_update(&sha256_ctx, data, data_len) != 0)
    {
        return retStatus::hashCalculationError;
    }
    if (mbedtls_sha256_finish(&sha256_ctx, hash) != 0)
    {
        return retStatus::hashCalculationError;
    }

    mbedtls_sha256_free(&sha256_ctx);

    return retStatus::valid;
}

SecureBoot::retStatus SecureBoot::validateFirmware(const unsigned char *signature, size_t sig_len,
                                                   const unsigned char *data, size_t data_len)
{
    unsigned char hash[32];
    mbedtls_pk_context pkCtx;
    const mbedtls_md_type_t mdAlg = MBEDTLS_MD_SHA256;
    retStatus hashStatus = calculateHash(data, data_len, hash);

    if (hashStatus != retStatus::valid)
    {
        return hashStatus;
    }

    mbedtls_pk_init(&pkCtx);
    size_t publicKeyLen = strlen((const char *)publicKey) + 1;

    if (mbedtls_pk_parse_public_key(&pkCtx, reinterpret_cast<const uint8_t *>(publicKey), publicKeyLen) != 0)
    {
        return retStatus::publicKeyError;
    }
    if (mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_RSA) != 1)
    {
        return retStatus::publicKeyError;
    }

    mbedtls_rsa_context *rsaCtx = mbedtls_pk_rsa(pkCtx);

    if (mbedtls_rsa_set_padding(rsaCtx, MBEDTLS_RSA_PKCS_V21, mdAlg) != 0)
    {
        return retStatus::paddingError;
    }
    if (mbedtls_rsa_pkcs1_verify(rsaCtx, mdAlg, sizeof(hash), hash, signature) != 0)
    {
        return retStatus::invalidSignature;
    }

    mbedtls_pk_free(&pkCtx);

    return retStatus::valid;
}