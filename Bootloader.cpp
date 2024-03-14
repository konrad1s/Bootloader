#include "bootloader.h"

void Bootloader::setupPacketHandler()
{
    beecom::PacketHandler packetHandler = [this](const beecom::Packet &packet,
                                                 bool crcValid,
                                                 beecom::SendFunction send) {

    };
}
