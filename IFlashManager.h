#pragma once

#include <cstddef>
#include <stdint.h>
#include "IFlashMapping.h"

class IFlashManager
{
public:
    enum class state
    {
        eOk,
        eNotOk
    };

    virtual ~IFlashManager() = default;
    virtual state Erase(uint32_t startAddress, uint32_t endAddress) = 0;
    virtual state Write(uint32_t startAddress, const void *data, size_t size) = 0;
    virtual state Read() = 0;
    virtual state Unlock() = 0;
    virtual state Lock() = 0;
};
