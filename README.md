# Anjay-freertos-client [<img align="right" height="50px" src="https://avsystem.github.io/Anjay-doc/_images/avsystem_logo.png">](http://www.avsystem.com/)


## Supported hardware and overview

This repository contains LwM2M Client application based on open-source [Anjay](https://github.com/AVSystem/Anjay) library and [FreeRTOS](https://www.freertos.org) as well as [ARM mbedtls](https://github.com/ARMmbed/mbedtls). This example targets [P-L496G-CELL02 Discovery kit](https://www.st.com/en/evaluation-tools/p-l496g-cell02.html). The project was developed using [STM32Cube tools](https://www.st.com/en/ecosystems/stm32cube.html) ([STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) in particular).

The following LwM2M Objects are supported:

- Security (/0),
- Server (/1),
- Access Control (/2),
- Device (/3),
- Temperature (/3303),
- Humidity (/3304),
- Accelerometer (/3313),
- Magnetometer (/3314),
- Barometer (/3315),
- Gyrometer (/3334),
- Multiple Axis Joystick (/3345).


## Cloning the repository

```
git clone --recursive https://github.com/AVSystem/Anjay-freertos-client
```

## Compilation guide

 - Download [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
 - Import the cloned project to your workspace
 - Setup Debug/Release Run configurations
 - Build and run the application

## Connecting to the LwM2M Server

To connect to [Coiote IoT Device Management](https://www.avsystem.com/products/coiote-iot-device-management-platform/) LwM2M Server, please register at https://www.avsystem.com/try-anjay/. Then have a look at the Configuration menu to configure security credentials.

You must upgrade the firmware of BG96 modem to at least `BG96MAR02A07M1G` revision. Older versions may cause unexpected loss of connection.

[Video guide showing basic usage of Try-Anjay](https://www.youtube.com/watchv=fgy38XfttM8) is available on YouTube.


## Configuration menu

While connected to a serial port interface, and during bootup, the device shows:

```
Press any key in 3 seconds to enter config menu...
```

You can then press any key on your keyboard to enter the configuration menu. After that you'll see a few configuration options that can be altered and persisted within the flash memory for future bootups.
