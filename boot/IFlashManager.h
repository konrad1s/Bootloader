#pragma once

#include <cstddef>
#include <stdint.h>
#include "FlashMapping.h"

class IFlashManager
{
public:
    enum class RetStatus
    {
        eOk,
        eNotOk,
        einvalidSector,
        eflashedLocked
    };

    struct sectorRange
    {
        uint32_t startSector;
        uint32_t sectorCount;
    };

    virtual ~IFlashManager() = default;
    virtual RetStatus Erase(uint32_t startAddress, uint32_t endAddress) = 0;
    virtual RetStatus Write(uint32_t startAddress, const void *data, size_t size) = 0;
    virtual RetStatus Read(uint32_t startAddress, void* buffer, size_t size) = 0;
    virtual RetStatus Unlock() = 0;
    virtual RetStatus Lock() = 0;
};
