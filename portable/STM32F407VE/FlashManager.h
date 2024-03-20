#pragma once

#include "IFlashManager.h"

class FlashManager : public IFlashManager
{
public:
    FlashManager() = default;
    state Erase() override;
    state Write(uint32_t startAddress, const void *data, size_t size) override;
    state Read() override;
    state Unlock() override;
    state Lock() override;
};
