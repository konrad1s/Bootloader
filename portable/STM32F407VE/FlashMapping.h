#pragma once

#include "IFlashMapping.h"

class FlashMapping : public IFlashMapping
{
public:
    FlashMapping() = default;

    static constexpr uint32_t appStartAddress = 0x08020000U;
    static constexpr uint32_t appEndAddress = 0x080FFFFFU;
};
