/**
  ******************************************************************************
  * @file    com_sockets_addr_compat.h
  * @author  MCD Application Team
  * @brief   Com sockets address definition for compatibility LwIP / Modem
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef COM_SOCKETS_ADDR_COMPAT_H
#define COM_SOCKETS_ADDR_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

#include "plf_config.h"

#include <stdbool.h>
#include <stdint.h>

#include "com_common.h"

/* Common Exported constants -------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Constants
  * @{
  */

/** @note Next define is to ensure compatibility with previous release */
#define COM_SOCKETS_FALSE  false
/** @note Next define is to ensure compatibility with previous release */
#define COM_SOCKETS_TRUE   true

/**
  * @}
  */

/* Common Exported types -----------------------------------------------------*/
/** @addtogroup COM_PING_Types
  * @{
  */

#if (USE_COM_PING == 1)

typedef struct
{
  int32_t  status; /*!< if COM_SOCKETS_ERR_OK then time and size are set */
  uint32_t time;   /*!< time wait for the response of the ping request. (in ms) */
  uint16_t size;   /*!< length of each sent ping request. (in bytes) */
  uint8_t  ttl;    /*!< Time to live */
} com_ping_rsp_t;

#endif /* USE_COM_PING == 1 */

/**
  * @}
  */

/* IP address definition is needed for cellular_control_apih.h */
#if ((USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) || (USE_COM_SOCKETS == 0))

/* Exported constants --------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Constants
  * @{
  */

#define COM_SOCKETS_LITTLE_ENDIAN (0)
#define COM_SOCKETS_BIG_ENDIAN    (1)

#ifndef COM_SOCKETS_BYTE_ORDER
#define COM_SOCKETS_BYTE_ORDER COM_SOCKETS_LITTLE_ENDIAN
#endif /* COM_SOCKETS_BYTE_ORDER */

#define COM_SIN_ZERO_LEN  8

/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Types
  * @{
  */

typedef struct
{
  uint32_t addr;
} com_ip4_addr_t;

typedef struct
{
  uint32_t s_addr;
} com_in_addr_t;

typedef com_ip4_addr_t com_ip_addr_t;

typedef struct
{
  uint8_t    sa_len;
  uint8_t    sa_family;
  com_char_t sa_data[14];
} com_sockaddr_t;

/* IPv4 Socket Address structure */
typedef struct
{
  uint8_t       sin_len;
  uint8_t       sin_family;
  uint16_t      sin_port;
  com_in_addr_t sin_addr;
  com_char_t    sin_zero[COM_SIN_ZERO_LEN];
} com_sockaddr_in_t;

/**
  * @}
  */

/* External variables --------------------------------------------------------*/
/* None */

/* Exported macros -----------------------------------------------------------*/
/** @addtogroup COM_SOCKETS_Macros
  * @{
  */

#if (COM_SOCKETS_BYTE_ORDER == COM_SOCKETS_BIG_ENDIAN)

#define COM_HTONL(x)   (x)
#define COM_NTOHL(x)   (x)
#define COM_HTONS(x)   (x)
#define COM_NTOHS(x)   (x)

#else /* COM_SOCKETS_BYTE_ORDER != COM_SOCKETS_BIG_ENDIAN */

#define COM_HTONL(x)   ((uint32_t)(((x) & 0xff000000U) >> 24) | \
                        (uint32_t)(((x) & 0x00ff0000U) >> 8) | \
                        (uint32_t)(((x) & 0x0000ff00U) << 8) | \
                        (uint32_t)(((x) & 0x000000ffU) << 24))

#define COM_NTOHL(x)    COM_HTONL(x)

#define COM_HTONS(x)    ((uint16_t)(((x) & 0xff00U) >> 8) | \
                         (uint16_t)(((x) & 0x00ffU) << 8))

#define COM_NTOHS(x)    COM_HTONS(x)

#endif /* COM_SOCKETS_BYTE_ORDER == COM_SOCKETS_BIG_ENDIAN */

/* Format an uin32_t address */
#define COM_IP4_ADDR(ipaddr, a,b,c,d) \
  (ipaddr)->addr = COM_HTONL(((uint32_t)((uint32_t)((a) & 0xffU) << 24) | \
                              (uint32_t)((uint32_t)((b) & 0xffU) << 16) | \
                              (uint32_t)((uint32_t)((c) & 0xffU) << 8)  | \
                              (uint32_t)((uint32_t)((d) & 0xffU))))

