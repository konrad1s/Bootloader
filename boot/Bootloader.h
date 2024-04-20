#pragma once

#include <array>
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
        eNotOk,
        okNoResponse
    };

    using HandlerFunction = retStatus (Bootloader::*)(const beecom::Packet &);

    Bootloader(beecom::BeeCOM &beecom, IFlashManager &flashManager);

    void boot();

private:
    beecom::BeeCOM &beecom_;
    IFlashManager &flashManager_;
    AppJumper appJumper;
    BootState state{BootState::idle};
    std::array<HandlerFunction, static_cast<size_t>(packetType::numberOfPacketTypes)> packetHandlers;

    void setupPacketHandler();

   void handleValidPacket(const beecom::Packet& packet);
    BootState determineTargetState(packetType type);
    bool transitionState(BootState newState);
    void sendResponse(bool success, packetType type, const uint8_t* data = nullptr, size_t dataSize = 0);
    uint32_t extractAddress(const beecom::Packet& packet);

    bool validateFirmware();
    retStatus handleFlashData(const beecom::Packet& packet);
    retStatus handleFlashStart(const beecom::Packet& packet);
    retStatus handleValidateSignature(const beecom::Packet& packet);
    retStatus handleReadDataRequest(const beecom::Packet &packet);
};
