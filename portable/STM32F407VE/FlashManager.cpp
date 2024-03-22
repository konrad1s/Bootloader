#include "FlashManager.h"
#include "stm32f4xx_hal.h"

FlashManager::state FlashManager::Erase(uint32_t startAddress, uint32_t endAddress)
{
    if ((startAddress % FLASH_PAGE_SIZE) != 0U || (endAddress % FLASH_PAGE_SIZE) != 0U)
    {
        return state::eNotOk;
    }

    const uint32_t sectorsNumber = (endAddress - startAddress) / FLASH_PAGE_SIZE;
    const uint32_t sectorStart = startAddress / FLASH_PAGE_SIZE;

    uint32_t sectorError = 0xFFFFFFFFU;
    FLASH_EraseInitTypeDef eraseData{
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Banks = FLASH_BANK_1,
        .Sector = sectorStart,
        .NbSectors = sectorsNumber,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3};

    Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&eraseData, &sectorError);
    Lock();

    return (status == HAL_OK) ? state::eOk : state::eNotOk;
}

FlashManager::state FlashManager::Write(uint32_t startAddress, const void *data, size_t size)
{
    uint8_t programType;
    uint64_t programData;
    uint32_t currentAddress = startAddress;
    uint32_t nextAddress = startAddress;
    const uint8_t *pData = static_cast<const uint8_t *>(data);

    Unlock();
    while (size > 0U)
    {
        /* Check the program type from the highest probability to lowest */
        if (size >= 8U)
        {
            programType = FLASH_TYPEPROGRAM_DOUBLEWORD;
            programData = *reinterpret_cast<const uint64_t *>(pData);
            nextAddress += 8U;
            pData += 8U;
            size -= 8U;
        }
        else if (size >= 4U)
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

        if (HAL_OK != HAL_FLASH_Program(programType, currentAddress, programData))
        {
            Lock();
            return state::eNotOk;
        }

        currentAddress = nextAddress;
    }
    Lock();

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
