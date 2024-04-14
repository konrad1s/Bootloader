from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QPushButton, QFileDialog, QMessageBox)
from crypto_manager import CryptoManager
import logging

class SecurityTab(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.crypto_manager = CryptoManager()
        self.setupUI()

    def setupUI(self):
        layout = QVBoxLayout(self)
        
        self.gen_keys_button = QPushButton('Generate and save key pair', self)
        self.gen_keys_button.clicked.connect(self.generate_key_pair)
        layout.addWidget(self.gen_keys_button)

    def generate_key_pair(self):
        try:
            private_key, public_key = self.crypto_manager.generate_key_pair()
            QMessageBox.information(self, "Success", "Key pair generated successfully.")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to generate key pair: {e}")
            logging.error(f"Failed to generate key pair: {e}")
