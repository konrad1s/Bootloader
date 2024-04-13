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

        numberOfPacketTypes
    };

    enum class retStatus
    {
        eOk,
        eNotOk
    };

    Bootloader(beecom::BeeCOM &beecom, IFlashManager &flashManager) : beecom_(beecom), flashManager_(flashManager)
    {
        setupPacketHandler();
        initializeLookupTable();
    }

    beecom::BeeCOM &beecom_;

private:
    constexpr static uint8_t waitForBootActionMs = 50U;
    size_t bytesFlashed{0};
    IFlashManager &flashManager_;
    AppJumper appJumper;
    SecureBoot secureBoot;

    using handlerFunction = retStatus (Bootloader::*)(const beecom::Packet &);
    static constexpr size_t numberOfPacketTypes = static_cast<size_t>(packetType::numberOfPacketTypes);
    handlerFunction packetHandlers[numberOfPacketTypes];

    void setupPacketHandler();
    void handleValidPacket(const beecom::Packet &packet);
    void handleReadDataRequest(packetType type);
    void sendResponse(bool success, packetType type, const uint8_t *data = nullptr, size_t dataSize = 0);

    void getFirmwareVersion();

    void boot();
    retStatus extractAddress(const uint8_t *payload, uint32_t *address);

    void initializeLookupTable();
    retStatus handleFlashData(const beecom::Packet &packet);
    retStatus handleFlashStart(const beecom::Packet &packet);
    retStatus handleValidateSignature(const beecom::Packet &packet);
};
