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
 * @file    plf_config.h
 * @author  MCD Application Team
 * @brief   This file contains the common defines of the application
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
#ifndef PLF_CONFIG_H
#    define PLF_CONFIG_H

#    ifdef __cplusplus
extern "C" {
#    endif

/* Includes ------------------------------------------------------------------*/

/* Specific project Includes -------------------------------------------------*/
#    include "plf_custom_config.h"

/* Common projects Includes --------------------------------------------------*/
#    include "plf_features.h"
#    include "plf_hw_config.h"
#    include "plf_sw_config.h"
#    include "plf_thread_config.h"

#    ifdef __cplusplus
}
#    endif

#endif /* PLF_CONFIG_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
