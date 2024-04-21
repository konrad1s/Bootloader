#pragma once

#include "IFlashManager.h"
#include "FlashMapping.h"

class FlashManager : public IFlashManager
{
public:
    FlashManager() = default;
    RetStatus Erase(uint32_t startAddress, uint32_t endAddress) override;
    RetStatus Write(uint32_t startAddress, const void *data, size_t size) override;
    RetStatus Read(uint32_t startAddress, void *buffer, size_t size) override;
    RetStatus Unlock() override;
    RetStatus Lock() override;

private:
    static constexpr sectorRange GetSectorRange(uint32_t startAddress, uint32_t endAddress);
    RetStatus ToggleFlashLock(bool lock);
};
