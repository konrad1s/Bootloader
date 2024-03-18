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
    virtual state Erase() = 0;
    virtual state Write(uint32_t start_addr, const void *data, size_t size) = 0;
    virtual state Read() = 0;
    virtual state Unlock() = 0;
    virtual state Lock() = 0;
};
