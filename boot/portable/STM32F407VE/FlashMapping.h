#pragma once
#include "stm32f4xx_hal.h"

namespace FlashMapping
{
    static constexpr uint32_t appSignatureSizeAddress = 0x08020000U;
    static constexpr uint32_t appSignatureAddress = 0x08020002U;

    static constexpr uint32_t appStartAddress = 0x08020110U;
    static constexpr uint32_t appEndAddress = 0x080FFFFFU;
    static constexpr uint32_t appSize = appEndAddress - appStartAddress;

    static constexpr uint32_t maxDataSize = 16u;

    inline size_t getAppSignatureSize()
    {
        return static_cast<size_t>(*reinterpret_cast<uint16_t *>(appSignatureSizeAddress));
    }
};
