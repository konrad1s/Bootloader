from PyQt5.QtCore import QThread, pyqtSignal
from beecom_packet import BeeCOMPacket, PacketType
from hex_file_processor import HexFileProcessor
import struct

ACK_PACKET = b'\x55'

class FlashFirmwareThread(QThread):
    update_progress = pyqtSignal(int)
    progress_max = pyqtSignal(int)
    log_message = pyqtSignal(str)

    def __init__(self, hex_processor, uart_comm):
        QThread.__init__(self)
        self.hex_processor = hex_processor
        self.uart_comm = uart_comm

    def run(self):
        try:
            data_blocks = self.hex_processor.load_and_process_hex_file()
            max_payload_size = 256
            total_size = sum(len(data) for _, data in data_blocks)
            self.progress_max.emit(total_size)
            current_size = 0

            for address, data in data_blocks:
                for chunk in HexFileProcessor.chunk_data(data, max_payload_size - 4):
                    address_payload = struct.pack('>I', address) + bytes(chunk)
                    packet = BeeCOMPacket(packet_type=PacketType.flashData, payload=address_payload).create_packet()
                    self.uart_comm.send_packet(packet)
                    response = self.uart_comm.receive_packet()
                    response_packet, crc_received = BeeCOMPacket.parse_packet(response)
                    if not response_packet.validate_packet(crc_received, PacketType.flashData, ACK_PACKET):
                        raise ValueError("Packet validation failed.")
                    address += len(chunk)
                    current_size += len(chunk)
                    self.update_progress.emit(current_size)
                    self.log_message.emit(f"Flashed data to address {address}")
        except Exception as e:
            self.log_message.emit(f"Error flashing firmware: {e}")


class EraseFirmwareThread(QThread):
    finished = pyqtSignal(bool, str)

    def __init__(self, uart_comm):
        super().__init__()
        self.uart_comm = uart_comm

    def run(self):
        try:
            erase_packet = BeeCOMPacket(packet_type=PacketType.flashStart).create_packet()
            self.uart_comm.send_packet(erase_packet)

            response = self.uart_comm.receive_packet(timeout=10)
            response_packet, crc_received = BeeCOMPacket.parse_packet(response)

            if not response_packet.validate_packet(crc_received, PacketType.flashStart, ACK_PACKET):
                raise ValueError("Erase packet validation failed.")

            self.finished.emit(True, "Firmware erased successfully.")
        except Exception as e:
            self.finished.emit(False, f"Error erasing firmware: {str(e)}")

    def erase_firmware(self):
        self.log("Erasing firmware...")
        try:
            start_packet = BeeCOMPacket(packet_type=PacketType.flashStart).create_packet()
            self.uart_comm.send_packet(start_packet)
            response = self.uart_comm.receive_packet(timeout=10)
            response_packet, crc_received = BeeCOMPacket.parse_packet(response)
            if not response_packet.validate_packet(crc_received, PacketType.flashStart, self.ACK_PACKET):
                raise ValueError("Packet validation failed.")
            self.firmware_erased = True
            self.log("Firmware erased successfully.")
        except (ValueError, ConnectionError, TimeoutError) as e:
            self.log(str(e), level=logging.ERROR)
            self.firmware_erased = False
            return