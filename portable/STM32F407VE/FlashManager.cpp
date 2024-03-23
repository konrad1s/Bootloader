#include "FlashManager.h"
#include "stm32f4xx_hal.h"

FlashManager::state FlashManager::Erase(const uint32_t startAddress, const uint32_t endAddress)
{
    auto range = GetSectorRange(startAddress, endAddress);
    if (range.startSector == 0xFFFFFFFFU || range.sectorCount == 0U)
    {
        return state::eNotOk;
    }

    uint32_t sectorError = 0xFFFFFFFFU;
    FLASH_EraseInitTypeDef eraseData{
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Banks = FLASH_BANK_1,
        .Sector = range.startSector,
        .NbSectors = range.sectorCount,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3};

    if (state::eOk != Unlock())
    {
        return state::eNotOk;
    }
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseData, &sectorError);
    (void)Lock();

    return (status == HAL_OK) ? state::eOk : state::eNotOk;
}

FlashManager::state FlashManager::Write(uint32_t startAddress, const void *data, size_t size)
{
    uint8_t programType;
    uint64_t programData;
    uint32_t currentAddress = startAddress;
    uint32_t nextAddress = startAddress;
    const uint8_t *pData = static_cast<const uint8_t *>(data);

    while (size > 0U)
    {
        /* Check the program type from the highest probability to lowest */
        if (size >= 4U)
        {
            programType = FLASH_TYPEPROGRAM_WORD;
            programData = *reinterpret_cast<const uint32_t *>(pData);
            nextAddress += 4U;
            pData += 4U;
            size -= 4U;
        }
        else if (size >= 2U)
        {
            programType = FLASH_TYPEPROGRAM_HALFWORD;
            programData = *reinterpret_cast<const uint16_t *>(pData);
            nextAddress += 2U;
            pData += 2U;
            size -= 2U;
        }
        else
        {
            programType = FLASH_TYPEPROGRAM_BYTE;
            programData = *pData;
            nextAddress += 1U;
            pData += 1U;
            size -= 1U;
        }

        if (state::eOk != Unlock())
        {
            return state::eNotOk;
        }
        if (HAL_OK != HAL_FLASH_Program(programType, currentAddress, programData))
        {
            (void)Lock();
            return state::eNotOk;
        }

        (void)Lock();
        currentAddress = nextAddress;
    }

    return state::eOk;
}

FlashManager::state FlashManager::Read()
{
    return state::eOk;
}

FlashManager::state FlashManager::Unlock()
{
    return (HAL_FLASH_Unlock() == HAL_OK) ? state::eOk : state::eNotOk;
}

FlashManager::state FlashManager::Lock()
{
    return (HAL_FLASH_Lock() == HAL_OK) ? state::eOk : state::eNotOk;
}

constexpr FlashManager::sectorRange FlashManager::GetSectorRange(uint32_t startAddress, uint32_t endAddress)
{
    constexpr uint32_t sectorsAddresses[][2] = {
        {0x08000000U, 0x08003FFFU},
        {0x08004000U, 0x08007FFFU},
        {0x08008000U, 0x0800BFFFU},
        {0x0800C000U, 0x0800FFFFU},
        {0x08010000U, 0x0801FFFFU},
        {0x08020000U, 0x0803FFFFU},
        {0x08040000U, 0x0805FFFFU},
        {0x08060000U, 0x0807FFFFU},
        {0x08080000U, 0x0809FFFFU},
        {0x080A0000U, 0x080BFFFFU},
        {0x080C0000U, 0x080DFFFFU},
        {0x080E0000U, 0x080FFFFFU}};
    constexpr uint32_t sectorsCount = sizeof(sectorsAddresses) / sizeof(sectorsAddresses[0]);

    sectorRange range = {0xFFFFFFFFU, 0U};
    for (uint32_t i = 0U; i < sectorsCount; ++i)
    {
        if ((startAddress >= sectorsAddresses[i][0]) && (startAddress <= sectorsAddresses[i][1]))
        {
            range.startSector = i;
        }
        if ((endAddress >= sectorsAddresses[i][0]) && (endAddress <= sectorsAddresses[i][1]))
        {
            if (range.startSector != 0xFFFFFFFFU)
            {
                range.sectorCount = i - range.startSector + 1U;
            }
            break;
        }
    }

    return range;
}
