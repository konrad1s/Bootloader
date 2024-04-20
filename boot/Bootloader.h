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
        idle,
        booting,

        erasing,
        flashing,
        verifying,

        error,
        numStates
    };

    enum class packetType
    {
        invalidPacket,

        flashStart,
        flashData,
        flashMac,
        validateFlash,

        getBootloaderVersion,
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

    void boot();

private:
    BootState state{BootState::idle};

    beecom::BeeCOM &beecom_;
    IFlashManager &flashManager_;
    AppJumper appJumper;

    using handlerFunction = retStatus (Bootloader::*)(const beecom::Packet &);
    static constexpr size_t numberOfPacketTypes = static_cast<size_t>(packetType::numberOfPacketTypes);
    handlerFunction packetHandlers[numberOfPacketTypes];

    void setupPacketHandler();
    BootState determineTargetState(packetType type);
    void handleValidPacket(const beecom::Packet &packet);
    bool transitionState(BootState newState);
    void sendResponse(bool success, packetType type, const uint8_t *data = nullptr, size_t dataSize = 0);
    retStatus extractAddress(const uint8_t *payload, uint32_t *address);
    bool validateFirmware();

    void initializeLookupTable();
    void handleReadDataRequest(packetType type);
    void getFirmwareVersion();
    retStatus handleFlashData(const beecom::Packet &packet);
    retStatus handleFlashStart(const beecom::Packet &packet);
    retStatus handleValidateSignature(const beecom::Packet &packet);
};
