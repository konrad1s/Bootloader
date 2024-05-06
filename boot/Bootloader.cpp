#include <cstring>
#include <algorithm>
#include "Bootloader.h"
#include "BootConfig.h"

constexpr uint32_t applicationValidFlag = 0x5A5A5A5AU;

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
    beecom_.setObserver(&packetProcessor);
}

void Bootloader::HandleValidPacket(const beecom::Packet &packet)
{
    size_t index = static_cast<size_t>(packet.header.type);

    BootState targetState = DetermineTargetState(static_cast<packetType>(packet.header.type));

    if (TransitionState(targetState))
    {
        auto handler = packetHandlers[index];
        auto status = (this->*handler)(packet);

        if (status == RetStatus::eNotOk)
        {
            TransitionState(BootState::error);
        }
    }
    else
    {
        SendNackResponse(static_cast<packetType>(packet.header.type));
    }
}

void Bootloader::SendResponse(packetType type, const uint8_t *data, size_t dataSize)
{
    beecom_.send(static_cast<uint8_t>(type), data, dataSize);
}

void Bootloader::SendNackResponse(packetType type)
{
    const uint8_t nackValue = 0xAAU;

    SendResponse(type, &nackValue, sizeof(nackValue));
}

void Bootloader::SendAckResponse(packetType type)
{
    const uint8_t ackValue = 0x55U;

    SendResponse(type, &ackValue, sizeof(ackValue));
}

uint32_t Bootloader::ExtractAddress(const beecom::Packet &packet)
{
    uint32_t address = (static_cast<uint32_t>(packet.payload[0]) << 24U) |
                       (static_cast<uint32_t>(packet.payload[1]) << 16U) |
                       (static_cast<uint32_t>(packet.payload[2]) << 8U) |
                       static_cast<uint32_t>(packet.payload[3]);

    return address;
}

inline bool Bootloader::IsPresentFlagSet()
{
    return FlashMapping::GetMetaData()->appPresentFlag == applicationValidFlag;
}

Bootloader::RetStatus Bootloader::HandleFlashData(const beecom::Packet &packet)
{
    uint32_t startAddress = ExtractAddress(packet);
    size_t dataSize = packet.header.length - sizeof(uint32_t);
    const uint8_t *dataStart = packet.payload + sizeof(uint32_t);

    auto fStatus = flashManager_.Write(startAddress, dataStart, dataSize);

    if (fStatus == IFlashManager::RetStatus::eOk)
    {
        SendAckResponse(static_cast<packetType>(packet.header.type));
        return RetStatus::eOk;
    }
    else
    {
        SendNackResponse(static_cast<packetType>(packet.header.type));
        return RetStatus::eNotOk;
    }
}

Bootloader::RetStatus Bootloader::HandleFlashStart(const beecom::Packet &packet)
{
    auto fStatus = flashManager_.Erase(FlashMapping::appMinStartAddress, FlashMapping::appMaxEndAddress);

    if (fStatus == IFlashManager::RetStatus::eOk)
    {
        SendAckResponse(static_cast<packetType>(packet.header.type));
        return RetStatus::eOk;
    }
    else
    {
        SendNackResponse(static_cast<packetType>(packet.header.type));
        return RetStatus::eNotOk;
    }
}

Bootloader::RetStatus Bootloader::HandleValidateSignature(const beecom::Packet &packet)
{
    flashManager_.Write(FlashMapping::appSignatureSizeAddress, packet.payload, packet.header.length);
    bool valid = ValidateFirmware();

    if (valid)
    {
        /* Aplication valid, set the flag */
        auto fStatus = flashManager_.Write(FlashMapping::appValidFlagAddress, &applicationValidFlag, sizeof(applicationValidFlag));
        if (IFlashManager::RetStatus::eOk == fStatus)
        {
            TransitionState(BootState::booting);
            SendAckResponse(static_cast<packetType>(packet.header.type));
            return RetStatus::eOk;
        }
        else
        {
            SendNackResponse(static_cast<packetType>(packet.header.type));
            return RetStatus::eNotOk;
        }
    }
    else
    {
        SendNackResponse(static_cast<packetType>(packet.header.type));
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
        flashManager_.Read(FlashMapping::appSignatureAddress, dataBuffer, FlashMapping::GetMetaData()->signatureSize);
        dataSize = FlashMapping::GetMetaData()->signatureSize;
        break;
    case packetType::getBootloaderVersion:
        dataSize = sizeof(BootConfig::bootloaderVersion);
        std::memcpy(dataBuffer, BootConfig::bootloaderVersion, dataSize);
        break;
    default:
        status = Bootloader::RetStatus::eNotOk;
        break;
    }

    if (status == Bootloader::RetStatus::okNoResponse)
    {
        SendResponse(type, dataBuffer, dataSize);
    }

    return status;
}

bool Bootloader::ValidateFirmware()
{
    SecureBoot secureBoot;
    SecureBoot::RetStatus sStatus = secureBoot.ValidateFirmware(
        reinterpret_cast<const unsigned char *>(FlashMapping::appSignatureAddress),
        FlashMapping::GetMetaData()->signatureSize,
        reinterpret_cast<const unsigned char *>(FlashMapping::GetMetaData()->appStartAddress),
        FlashMapping::GetAppSize());

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
    case packetType::validateFlash:
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
                bool presentFlagSet = IsPresentFlagSet();
                bool firmwareValid = true;
#if (VALIDATE_APP_BEFORE_BOOT == 1)
                firmwareValid = ValidateFirmware();
#endif
                if (presentFlagSet && firmwareValid)
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
