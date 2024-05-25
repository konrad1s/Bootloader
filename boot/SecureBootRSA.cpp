#include "SecureBootRSA.h"
#include "BootConfig.h"
#include <string.h>

SecureBootRSA::SecureBootRSA() {}
SecureBootRSA::~SecureBootRSA() {}

SecureBoot::RetStatus SecureBootRSA::ValidateFirmware(
    const unsigned char* signature,
    size_t sig_len,
    const unsigned char* data,
    size_t data_len)
{
    unsigned char hash[hashSize];
    if (CalculateSHA256(data, data_len, hash) != RetStatus::valid)
    {
        return RetStatus::hashCalculationError;
    }

    mbedtls_pk_context pkCtx;
    mbedtls_pk_init(&pkCtx);

    size_t publicKeyLen = strlen((const char*)BootConfig::publicKey) + 1;
    if (mbedtls_pk_parse_public_key(&pkCtx, reinterpret_cast<const uint8_t*>(BootConfig::publicKey), publicKeyLen) != 0)
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::publicKeyError;
    }

    if (!mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_RSA))
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::publicKeyError;
    }

    mbedtls_rsa_context* rsaCtx = mbedtls_pk_rsa(pkCtx);
    if (mbedtls_rsa_set_padding(rsaCtx, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256) != 0)
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::paddingError;
    }

    if (mbedtls_rsa_pkcs1_verify(rsaCtx, MBEDTLS_MD_SHA256, sizeof(hash), hash, signature) != 0)
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::invalidSignature;
    }

    mbedtls_pk_free(&pkCtx);
    return RetStatus::valid;
}
