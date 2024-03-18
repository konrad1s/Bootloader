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

    enum class retStatus
    {
        eOk,
        eNotOk
    };

    Bootloader(beecom::BeeCOM &beecom, IFlashManager &flashManager) : beecom_(beecom), flashManager_(flashManager)
    {
        setupPacketHandler();
    }

    beecom::BeeCOM &beecom_;

private:
    IFlashManager &flashManager_;
    BootState state;

    void setupPacketHandler();
    void handleInvalidPacket();
    void handleValidPacket(const beecom::Packet &packet);
    void sendResponse(bool success);

    void getFirmwareVersion();

    void boot();
    retStatus extractAddressAndData(const uint8_t *payload, size_t length, uint32_t *address, const uint8_t **dataStart);
};
