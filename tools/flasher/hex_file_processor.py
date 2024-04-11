from hexrec.formats.ihex import IhexFile, IhexRecord
import struct

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

    @staticmethod
    def chunk_data(data, chunk_size):
        """Yield successive chunk_size chunks from data."""
        for i in range(0, len(data), chunk_size):
            yield data[i:i + chunk_size]
