from hexrec.formats.ihex import IhexFile, IhexRecord
from cryptography.hazmat.primitives import hashes

class HexFileProcessor:
    def __init__(self, file_path):
        self.file_path = file_path
        self.ihex = None

    def load_and_process_hex_file(self):
        """Load hex file, process records, and prepare for transmission."""
        self.ihex = IhexFile.load(self.file_path)
        return self.process_records()

    def process_records(self):
        """Process IHEX records to compute absolute addresses and prepare data chunks."""
        base_address = 0
        processed_data = []
        for record in self.ihex.records:
            if record.tag == IhexRecord.Tag.EXTENDED_LINEAR_ADDRESS:
                base_address = int.from_bytes(record.data, byteorder='big') << 16
            elif record.tag == IhexRecord.Tag.EXTENDED_SEGMENT_ADDRESS:
                base_address = int.from_bytes(record.data, byteorder='big') << 4
            elif record.tag == IhexRecord.Tag.DATA:
                address = base_address + record.address
                processed_data.append((address, record.data))
        return processed_data

    def calculate_hash(self, min_address, max_address):
        """Calculate SHA-256 hash of the hex file from min_address to max_address, filling gaps with 0xFF."""
        if not self.ihex:
            raise ValueError("No hex file loaded.")

        if min_address >= max_address:
            raise ValueError("min_address must be less than max_address.")

        data_map = {}
        base_address = 0
        for record in self.ihex.records:
            if record.tag == IhexRecord.Tag.EXTENDED_LINEAR_ADDRESS:
                base_address = int.from_bytes(record.data, byteorder='big') << 16
            elif record.tag == IhexRecord.Tag.EXTENDED_SEGMENT_ADDRESS:
                base_address = int.from_bytes(record.data, byteorder='big') << 4
            elif record.tag == IhexRecord.Tag.DATA:
                address = base_address + record.address
                if min_address <= address < max_address:
                    end_address = address + len(record.data)
                    if end_address > max_address:
                        trim_length = max_address - address
                        data_map[address] = record.data[:trim_length]
                    else:
                        data_map[address] = record.data

        full_data = bytearray((max_address - min_address) * [0xFF])

        for address, data in data_map.items():
            start_index = address - min_address
            full_data[start_index:start_index + len(data)] = data

        hash_context = hashes.Hash(hashes.SHA256())
        hash_context.update(full_data)
        return hash_context.finalize()

    @staticmethod
    def chunk_data(data, chunk_size):
        """Yield successive chunk_size chunks from data."""
        for i in range(0, len(data), chunk_size):
            yield data[i:i + chunk_size]
