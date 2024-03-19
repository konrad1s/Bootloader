#include "Bootloader.h"

Bootloader::retStatus Bootloader::extractAddress(const uint8_t *payload, size_t length, uint32_t *address)
{
    memcpy(address, payload, sizeof(uint32_t));
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
            handleInvalidPacket();
    };

    beecom_.setPacketHandler(packetHandler);
}

void Bootloader::handleInvalidPacket()
{
    constexpr uint8_t notAckValue = 0xAA;
    beecom::Packet packet;

    packet.header.sop = 0xA5;
    packet.header.type = 0x02;
    packet.header.length = 1;
    packet.payload[0] = notAckValue;

    beecom_.send(packet);
}

void Bootloader::sendResponse(bool success)
{
    constexpr uint8_t ackValue = 0x55;
    constexpr uint8_t notAckValue = 0xAA;

    beecom::Packet responsePacket;
    responsePacket.header.sop = 0xA5;
    responsePacket.header.type = 0x02;
    responsePacket.header.length = 1;
    responsePacket.payload[0] = success ? ackValue : notAckValue;

    beecom_.send(responsePacket);
}

void Bootloader::handleValidPacket(const beecom::Packet &packet)
{
    const uint8_t *dataStart = nullptr;
    uint32_t address = 0;

    const enum class packetType {
        prepareFlash = 0U,
        eraseData,
        flashData,
        eraseMac,
        flashMac,
        validateFlash
    } pt = static_cast<packetType>(packet.header.type);

    IFlashManager::state fStatus = IFlashManager::state::eNotOk;
    size_t dataSize = packet.header.length - sizeof(uint32_t);

    switch (pt)
    {
    case packetType::prepareFlash:
        fStatus = flashManager_.Unlock();
        break;
    case packetType::eraseData:
    case packetType::eraseMac:
        fStatus = flashManager_.Erase();
        break;
    case packetType::flashData:
    case packetType::flashMac:
        (void)extractAddress(packet.payload, packet.header.length, &address);
        dataStart = packet.payload + sizeof(uint32_t);
        fStatus = flashManager_.Write(address, dataStart, dataSize);
        break;
    case packetType::validateFlash:
        fStatus = flashManager_.Lock();
        break;
    default:
        break;
    }

    sendResponse(fStatus == IFlashManager::state::eOk);
}
