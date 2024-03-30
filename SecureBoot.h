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

    retStatus validateFirmware(const unsigned char *signature, size_t sig_len,
                               const unsigned char *data, size_t data_len);

private:
    mbedtls_pk_context pk_ctx;

    const char *publicKey = "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4MmPwXNM/ZeAgYfLuWX2\n"
    "M/99IZuN9stSl6hQnuCu7uBll/2ze+4jV0fzuYPLLLlVzAUrFGR3PtTDuemHBrux\n"
    "sl/DktcQe5cSyZapFv+GnPcdLh8Ufc53mleil6Gubq40kP04of6kxAkFpvZq0kSG\n"
    "jCzP14XLYm0F5+RtaNl3WDLMxUQ+fRnFY1zEuha0ogaVvnAOJOtOTuddjOP8KTUm\n"
    "IPZeWfMcwffzHIRmYtpRvGdLzb8oJSsT8yriikIQq89mIsaPFPWbJl2uzRYbx2jB\n"
    "7Gx9S/YRB1F0HW6QFmXHRwVGeziYGVBpivFuEdl+pNY4Ss/ABAJZIk15dR/mhicJ\n"
    "UQIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

    bool calculateHash(const unsigned char *data, size_t data_len, unsigned char *hash, size_t *hash_len);
};
