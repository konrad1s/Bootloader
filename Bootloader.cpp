#include "Bootloader.h"

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
            bytesFlashed = 0U;
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

void Bootloader::handleValidPacket(const beecom::Packet &packet)
{
    const uint8_t *dataStart = nullptr;
    uint32_t startAddress;
    const packetType pt = static_cast<packetType>(packet.header.type);
    IFlashManager::state fStatus = IFlashManager::state::eNotOk;
    size_t dataSize = packet.header.length - sizeof(uint32_t);

    switch (pt)
    {
    case packetType::flashStart:
        fStatus = flashManager_.Erase(FlashMapping::appStartAddress, FlashMapping::appEndAddress);
        break;
    case packetType::flashData:
    case packetType::flashMac:
        (void)extractAddress(packet.payload, &startAddress);
        dataStart = packet.payload + sizeof(startAddress);
        fStatus = flashManager_.Write(startAddress, dataStart, dataSize);
        bytesFlashed += dataSize;
        break;
    case packetType::validateFlash:
    {
        SecureBoot::retStatus sStatus = secureBoot.validateFirmware(packet.payload,
                                                                    packet.header.length,
                                                                    reinterpret_cast<const unsigned char *>(FlashMapping::appStartAddress),
                                                                    bytesFlashed);
        if (SecureBoot::retStatus::valid == sStatus)
        {
            fStatus = IFlashManager::state::eOk;
        }
        else
        {
            fStatus = IFlashManager::state::eNotOk;
        }
        break;
    }

        appJumper.jumpToApplication();
        break;
    case packetType::getAppVersion:
    case packetType::getAppSignature:
    case packetType::getBootloaderVersion:
        handleReadDataRequest(static_cast<packetType>(packet.header.type));
        return;
    default:
        break;
    }

    sendResponse(fStatus == IFlashManager::state::eOk, pt);
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

void Bootloader::boot()
{
    auto startTime = HAL_GetTick();

    while (startTime + waitForBootActionMs > HAL_GetTick())
    {
        /* TODO: Add return value receive */
        beecom_.receive();
        {
            // startTime = HAL_GetTick();
        }
    }

    // if (SecureBoot::retStatus::firmwareValid == secureBoot.validateFirmware())
    {
        appJumper.jumpToApplication();
    }
    // else
    {
        while (true)
        {
            beecom_.receive();
        }
    }
}