/* Get one byte from the 4-byte address */
#define COM_IP4_ADDR_GET_BYTE(ipaddr, idx) (((const uint8_t*)(&(ipaddr)->addr))[(idx)])
#define COM_IP4_ADDR1(ipaddr) COM_IP4_ADDR_GET_BYTE((ipaddr), 0U)
#define COM_IP4_ADDR2(ipaddr) COM_IP4_ADDR_GET_BYTE((ipaddr), 1U)
#define COM_IP4_ADDR3(ipaddr) COM_IP4_ADDR_GET_BYTE((ipaddr), 2U)
#define COM_IP4_ADDR4(ipaddr) COM_IP4_ADDR_GET_BYTE((ipaddr), 3U)

/* Get one byte from the 4-byte address, but argument is 'com_ip4_addr_t' not a pointer */
#define COM_IP4_ADDR_GET_BYTE_VAL(ipaddr, idx) ((uint8_t)(((ipaddr).addr >> ((idx) * 8U)) & 0xFFU))
#define COM_IP4_ADDR1_VAL(ipaddr) COM_IP4_ADDR_GET_BYTE_VAL((ipaddr), 0U)
#define COM_IP4_ADDR2_VAL(ipaddr) COM_IP4_ADDR_GET_BYTE_VAL((ipaddr), 1U)
#define COM_IP4_ADDR3_VAL(ipaddr) COM_IP4_ADDR_GET_BYTE_VAL((ipaddr), 2U)
#define COM_IP4_ADDR4_VAL(ipaddr) COM_IP4_ADDR_GET_BYTE_VAL((ipaddr), 3U)

/**
  * @}
  */

/* Exported functions ------------------------------------------------------- */
/* None */

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM) || (USE_COM_SOCKETS == 0) */


#if ((USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) && (USE_COM_SOCKETS == 1))

/* Includes ------------------------------------------------------------------*/
/* only when USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */
/* LwIP is a Third Party so MISRAC messages linked to it are ignored */

/*cstat -MISRAC2012-* */
#include "lwip/sockets.h"
#include "lwip/ip4.h"
/*cstat +MISRAC2012-* */

/* Exported constants --------------------------------------------------------*/
#define COM_SIN_ZERO_LEN  SIN_ZERO_LEN

/* Exported types ------------------------------------------------------------*/

/* Re-Route com define to LwIP define */
typedef ip4_addr_t      com_ip4_addr_t;
typedef ip_addr_t       com_ip_addr_t;
typedef in_addr_t       com_in_addr_t;
typedef struct sockaddr com_sockaddr_t;       /* CodingRules: Rule-12 Typedef struct name does not end with '_t' is ok
                                                 when struct comes from Third Party */
typedef struct sockaddr_in com_sockaddr_in_t; /* CodingRules: Rule-12 Typedef struct name does not end with '_t' is ok
                                                 when struct comes from Third Party */

/* External variables --------------------------------------------------------*/

/* Exported macros -----------------------------------------------------------*/
#define COM_HTONL(A)   htonl((A))
#define COM_NTOHL(A)   ntonl((A))
#define COM_HTONS(A)   htons((A))
#define COM_NTOHS(A)   htons((A))

#define COM_IP4_ADDR(ipaddr, a,b,c,d) IP4_ADDR((ipaddr), (a),(b),(c),(d))

/* Get one byte from the 4-byte address */
#define COM_IP4_ADDR1(ipaddr) ip4_addr1((ipaddr))
#define COM_IP4_ADDR2(ipaddr) ip4_addr2((ipaddr))
#define COM_IP4_ADDR3(ipaddr) ip4_addr3((ipaddr))
#define COM_IP4_ADDR4(ipaddr) ip4_addr4((ipaddr))

/* Get one byte from the 4-byte address, but argument is 'ip4_addr_t' not a pointer */
#define COM_IP4_ADDR1_VAL(ipaddr) ip4_addr1_val((ipaddr))
#define COM_IP4_ADDR2_VAL(ipaddr) ip4_addr2_val((ipaddr))
#define COM_IP4_ADDR3_VAL(ipaddr) ip4_addr3_val((ipaddr))
#define COM_IP4_ADDR4_VAL(ipaddr) ip4_addr4_val((ipaddr))

/* Exported functions ------------------------------------------------------- */

#endif /* (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP) && (USE_COM_SOCKETS == 1) */

#ifdef __cplusplus
}
#endif

#endif /* COM_SOCKETS_ADDR_COMPAT_H */
