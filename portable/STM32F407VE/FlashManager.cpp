#include "FlashManager.h"

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
    return state::eOk;
}

FlashManager::state FlashManager::Lock()
{
    return state::eOk;
}
