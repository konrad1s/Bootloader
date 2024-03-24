#pragma once

#include "BeeCom.h"
#include "IFlashManager.h"
#include "AppJumper.h"
#include "SecureBoot.h"

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
        validateFlash,

        getBootloaderVersion,
        getAppVersion,
        getAppSignature,
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
    constexpr static uint8_t waitForBootActionMs = 50U;

    IFlashManager &flashManager_;
    BootState state{BootState::IDLE};
    AppJumper appJumper;
    SecureBoot secureBoot;

    void setupPacketHandler();
    void handleValidPacket(const beecom::Packet &packet);
    void handleReadDataRequest(packetType type);
    void sendResponse(bool success, packetType type, const uint8_t *data = nullptr, size_t dataSize = 0);

    void getFirmwareVersion();

    void boot();
    retStatus extractAddress(const uint8_t *payload, uint32_t *address);
};
