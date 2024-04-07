#pragma once
#include "stm32f4xx_hal.h"

namespace FlashMapping
{
    static constexpr uint32_t appSignatureAddress = 0x08020000U;
    static constexpr uint32_t appSignatureSize = 256U;
    static constexpr uint32_t appSignatureAddressEnd = appSignatureAddress + appSignatureSize;

    static constexpr uint32_t appVersionAddress = appSignatureAddressEnd;
    static constexpr uint32_t appVersionSize = 3U;
    static constexpr uint32_t appVersionAddressEnd = appVersionAddress + appVersionSize;

    static constexpr uint32_t appPaddingAddress = appVersionAddressEnd;
    static constexpr uint32_t appPaddingNotUsedSize = 16U - appVersionSize;
    static constexpr uint32_t appPaddingAddressEnd = appPaddingAddress + appPaddingNotUsedSize;

    static constexpr uint32_t appStartAddress = appPaddingAddressEnd;
    static constexpr uint32_t appEndAddress = 0x080FFFFFU;
    static constexpr uint32_t appSize = appEndAddress - appStartAddress;

    static constexpr uint32_t maxDataSize = 16u;
};
