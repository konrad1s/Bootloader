from PyQt5.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QPushButton, QLineEdit, QLabel, QMessageBox, QFileDialog, QTextEdit)
from crypto_manager import CryptoManager
import logging

class SecurityTab(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.crypto_manager = CryptoManager()
        self.setupUI()

    def setupUI(self):
        main_layout = QVBoxLayout(self)

        password_layout = QHBoxLayout()
        self.password_label = QLabel("Password (for encrypting private key):", self)
        password_layout.addWidget(self.password_label)
        
        self.password_input = QLineEdit(self)
        self.password_input.setPlaceholderText("Enter password here")
        self.password_input.setEchoMode(QLineEdit.Password)
        password_layout.addWidget(self.password_input)

        main_layout.addLayout(password_layout)

        self.gen_keys_button = QPushButton('Generate Key Pair', self)
        self.gen_keys_button.clicked.connect(self.generate_key_pair)
        main_layout.addWidget(self.gen_keys_button)
        
        self.save_private_key_button = QPushButton('Save Private Key', self)
        self.save_private_key_button.clicked.connect(self.save_private_key)
        main_layout.addWidget(self.save_private_key_button)

        self.save_public_key_button = QPushButton('Save Public Key', self)
        self.save_public_key_button.clicked.connect(self.save_public_key)
        main_layout.addWidget(self.save_public_key_button)

        self.public_key_info_label = QLabel('Public key to be copied in "BootConfig.h":', self)
        main_layout.addWidget(self.public_key_info_label)

        self.public_key_display = QTextEdit(self)
        self.public_key_display.setReadOnly(True)
        main_layout.addWidget(self.public_key_display)

    def generate_key_pair(self):
        try:
            password = self.password_input.text()
            if password == "":
                password = None
            self.private_key, self.public_key = self.crypto_manager.generate_key_pair(password=password)
            self.display_public_key()
            QMessageBox.information(self, "Success", "Key pair generated successfully.")
        except Exception as e:
            QMessageBox.critical(self, "Error", f"Failed to generate key pair: {e}")
            logging.error(f"Failed to generate key pair: {e}")

    def display_public_key(self):
        lines = self.public_key.decode().split('\n')
        formatted_public_key = 'constexpr char publicKey[] = \n' + \
                            '\n'.join(f'"{line}\\n"' for line in lines if line) + ';'
        self.public_key_display.setText(formatted_public_key)

    def save_private_key(self):
        options = QFileDialog.Options()
        private_key_path, _ = QFileDialog.getSaveFileName(self, "Save Private Key", "", "PEM Files (*.pem);;All Files (*)", options=options)
        if private_key_path:
            try:
                with open(private_key_path, 'wb') as f:
                    f.write(self.private_key)
                QMessageBox.information(self, "Success", "Private key saved successfully.")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to save private key: {e}")
                logging.error(f"Failed to save private key: {e}")

    def save_public_key(self):
        options = QFileDialog.Options()
        public_key_path, _ = QFileDialog.getSaveFileName(self, "Save Public Key", "", "PEM Files (*.pem);;All Files (*)", options=options)
        if public_key_path:
            try:
                with open(public_key_path, 'wb') as f:
                    f.write(self.public_key)
                QMessageBox.information(self, "Success", "Public key saved successfully.")
            except Exception as e:
                QMessageBox.critical(self, "Error", f"Failed to save public key: {e}")
                logging.error(f"Failed to save public key: {e}")
