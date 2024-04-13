#include "Bootloader.h"
#include "BootConfig.h"

Bootloader::retStatus Bootloader::extractAddress(const uint8_t *payload, uint32_t *address)
{
    *address = (static_cast<uint32_t>(payload[0]) << 24U) |
               (static_cast<uint32_t>(payload[1]) << 16U) |
               (static_cast<uint32_t>(payload[2]) << 8U) |
               static_cast<uint32_t>(payload[3]);

    return retStatus::eOk;
}

void Bootloader::setupPacketHandler()
{
    beecom::PacketHandler packetHandler = [this](const beecom::Packet &packet,
                                                 bool crcValid,
                                                 beecom::SendFunction send)
    {
        if (crcValid)
            handleValidPacket(packet);
        else
        {
            sendResponse(false, packetType::invalidPacket);
        }
    };

    beecom_.setPacketHandler(packetHandler);
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

bool Bootloader::validateFirmware()
{
    SecureBoot::retStatus sStatus;
    SecureBoot secureBoot;

    sStatus = secureBoot.validateFirmware(reinterpret_cast<const unsigned char *>(FlashMapping::appSignatureAddress),
                                          FlashMapping::appSignatureSize,
                                          reinterpret_cast<const unsigned char *>(FlashMapping::appStartAddress),
                                          FlashMapping::appSize);

    return SecureBoot::retStatus::valid == sStatus;
}

void Bootloader::initializeLookupTable()
{
    for (size_t i = 0U; i < numberOfPacketTypes; ++i)
    {
        packetHandlers[i] = nullptr;
    }

    packetHandlers[static_cast<size_t>(packetType::flashData)] = &Bootloader::handleFlashData;
    packetHandlers[static_cast<size_t>(packetType::flashStart)] = &Bootloader::handleFlashStart;
    packetHandlers[static_cast<size_t>(packetType::validateFlash)] = &Bootloader::handleValidateSignature;
}

Bootloader::retStatus Bootloader::handleFlashData(const beecom::Packet &packet)
{
    uint32_t startAddress;
    size_t dataSize = packet.header.length - sizeof(uint32_t);

    (void)extractAddress(packet.payload, &startAddress);

    const uint8_t *dataStart = packet.payload + sizeof(startAddress);
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
    flashManager_.Write(FlashMapping::appSignatureAddress, packet.payload, packet.header.length);
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

Bootloader::BootState Bootloader::determineTargetState(packetType type)
{
    switch (type)
    {
    case packetType::flashStart:
        return BootState::erasing;
    case packetType::flashData:
        return BootState::flashing;
    case packetType::flashMac:
    case packetType::validateFlash:
        return BootState::verifying;
    default:
        return state;
    }
}

void Bootloader::handleValidPacket(const beecom::Packet &packet)
{
    size_t index = static_cast<size_t>(packet.header.type);

    if ((index < numberOfPacketTypes) && (packetHandlers[index] != nullptr))
    {
        /* Check if the state transition is valid before processing the packet */
        BootState targetState = determineTargetState(static_cast<packetType>(packet.header.type));

        if (transitionState(targetState))
        {
            auto handler = packetHandlers[index];
            auto status = (this->*handler)(packet);

            if (retStatus::eOk == status)
            {
                sendResponse(true, static_cast<packetType>(packet.header.type));
            }
            else
            {
                transitionState(BootState::error);
                sendResponse(false, static_cast<packetType>(packet.header.type));
            }
        }
        else
        {
            sendResponse(false, static_cast<packetType>(packet.header.type));
        }
    }
    else
    {
        transitionState(BootState::error);
        sendResponse(false, static_cast<packetType>(packet.header.type));
    }
}

void Bootloader::handleReadDataRequest(packetType type)
{
    uint8_t dataBuffer[FlashMapping::maxDataSize];
    size_t dataSize = 0;

    switch (type)
    {
    case packetType::getAppVersion:
        flashManager_.Read(FlashMapping::appVersionAddress, dataBuffer, FlashMapping::appVersionSize);
        dataSize = FlashMapping::appVersionSize;
        break;
    case packetType::getAppSignature:
        flashManager_.Read(FlashMapping::appSignatureAddress, dataBuffer, FlashMapping::appSignatureSize);
        dataSize = FlashMapping::appSignatureSize;
        break;
    case packetType::getBootloaderVersion:
        dataSize = 0;
        break;
    default:
        sendResponse(false, type);
        return;
    }

    sendResponse(true, type, dataBuffer, dataSize);
}

bool Bootloader::transitionState(BootState newState)
{
    constexpr bool validTransitions[static_cast<int>(BootState::numStates)][static_cast<int>(BootState::numStates)] = {
                      /* idle, booting, erasing, flashing, verifying, error */
        /* idle */      {false, true,   true,    false,     false,    true},
        /* booting */   {false, true,   true,    false,     false,    true},
        /* erasing */   {false, false,  false,   true,      false,    true},
        /* flashing */  {false, false,  false,   true,      true,     true},
        /* verifying */ {true,  true,   false,   false,     false,    false},
        /* error */     {false, false,  true,    false,     false,    false}
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
        /* Invalid transition, always go to error state */
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
                /* Booting state */
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
