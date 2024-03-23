#pragma once

#include "IFlashMapping.h"

class FlashMapping : public IFlashMapping
{
public:
    FlashMapping() = default;

    static constexpr uint32_t appStartAddress = 0x0U; // TODO
    static constexpr uint32_t appEndAddress = 0x0U; // TODO
};
