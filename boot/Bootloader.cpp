#include <cstring>
#include <algorithm>
#include "Bootloader.h"
#include "BootConfig.h"

Bootloader::Bootloader(beecom::BeeCOM &beecom, IFlashManager &flashManager)
    : beecom_(beecom), flashManager_(flashManager)
{
    packetHandlers = {
        nullptr,
        &Bootloader::HandleFlashStart,
        &Bootloader::HandleFlashData,
        nullptr,
        &Bootloader::HandleValidateSignature,
        &Bootloader::HandleReadDataRequest,
        &Bootloader::HandleReadDataRequest};
    SetupPacketHandler();
}

void Bootloader::SetupPacketHandler()
{
    auto packetHandler = [this](const beecom::Packet &packet, bool crcValid, beecom::SendFunction send)
    {
        if (crcValid)
        {
            HandleValidPacket(packet);
        }
        else
        {
            SendResponse(false, packetType::invalidPacket);
        }
    };

    beecom_.setPacketHandler(packetHandler);
}

void Bootloader::HandleValidPacket(const beecom::Packet &packet)
{
    size_t index = static_cast<size_t>(packet.header.type);

    if ((index < packetHandlers.size()) && (packetHandlers[index] != nullptr))
    {
        BootState targetState = DetermineTargetState(static_cast<packetType>(packet.header.type));

        if (TransitionState(targetState))
        {
            auto handler = packetHandlers[index];
            auto status = (this->*handler)(packet);

            if (status == RetStatus::eNotOk)
            {
                TransitionState(BootState::error);
                SendResponse(false, static_cast<packetType>(packet.header.type));
            }
            else if (status == RetStatus::eOk)
            {
                SendResponse(true, static_cast<packetType>(packet.header.type));
            }
        }
        else
        {
            SendResponse(false, static_cast<packetType>(packet.header.type));
        }
    }
    else
    {
        SendResponse(false, static_cast<packetType>(packet.header.type));
    }
}

void Bootloader::SendResponse(bool success, packetType type, const uint8_t *data, size_t dataSize)
{
    const uint8_t ackValue = 0x55U;
    const uint8_t notAckValue = 0xAAU;

    beecom::Packet responsePacket;
    responsePacket.header.sop = 0xA5U;
    responsePacket.header.type = static_cast<uint8_t>(type);
    responsePacket.header.length = 1U + dataSize;
    responsePacket.payload[0] = success ? ackValue : notAckValue;

    if (data != nullptr && dataSize > 0)
    {
        std::memcpy(responsePacket.payload + 1, data, dataSize);
    }

    beecom_.send(responsePacket);
}

uint32_t Bootloader::ExtractAddress(const beecom::Packet &packet)
{
    uint32_t address = (static_cast<uint32_t>(packet.payload[0]) << 24U) |
                       (static_cast<uint32_t>(packet.payload[1]) << 16U) |
                       (static_cast<uint32_t>(packet.payload[2]) << 8U) |
                       static_cast<uint32_t>(packet.payload[3]);

    return address;
}

Bootloader::RetStatus Bootloader::HandleFlashData(const beecom::Packet &packet)
{
    uint32_t startAddress = ExtractAddress(packet);
    size_t dataSize = packet.header.length - sizeof(uint32_t);
    const uint8_t *dataStart = packet.payload + sizeof(uint32_t);

    auto fStatus = flashManager_.Write(startAddress, dataStart, dataSize);

    return (fStatus == IFlashManager::RetStatus::eOk) ? RetStatus::eOk : RetStatus::eNotOk;
}

Bootloader::RetStatus Bootloader::HandleFlashStart(const beecom::Packet &packet)
{
    auto fStatus = flashManager_.Erase(FlashMapping::appStartAddress, FlashMapping::appEndAddress);

    return (fStatus == IFlashManager::RetStatus::eOk) ? RetStatus::eOk : RetStatus::eNotOk;
}

