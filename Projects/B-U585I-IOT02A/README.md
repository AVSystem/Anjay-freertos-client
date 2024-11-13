# Anjay-freertos-client-B-U585I-IOT02A with FOTA [<img align="right" height="50px" src="https://avsystem.github.io/Anjay-doc/_images/avsystem_logo.png">](http://www.avsystem.com/)

Anjay client with FOTA for B-U585I-IOT02A development kit.

This example is built upon Secure Boot and Secure Firmware Update Application
(v1.0.2) provided by STMicroelectronics: [STM32CubeU5 SBSFU
Application](https://github.com/STMicroelectronics/STM32CubeU5/tree/v1.0.2/Projects/B-U585I-IOT02A/Applications/SBSFU)

In order to **not use FOTA** with Secure Boot and Secure Firmware Update
applications, switch the project's **Build configuration** to **Debug** and
simply build the application. If B-U585I-IOT02A board is in secure mode
(TrustZone is active), before flashing follow [Disabling
TrustZone](#Disabling-TrustZone) section.

## Secure Boot and Secure Firmware Update Application

The Secure Boot and Secure Firmware Update solution allows the update of the
STM32 microcontroller built-in program with new firmware versions, adding new
features and correcting issues. The update process is performed in a secure way
to prevent unauthorized updates and access to confidential on-device data.

The Secure Boot (Root of Trust services) is immutable code, always executed
after a system reset, that checks STM32 static protections, activates STM32
runtime protections and then verifies the authenticity (RSA or ECDSA signature)
and integrity (SHA256) of the application code before execution in order to
ensure that invalid or malicious code cannot be run. The default asymmetric key
(RSA or ECDSA) is taken from `SBSFU_Boot/Src/keys.c`

For more details, refer to [UM2262 "Getting started with SBSFU - software
expansion for
STM32Cube"](https://www.st.com/resource/en/user_manual/um2262-getting-started-with-the-xcubesbsfu-stm32cube-expansion-package-stmicroelectronics.pdf)

## Directory contents

- **SBSFU_Boot**<br/>
  Secure boot and secure firmware update application.
- **SBSFU_Secure_App**<br/>
  Secure services application that allows non-secure application to refer to secure peripherals and memory regions
- **Linker**<br/>
  Memory layout definition shared between SBSFU_Boot, SBSFU_Secure_App and Anjay application
- **st_low_level**<br/>
  Device definition for low_level_device drivers
- **UserApp**<br/>
  Anjay application using BG96 module, with Firmware Update object

## Dependencies

1. STM32CubeProgrammer
1. Support for shell scripts execution (on Windows for example Git or Cygwin can
   be used)
1. Python3
1. Python modules: `jinja2`, `pyyaml`, `pycryptodomex`, `ecdsa`, `numpy`,
   `pyelftools`, `click`, `cryptography`, `cbor`, `intelhex`. 
   To install all at once, run:<br/>
   ```
   pip3 install jinja2 pyyaml pycryptodomex ecdsa numpy pyelftools click cryptography cbor intelhex
   ```

## Flashing the board

You need to follow a strict compilation order:

1. Compile **SBSFU_Boot** application<br/>
   This step creates the secure boot and secure firmware update binary including
   provisioned user data (keys, IDs...).

1. Compile **SBSFU_Secure_App** application<br/>
   This step creates the SBSFU Application Secure binary.

1. Compile **Anjay-freertos-client-B-U585I-IOT02A-BG96** application (set
   **Build configuartion** to **Release**)<br/>
   It generates:<br/>
   - A binary file containing secure and non-secure application for use in
     flashing process<br/>
     (`Projects/B-U585I-IOT02A/UserApp/BG96/Binary/sbsfu.bin`).
   - The encrypted assembled SBSFU signed image for use in FOTA process<br/>
     (`Projects/B-U585I-IOT02A/UserApp/BG96/Binary/sbsfu_enc_sign.bin`).
   - The assembled SBSFU signed image for use in FOTA process<br/>
     (`Projects/B-U585I-IOT02A/UserApp/BG96/Binary/sbsfu_sign.bin`).

1. Flashing<br/>
   - Ensure the switch SW1 (BOOT0 pin) on B-U585I-IOT02A board is on position 0
     to boot from Flash.
   - Connect microUSB cable to the B-U585I-IOT02A board (CN8)
   - Execute regression script to perform device initialization
     (`Projects/B-U585I-IOT02A/SBSFU_Boot/STM32CubeIDE/regression.sh`)
   - Program Boot and Application binaries by executing SBSFU_UPDATE.sh script
     (`Projects/B-U585I-IOT02A/SBSFU_Boot/STM32CubeIDE/SBSFU_UPDATE.sh`)
   - Unplug jumper JP3 on B-U585I-IOT02A board, press reset button (B2) and plug
     JP3 to run the application

## Performing firmware update

In order to perform firmware update:

1. Build the application and flash the board with `FIRMWARE_UPDATE` define set
   to the proper version (see [Flashing the board](#Flashing-the-board) step),
   e.g.
    ```
    #define FIRMWARE_VERSION "v1.0"
    ```
1. Make changes to the code (optionally), set `FIRMWARE_UPDATE` define to a
   different version, e.g.
    ```
    #define FIRMWARE_VERSION "v2.0"
    ```
    and build the application with a new firmware.
1. Upload the generated firmware file (`sbsfu_sign.bin`) to
   [Coiote DM](https://eu.iot.avsystem.cloud) (go to Device management and
   select `Firmware update`) and click `Upgrade`.
1. After the FOTA finishes, the device will reboot and the following log should
   appear:
    ```
    Firmware updated from version 'v1.0' to 'v2.0'
    ```
    where `v1.0` and `v2.0` will be set to firmware versions you set earlier.

## Disabling TrustZone

- Make sure that jumper JP3 is plugged
- Set the SW1 (BOOT0) to the "1" position - this will cause the MCU to boot in
  the DFU mode
- Connect your computer to the USB-C connector at the top (CN1);<br/>
  **NOTE**: the microUSB cable (CN8, ST-Link) needs to still be plugged in for
  power
- Reset the board
- Run:
  ```
  STM32_Programmer_CLI -c port=USB1 -tzenreg
  STM32_Programmer_CLI -c port=USB1 -ob RDP=0xDC
  STM32_Programmer_CLI -c port=USB1 -tzenreg -rdu
  ```
- Set SW1 back to the "0" position and reset the board
- Reset option bits to their default values:
  ```
  STM32_Programmer_CLI -c port=SWD -ob RDP=0xAA BOR_LEV=0x0 nRST_STOP=0x1 nRST_STDBY=0x1 nRST_SHDW=0x1 SRAM134_RST=0x1 IWDG_SW=0x1 IWDG_STOP=0x1 IWDG_STDBY=0x1 WWDG_SW=0x1 SWAP_BANK=0x0 DBANK=0x1 SRAM2_PE=0x1 SRAM2_RST=0x1 nSWBOOT0=0x1 nBOOT0=0x1 PA15_PUPEN=0x1 BKPRAM_ECC=0x1 SRAM3_ECC=0x1 SRAM2_ECC=0x1 IO_VDD_HSLV=0x0 IO_VDDIO2_HSLV=0x0 TZEN=0x0 NSBOOTADD0=0x100000 NSBOOTADD1=0x17F200 SECBOOTADD0=0x180000 BOOT_LOCK=0x0 SECWM1_PSTRT=0x0 SECWM1_PEND=0x7F HDP1_PEND=0x0 HDP1EN=0x0 WRP1A_PSTRT=0x7F WRP1A_PEND=0x0 UNLOCK_1A=0x1 WRP1B_PSTRT=0x7F WRP1B_PEND=0x0 UNLOCK_1B=0x1 SECWM2_PSTRT=0x0 SECWM2_PEND=0x7F HDP2_PEND=0x0 HDP2EN=0x0 WRP2A_PSTRT=0x7F WRP2A_PEND=0x0 UNLOCK_2A=0x1 WRP2B_PSTRT=0x7F WRP2B_PEND=0x0 UNLOCK_2B=0x1
  ```
