#pragma once
#include "SecureBoot.h"
#include "mbedtls/rsa.h"
#include "mbedtls/pk.h"

class SecureBootRSA : public SecureBoot
{
  public:
    SecureBootRSA();
    ~SecureBootRSA();

    RetStatus ValidateFirmware(
        const unsigned char* signature,
        size_t sig_len,
        const unsigned char* data,
        size_t data_len) override;
};
