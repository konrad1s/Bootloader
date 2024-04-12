import sys
import logging
from PyQt5.QtWidgets import (QApplication, QMainWindow, QPushButton, QVBoxLayout, QHBoxLayout,
                             QWidget, QFileDialog, QLabel, QLineEdit, QTextEdit, QComboBox, QStatusBar,
                             QProgressBar, QMessageBox)
from PyQt5.QtCore import Qt
from uart_com import UARTCommunication
from crypto_manager import CryptoManager
from hex_file_processor import HexFileProcessor
from qt_threads import FlashFirmwareThread, EraseFirmwareThread

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
        self.firmware_erased = False
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
        self.erase_button = self.setupActionButton(layout, 'Erase firmware', self.erase_firmware, False)
        self.flash_button = self.setupActionButton(layout, 'Flash firmware', self.flash_firmware, False)
        self.verify_button = self.setupActionButton(layout, 'Validate application', self.validate_app, False)
        main_layout.addLayout(layout)

        read_sig_layout = QHBoxLayout()
        self.read_sig_button = self.setupActionButton(read_sig_layout, 'Read Signature', self.read_signature, False)
        main_layout.addLayout(read_sig_layout)

    def setupActionButton(self, layout, title, method, enabled=True):
        button = QPushButton(title, self)
        button.clicked.connect(method)
        button.setEnabled(enabled)
        layout.addWidget(button)
        return button

    def setupProgressAndLog(self, main_layout):
        log_layout = QVBoxLayout()
        self.log_area = QTextEdit(self)
        self.log_area.setReadOnly(True)
        log_layout.addWidget(self.log_area)

        self.flash_progress_bar = QProgressBar(self)
        self.flash_progress_bar.setAlignment(Qt.AlignCenter)
        log_layout.addWidget(self.flash_progress_bar)

        main_layout.addLayout(log_layout)

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

    def erase_firmware(self):
        self.log("Initiating firmware erase...")
        self.erase_thread = EraseFirmwareThread(self.uart_comm)
        self.erase_thread.finished.connect(self.on_erase_finished)
        self.erase_thread.start()

    def on_erase_finished(self, success, message):
        if success:
            self.firmware_erased = True
            self.log(message)
        else:
            self.log(message, level=logging.ERROR)
            self.show_error_message(message)

    def flash_firmware(self):
        if not self.firmware_erased:
            self.log("Firmware not erased, erasing now...")
            self.erase_firmware()
            if not self.firmware_erased:
                return

        self.flash_thread = FlashFirmwareThread(self.hex_processor, self.uart_comm)
        self.flash_thread.progress_max.connect(self.flash_progress_bar.setMaximum)
        self.flash_thread.update_progress.connect(self.flash_progress_bar.setValue)
        self.flash_thread.log_message.connect(self.log)
        self.flash_thread.start()

    def validate_app(self):
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
        self.erase_button.setEnabled(enable)
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
