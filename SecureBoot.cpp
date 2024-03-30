#include "SecureBoot.h"

SecureBoot::SecureBoot()
{
    mbedtls_pk_init(&pk_ctx);
}

SecureBoot::~SecureBoot()
{
    mbedtls_pk_free(&pk_ctx);
}

bool SecureBoot::calculateHash(const unsigned char *data, size_t data_len, unsigned char *hash, size_t *hash_len)
{
    if (*hash_len < 32)
    {
        return false;
    }

    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    if (mbedtls_sha256_starts(&sha256_ctx, 0) != 0)
    {
        return false;
    }
    if (mbedtls_sha256_update(&sha256_ctx, data, data_len) != 0)
    {
        return false;
    }
    if (mbedtls_sha256_finish(&sha256_ctx, hash) != 0)
    {
        return false;
    }

    *hash_len = 32;
    return true;
}

SecureBoot::retStatus SecureBoot::validateFirmware(const unsigned char *signature, size_t sig_len,
                                                   const unsigned char *data, size_t data_len)
{
    unsigned char hash[32];
    size_t hash_len = sizeof(hash);

    if (!calculateHash(data, data_len, hash, &hash_len))
    {
        return retStatus::firmwareCorrupted;
    }

    if (mbedtls_pk_parse_public_key(&pk_ctx, reinterpret_cast<const uint8_t *>(publicKey), sizeof(publicKey)) != 0)
    {
        return retStatus::firmwareInvalid;
    }

    if (mbedtls_pk_verify(&pk_ctx, MBEDTLS_MD_SHA256, hash, hash_len, signature, sig_len) == 0)
    {
        return retStatus::firmwareValid;
    }
    else
    {
        return retStatus::firmwareInvalid;
    }
}
