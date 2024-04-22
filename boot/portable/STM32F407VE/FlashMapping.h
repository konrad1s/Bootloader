#pragma once
#include "stm32f4xx_hal.h"

namespace FlashMapping
{
    static constexpr uint32_t appMinStartAddress = 0x08020000U;
    static constexpr uint32_t appMaxEndAddress = 0x080FFFFFU;

    static constexpr uint32_t appMetaDataAddress = 0x08020000U;
    static constexpr uint32_t appSignatureMaxSize = 256U;
    static constexpr uint32_t maxDataSize = 16u;

    struct MetaData
    {
        uint16_t signatureSize;
        uint8_t signature[appSignatureMaxSize];
        uint8_t notUsed[2];
        uint32_t appStartAddress;
        uint32_t appEndAddress;
        uint32_t appPresentFlag;
    } __attribute__((__packed__));

    inline MetaData *GetMetaData()
    {
        return reinterpret_cast<MetaData *>(appMetaDataAddress);
    }

    inline size_t GetAppSize()
    {
        MetaData *metaData = GetMetaData();
        return metaData->appEndAddress - metaData->appStartAddress;
    }

    constexpr uint32_t appSignatureSizeAddress = appMetaDataAddress + offsetof(MetaData, signatureSize);
    constexpr uint32_t appSignatureAddress = appMetaDataAddress + offsetof(MetaData, signature);
    constexpr uint32_t appValidFlagAddress = appMetaDataAddress + offsetof(MetaData, appPresentFlag);
};
