from hexrec.formats.ihex import IhexFile, IhexRecord
from cryptography.hazmat.primitives import hashes
import logging

class HexFileProcessor:
    def __init__(self, file_path):
        self.file_path = file_path
        self.ihex = None

    def load_and_process_hex_file(self):
        """Load hex file, process records, and prepare for transmission."""
        self.ihex = IhexFile.load(self.file_path)
        return self._process_records()

    def _process_records(self):
        """Process IHEX records to compute absolute addresses and prepare data chunks."""
        base_address = 0
        processed_data = []
        for record in self.ihex.records:
            base_address = self._update_base_address(record, base_address)
            if record.tag == IhexRecord.Tag.DATA:
                address = base_address + record.address
                processed_data.append((address, record.data))
        return processed_data

    def _update_base_address(self, record, base_address):
        if record.tag == IhexRecord.Tag.EXTENDED_LINEAR_ADDRESS:
            base_address = int.from_bytes(record.data, byteorder='big') << 16
        elif record.tag == IhexRecord.Tag.EXTENDED_SEGMENT_ADDRESS:
            base_address = int.from_bytes(record.data, byteorder='big') << 4
        return base_address

    def calculate_hash(self):
        """Calculate SHA-256 hash of the hex file from min_address to max_address, filling gaps with 0xFF."""
        if not self.ihex:
            raise ValueError("No hex file loaded.")

        base_address, data_map = 0, {}
        for record in self.ihex.records:
            base_address = self._update_base_address(record, base_address)
            if record.tag == IhexRecord.Tag.DATA:
                address = base_address + record.address
                data_map[address] = record.data

        if not data_map:
            raise ValueError("No data in hex file.")

        min_address = min(data_map.keys())
        max_address = max(addr + len(data) for addr, data in data_map.items())
        logging.debug(f"Min address: {min_address}, Max address: {max_address}")

        full_data = self._create_full_data(data_map, min_address, max_address)
        return self._compute_sha256(full_data)

    def _create_full_data(self, data_map, min_address, max_address):
        full_data = bytearray((max_address - min_address) * [0xFF])
        for address, data in data_map.items():
            start_index = address - min_address
            full_data[start_index:start_index + len(data)] = data
        return full_data

    def _compute_sha256(self, data):
        hash_context = hashes.Hash(hashes.SHA256())
        hash_context.update(data)
        return hash_context.finalize()

    @staticmethod
    def chunk_data(data, chunk_size):
        """Yield successive chunk_size chunks from data."""
        for i in range(0, len(data), chunk_size):
            yield data[i:i + chunk_size]