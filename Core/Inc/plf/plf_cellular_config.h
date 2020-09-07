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
 * @file    plf_cellular_config.h
 * @author  MCD Application Team
 * @brief   Includes cellular configuration
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
#ifndef PLF_CELLULAR_CONFIG_H
#    define PLF_CELLULAR_CONFIG_H

#    ifdef __cplusplus
extern "C" {
#    endif

/* Includes ------------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** @addtogroup PLF_CELLULAR_CONFIG_Constants
 * @{
 */

/** @note Cellular config parameters */
#    define PLF_CELLULAR_SIM_SLOT ((uint8_t *) "0") //!< SIM SLOT selected
//"0": MODEM SOCKET (default)
//"1": EMBEDDED_SIM)

#    define PLF_CELLULAR_APN ((uint8_t *) "Plus") /*!< APN */
#    define PLF_CELLULAR_CID ((uint8_t *) "1")    /*!< CID ("1"-"9") */
#    define PLF_CELLULAR_USERNAME \
        ((uint8_t *) "") /*!< User name  ( "": No Authentication) */
#    define PLF_CELLULAR_PASSWORD \
        ((uint8_t *) "") /*!< Password   ( "": No Authentication) */
#    define PLF_CELLULAR_TARGET_STATE \
        ((uint8_t *) "2") //!< Modem target state
                          //"0": modem off
                          //"1": SIM only
//"2": Full data transfert enabled (default)

#    define PLF_CELLULAR_ATTACHMENT_TIMEOUT \
        ((uint8_t *) "180000") /*!< Attachment timeout in ms (3 minutes) */

#    define PLF_CELLULAR_NFMC_ACTIVATION \
        ((uint8_t *) "0") //!< NFMC activation
                          //"0": NFMC disabled (default)
                          //"1": NFMC enabled

#    define PLF_CELLULAR_NFMC_TEMPO1 ((uint8_t *) "60000")  //!< NFMC value 1
#    define PLF_CELLULAR_NFMC_TEMPO2 ((uint8_t *) "120000") //!< NFMC value 2
#    define PLF_CELLULAR_NFMC_TEMPO3 ((uint8_t *) "240000") //!< NFMC value 3
#    define PLF_CELLULAR_NFMC_TEMPO4 ((uint8_t *) "480000") //!< NFMC value 4
#    define PLF_CELLULAR_NFMC_TEMPO5 ((uint8_t *) "960000") //!< NFMC value 5
#    define PLF_CELLULAR_NFMC_TEMPO6 \
        ((uint8_t *) "1920000") /*!< NFMC value 6 */
#    define PLF_CELLULAR_NFMC_TEMPO7 \
        ((uint8_t *) "3840000") /*!< NFMC value 6 */

/* Exported types ------------------------------------------------------------*/
/* External variables --------------------------------------------------------*/
/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#    ifdef __cplusplus
}
#    endif

#endif /* PLF_CELLULAR_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
