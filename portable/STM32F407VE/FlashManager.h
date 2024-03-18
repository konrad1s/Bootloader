#pragma once

#include "IFlashManager.h"

class FlashManager : public IFlashManager
{
public:
    FlashManager() = default;
    state FlashErase() override;
    state FlashWrite(uint32_t start_addr, const void *data, size_t size) override;
    state FlashRead() override;
    state FlashUnlock() override;
    state FlashLock() override;
};
