# Changelog

## 23.09 (Sep 7th, 2023)

### Features
- (commercial feature) Added support Anjay Core Persistence
- (commercial feature) Added support for SIM Bootstrap for STM32L496G-DISCOVERY with BG96 project
- Added persistence support of Security object, Server object and Attribute
  Storage, controllable through shell
- B-U585I-IOT02A and B-L462E-CELL1 targets now use external flash as storage for
  runtime configuration instead of EEPROM
- MONARCH project removed due to suspended X-Cube-Cellular support
- Added TCP support, both for management connection and CoAP+TCP FOTA downloads
- Added the Current Time resource in the Device object, and implemented real time synchronization using the cellular modem features

### Improvements
- Updated X-Cube-Cellular to version 7.1.0
- Updated Mbed TLS to version 3.4.1 and enabled DTLS Connection ID support
- Enabled LwM2M 1.1 by default
- Introduced various flash footprint optimizations
- Added support for the User Button on B-U585I-IOT02A

### Bugfixes
- Fixed B-U585I-IOT02A + BG96 demo app freezing in Release mode

## 23.02 (Feb 21st, 2023)

### Features
- Reworked socket implementation to use X-Cube-Cellular offloaded sockets
  (Com sockets) instead of LwIP (NOTE: currently supporting only UDP)
- Added Firmware Update Object (/5) to STM32L496G-DISCOVERY and B-L462E-CELL1 projects

### Improvements
- Revamped configuration of Anjay and its dependencies
- Updated Anjay to version 3.3.0

## 22.09 (Sep 21st, 2022)

### Features
- Added preliminary support for B-U585I-IOT02A board
- Added TinyML AI Bridging Protocol example running on B-L462E-CELL1 discovery kit

### Improvements
- Updated Anjay to version 3.1.2
- Updated X-Cube-Cellular to version 7.0.0
- Revamped project structure to correctly include core type-dependent sources
- Added backspace handling in serial port interface configuration menu

## 21.10 (Oct 7th, 2021)

### Improvements
- Updated Anjay to version 2.14.0
  - Used pre-implemented event loop
  - Provided threading compatiblity sources
  - Used IPSO Objects API to implement sensors and push button
- Changed displayed sensor units


## 21.06 (Jun 21st, 2021)

### Features
- Support for B-L462E-CELL1 discovery kit
- Support for Monarch modem on P-L496G-CELL02 discovery kit
- APN configuration t menu

### Improvements
- Updated Anjay to version 2.12.0
- Updated X-Cube-Cellular to version 6.0.0

## 20.09 (Sep 7th, 2020)

### Features
- Initial release
