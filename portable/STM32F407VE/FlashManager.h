#pragma once

#include "IFlashManager.h"
#include "FlashMapping.h"

class FlashManager : public IFlashManager
{
public:
    FlashManager() = default;
    state Erase(uint32_t startAddress, uint32_t endAddress) override;
    state Write(uint32_t startAddress, const void *data, size_t size) override;
    state Read() override;
    state Unlock() override;
    state Lock() override;

private:
    static const uint32_t FLASH_PAGE_SIZE = 2048U;
    FlashMapping flashMap_;
};
