#pragma once

#include <cstddef>
#include <stdint.h>

class IFlashManager
{
public:
    enum class state
    {
        eOk,
        eNotOk
    };

    virtual ~IFlashManager() = default;
    virtual state FlashErase() = 0;
    virtual state FlashWrite(uint32_t start_addr, const void *data, size_t size) = 0;
    virtual state FlashRead() = 0;
    virtual state FlashUnlock() = 0;
    virtual state FlashLock() = 0;
};
