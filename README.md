# Anjay-freertos-client [<img align="right" height="50px" src="https://avsystem.github.io/Anjay-doc/_images/avsystem_logo.png">](http://www.avsystem.com/)


## Supported hardware and overview

This repository contains LwM2M Client application based on open-source [Anjay](https://github.com/AVSystem/Anjay) library and [X-Cube-Cellular](https://www.st.com/en/embedded-software/x-cube-cellular.html) package which includes cellular modem drivers, [FreeRTOS](https://www.freertos.org) as well as [mbedtls](https://github.com/ARMmbed/mbedtls). This example supports the following configurations:

| Project Path | Board | Modem |
|--------------|-------|-------|
| Projects/STM32L496G-DISCO/UserApp/BG96 | [P-L496G-CELL02 Discovery kit](https://www.st.com/en/evaluation-tools/p-l496g-cell02.html) | [Quectel BG96](https://www.quectel.com/product/lpwa-bg96-cat-m1-nb1-egprs) default modem provided with kit |
| Projects/B-L462E-CELL1/UserApp/TYPE1SC | [B-L462E-CELL1 Discovery kit](https://www.st.com/en/evaluation-tools/b-l462e-cell1.html) | [Murata TYPE 1SE](https://www.murata.com/en-eu/products/connectivitymodule/lpwa/overview/lineup/type-1se) module with built-in eSIM (ST4SIM-200M)|
| Projects/B-L462E-CELL1/UserApp/TYPE1SC-AIBP | [B-L462E-CELL1 Discovery kit](https://www.st.com/en/evaluation-tools/b-l462e-cell1.html) | [Murata TYPE 1SE](https://www.murata.com/en-eu/products/connectivitymodule/lpwa/overview/lineup/type-1se) module with built-in eSIM (ST4SIM-200M) <br /> Supports AI Bridging Protocol.|
| Projects/B-U585I-IOT02A/UserApp/BG96<br> | [B-U585I-IOT02A Discovery kit](https://www.st.com/en/evaluation-tools/b-u585i-iot02a.html) | [Quectel BG96](https://www.quectel.com/product/lpwa-bg96-cat-m1-nb1-egprs) (provided with P-L496G-CELL02 devkit)

The project was developed using [STM32Cube tools](https://www.st.com/en/ecosystems/stm32cube.html) ([STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) in particular).

The following LwM2M Objects are supported:

- Security (/0),
- Server (/1),
- Device (/3),
- Firmware Update (/5),
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

 - Download [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) version 1.12.1
 - Import the cloned project to your workspace
 - In the Project Explorer navigate to one of the [Project Path] taken from the table above, depending on the hardware you use.
 - To build and run the project, please follow the instructions in
   `Projects/<board_name>/README.md` files.

## Connecting to the LwM2M Server

To connect to [Coiote IoT Device Management](https://www.avsystem.com/products/coiote-iot-device-management-platform/) LwM2M Server, please register at https://eu.iot.avsystem.cloud/. Then have a look at the Configuration menu to configure security credentials.

If you use BG96-based configuration, you must upgrade the firmware of the modem to at least `BG96MAR02A08M1G` revision. Older versions may cause an unexpected loss of connection.

[Guide showing basic usage of Coiote DM](https://iotdevzone.avsystem.com/docs/Coiote_IoT_DM/Quick_Start/Connect_device_quickstart/) is available on IoT Developer Zone.


## Configuration menu

While connected to a serial port interface, and during bootup, the device shows:

```
Press any key in 3 seconds to enter config menu...
```

You can then press any key on your keyboard to enter the configuration menu. After that, you'll see a few configuration options that can be altered and persisted within the flash memory for future bootups.

## Module persistence

This application features persistence of the Security object, Server object and
client's Attribute Storage to retain the configuration of those modules over
power cycles, to e.g. keep bootstrapped server configuration.

To enable module persistence, set properly "Use persistence" option in the
shell config menu. The application will attempt to load those modules from a
non-volatile memory. Configuration of those modules from the shell will be
ignored unless the application fails to restore the state from the memory.

The persisted state of the aforementioned modules may be cleared using the
"Clear module persistence" option.

## Core Persistence (commercial feature)

This application features **Core Persistence** that allows to retain the core
library state over power cycles, to e.g. keep the TLS session valid.

**This feature is available only with releases of Anjay that include the *Core
Persistence* commercial feature.** To use the Core Persistence feature, make
sure that the `ANJAY_WITH_CORE_PERSISTENCE` option is enabled, which requires
also to enable `ANJAY_WITH_OBSERVE` in `Application/Inc/anjay/anjay_config.h`,
`AVS_COMMONS_WITH_AVS_PERSISTENCE` in
`Application/Inc/avsystem/commons/avs_commons_config.h` and
`WITH_AVS_COAP_OBSERVE_PERSISTENCE` in
`Application/Inc/avsystem/coap/avs_coap_config.h`

If the Core Persistence is compiled in, it works on a similar basis in terms of
enabling/disabling and clearing it as the module persistence.

## SIM Bootstrap (commercial feature)

`STM32L496G-DISCO-BG96` and `B-U585I-IOT02A/BG96` projects now feature the
**SIM Bootstrap** that allows bootstrapping the device using the SIM Smartcard.
**This feature is available only with releases of Anjay that include the
*bootstrapper* commercial feature.** To use the SIM Bootstrap feature, define
`USE_SIM_BOOTSTRAP` in the `STM32L496G-DISCO-BG96` or `B-U585I-IOT02A-BG96`
project settings and make sure that the `ANJAY_WITH_MODULE_BOOTSTRAPPER` and
`ANJAY_WITH_MODULE_SIM_BOOTSTRAP` options are enabled in
`Application/Inc/anjay/anjay_config.h`.

## SMS Trigger (commercial feature)

Every project now features the **SMS Trigger**. **This feature is available
only with releases of Anjay that include the *SMS* commercial feature.** To use
the SMS Trigger feature, define `USE_SMS_TRIGGER` in the project settings and
make sure that the `ANJAY_WITH_SMS` option is enabled in
`Application/Inc/anjay/anjay_config.h`.

To enable the SMS trigger, set the "Use SMS trigger" option to `y` in the shell
config menu. Then set local's and server's MSISDN numbers properly.
