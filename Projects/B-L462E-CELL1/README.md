# Anjay-freertos-client-B-L462E-CELL1 with FOTA [<img align="right" height="50px" src="https://avsystem.github.io/Anjay-doc/_images/avsystem_logo.png">](http://www.avsystem.com/)

Anjay client with FOTA for B-L462E-CELL1 development kit.

This example is built upon Secure Boot and Secure Firmware Update Demo Application provided by STMicroelectronics.

In order to **not use FOTA** with Secure Boot and Secure Firmware Update applications, switch the project's
**Build configuration** to **Debug** and simply build the application.

## Secure Boot and Secure Firmware Update Demo Application

The **X-CUBE-SBSFU Secure Boot and Secure Firmware Update** solution allows the update of the STM32 microcontroller built-in
program with new firmware versions, adding new features and correcting issues. The update process is performed
in a secure way to prevent unauthorized updates and access to confidential on-device data such as code and
firmware encryption key.

The **Secure Boot** (Root of Trust services) is immutable code, always executed after a system reset, that checks STM32
static protections, activates STM32 runtime protections and then verifies the authenticity and integrity of user
application code before every execution in order to ensure that invalid or malicious code won't be run.

This example is provided with a dual firmware image configuration in order to ensure safe image installation and enable over-the-air firmware update capability commonly used in IoT devices.

This example is based on third-party cryptographic middleware: **mbedTLS**.
Cryptographic services are delivered as open source code. It can be configured to use asymmetric or symmetric
cryptographic schemes with or without firmware encryption.

For more details, refer to [UM2262 "Getting started with SBSFU - software expansion for STM32Cube"](https://www.st.com/resource/en/user_manual/um2262-getting-started-with-the-xcubesbsfu-stm32cube-expansion-package-stmicroelectronics.pdf)

## Directory contents

- **2_Images_SECoreBin**<br/>
  Generate secure engine binary including all the trusted code
- **2_Images_SBSFU**<br/>
  Secure boot and secure firmware update application
- **Linker_Common**<br/>
  Linker files definition shared between SECoreBin, SBSFU and Anjay application
- **UserApp**<br/>
  Anjay application with Firmware Update object

## Flashing the board

You need to follow a strict compilation order:

1. Compile **SECoreBin** application<br/>
   This step is needed to create the Secure Engine core binary including all the trusted code and keys mapped inside
   the protected environment. The binary is linked with the SBSFU code in step 2.

   NOTE: to successfully build the SECoreBin application, you will need to have Python and the following modules: `pycryptodomex`, `ecdsa`, `numpy`, `pyelftools` installed on your system.

1. Compile **SBSFU** application<br/>
   This step compiles the SBSFU source code implementing the state machine and configuring the protections. In addition,
   it links the code with the SECore binary generated at step 1 in order to generate a single SBSFU binary including the
   SE trusted code.
1. Compile **UserApp** application (set **Build configuartion** to **Release**)<br/>
   It generates:<br/>
   - The user application binary file that is uploaded to the device using the Secure Firmware Update process <br/>
     (`Projects/B-L462E-CELL1/UserApp/Binary/Anjay-freertos-client-B-L462E-CELL1-[MODEM].sfb`).
   - A binary file concatenating the SBSFU binary, the user application binary in clear format, and the corresponding
     FW header <br/>
     (`Projects/B-L462E-CELL1/UserApp/Binary/SBSFU_Anjay-freertos-client-B-L462E-CELL1-[MODEM].bin`).

   You can set a custom firmware version in the `Application/Inc/default_config.h` file (using `FIRMWARE_VERSION` define).
   It will be useful when performing FOTA to distinguish the firmware images from each other.
1. Flashing<br/>
   Use **STM32CubeProgrammer** application with `SBSFU_Anjay-freertos-client-B-L462E-CELL1-[MODEM].bin` file to program the board (it is advisable to perform **Full chip erase** first). You can open serial port to change default credentials in order to connect to Coiote DM.
   After that, you can use Coiote DM to perform firmware update with `Anjay-freertos-client-B-L462E-CELL1-[MODEM].sfb` file.

## Performing firmware update

In order to perform firmware update:

1. Build the application and flash the board with `FIRMWARE_UPDATE` define set to the proper version (see [Flashing the board](#Flashing-the-board) step), e.g.
    ```
    #define FIRMWARE_VERSION "v1.0"
    ```
1. Make changes to the code (optionally), set `FIRMWARE_UPDATE` define to a different version, e.g.
    ```
    #define FIRMWARE_VERSION "v2.0"
    ```
    and build the application with a new firmware.
1. Upload the generated firmware file (`Anjay-freertos-client-B-L462E-CELL1-[MODEM].sfb`) to [Coiote DM](https://eu.iot.avsystem.cloud) (go to Device management and select `Firmware update`) and click `Upgrade`.
1. After the FOTA finishes, the device will reboot and the following log should appear:
    ```
    Firmware updated from version 'v1.0' to 'v2.0'
    ```
    where `v1.0` and `v2.0` will be set to firmware versions you set earlier.
