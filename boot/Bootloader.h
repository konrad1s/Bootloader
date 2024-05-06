#pragma once

#include <array>
#include "BeeCom.h"
#include "IFlashManager.h"
#include "AppJumper.h"
#include "SecureBoot.h"

class Bootloader : beecom::IPacketObserver
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

    void onPacketReceived(const beecom::Packet &packet, bool crcValid, void *beeComInstance);
    void Boot();

private:
    beecom::BeeCOM &beecom_;
    IFlashManager &flashManager_;
    AppJumper appJumper;
    BootState state{BootState::idle};
    std::array<HandlerFunction, static_cast<size_t>(packetType::numberOfPacketTypes)> packetHandlers;

    bool TransitionState(BootState newState);
    BootState DetermineTargetState(packetType type);

    void SendResponse(packetType type, const uint8_t *data, size_t dataSize);
    void SendAckResponse(packetType type);
    void SendNackResponse(packetType type);

    bool IsPresentFlagSet();
    bool ValidateFirmware();

    void PacketHandlerFunction(const beecom::Packet &packet, bool crcValid, beecom::SendFunction send);
    void SetupPacketHandler();
    void HandleValidPacket(const beecom::Packet &packet);
    uint32_t ExtractAddress(const beecom::Packet &packet);
    RetStatus HandleFlashData(const beecom::Packet &packet);
    RetStatus HandleFlashStart(const beecom::Packet &packet);
    RetStatus HandleValidateSignature(const beecom::Packet &packet);
    RetStatus HandleReadDataRequest(const beecom::Packet &packet);
};
