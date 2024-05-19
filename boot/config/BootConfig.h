#pragma once

namespace BootConfig {
#define RSA_FIRMWARE_VALIDATION 0
#define ECC_FIRMWARE_VALIDATION 1

#define VALIDATE_APP_BEFORE_BOOT 1

constexpr char bootloaderVersion[] = "1.0.0";

constexpr size_t waitForBootActionMs = 50000U;
constexpr size_t actionBootExtensionMs = 50000U;

constexpr char publicKey[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEZdR4u/SQRKrNl9jL6AEmgIHMGbA8\n"
    "5BXcHgySaR+eDLgej9YT87s692Dh3TLNiSJUhdLDH6gcLVkXoewzlFLFRA==\n"
    "-----END PUBLIC KEY-----\n";
}; // namespace BootConfig
