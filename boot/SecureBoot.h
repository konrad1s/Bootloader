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
    enum class RetStatus
    {
        valid,
        invalidSignature,
        hashCalculationError,
        publicKeyError,
        paddingError
    };

    SecureBoot();
    virtual ~SecureBoot();

    virtual RetStatus ValidateFirmware(
        const unsigned char* signature,
        size_t sig_len,
        const unsigned char* data,
        size_t data_len) = 0;

    static const size_t hashSize = 32;

  protected:
    unsigned char mbedtlsBuff[8192];
    RetStatus CalculateSHA256(const unsigned char* data, size_t data_len, unsigned char* hash);
};
