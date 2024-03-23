#pragma once
#include "stm32f4xx_hal.h"

namespace FlashMapping
{
    static constexpr uint32_t appStartAddress = 0x08020000U;
    static constexpr uint32_t appEndAddress = 0x080FFFFFU;
};
