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
 * @file    plf_features.h
 * @author  MCD Application Team
 * @brief   Includes feature list to include in firmware
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
#ifndef PLF_FEATURES_H
#    define PLF_FEATURES_H

#    ifdef __cplusplus
extern "C" {
#    endif

/* Includes ------------------------------------------------------------------*/
#    include "plf_custom_config.h"

/* Exported constants --------------------------------------------------------*/

/* ================================================= */
/*          USER MODE                                */
/* ================================================= */

/* ===================================== */
/* BEGIN - Cellular data mode            */
/* ===================================== */

/* Possible values for USE_SOCKETS_TYPE */
#    define USE_SOCKETS_LWIP \
        (0) /* define value affected to LwIP sockets type  */
#    define USE_SOCKETS_MODEM \
        (1) /* define value affected to Modem sockets type */

/* Sockets location */
#    if !defined USE_SOCKETS_TYPE
#        define USE_SOCKETS_TYPE (USE_SOCKETS_MODEM)
#    endif /* !defined USE_SOCKETS_TYPE */

/* ===================================== */
/* END - Cellular data mode              */
/* ===================================== */

/* ===================================== */
/* BEGIN - Applications to include       */
/* ===================================== */

/* ===================================== */
/* END   - Applications to include       */
/* ===================================== */

/* ======================================= */
/* BEGIN -  Miscellaneous functionalities  */
/* ======================================= */

/* ======================================= */
/* END   -  Miscellaneous functionalities  */
/* ======================================= */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#    ifdef __cplusplus
}
#    endif

#endif /* PLF_FEATURES_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
