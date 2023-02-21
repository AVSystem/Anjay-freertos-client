/**
  @page Secure Boot and Secure Firmware Update Demo Application

  @verbatim
  ******************** (C) COPYRIGHT 2017 STMicroelectronics *******************
  * @file    readme.txt
  * @brief   This application shows Secure Boot and Secure Firmware Update example.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  @endverbatim

@par Application Description

The Secure Boot (SB) and Secure Firmware Update (SFU) solution allows the update of the STM32 microcontroller built-in
program with new firmware versions, adding new features and correcting potential issues. The update process is performed
in a secure way to prevent unauthorized updates and access to confidential on-device data such as secret code and
firmware encryption key.
In addition, Secure Boot (Root of Trust services) checks and activates the STM32 security mechanisms, and checks the
authenticity and integrity of user application code before every execution to ensure that invalid or malicious code
cannot be run. The Secure Firmware Update application receives the encrypted firmware image, decrypts it, then checks
the authenticity and integrity of the code before installing it.

This example supports multiple images feature configured with 2 active images for execution from internal flash and 2
download areas located in external flash. To ensure the firmware confidentiality in external flash the installation is
performed without swap between active slot and download area. The download area is kept encrypted.
This example allows to demonstrate features like:
   - download a new firmware image from the application
   - resume firmware update procedure in case of power off during installation

For more details, refer to UM2262 "Getting started with SBSFU - software expansion for STM32Cube" available from the
STMicroelectronics microcontroller website www.st.com.

@par Directory contents

   - 2_Images_SBSFU/Core/Src/main.c                           Main application file
   - 2_Images_SBSFU/Core/Src/stm32l4xx_hal_msp.c              HAL MSP module
   - 2_Images_SBSFU/Core/Src/stm32l4xx_it.c                   STM32 interrupt handlers
   - 2_Images_SBSFU/Core/Inc/app_hw.h                         Hardware definition for application
   - 2_Images_SBSFU/Core/Inc/main.h                           Header file for main.c
   - 2_Images_SBSFU/Core/Inc/stm32l4xx_hal_conf.h             HAL configuration file
   - 2_Images_SBSFU/Core/Inc/stm32l4xx_it.h                   Header file for stm32l4xx_it.c
   - 2_Images_SBSFU/SBSFU/App/sfu_boot.c                      Secure Boot (SB): entry/exit points and state machine
   - 2_Images_SBSFU/SBSFU/App/sfu_com_loader.c                SBSFU communication module: local loader part
   - 2_Images_SBSFU/SBSFU/App/sfu_com_trace.c                 SBSFU communication module: trace part
   - 2_Images_SBSFU/SBSFU/App/sfu_error.c                     SBSFU errors management
   - 2_Images_SBSFU/SBSFU/App/sfu_fwimg_common.c              SBSFU image handling: common functionalities/services
   - 2_Images_SBSFU/SBSFU/App/sfu_fwimg_swap.c                SBSFU image handling: FW upgrade without swap area services
   - 2_Images_SBSFU/SBSFU/App/sfu_fwimg_no_swap.c             SBSFU image handling: FW upgrade with swap area services
   - 2_Images_SBSFU/SBSFU/App/sfu_loader.c                    SBSFU Local Loader
   - 2_Images_SBSFU/SBSFU/App/sfu_new_image.c                 SBSFU image handling: new image storage and installation request
   - 2_Images_SBSFU/SBSFU/App/sfu_test.c                      SBSFU security protection automatic test
   - 2_Images_SBSFU/SBSFU/App/app_sfu.h                       Software configuration of SBSFU application
   - 2_Images_SBSFU/SBSFU/App/sfu_boot.h                      Header file for sfu_boot.c
   - 2_Images_SBSFU/SBSFU/App/sfu_com_loader.h                Header file for sfu_com_loader.c
   - 2_Images_SBSFU/SBSFU/App/sfu_com_trace.h                 Header file for sfu_com_trace.c
   - 2_Images_SBSFU/SBSFU/App/sfu_def.h                       General definition for SBSFU application
   - 2_Images_SBSFU/SBSFU/App/sfu_error.h                     Header file for sfu_error.c file
   - 2_Images_SBSFU/SBSFU/App/sfu_fsm_states.h                SBSFU FSM states definitions
   - 2_Images_SBSFU/SBSFU/App/sfu_fwimg_internal.h            Internal definitions for firmware image handling
   - 2_Images_SBSFU/SBSFU/App/sfu_fwimg_regions.h             FLASH regions definitions for image handling
   - 2_Images_SBSFU/SBSFU/App/sfu_fwimg_services.h            Header file for sfu_fwimg_services.c
   - 2_Images_SBSFU/SBSFU/App/sfu_loader.h                    Header file for sfu_loader.c
   - 2_Images_SBSFU/SBSFU/App/sfu_new_image.h                 Header file for sfu_new_image.c
   - 2_Images_SBSFU/SBSFU/App/sfu_standalone_loader.h         Interface through shared memory with standalone loader
   - 2_Images_SBSFU/SBSFU/App/sfu_test.h                      Header file for sfu_test.c
   - 2_Images_SBSFU/SBSFU/App/sfu_trace.h                     Header file for sfu_trace.c
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level.c              SBSFU general low level interface
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_flash.c        SBSFU flash low level interface (wrapper)
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_flash_int.c    SBSFU internal flash low level interface
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_flash_ext.c    SBSFU external flash low level interface
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_security.c     SBSFU security low level interface
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level.h              Header file for general low level interface
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_flash.h        Header file for flash low level interface (wrapper)
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_flash_int.h    Header file for internal flash low level interface
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_flash_ext.h    Header file for external flash low level interface
   - 2_Images_SBSFU/SBSFU/Target/sfu_low_level_security.h     Header file for security low level interface

@par Hardware and Software environment

   - This example runs on STM32L475xx devices.
   - This example has been tested with B-L475E-IOT01 board and can be easily tailored to any other supported device and
     development board.
   - An up-to-date version of ST-LINK firmware is required. Upgrading ST-LINK firmware is a feature provided by
     STM32Cube programmer available on www.st.com.
   - This example is linked with SE_Core binary generated by Secure Engine Core binary generation project.
   - This example needs a terminal emulator.
   - By default, SBSFU is configured in development mode. To switch in production mode, refer to AN5056 "Integration
     guide - software expansion for STM32Cube" available from the STMicroelectronics microcontroller website
     www.st.com.
   - Microsoft Windows has a limitation whereby paths to files and directories cannot be longer than 256 characters.
     Paths to files exceeding that limits cause tools (e.g. compilers, shell scripts) to fail reading from or writing
     to such files.
     As a workaround, it is advised to use the subst.exe command from within a command prompt to set up a local drive
     out of an existing directory on the hard drive, such as:
     C:\> subst X: <PATH_TO_CUBEFW>\Firmware

@par How to use it ?

Severals steps to run SBSFU application :

1. Compile projects in the following order. This is mandatory as each project requests some objects generated by the
   compilation of the previous one:
   - 2_Images_SECoreBin (see also SECoreBin/readme.txt)
   - 2_Images_SBSFU
   - 2_Images_UserApp (see also UserApp/readme.txt)

2. Before loading SBSFU image into the target, please ensure with STM32CubeProgammer available on www.st.com that the
   following are valid for the device:
   - RDP Level 0
   - PCROP disabled
   - Write Protection disabled on all FLASH pages
   - BFB2 bit disabled
   - Chip has been erased

3. Use a terminal emulator (Tera Term for example, open source free software terminal emulator that can be downloaded
   from https://osdn.net/projects/ttssh2/) for UART connection with the board.
   Support of YMODEM protocol is required. Serial port configuration should be :
   - Baud rate = 115200
   - Data = 8 bits
   - Parity = none
   - Stop = 1 bit
   - Flow control = none

4. Load SBSFU image into target memory with your preferred toolchain or STM32CubeProgammer.

5. Once the SB_SFU software is downloaded, power cycle the board (unplug/plug USB cable) : the SBSFU application starts
   and configures the security mechanisms.

6. Power cycle the board a second time (unplug/plug the USB cable): the SBSFU application starts with the configured
   securities turned on and the Tera Term connection is possible.
   Caution: Make sure to use an up-to-date version of ST-LINK firmware else SBSFU may not start.

7. At startup (Power On or Reset button pushed) welcome information are displayed on terminal emulator.
   Green LED blinks every 3 seconds when a local download is waited.
   Green LED blinks every 250 ms in case of error in option bytes configuration.

8. Send the user encrypted firmware file (\2_Images\2_Images_UserApp\Binary\UserApp.sfb) with Tera Term by using menu
   "File > Transfer > YMODEM > Send..."

Note1 : Press User push-button at reset to force a local download if an application is already installed.
Note2 : TAMPER detection can be very sensitive. Protection may be disabled if too many reset occur during tests.
Note3 : for Linux users Minicom can be used but to do so you need to compile the SBSFU project with the MINICOM_YMODEM
        switch enabled (app_sfu.h)

