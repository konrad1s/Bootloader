from PyQt5.QtCore import QThread, pyqtSignal
from beecom_packet import BeeCOMPacket, PacketType
from hex_file_processor import HexFileProcessor
import logging
import struct

ACK_PACKET = b'\x55'

class FlashFirmwareThread(QThread):
    update_progress = pyqtSignal(int)
    progress_max = pyqtSignal(int)
    log_message = pyqtSignal(str)

    def __init__(self, hex_processor, uart_comm):
        super().__init__()
        self.hex_processor = hex_processor
        self.uart_comm = uart_comm

    def run(self):
        try:
            data_blocks = self.hex_processor.load_and_process_hex_file()
            self._flash_data_blocks(data_blocks)
        except Exception as e:
            self.log_message.emit(f"Error: {str(e)}")
            raise

    def _flash_data_blocks(self, data_blocks):
        max_payload_size = 512
        total_size = sum(len(data) for _, data in data_blocks)
        self.progress_max.emit(total_size)

        current_size = 0
        merged_data = []
        merged_address = None

        app_start_address = None
        app_end_address = None

        for address, data in data_blocks:
            logging.debug(f"Address: {address}, Data: {data}")
            app_start_address = min(address, app_start_address) if app_start_address is not None else address
            app_end_address = max(address + len(data), app_end_address) if app_end_address is not None else address + len(data)

            if self._can_merge_data(merged_address, address, merged_data, data, max_payload_size):
                merged_data.extend(data)
            else:
                current_size = self._send_and_update_progress(merged_address, merged_data, current_size)
                merged_address = address
                merged_data = list(data)

        if merged_data:
            size = self._send_and_update_progress(merged_address, merged_data, current_size)
            self.log_message.emit(f"Firmware flashed successfully. Bytes sent: {size}")

        if app_start_address is not None and app_end_address is not None:
            self._flash_app_addresses(app_start_address, app_end_address)
        else:
            raise ValueError("Could not determine application start and end addresses from data blocks.")

    def _can_merge_data(self, merged_address, address, merged_data, data, max_payload_size):
        return (merged_address is not None
                and address == merged_address + len(merged_data)
                and len(merged_data) + len(data) <= max_payload_size - 4)

    def _send_and_update_progress(self, address, data, current_size):
        if data:
            logging.debug(f"Sending data to address {address}")
            self.send_data_chunk(address, data)
            current_size += len(data)
            self.update_progress.emit(current_size)
        return current_size

    def _flash_app_addresses(self, app_start_address, app_end_address):
        logging.debug(f"Flashing application address: {app_start_address}, end address: {app_end_address}")
        app_data_payload = (struct.pack('>I', 0x8020104) +
                            struct.pack('<I', app_start_address) +
                            struct.pack('<I', app_end_address))

        app_data_packet = BeeCOMPacket(packet_type=PacketType.flashData, payload=app_data_payload).create_packet()
        self.uart_comm.send_packet(app_data_packet)
        response = self.uart_comm.receive_packet()
        response_packet, crc_received = BeeCOMPacket.parse_packet(response)

        if not response_packet.validate_packet(crc_received, PacketType.flashData, ACK_PACKET):
            raise ValueError("Packet validation failed.")

        self.log_message.emit("Aplication metadata flashed successfully.")

    def send_data_chunk(self, address, data):
        """Sends a chunk of data to the specified address."""
        for chunk in HexFileProcessor.chunk_data(data, 2040 - 4):
            address_payload = struct.pack('>I', address) + bytes(chunk)
            packet = BeeCOMPacket(packet_type=PacketType.flashData, payload=address_payload).create_packet()
            self.uart_comm.send_packet(packet)

            response = self.uart_comm.receive_packet()
            response_packet, crc_received = BeeCOMPacket.parse_packet(response)
            if not response_packet.validate_packet(crc_received, PacketType.flashData, ACK_PACKET):
                raise ValueError("Packet validation failed.")

            address += len(chunk)
            logging.debug(f"Sent data chunk to address {address}.")

class EraseFirmwareThread(QThread):
    finished = pyqtSignal(bool, str)

    def __init__(self, uart_comm):
        super().__init__()
        self.uart_comm = uart_comm

    def run(self):
        try:
            self._erase_firmware()
            self.finished.emit(True, "Firmware erased successfully.")
        except Exception as e:
            self.finished.emit(False, f"Error erasing firmware: {str(e)}")

    def _erase_firmware(self):
        erase_packet = BeeCOMPacket(packet_type=PacketType.flashStart).create_packet()
        self.uart_comm.send_packet(erase_packet)

        response = self.uart_comm.receive_packet(timeout=10)
        response_packet, crc_received = BeeCOMPacket.parse_packet(response)

        if not response_packet.validate_packet(crc_received, PacketType.flashStart, ACK_PACKET):
            raise ValueError("Erase packet validation failed.")