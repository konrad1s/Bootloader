#include "SecureBootECC.h"
#include "BootConfig.h"
#include <string.h>

SecureBootECC::SecureBootECC() {}
SecureBootECC::~SecureBootECC() {}

SecureBoot::RetStatus SecureBootECC::ValidateFirmware(
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

    if (mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_ECKEY) != 1)
    {
        return RetStatus::publicKeyError;
    }

    mbedtls_ecdsa_context ecdsaCtx;
    mbedtls_ecdsa_init(&ecdsaCtx);

    mbedtls_ecp_keypair* ecp = mbedtls_pk_ec(pkCtx);
    if (mbedtls_ecdsa_from_keypair(&ecdsaCtx, ecp) != 0)
    {
        mbedtls_ecdsa_free(&ecdsaCtx);
        return RetStatus::publicKeyError;
    }

    if (mbedtls_ecdsa_read_signature(&ecdsaCtx, hash, sizeof(hash), signature, sig_len) != 0)
    {
        mbedtls_ecdsa_free(&ecdsaCtx);
        return RetStatus::invalidSignature;
    }

    mbedtls_ecdsa_free(&ecdsaCtx);
    mbedtls_pk_free(&pkCtx);

    return RetStatus::valid;
}
