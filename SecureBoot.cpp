#include "SecureBoot.h"

SecureBoot::SecureBoot()
{
    // mbedtls_pk_init(&pk_ctx);
    // mbedtls_ctr_drbg_init(&ctr_drbg);
    // mbedtls_entropy_init(&entropy);
}

SecureBoot::~SecureBoot()
{
    // mbedtls_pk_free(&pk_ctx);
    // mbedtls_ctr_drbg_free(&ctr_drbg);
    // mbedtls_entropy_free(&entropy);
}

SecureBoot::retStatus SecureBoot::validateFirmware(const unsigned char *public_key, size_t public_key_len,
                                                   const unsigned char *signature, size_t sig_len,
                                                   const unsigned char *hash, size_t hash_len)
{
    // int ret = mbedtls_pk_parse_public_key(&pk_ctx, public_key, public_key_len);
    // if (ret != 0)
    //     return retStatus::firmwareCorrupted;

    // ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
    // if (ret != 0)
    //     return retStatus::firmwareCorrupted;

    // ret = mbedtls_pk_verify(&pk_ctx, MBEDTLS_MD_SHA256, hash, hash_len, signature, sig_len);
    // if (ret == 0)
    //     return retStatus::firmwareValid;
    // else
    //     return retStatus::firmwareInvalid;
}
