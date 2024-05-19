#pragma once

#include "FlashMapping.h"

class FlashManager
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

    RetStatus Erase(uint32_t startAddress, uint32_t endAddress);
    RetStatus Write(uint32_t startAddress, const void* data, size_t size);
    RetStatus Read(uint32_t startAddress, void* buffer, size_t size);
    RetStatus Unlock();
    RetStatus Lock();

  private:
    static constexpr sectorRange GetSectorRange(uint32_t startAddress, uint32_t endAddress);
    RetStatus ToggleFlashLock(bool lock);
};
