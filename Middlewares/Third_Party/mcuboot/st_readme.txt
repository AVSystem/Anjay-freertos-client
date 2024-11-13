t
  @verbatim
  ******************************************************************************
  *
  *         Portions COPYRIGHT 2016-2020 STMicroelectronics, All Rights Reserved
  *         Copyright (C) 2006-2015, ARM Limited, All Rights Reserved
  *
  * @file    st_readme.txt 
  * @author  MCD Application Team
  * @brief   This file lists the main modification done by STMicroelectronics on
  *          mcuboot for integration with STM32Cube solution.
  ******************************************************************************
  *
  * original licensing conditions
  * as issued by SPDX-License-Identifier: Apache-2.0
  *
  ******************************************************************************
  @endverbatim

### 2-Jun-2021 ###
========================
    + imgtool: Fix compatibility with click v8.0 package
    + imgtool: update windows executable with ST digital signature

### 18-May-2021 ###
========================
    + Add LICENSE.txt

### 1-March-2021 ###
==========================
    + Fix Keil compile warnings

### 29-January-2021 ###
==========================
    + Add logs for swap
    + Replace while (1) by Error_Handler() call for security
    + Fix compile warnings for no encryption config

### 11-December-2020 ###
==========================
    + Fix compile warnings for swap mode and no encryption mode
    + imgtool.exe with ST digital signature
    + imgtool : update window executable
    + Fix initial attestation key generation
    + Fix: swap support and without encrypt and primary_only config
    + mcuboot: fix no to decrypt empty slot
    + imgtool : add 16 bytes minimum write and clear option
    + Fix decrypt size during image copy when image TLV start in first chunk of 1 sector
    + Scripts: Fix after commit Remove key and evolution for regeneration of keys
    + Scripts: keygen ssl mode added
    + Scripts: Remove second batch of keys
    + Scripts: Remove key and evolution for regeneration of keys
    + Clean Encryption key, context cleaning after usage
    + Ensure cleanup of boot_status structure (inc. keys) at end of mcuboot process

### 20-October-2020 ###
==========================
    + fix buffer size for MUCBOOT_PRIMARY_ONLY
    + Add flow control check to ensure HW protections are enabled prior fw images access

### 25-August-2020 ###
==========================
    + imgtool: imgtool.exe is signed ST

### 23-June-2020 ###
==========================
    + imgtool: Cryptodome python module not required
    + imgtool: numpy python module added in requirements.txt

### 12-June-2020 ###
==========================
    + fix start offset of code injection check in padding area

### 11-June-2020 ###
==========================
    + fix to accept TLV DEPENDENCY in during TLV verification

### 02-June-2020 ###
==========================
    + mcuboot release v1.5.0
    + adaptation for BL2
    + imgtool: add flash and ass commands
    + imgtool: add otfdec support and primary-only image support
    + code adaptation for otfdec
    + fix don't check secure image with non secure key
    + fix for MCUBOOT_IMAGE_NUMBER = 1
    + Introduce switch to allow double signature check with single signature computation.
    + fix code injection in padding area.
    + fix compile warnings