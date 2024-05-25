#include "SecureBoot.h"

SecureBoot::SecureBoot()
{
    mbedtls_memory_buffer_alloc_init(mbedtlsBuff, sizeof(mbedtlsBuff));
}

SecureBoot::~SecureBoot()
{
    mbedtls_memory_buffer_alloc_free();
}

SecureBoot::RetStatus SecureBoot::CalculateSHA256(const unsigned char* data, size_t data_len, unsigned char* hash)
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
