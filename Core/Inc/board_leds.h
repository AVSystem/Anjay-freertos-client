/*
 * Copyright 2020 AVSystem <avsystem@avsystem.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 ******************************************************************************
 * @file    board_leds.h
 * @author  MCD Application Team
 * @brief   Header for board_leds.c module
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef BOARD_LEDS_H
#    define BOARD_LEDS_H

#    ifdef __cplusplus
extern "C" {
#    endif

/* Includes ------------------------------------------------------------------*/
#    include <stdint.h>

/* include BSP for disco-L496 */
/* MISRAC messages linked to BSP include are ignored */
/*cstat -MISRAC2012-* */
#    include "stm32l496g_discovery.h"
#    include "stm32l496g_discovery_io.h"
/*cstat +MISRAC2012-* */

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/

/* Exported functions ------------------------------------------------------- */
void BL_LED_Init(uint8_t bl_led);
void BL_LED_Off(uint8_t bl_led);
void BL_LED_On(uint8_t bl_led);

#    ifdef __cplusplus
}
#    endif

#endif /* BOARD_LEDS_H */
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
