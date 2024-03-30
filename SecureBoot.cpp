#include "SecureBoot.h"

SecureBoot::SecureBoot()
{
    mbedtls_pk_init(&pk_ctx);
}

SecureBoot::~SecureBoot()
{
    mbedtls_pk_free(&pk_ctx);
}

SecureBoot::retStatus SecureBoot::calculateHash(const unsigned char *data, size_t data_len,
                                                unsigned char *hash, size_t *hash_len)
{
    if (*hash_len < 32)
    {
        return retStatus::dataCorrupted;
    }

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

    *hash_len = 32;
    mbedtls_sha256_free(&sha256_ctx);

    return retStatus::valid;
}

SecureBoot::retStatus SecureBoot::validateFirmware(const unsigned char *signature, size_t sig_len,
                                                   const unsigned char *data, size_t data_len)
{
    unsigned char hash[32];
    size_t hash_len = sizeof(hash);

    retStatus hashStatus = calculateHash(data, data_len, hash, &hash_len);
    if (hashStatus != retStatus::valid)
    {
        return hashStatus;
    }

    if (mbedtls_pk_parse_public_key(&pk_ctx, reinterpret_cast<const uint8_t *>(publicKey), sizeof(publicKey)) != 0)
    {
        return retStatus::publicKeyError;
    }

    if (mbedtls_pk_verify(&pk_ctx, MBEDTLS_MD_SHA256, hash, hash_len, signature, sig_len) == 0)
    {
        return retStatus::valid;
    }
    else
    {
        return retStatus::invalidSignature;
    }
}
