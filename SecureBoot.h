#pragma once

extern "C"
{
#include "mbedtls/platform.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/md.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
}

class SecureBoot
{
public:
    enum class retStatus
    {
        firmwareValid,
        firmwareInvalid,
        firmwareCorrupted
    };

    SecureBoot();
    ~SecureBoot();

    retStatus validateFirmware(const unsigned char *public_key, size_t public_key_len,
                               const unsigned char *signature, size_t sig_len,
                               const unsigned char *data, size_t data_len);

private:
    mbedtls_pk_context pk_ctx;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    bool calculateHash(const unsigned char *data, size_t data_len, unsigned char *hash, size_t *hash_len);
};
