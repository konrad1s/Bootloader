#pragma once

#include "BeeCom.h"
#include "IFlashManager.h"

class Bootloader
{
public:
    enum class BootState
    {
        IDLE,
        RECEIVING_DATA,
        FLASHING,
        VERIFIYING,
        UPDATE_COMPLETED
    };

    Bootloader(beecom::BeeCOM &beecom, IFlashManager &flashManager) : beecom_(beecom), flashManager_(flashManager)
    {
        setupPacketHandler();
    }

private:
    beecom::BeeCOM &beecom_;
    IFlashManager &flashManager_;
    BootState state;

    void setupPacketHandler();
    void handleInvalidPacket();
    void handleValidPacket();

    void getFirmwareVersion();
};
