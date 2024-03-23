#pragma once

#include "BeeCom.h"
#include "IFlashManager.h"
#include "AppJumper.h"

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

    enum class packetType
    {
        invalidPacket,
        flashStart,
        flashData,
        flashMac,
        validateFlash
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
    BootState state{BootState::IDLE};
    AppJumper appJumper;

    void setupPacketHandler();
    void handleValidPacket(const beecom::Packet &packet);
    void sendResponse(bool success, packetType type);

    void getFirmwareVersion();

    void boot();
    retStatus extractAddress(const uint8_t *payload, uint32_t *address);
};
