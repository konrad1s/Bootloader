# Bootloader
This repository contains a bootloader implementation for embedded systems. It includes secure boot features, communication protocols, and a flasher tool.

## Overview
The bootloader is a small program that allows you to update the firmware of your embedded device securely. It supports features such as secure boot, firmware validation, and communication using the BeeCOM protocol (e.g. UART).

## Features
- Secure Boot: Ensures that only authenticated firmware can run on the device.
- Firmware Update: Supports updating the firmware over any protocol.
- Firmware Validation: Uses [RSA](https://en.wikipedia.org/wiki/RSA_(cryptosystem))/[ECC](https://en.wikipedia.org/wiki/Elliptic-curve_cryptography) for verifying the integrity and authenticity of the firmware.
- Flasher Tool: A Python-based GUI tool for flashing firmware onto the device.

## Getting started

### Integration with STM32

1. **Cloning the Repository with Submodules**\
To clone this repository along with its submodules, use the following command:
```
git clone --recurse-submodules https://github.com/konrad1s/Bootloader.git
```
If you have already cloned the repository and want to initialize and update the submodules, run:
```
git submodule update --init --recursive
```
2. **Configure communication protocol and integrate with [BeeCom](https://github.com/konrad1s/BeeCom-Protocol)**\
Add the Bootloader source files and includes into your Makefile.\
Example of how to integrate BeeCom with bootloader on STM32:
```cpp
    constexpr size_t bufferSize = 1024U;
    uint8_t buffer[bufferSize];

    auto receive = [](uint8_t *uartRxByte) -> bool
    {
        return (HAL_UART_Receive(&huart1, uartRxByte, 1, 0) == HAL_OK);
    };

    auto transmit = [](const uint8_t *buffer, size_t size)
    {
        HAL_UART_Transmit(&huart1, const_cast<uint8_t *>(buffer), size, 100);
    };

    beecom::BeeComBuffer beecomBuffer(buffer, bufferSize);
    beecom::BeeCOM beecom(receive, transmit, beecomBuffer);
    FlashManager flashManager;
    Bootloader bootInstance(beecom, flashManager);

    while (1)
    {
        bootInstance.Boot();
    }
```
3. **Implement portable files**\
The bootloader is designed to be portable across different MCUs. The portable files include the following key components:
- FlashManager: Manages flash operations such as reading, writing, and erasing flash memory.
- AppJumper: Handles the transition from the bootloader to the application.
- FlashMapping: Provides metadata about the application, such as start and end addresses.
If there is no support for your specific platform, you must provide the implementation for these components. You can refer to the portable directory in the repository for examples and guidance on how to create these implementations.

4. **Optimization and Compilation Flags**\
Since the default optimization level (generated from STM32 Cube) is -Og, it is recommended to replace it with -Os for size optimization.\
Additionally, add the -specs=nano.specs linker flag to use the smaller version of the standard library, which is more suitable for embedded systems.

5. **Preparing Your Application**\
To ensure the bootloader works correctly with your application, you must modify the linker script of your application to change the beginning of the flash memory where your application will reside. The bootloader typically occupies the beginning of the flash, so your application must start after the bootloader.\
Example modification in the linker script:
```
/* Change the start address of flash to the end of the bootloader section */
FLASH (rx) : ORIGIN = 0x08020000, LENGTH = 512K
```
Modifying the system_stm32[...].c file.\
You need to define the vector table addresses in line with the linker configuration. This ensures the vector table is correctly relocated.
```
/* Note: Following vector table addresses must be defined in line with linker
         configuration. */
/*!< Uncomment the following line if you need to relocate the vector table
     anywhere in Flash or Sram, else the vector table is kept at the automatic
     remap of boot address selected */
#define USER_VECT_TAB_ADDRESS

#if defined(USER_VECT_TAB_ADDRESS)
/*!< Uncomment the following line if you need to relocate your vector Table
     in Sram else user remap will be done in Flash. */
#define VECT_TAB_SRAM
#if defined(VECT_TAB_SRAM)
#define VECT_TAB_BASE_ADDRESS   SRAM_BASE       /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.
                                                     This value must be a multiple of 0x200. */
#else
#define VECT_TAB_BASE_ADDRESS   FLASH_BASE      /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.
                                                     This value must be a multiple of 0x200. */
#endif /* VECT_TAB_SRAM */
#endif /* USER_VECT_TAB_ADDRESS */
```

6. **Using the Python Flasher Tool**\
A Python-based GUI tool is provided for flashing firmware, erasing firmware, reading the bootloader version, and validating the application. This tool also allows you to generate and manage key pairs for secure boot.

Software Download Tab:
- Select UART port, set baud rate, and connect to the device.
- Enter bootloader mode, read bootloader version, select .hex file, load key, erase firmware, flash firmware, and validate the application.

Security Tab:
- Select key type (RSA or ECC), enter password for encrypting the private key, generate key pair, save private key, save public key, and display the public key to be copied into BootConfig.h.

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
