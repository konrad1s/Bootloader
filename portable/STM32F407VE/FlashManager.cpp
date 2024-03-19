#include "FlashManager.h"
#include "stm32f4xx_hal.h"

FlashManager::state FlashManager::Erase()
{
    return state::eOk;
}

FlashManager::state FlashManager::Write(uint32_t start_addr, const void *data, size_t size)
{
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
