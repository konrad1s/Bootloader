from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import padding, utils, rsa 
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
        """Sign data using the loaded private key with PKCS#1 v2.1 PSS padding."""
        if not self.private_key:
            raise ValueError("Private key not loaded.")
        signature = self.private_key.sign(
            data,
            padding.PSS(
                mgf=padding.MGF1(hashes.SHA256()),
                salt_length=padding.PSS.MAX_LENGTH
            ),
            utils.Prehashed(hashes.SHA256())
        )
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

    @staticmethod
    def generate_key_pair(password=None):
        """Generate an RSA key pair and return PEM-encoded keys, optionally encrypted with a password."""
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        public_key = private_key.public_key()

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
        pem_public_key = public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )

        return pem_private_key, pem_public_key
