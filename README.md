# Bootloader
This repository contains a bootloader implementation for embedded systems. It includes secure boot features, communication protocols, and a flasher tool.

## Overview
The bootloader is a small program that allows you to update the firmware of your embedded device securely. It supports features such as secure boot, firmware validation, and communication using the BeeCOM protocol (e.g. UART).

## Features
- Secure Boot: Ensures that only authenticated firmware can run on the device.
- Firmware Update: Supports updating the firmware over any protocol.
- Firmware Validation: Uses RSA/ECC for verifying the integrity and authenticity of the firmware.
- Flasher Tool: A Python-based GUI tool for flashing firmware onto the device.
