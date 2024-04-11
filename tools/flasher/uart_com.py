import serial
import serial.tools.list_ports
import time

class UARTCommunication:
    def __init__(self, timeout=1):
        self.ser = None
        self.timeout = timeout

    def refresh_ports(self):
        ports = serial.tools.list_ports.comports()
        return [port.device for port in ports]

    def connect(self, port, baudrate):
        try:
            self.ser = serial.Serial(port, baudrate, timeout=self.timeout)
            return True
        except serial.SerialException as e:
            raise ConnectionError(f"Failed to connect to {port} at {baudrate} baud: {e}")

    def disconnect(self):
        if self.ser and self.ser.is_open:
            self.ser.close()
            return True
        raise ConnectionError("No active connection to disconnect.")

    def send_packet(self, packet):
        if not self.ser or not self.ser.is_open:
            raise ConnectionError("Attempted to send on a closed connection.")
        self.ser.write(packet)

    def receive_packet(self, timeout=10):
        if not self.ser or not self.ser.is_open:
            raise ConnectionError("Attempted to receive on a closed connection.")

        timeout = time.time() + timeout
        while time.time() < timeout:
            if self.ser.in_waiting > 0:
                data = self.ser.read(self.ser.in_waiting)
                return data
            time.sleep(0.1)
        if not data:
            raise TimeoutError("No data received within the specified timeout.")
        return data
