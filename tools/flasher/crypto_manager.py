from PyQt5.QtWidgets import (QApplication, QMainWindow, QTabWidget, QInputDialog, QLineEdit)
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, utils, rsa, ec
from cryptography.hazmat.primitives.serialization import load_pem_private_key, load_pem_public_key
from cryptography.hazmat.primitives.serialization import BestAvailableEncryption, NoEncryption
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
            key_data = key_file.read()
            try:
                self.private_key = load_pem_private_key(
                    key_data,
                    password=password,
                    backend=default_backend()
                )
            except TypeError:
                if password is None:
                    user_password = self.prompt_password()
                    self.private_key = load_pem_private_key(
                        key_data,
                        password=user_password.encode() if user_password else None,
                        backend=default_backend()
                    )
                else:
                    raise ValueError("Incorrect password provided for the private key.")

    def prompt_password(self):
        """Prompt the user to enter a password for the private key."""
        app = QApplication.instance()
        if not app:
            raise RuntimeError("No QApplication instance found.")
        password, ok = QInputDialog.getText(None, 'Enter Password',
                                            'Private key is encrypted. Please enter the password:', QLineEdit.Password)
        if ok and password:
            return password
        else:
            raise ValueError("Password entry cancelled or no password provided.")

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
        """Sign data using the loaded private key with PKCS#1 v2.1 PSS padding."""
        if not self.private_key:
            raise ValueError("Private key not loaded.")
        if isinstance(self.private_key, rsa.RSAPrivateKey):
            signature = self.private_key.sign(
                data,
                padding.PSS(
                    mgf=padding.MGF1(hashes.SHA256()),
                    salt_length=padding.PSS.MAX_LENGTH
                ),
                utils.Prehashed(hashes.SHA256())
            )
        elif isinstance(self.private_key, ec.EllipticCurvePrivateKey):
            signature = self.private_key.sign(
                data,
                ec.ECDSA(utils.Prehashed(hashes.SHA256()))
            )
            print(f"len of signature: {len(signature)}")
        else:
            raise TypeError("Unsupported key type")
        
        return signature

    def verify_signature(self, data, signature):
        """Verify the signature of the data using the loaded public key with PKCS#1 v2.1 PSS padding."""
        if not self.public_key:
            raise ValueError("Public key not loaded.")
        try:
            self.public_key.verify(
                signature,
                data,
                padding.PSS(
                    mgf=padding.MGF1(hashes.SHA256()),
                    salt_length=padding.PSS.MAX_LENGTH
                ),
                utils.Prehashed(hashes.SHA256())
            )
            return True
        except Exception as e:
            return False

    def generate_rsa_key_pair(self, password=None):
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        return self.serialize_keys(private_key, password)

    def generate_ecc_key_pair(self, password=None):
        private_key = ec.generate_private_key(
            ec.SECP256R1(),
            backend=default_backend()
        )
        return self.serialize_keys(private_key, password)

    def serialize_keys(self, private_key, password):
        if password:
            if isinstance(password, str):
                password = password.encode()
            encryption_algorithm = BestAvailableEncryption(password)
        else:
            encryption_algorithm = NoEncryption()

        pem_private_key = private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.TraditionalOpenSSL,
            encryption_algorithm=encryption_algorithm
        )
        pem_public_key = private_key.public_key().public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )
        return pem_private_key, pem_public_key
