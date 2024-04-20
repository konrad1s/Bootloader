#include <cstring>
#include <algorithm>
#include "Bootloader.h"
#include "BootConfig.h"

Bootloader::Bootloader(beecom::BeeCOM &beecom, IFlashManager &flashManager)
    : beecom_(beecom), flashManager_(flashManager)
{
    packetHandlers = {
        nullptr,
        &Bootloader::handleFlashStart,
        &Bootloader::handleFlashData,
        nullptr,
        &Bootloader::handleValidateSignature,
        &Bootloader::handleReadDataRequest,
        &Bootloader::handleReadDataRequest};
    setupPacketHandler();
}

void Bootloader::setupPacketHandler()
{
    auto packetHandler = [this](const beecom::Packet &packet, bool crcValid, beecom::SendFunction send)
    {
        if (crcValid)
        {
            handleValidPacket(packet);
        }
        else
        {
            sendResponse(false, packetType::invalidPacket);
        }
    };

    beecom_.setPacketHandler(packetHandler);
}

void Bootloader::handleValidPacket(const beecom::Packet &packet)
{
    size_t index = static_cast<size_t>(packet.header.type);

    if ((index < packetHandlers.size()) && (packetHandlers[index] != nullptr))
    {
        BootState targetState = determineTargetState(static_cast<packetType>(packet.header.type));

        if (transitionState(targetState))
        {
            auto handler = packetHandlers[index];
            auto status = (this->*handler)(packet);

            if (status == retStatus::eNotOk)
            {
                transitionState(BootState::error);
                sendResponse(false, static_cast<packetType>(packet.header.type));
            }
            else if (status == retStatus::eOk)
            {
                sendResponse(true, static_cast<packetType>(packet.header.type));
            }
        }
        else
        {
            sendResponse(false, static_cast<packetType>(packet.header.type));
        }
    }
    else
    {
        sendResponse(false, static_cast<packetType>(packet.header.type));
    }
}

void Bootloader::sendResponse(bool success, packetType type, const uint8_t *data, size_t dataSize)
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

uint32_t Bootloader::extractAddress(const beecom::Packet &packet)
{
    uint32_t address = (static_cast<uint32_t>(packet.payload[0]) << 24U) |
                       (static_cast<uint32_t>(packet.payload[1]) << 16U) |
                       (static_cast<uint32_t>(packet.payload[2]) << 8U) |
                       static_cast<uint32_t>(packet.payload[3]);

    return address;
}

Bootloader::retStatus Bootloader::handleFlashData(const beecom::Packet &packet)
{
    uint32_t startAddress = extractAddress(packet);
    size_t dataSize = packet.header.length - sizeof(uint32_t);
    const uint8_t *dataStart = packet.payload + sizeof(uint32_t);

    auto fStatus = flashManager_.Write(startAddress, dataStart, dataSize);

    return (fStatus == IFlashManager::state::eOk) ? retStatus::eOk : retStatus::eNotOk;
}

Bootloader::retStatus Bootloader::handleFlashStart(const beecom::Packet &packet)
{
    auto fStatus = flashManager_.Erase(FlashMapping::appStartAddress, FlashMapping::appEndAddress);

    return (fStatus == IFlashManager::state::eOk) ? retStatus::eOk : retStatus::eNotOk;
}

Bootloader::retStatus Bootloader::handleValidateSignature(const beecom::Packet &packet)
{
    flashManager_.Write(FlashMapping::appSignatureSizeAddress, packet.payload, packet.header.length);
    bool valid = validateFirmware();

    if (valid)
    {
        transitionState(BootState::booting);
        return retStatus::eOk;
    }
    else
    {
        return retStatus::eNotOk;
    }
}

Bootloader::retStatus Bootloader::handleReadDataRequest(const beecom::Packet &packet)
{
    uint8_t dataBuffer[FlashMapping::maxDataSize];
    size_t dataSize = 0;
    packetType type = static_cast<packetType>(packet.header.type);
    Bootloader::retStatus status = Bootloader::retStatus::okNoResponse;

    switch (type)
    {
    case packetType::getAppSignature:
        flashManager_.Read(FlashMapping::appSignatureAddress, dataBuffer, FlashMapping::getAppSignatureSize());
        dataSize = FlashMapping::getAppSignatureSize();
        break;
    case packetType::getBootloaderVersion:
        dataSize = sizeof(BOOTLOADER_VERSION);
        std::memcpy(dataBuffer, BOOTLOADER_VERSION, dataSize);
        break;
    default:
        status = Bootloader::retStatus::eNotOk;
        break;
    }

    if (status == Bootloader::retStatus::okNoResponse)
    {
        sendResponse(true, type, dataBuffer, dataSize);
    }

    return status;
}

bool Bootloader::validateFirmware()
{
    SecureBoot secureBoot;
    SecureBoot::retStatus sStatus = secureBoot.validateFirmware(
        reinterpret_cast<const unsigned char *>(FlashMapping::appSignatureAddress),
        FlashMapping::getAppSignatureSize(),
        reinterpret_cast<const unsigned char *>(FlashMapping::appStartAddress),
        FlashMapping::appSize);

    return sStatus == SecureBoot::retStatus::valid;
}

Bootloader::BootState Bootloader::determineTargetState(packetType type)
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

bool Bootloader::transitionState(BootState newState)
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

void Bootloader::boot()
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
            if (transitionState(BootState::booting))
            {
                if (validateFirmware())
                {
                    appJumper.jumpToApplication();
                    return;
                }
                else
                {
                    transitionState(BootState::error);
                }
            }
        }
    }
}
