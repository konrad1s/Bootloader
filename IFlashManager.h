#pragma once

class IFlashManager
{
public:
    virtual ~IFlashManager() = default;
    virtual void FlashErase() = 0;
    virtual void FlashWrite(const void* data, size_t size) = 0;
    virtual void FlashRead() = 0;
    virtual void FlashUnlock() = 0;
    virtual void FlashLock() = 0;
};
