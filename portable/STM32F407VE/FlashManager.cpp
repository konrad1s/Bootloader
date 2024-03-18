#include "FlashManager.h"

FlashManager::state FlashManager::FlashErase()
{
    return state::eOk;
}

FlashManager::state FlashManager::FlashWrite(uint32_t start_addr, const void *data, size_t size)
{
    return state::eOk;
}

FlashManager::state FlashManager::FlashRead()
{
    return state::eOk;
}

FlashManager::state FlashManager::FlashUnlock()
{
    return state::eOk;
}

FlashManager::state FlashManager::FlashLock()
{
    return state::eOk;
}
