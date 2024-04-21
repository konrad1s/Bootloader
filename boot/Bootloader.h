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

    enum class RetStatus
    {
        eOk,
        eNotOk,
        okNoResponse
    };

    using HandlerFunction = RetStatus (Bootloader::*)(const beecom::Packet &);

    Bootloader(beecom::BeeCOM &beecom, IFlashManager &flashManager);

    void Boot();

private:
    beecom::BeeCOM &beecom_;
    IFlashManager &flashManager_;
    AppJumper appJumper;
    BootState state{BootState::idle};
    std::array<HandlerFunction, static_cast<size_t>(packetType::numberOfPacketTypes)> packetHandlers;

    void SetupPacketHandler();

    void HandleValidPacket(const beecom::Packet &packet);
    BootState DetermineTargetState(packetType type);
    bool TransitionState(BootState newState);
    void SendResponse(bool success, packetType type, const uint8_t *data = nullptr, size_t dataSize = 0);
    uint32_t ExtractAddress(const beecom::Packet &packet);

    bool ValidateFirmware();
    RetStatus HandleFlashData(const beecom::Packet &packet);
    RetStatus HandleFlashStart(const beecom::Packet &packet);
    RetStatus HandleValidateSignature(const beecom::Packet &packet);
    RetStatus HandleReadDataRequest(const beecom::Packet &packet);
};
