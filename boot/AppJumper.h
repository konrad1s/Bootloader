#pragma once
#include "FlashMapping.h"

class AppJumper
{
public:
    void JumpToApplication() const
    {
        using AppResetHandler = void (*)(void);
        auto metaData = FlashMapping::GetMetaData();
        auto appResetHandlerPtr = reinterpret_cast<AppResetHandler>(metaData->appStartAddress);

        SCB->VTOR = metaData->appStartAddress;
        __set_MSP(*reinterpret_cast<volatile uint32_t *>(metaData->appStartAddress));
        appResetHandlerPtr();
    }
};