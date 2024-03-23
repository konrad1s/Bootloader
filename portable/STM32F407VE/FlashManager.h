#pragma once

#include "IFlashManager.h"
#include "FlashMapping.h"

class FlashManager : public IFlashManager
{
public:
    FlashManager() = default;
    state Erase(const uint32_t startAddress, const uint32_t endAddress) override;
    state Write(uint32_t startAddress, const void *data, size_t size) override;
    state Read() override;
    state Unlock() override;
    state Lock() override;

private:
    FlashMapping flashMap_;

    static constexpr sectorRange GetSectorRange(uint32_t startAddress, uint32_t endAddress);
};
