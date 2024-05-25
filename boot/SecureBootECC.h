#pragma once
#include "SecureBoot.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/pk.h"

class SecureBootECC : public SecureBoot
{
  public:
    SecureBootECC();
    ~SecureBootECC();

    RetStatus ValidateFirmware(
        const unsigned char* signature,
        size_t sig_len,
        const unsigned char* data,
        size_t data_len) override;
};
