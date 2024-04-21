#pragma once

namespace BootConfig
{
    #define RSA_FIRMWARE_VALIDATION     0
    #define ECC_FIRMWARE_VALIDATION     1

    #define VALIDATE_APP_BEFORE_BOOT    1

    constexpr char bootloaderVersion[] = "1.0.0";

    constexpr size_t waitForBootActionMs = 50000U;
    constexpr size_t actionBootExtensionMs = 50000U;

    constexpr char publicKey[] = "-----BEGIN PUBLIC KEY-----\n"
                                 "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA4MmPwXNM/ZeAgYfLuWX2\n"
                                 "M/99IZuN9stSl6hQnuCu7uBll/2ze+4jV0fzuYPLLLlVzAUrFGR3PtTDuemHBrux\n"
                                 "sl/DktcQe5cSyZapFv+GnPcdLh8Ufc53mleil6Gubq40kP04of6kxAkFpvZq0kSG\n"
                                 "jCzP14XLYm0F5+RtaNl3WDLMxUQ+fRnFY1zEuha0ogaVvnAOJOtOTuddjOP8KTUm\n"
                                 "IPZeWfMcwffzHIRmYtpRvGdLzb8oJSsT8yriikIQq89mIsaPFPWbJl2uzRYbx2jB\n"
                                 "7Gx9S/YRB1F0HW6QFmXHRwVGeziYGVBpivFuEdl+pNY4Ss/ABAJZIk15dR/mhicJ\n"
                                 "UQIDAQAB\n"
                                 "-----END PUBLIC KEY-----\n";
};