Bootloader::RetStatus Bootloader::HandleValidateSignature(const beecom::Packet &packet)
{
    flashManager_.Write(FlashMapping::appSignatureSizeAddress, packet.payload, packet.header.length);
    bool valid = ValidateFirmware();

    if (valid)
    {
        TransitionState(BootState::booting);
        return RetStatus::eOk;
    }
    else
    {
        return RetStatus::eNotOk;
    }
}

Bootloader::RetStatus Bootloader::HandleReadDataRequest(const beecom::Packet &packet)
{
    uint8_t dataBuffer[FlashMapping::maxDataSize];
    size_t dataSize = 0;
    packetType type = static_cast<packetType>(packet.header.type);
    Bootloader::RetStatus status = Bootloader::RetStatus::okNoResponse;

    switch (type)
    {
    case packetType::getAppSignature:
        flashManager_.Read(FlashMapping::appSignatureAddress, dataBuffer, FlashMapping::GetAppSignatureSize());
        dataSize = FlashMapping::GetAppSignatureSize();
        break;
    case packetType::getBootloaderVersion:
        dataSize = sizeof(BOOTLOADER_VERSION);
        std::memcpy(dataBuffer, BOOTLOADER_VERSION, dataSize);
        break;
    default:
        status = Bootloader::RetStatus::eNotOk;
        break;
    }

    if (status == Bootloader::RetStatus::okNoResponse)
    {
        SendResponse(true, type, dataBuffer, dataSize);
    }

    return status;
}

bool Bootloader::ValidateFirmware()
{
    SecureBoot secureBoot;
    SecureBoot::RetStatus sStatus = secureBoot.ValidateFirmware(
        reinterpret_cast<const unsigned char *>(FlashMapping::appSignatureAddress),
        FlashMapping::GetAppSignatureSize(),
        reinterpret_cast<const unsigned char *>(FlashMapping::appStartAddress),
        FlashMapping::appSize);

    return sStatus == SecureBoot::RetStatus::valid;
}

Bootloader::BootState Bootloader::DetermineTargetState(packetType type)
{
    switch (type)
    {
    case packetType::flashStart:
        return BootState::erasing;
    case packetType::flashData:
        return BootState::flashing;
    case packetType::flashMac:
        return BootState::verifying;
    default:
        return state;
    }
}

bool Bootloader::TransitionState(BootState newState)
{
    constexpr bool validTransitions[static_cast<int>(BootState::numStates)][static_cast<int>(BootState::numStates)] = {
                      /* idle, booting, erasing, flashing, verifying, error */
        /* idle */      {true,  true,   true,    false,     false,    true},
        /* booting */   {false, true,   true,    false,     false,    true},
        /* erasing */   {false, false,  true,    true,      false,    true},
        /* flashing */  {false, false,  false,   true,      true,     true},
        /* verifying */ {true,  true,   false,   false,      true,    false},
        /* error */     {false, false,  true,    false,     false,    true}
    };

    int currIndex = static_cast<int>(state);
    int newIndex = static_cast<int>(newState);

    if (validTransitions[currIndex][newIndex])
    {
        state = newState;
        return true;
    }
    else
    {
        state = BootState::error;
        return false;
    }
}

void Bootloader::Boot()
{
    uint32_t startTime = HAL_GetTick();
    uint32_t bootWaitTime = BootConfig::waitForBootActionMs;

    while (true)
    {
        if (beecom_.receive() > 0U)
        {
            startTime = HAL_GetTick();
            bootWaitTime = BootConfig::actionBootExtensionMs;
        }

        if ((HAL_GetTick() - startTime > bootWaitTime) || (state == BootState::booting))
        {
            if (TransitionState(BootState::booting))
            {
                if (ValidateFirmware())
                {
                    appJumper.JumpToApplication();
                    return;
                }
                else
                {
                    TransitionState(BootState::error);
                }
            }
        }
    }
}
