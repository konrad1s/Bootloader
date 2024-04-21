#include "FlashManager.h"
#include "stm32f4xx_hal.h"
#include <cstring>

namespace FlashConstants
{
    constexpr uint32_t sectorAddresses[][2] = {
        {0x08000000U, 0x08003FFFU}, {0x08004000U, 0x08007FFFU}, {0x08008000U, 0x0800BFFFU},
        {0x0800C000U, 0x0800FFFFU}, {0x08010000U, 0x0801FFFFU}, {0x08020000U, 0x0803FFFFU},
        {0x08040000U, 0x0805FFFFU}, {0x08060000U, 0x0807FFFFU}, {0x08080000U, 0x0809FFFFU},
        {0x080A0000U, 0x080BFFFFU}, {0x080C0000U, 0x080DFFFFU}, {0x080E0000U, 0x080FFFFFU}};
    constexpr uint32_t sectorCount = sizeof(sectorAddresses) / sizeof(sectorAddresses[0]);
    constexpr uint32_t sectorNotFound = 0xFFFFFFFFU;
}

FlashManager::RetStatus FlashManager::ToggleFlashLock(bool lock)
{
    auto operation = lock ? HAL_FLASH_Lock : HAL_FLASH_Unlock;
    return (operation() == HAL_OK) ? RetStatus::eOk : RetStatus::eNotOk;
}

constexpr FlashManager::sectorRange FlashManager::GetSectorRange(uint32_t startAddress, uint32_t endAddress)
{
    sectorRange range = {FlashConstants::sectorNotFound, 0U};

    for (uint32_t i = 0U; i < FlashConstants::sectorCount; ++i)
    {
        bool isStartInRange = (startAddress >= FlashConstants::sectorAddresses[i][0]) &&
                              (startAddress <= FlashConstants::sectorAddresses[i][1]);
        bool isEndInRange = (endAddress >= FlashConstants::sectorAddresses[i][0]) &&
                            (endAddress <= FlashConstants::sectorAddresses[i][1]);

        if (isStartInRange)
        {
            range.startSector = i;
        }

        if (isEndInRange && (range.startSector != FlashConstants::sectorNotFound))
        {
            range.sectorCount = i - range.startSector + 1U;
            break;
        }
    }

    return range;
}

FlashManager::RetStatus FlashManager::Erase(uint32_t startAddress, uint32_t endAddress)
{
    auto range = GetSectorRange(startAddress, endAddress);
    if ((range.startSector == FlashConstants::sectorNotFound) || (range.sectorCount == 0U))
    {
        return RetStatus::einvalidSector;
    }

    FLASH_EraseInitTypeDef eraseData = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Banks = FLASH_BANK_1,
        .Sector = range.startSector,
        .NbSectors = range.sectorCount,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3};

    if (ToggleFlashLock(false) != RetStatus::eOk)
    {
        return RetStatus::eNotOk;
    }

    uint32_t sectorError = FlashConstants::sectorNotFound;
    auto status = HAL_FLASHEx_Erase(&eraseData, &sectorError);
    ToggleFlashLock(true);

    return (status == HAL_OK) ? RetStatus::eOk : RetStatus::eNotOk;
}

FlashManager::RetStatus FlashManager::Write(uint32_t startAddress, const void *data, size_t size)
{
    const uint8_t *pData = static_cast<const uint8_t *>(data);
    uint32_t currentAddress = startAddress;

    if (ToggleFlashLock(false) != RetStatus::eOk)
    {
        return RetStatus::eNotOk;
    }

    while (size > 0)
    {
        uint8_t programType;
        uint64_t programData;
        size_t increment;

        if (size >= 4)
        {
            programType = FLASH_TYPEPROGRAM_WORD;
            programData = *reinterpret_cast<const uint32_t *>(pData);
            increment = 4;
        }
        else if (size >= 2)
        {
            programType = FLASH_TYPEPROGRAM_HALFWORD;
            programData = *reinterpret_cast<const uint16_t *>(pData);
            increment = 2;
        }
        else
        {
            programType = FLASH_TYPEPROGRAM_BYTE;
            programData = *pData;
            increment = 1;
        }

        if (HAL_FLASH_Program(programType, currentAddress, programData) != HAL_OK)
        {
            ToggleFlashLock(true);
            return RetStatus::eNotOk;
        }

        currentAddress += increment;
        pData += increment;
        size -= increment;
    }

    ToggleFlashLock(true);
    return RetStatus::eOk;
}

FlashManager::RetStatus FlashManager::Read(uint32_t startAddress, void *buffer, size_t size)
{
    std::memcpy(buffer, reinterpret_cast<const void *>(startAddress), size);

    return RetStatus::eOk;
}

FlashManager::RetStatus FlashManager::Unlock()
{
    return ToggleFlashLock(false);
}

FlashManager::RetStatus FlashManager::Lock()
{
    return ToggleFlashLock(true);
}
