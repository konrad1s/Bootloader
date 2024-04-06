#pragma once
#include "FlashMapping.h"

class AppJumper
{
public:
    void jumpToApplication() const
    {
        using AppResetHandler = void (*)(void);
        auto appResetHandlerPtr = reinterpret_cast<AppResetHandler>(FlashMapping::appStartAddress);

        SCB->VTOR = FlashMapping::appStartAddress;
        __set_MSP(*reinterpret_cast<volatile uint32_t*>(FlashMapping::appStartAddress));
        appResetHandlerPtr();
    }
};