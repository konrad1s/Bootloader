#pragma once

extern "C"
{
#include "mbedtls/platform.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"
#include "mbedtls/md.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/pk.h"
#include "mbedtls/memory_buffer_alloc.h"
}

class SecureBoot
{
public:
    enum class retStatus
    {
        valid,
        invalidSignature,
        hashCalculationError,
        publicKeyError,
        paddingError
    };

    SecureBoot();
    ~SecureBoot();

    retStatus validateFirmware(const unsigned char *signature, size_t sig_len,
                               const unsigned char *data, size_t data_len);

private:
    unsigned char mbedtlsBuff[8192];
    retStatus validateFirmwareRSA(const unsigned char *signature, size_t sig_len,
                                  const unsigned char *data, size_t data_len);
    retStatus validateFirmwareECC(const unsigned char *signature, size_t sig_len,
                                  const unsigned char *data, size_t data_len);
    retStatus calculateSHA256(const unsigned char *data, size_t data_len, unsigned char *hash);
};
