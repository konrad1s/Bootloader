#pragma once

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
                               const unsigned char *hash, size_t hash_len);

private:
    //TODO:
    // mbedtls_pk_context pk_ctx;
    // mbedtls_entropy_context entropy;
    // mbedtls_ctr_drbg_context ctr_drbg;
};
