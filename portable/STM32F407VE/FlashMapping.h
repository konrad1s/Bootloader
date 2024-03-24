#pragma once
#include "stm32f4xx_hal.h"

namespace FlashMapping
{
    static constexpr uint32_t appStartAddress = 0x08020000U;
    static constexpr uint32_t appEndAddress = 0x080FFFFFU;

    static constexpr uint32_t appSignatureSize = 16U;
    static constexpr uint32_t appSignatureAddress = appEndAddress - appSignatureSize;

    static constexpr uint32_t appVersionSize = 3U;
    static constexpr uint32_t appVersionAddress = appSignatureAddress - appVersionSize;

    static constexpr uint32_t maxDataSize = 16u;
};
