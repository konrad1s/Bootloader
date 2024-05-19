#pragma once
#include "FlashMapping.h"
#include "stm32f4xx.h"

class AppJumper
{
  public:
    void JumpToApplication() const
    {
        auto metaData = FlashMapping::GetMetaData();
        uint32_t appStartAddress = metaData->appStartAddress;

        DisableInterrupts();
        SetVectorTableAddress(appStartAddress);
        SetMainStackPointer(appStartAddress);
        JumpToResetHandler(appStartAddress);

        while (true)
            ;
    }

  private:
    using AppResetHandler = void (*)(void);

    void DisableInterrupts() const
    {
        __disable_irq();
        for (int i = 0U; i < 8U; ++i)
        {
            NVIC->ICER[i] = 0xFFFFFFFFU;
            NVIC->ICPR[i] = 0xFFFFFFFFU;
        }

        SysTick->CTRL = 0U;
        SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

        SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk | SCB_SHCSR_BUSFAULTENA_Msk | SCB_SHCSR_MEMFAULTENA_Msk);
    }

    void SetVectorTableAddress(uint32_t address) const
    {
        SCB->VTOR = address;
    }

    void SetMainStackPointer(uint32_t address) const
    {
        __set_MSP(*reinterpret_cast<volatile uint32_t*>(address));
    }

    void JumpToResetHandler(uint32_t address) const
    {
        auto appResetHandlerPtr = reinterpret_cast<AppResetHandler>(*(volatile uint32_t*)(address + 4U));

        __DSB();
        __ISB();

        appResetHandlerPtr();
    }
};
