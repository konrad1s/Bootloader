from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.asymmetric import padding, utils
from cryptography.hazmat.primitives.serialization import load_pem_private_key, load_pem_public_key
from cryptography.hazmat.backends import default_backend
import os

class CryptoManager:
    def __init__(self):
        self.private_key = None
        self.public_key = None

    def load_private_key(self, file_path, password=None):
        """Load a private RSA key from a PEM file."""
        if not os.path.exists(file_path):
            raise FileNotFoundError("Private key file not found.")
        with open(file_path, 'rb') as key_file:
            self.private_key = load_pem_private_key(
                key_file.read(),
                password=password,
                backend=default_backend()
            )

    def load_public_key(self, file_path):
        """Load a public RSA key from a PEM file."""
        if not os.path.exists(file_path):
            raise FileNotFoundError("Public key file not found.")
        with open(file_path, 'rb') as key_file:
            self.public_key = load_pem_public_key(
                key_file.read(),
                backend=default_backend()
            )

    def sign_data(self, data):
        """Sign data using the loaded private key."""
        if not self.private_key:
            raise ValueError("Private key not loaded.")
        signature = self.private_key.sign(
            data,
            padding.PKCS1v15(),
            utils.Prehashed(hashes.SHA256())
        )
        return signature

    def verify_signature(self, data, signature):
        """Verify the signature of the data using the loaded public key."""
        if not self.public_key:
            raise ValueError("Public key not loaded.")
        try:
            self.public_key.verify(
                signature,
                data,
                padding.PKCS1v15(),
                utils.Prehashed(hashes.SHA256())
            )
            return True
        except Exception as e:
            return False
