import sys
import logging
from PyQt5.QtWidgets import (QApplication, QMainWindow, QPushButton, QVBoxLayout, QHBoxLayout,
                             QWidget, QFileDialog, QLabel, QLineEdit, QTextEdit, QComboBox, QStatusBar,
                             QProgressBar, QMessageBox)
from PyQt5.QtCore import Qt
from uart_com import UARTCommunication
from beecom_packet import BeeCOMPacket, PacketType
from crypto_manager import CryptoManager
from hex_file_processor import HexFileProcessor
import struct

logging.basicConfig(level=logging.INFO, filename='beecom_flasher.log', filemode='a',
                    format='%(asctime)s - %(levelname)s - %(message)s')

class QTextEditLogger(logging.Handler):
    def __init__(self, widget):
        super().__init__()
        self.widget = widget
        self.widget.setReadOnly(True)
    
    def emit(self, record):
        msg = self.format(record)
        self.widget.append(msg)


class BeeComFlasher(QMainWindow):
    ACK_PACKET = b'\x55'

    def __init__(self):
        super().__init__()
        self.uart_comm = UARTCommunication()
        self.crypto_manager = CryptoManager()
        self.hex_processor = None
        self.setupUI()
        self.setupLogger()

    def setupUI(self):
        self.setWindowTitle('BeeCOM Flasher')
        self.setGeometry(100, 100, 450, 500)
        self.setupCentralWidget()
        self.setupStatusBar()
        self.refresh_ports()
        self.show()

    def setupCentralWidget(self):
        central_widget = QWidget(self)
        self.setCentralWidget(central_widget)
        main_layout = QVBoxLayout(central_widget)
        self.setupTopLayout(main_layout)
        self.setupFileLayout(main_layout)
        self.setupButtonsLayout(main_layout)
        self.setupProgressAndLog(main_layout)

    def setupStatusBar(self):
        self.status_bar = QStatusBar()
        self.setStatusBar(self.status_bar)

    def setupTopLayout(self, main_layout):
        layout = QHBoxLayout()
        self.setupPortSelection(layout)
        self.setupBaudRateSelection(layout)
        self.setupConnectButton(layout)
        main_layout.addLayout(layout)

    def setupFileLayout(self, main_layout):
        layout = QHBoxLayout()
        self.file_button = QPushButton('Select .hex File', self)
        self.file_button.clicked.connect(self.select_hex_file)
        layout.addWidget(self.file_button)

        self.private_key_button = QPushButton('Load Key', self)
        self.private_key_button.clicked.connect(self.load_key)
        layout.addWidget(self.private_key_button)

        main_layout.addLayout(layout)

    def setupButtonsLayout(self, main_layout):
        layout = QHBoxLayout()
        self.flash_button = self.setupActionButton(layout, 'Flash firmware', self.flash_firmware, False)
        self.verify_button = self.setupActionButton(layout, 'Verify Signature', self.verify_signature, False)
        self.read_sig_button = self.setupActionButton(layout, 'Read Signature', self.read_signature, False)

        main_layout.addLayout(layout)
        main_layout.addLayout(layout)

    def setupActionButton(self, layout, title, method, enabled=True):
        button = QPushButton(title, self)
        button.clicked.connect(method)
        button.setEnabled(enabled)
        layout.addWidget(button)
        return button

    def setupProgressAndLog(self, main_layout):
        self.flash_progress_bar = QProgressBar(self)
        self.flash_progress_bar.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(self.flash_progress_bar)
        
        self.log_area = QTextEdit(self)
        self.log_area.setReadOnly(True)
        main_layout.addWidget(self.log_area)

    def setupPortSelection(self, layout):
        self.port_label = QLabel("Select UART port:", self)
        self.port_combo_box = QComboBox(self)
        layout.addWidget(self.port_label)
        layout.addWidget(self.port_combo_box)
        self.refresh_button = QPushButton('Refresh Ports', self)
        self.refresh_button.clicked.connect(self.refresh_ports)
        layout.addWidget(self.refresh_button)

    def setupBaudRateSelection(self, layout):
        self.baud_rate_label = QLabel("Set baud rate:", self)
        self.baud_rate_input = QLineEdit(self)
        self.baud_rate_input.setText("115200")
        layout.addWidget(self.baud_rate_label)
        layout.addWidget(self.baud_rate_input)

    def setupConnectButton(self, layout):
        self.connect_button = QPushButton('Connect', self)
        self.connect_button.clicked.connect(self.connect_to_device)
        layout.addWidget(self.connect_button)

    def setupLogger(self):
        logTextBox = QTextEditLogger(self.log_area)
        logTextBox.setLevel(logging.INFO)
        formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
        logTextBox.setFormatter(formatter)
        logging.getLogger().addHandler(logTextBox)
        logging.getLogger().setLevel(logging.INFO)

    def select_hex_file(self):
        file_name, _ = QFileDialog.getOpenFileName(self, "Open HEX file", "", "HEX files (*.hex)")
        if file_name:
            try:
                self.hex_processor = HexFileProcessor(file_name)
                self.log("HEX file successfully loaded.")
            except FileNotFoundError:
                self.log("Failed to load HEX file: File not found.", level=logging.ERROR)
            except Exception as e:
                self.log(f"Failed to load HEX file: {e}", level=logging.ERROR)

    def load_key(self):
        """Load a private key from a .pem file using a file dialog."""
        file_name, _ = QFileDialog.getOpenFileName(self, "Open Private Key File", "", "PEM files (*.pem)")
        if file_name:
            try:
                self.crypto_manager.load_private_key(file_path=file_name)
                self.log("Private key successfully loaded.")
            except FileNotFoundError:
                self.log("Failed to load private key: File not found.", level=logging.ERROR)
            except ValueError as e:
                self.log(f"Failed to load private key: {e}", level=logging.ERROR)
            except Exception as e:
                self.log(f"Unexpected error: {e}", level=logging.ERROR)

    def connect_to_device(self):
        selected_port = self.port_combo_box.currentText()
        baud_rate = int(self.baud_rate_input.text())
        try:
            if self.uart_comm.connect(selected_port, baud_rate):
                self.log("Successfully connected to the device.")
                self.enable_flashing_buttons(True)
        except Exception as e:
            self.log(f"Failed to connect to the device: {e}", logging.ERROR)
            QMessageBox.critical(self, "Error", "Failed to connect to the device.")
            self.enable_flashing_buttons(False)

    def flash_firmware(self):
        """Send firmware flashing start command and handle response."""
        self.log("Sending flash start command...")
        try:
            start_packet = BeeCOMPacket(packet_type=PacketType.flashStart).create_packet()
            self.uart_comm.send_packet(start_packet)
            response = self.uart_comm.receive_packet(timeout=10)
            response_packet, crc_received = BeeCOMPacket.parse_packet(response)
            if not response_packet.validate_packet(crc_received, PacketType.flashStart, self.ACK_PACKET):
                raise ValueError("Packet validation failed.")
            self.log("Flash start ACK received.")
        except (ValueError, ConnectionError, TimeoutError) as e:
            self.log(str(e), level=logging.ERROR)
            return

        try:
            data_blocks = self.hex_processor.load_and_process_hex_file()
            max_payload_size = 256
            total_size = sum(len(data) for _, data in data_blocks)
            self.flash_progress_bar.setMaximum(total_size)
            self.flash_progress_bar.setValue(0)
            current_size = 0

            for address, data in data_blocks:
                for chunk in HexFileProcessor.chunk_data(data, max_payload_size - 4):
                    address_payload = struct.pack('>I', address) + bytes(chunk)
                    packet = BeeCOMPacket(packet_type=PacketType.flashData, payload=address_payload).create_packet()
                    self.uart_comm.send_packet(packet)
                    response = self.uart_comm.receive_packet()
                    response_packet, crc_received = BeeCOMPacket.parse_packet(response)
                    if not response_packet.validate_packet(crc_received, PacketType.flashData, self.ACK_PACKET):
                        raise ValueError("Packet validation failed.")
                    address += len(chunk)
                    current_size += len(chunk)
                    self.flash_progress_bar.setValue(current_size)
                    self.log(f"Flashed data to address {address}")
        except Exception as e:
            self.log(f"Error flashing firmware: {e}", logging.ERROR)

    def verify_signature(self):
        self.log("Verifying firmware signature...")

    def read_signature(self):
        self.log("Reading firmware signature...")

    def refresh_ports(self):
        port_list = self.uart_comm.refresh_ports()
        self.port_combo_box.clear()
        self.port_combo_box.addItems(port_list)
        self.log("Ports refreshed.")

    def enable_flashing_buttons(self, enable):
        """Enable or disable the flashing-related buttons."""
        self.flash_button.setEnabled(enable)
        self.verify_button.setEnabled(enable)
        self.read_sig_button.setEnabled(enable)

    def log(self, message, level=logging.INFO):
        logging.log(level, message)

    def show_error_message(self, message):
        QMessageBox.critical(self, "Error", message)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = BeeComFlasher()
    sys.exit(app.exec_())
