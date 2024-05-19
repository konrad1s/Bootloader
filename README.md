# Bootloader
This repository contains a bootloader implementation for embedded systems. It includes secure boot features, communication protocols, and a flasher tool.

## Overview
The bootloader is a small program that allows you to update the firmware of your embedded device securely. It supports features such as secure boot, firmware validation, and communication using the BeeCOM protocol (e.g. UART).

## Features
- Secure Boot: Ensures that only authenticated firmware can run on the device.
- Firmware Update: Supports updating the firmware over any protocol.
- Firmware Validation: Uses RSA/ECC for verifying the integrity and authenticity of the firmware.
- Flasher Tool: A Python-based GUI tool for flashing firmware onto the device.

## Process Overview
### Bootloader to Application Jump
1. Power On: The system is powered on, activating the bootloader.
2. Waiting for Boot Action: The bootloader waits for a specified time (BootConfig::waitForBootActionMs) for a frame from the Flasher application.
3. Frame Reception:
 - If a frame is received, the bootloader acknowledges it and extends the wait time by BootConfig::actionBootExtensionMs.
 - If no frame is received, the bootloader checks if the application is valid (using RSA/ECC validation).
   * If valid, the bootloader jumps to the application.
   * If invalid, the bootloader remains in its current state.
![](https://github.com/konrad1s/Bootloader/blob/master/uml/bootToAppJump.png)
![](https://github.com/konrad1s/Bootloader/blob/master/uml/enterBootFromApp.png?raw=true)

### Reflashing and Signature Validation
1. Flash Start: The application sends a flash start packet to the bootloader, which erases the application area.
2. Flash Data: The application sends flash data packets to the bootloader, which writes the data to flash memory.
3. Validate Signature:
 - The application sends a validate signature packet to the bootloader, which writes the signature data to flash memory and validates the firmware.
 - If the validation is successful, the bootloader sets a valid flag and transitions to the booting state, then jumps to the application.
 - If the validation fails, the bootloader sends a negative acknowledgment response.
![](https://github.com/konrad1s/Bootloader/blob/master/uml/reflashingAndValidation.png)
