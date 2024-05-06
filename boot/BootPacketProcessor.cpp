#include "Bootloader.h"

BootPacketProcessor::BootPacketProcessor(Bootloader& bootloader) : bootloader_(bootloader) {}

void BootPacketProcessor::onPacketReceived(const beecom::Packet &packet, bool crcValid, void *beeComInstance)
{
    if (!crcValid)
    {
        bootloader_.SendNackResponse(Bootloader::packetType::invalidPacket);
        return;
    }

    size_t index = static_cast<size_t>(packet.header.type);
    if (index < bootloader_.packetHandlers.size() && bootloader_.packetHandlers[index] != nullptr)
    {
        bootloader_.HandleValidPacket(packet);
    }
    else
    {
        bootloader_.SendNackResponse(static_cast<Bootloader::packetType>(packet.header.type));
    }
}
