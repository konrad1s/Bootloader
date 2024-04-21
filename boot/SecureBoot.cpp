#include "SecureBoot.h"
#include "BootConfig.h"
#include <string.h>

#if (RSA_FIRMWARE_VALIDATION == 1) && (ECC_FIRMWARE_VALIDATION == 1)
#error "Only one of RSA_FIRMWARE_VALIDATION or ECC_FIRMWARE_VALIDATION can be defined"
#endif

static const size_t HASH_SIZE = 32;

SecureBoot::SecureBoot()
{
    mbedtls_memory_buffer_alloc_init(mbedtlsBuff, sizeof(mbedtlsBuff));
}

SecureBoot::~SecureBoot()
{
    mbedtls_memory_buffer_alloc_free();
}

SecureBoot::RetStatus SecureBoot::CalculateSHA256(const unsigned char *data, size_t data_len, unsigned char *hash)
{
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);

    if (mbedtls_sha256_starts(&sha256_ctx, 0) != 0)
    {
        mbedtls_sha256_free(&sha256_ctx);
        return RetStatus::hashCalculationError;
    }

    if (mbedtls_sha256_update(&sha256_ctx, data, data_len) != 0)
    {
        mbedtls_sha256_free(&sha256_ctx);
        return RetStatus::hashCalculationError;
    }

    if (mbedtls_sha256_finish(&sha256_ctx, hash) != 0)
    {
        mbedtls_sha256_free(&sha256_ctx);
        return RetStatus::hashCalculationError;
    }

    mbedtls_sha256_free(&sha256_ctx);
    return RetStatus::valid;
}

SecureBoot::RetStatus SecureBoot::ValidateFirmwareRSA(const unsigned char *signature, size_t sig_len,
                                                      const unsigned char *data, size_t data_len)
{
    unsigned char hash[HASH_SIZE];
    if (CalculateSHA256(data, data_len, hash) != RetStatus::valid)
    {
        return RetStatus::hashCalculationError;
    }

    mbedtls_pk_context pkCtx;
    mbedtls_pk_init(&pkCtx);

    size_t publicKeyLen = strlen((const char *)BootConfig::publicKey) + 1;
    if (mbedtls_pk_parse_public_key(&pkCtx, reinterpret_cast<const uint8_t *>(BootConfig::publicKey), publicKeyLen) != 0)
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::publicKeyError;
    }

    if (!mbedtls_pk_can_do(&pkCtx, MBEDTLS_PK_RSA))
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::publicKeyError;
    }

    mbedtls_rsa_context *rsaCtx = mbedtls_pk_rsa(pkCtx);
    if (mbedtls_rsa_set_padding(rsaCtx, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256) != 0)
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::paddingError;
    }

    if (mbedtls_rsa_pkcs1_verify(rsaCtx, MBEDTLS_MD_SHA256, HASH_SIZE, hash, signature) != 0)
    {
        mbedtls_pk_free(&pkCtx);
        return RetStatus::invalidSignature;
    }

    mbedtls_pk_free(&pkCtx);
    return RetStatus::valid;
}

SecureBoot::RetStatus SecureBoot::ValidateFirmwareECC(const unsigned char *signature, size_t sig_len,
                                                      const unsigned char *data, size_t data_len)
{
    unsigned char hash[HASH_SIZE];
    if (CalculateSHA256(data, data_len, hash) != RetStatus::valid)
    {
        return RetStatus::hashCalculationError;
    }

    mbedtls_pk_context pkCtx;
    mbedtls_pk_init(&pkCtx);

    size_t publicKeyLen = strlen((const char *)BootConfig::publicKey) + 1;
    if (mbedtls_pk_parse_public_key(&pkCtx, reinterpret_cast<const uint8_t *>(BootConfig::publicKey), publicKeyLen) != 0)
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

    mbedtls_ecp_keypair *ecp = mbedtls_pk_ec(pkCtx);
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

SecureBoot::RetStatus SecureBoot::ValidateFirmware(const unsigned char *signature, size_t sig_len,
                                                   const unsigned char *data, size_t data_len)
{
#if (RSA_FIRMWARE_VALIDATION == 1)
    return ValidateFirmwareRSA(signature, sig_len, data, data_len);
#elif (ECC_FIRMWARE_VALIDATION == 1)
    return ValidateFirmwareECC(signature, sig_len, data, data_len);
#else
    return RetStatus::invalidSignature;
#endif
}
