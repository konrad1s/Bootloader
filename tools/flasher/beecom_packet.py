import struct
import logging

logger = logging.getLogger(__name__)

def calculate_crc16_ccitt(data, poly=0x1021, crc=0x1D0F):
    """Calculate CRC16 CCITT variant."""
    for byte in data:
        crc ^= byte << 8
        for _ in range(8):
            if crc & 0x8000:
                crc = (crc << 1) ^ poly
            else:
                crc = crc << 1
            crc &= 0xFFFF
    return crc


class PacketType:
    invalidPacket = 0
    flashStart = 1
    flashData = 2
    flashMac = 3
    validateFlash = 4
    sendSignature = 5


class BeeCOMPacket:
    EXPECTED_SOP = 0xA5

    def __init__(self, sop=EXPECTED_SOP, packet_type=0x00, payload=b''):
        self.sop = sop
        self.packet_type = packet_type
        self.payload = payload
        self.logger = logging.getLogger(self.__class__.__name__)

    def create_packet(self):
        """Create a BeeCOM packet from instance variables."""
        length = len(self.payload)
        header = struct.pack('BBH', self.sop, self.packet_type, length)
        crc = calculate_crc16_ccitt(header + self.payload)
        return header + self.payload + struct.pack('H', crc)

    @classmethod
    def parse_packet(cls, packet):
        """Parse a received BeeCOM packet and return an instance."""
        if len(packet) < 5:
            raise ValueError("Packet too short to parse.")
        
        header = packet[:4]
        sop, packet_type, length = struct.unpack('BBH', header)
        if len(packet) - 6 != length:
            raise ValueError("Payload length mismatch.")
        
        payload = packet[4:-2]
        crc_received = struct.unpack('H', packet[-2:])[0]
        instance = cls(sop, packet_type, payload)
        return instance, crc_received

    def validate_packet(self, crc_received, expected_packet_type=None, expected_response=None):
        """Validate the packet by checking all necessary fields."""
        if self.sop != self.EXPECTED_SOP:
            raise ValueError(f"Invalid SOP: received {self.sop}, expected {self.EXPECTED_SOP}")

        if expected_packet_type is not None and self.packet_type != expected_packet_type:
            raise ValueError(f"Unexpected packet type: received {self.packet_type}, expected {expected_packet_type}")

        if expected_response is not None and self.payload != expected_response:
            raise ValueError(f"Unexpected response: received {self.payload}, expected {expected_response}")

        packet_without_crc = struct.pack('BBH', self.sop, self.packet_type, len(self.payload)) + self.payload
        crc_calculated = calculate_crc16_ccitt(packet_without_crc)
        if not crc_received == crc_calculated:
            raise ValueError("CRC mismatch: packet may be corrupted or altered.")

        return True
