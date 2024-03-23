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
            sendResponse(false, packetType::invalidPacket);
    };

    beecom_.setPacketHandler(packetHandler);
}

void Bootloader::sendResponse(bool success, packetType type)
{
    const uint8_t ackValue = 0x55U;
    const uint8_t notAckValue = 0xAAU;

    beecom::Packet responsePacket;
    responsePacket.header.sop = 0xA5U;
    responsePacket.header.type = static_cast<uint8_t>(type);
    responsePacket.header.length = 1U;
    responsePacket.payload[0] = success ? ackValue : notAckValue;

    beecom_.send(responsePacket);
}

void Bootloader::handleValidPacket(const beecom::Packet &packet)
{
    const uint8_t *dataStart = nullptr;
    uint32_t startAddress;
    uint32_t endAddress;

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
        break;
    case packetType::validateFlash:
        /* TODO: dummy */
        appJumper.jumpToApplication();
        break;
    default:
        break;
    }

    sendResponse(fStatus == IFlashManager::state::eOk, pt);
}
