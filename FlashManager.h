#pragma once

#include "IFlashManager.h"

class FlashManager : public IFlashManager
{
public:
    FlashManager() = default;
    void FlashErase();
    void FlashWrite();
    void FlashRead();
    void FlashUnlock();
    void FlashLock();
};
