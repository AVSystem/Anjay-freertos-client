/**
  ******************************************************************************
  * @file    com_sockets_ip_modem.c
  * @author  MCD Application Team
  * @brief   This file implements Socket IP on MODEM side
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

/* Includes ------------------------------------------------------------------*/
#include "com_sockets_ip_modem.h"

#if (USE_COM_SOCKETS == 1)

#if (USE_SOCKETS_TYPE == USE_SOCKETS_MODEM)

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "rtosal.h"

#include "com_sockets_net_compat.h"
#include "com_sockets_err_compat.h"
#include "com_sockets_statistic.h"
#include "com_trace.h"

#include "cellular_service_os.h"
#if (USE_LOW_POWER == 1)
#include "cellular_service_power.h"
#endif /* USE_LOW_POWER == 1 */

#include "dc_common.h"
#include "cellular_service_datacache.h"

/* Private defines -----------------------------------------------------------*/

/* Maximum data that can be passed between COM and low level */
/* Use low level define according to the modem and hardware capabilities */
#define COM_MODEM_MAX_TX_DATA_SIZE CONFIG_MODEM_MAX_SOCKET_TX_DATA_SIZE
#define COM_MODEM_MAX_RX_DATA_SIZE CONFIG_MODEM_MAX_SOCKET_RX_DATA_SIZE

#define COM_SOCKET_LOCAL_ID_NB 1U /* Socket local id number : 1 for ping */

#define COM_LOCAL_PORT_BEGIN  0xc000U /* 49152 */
#define COM_LOCAL_PORT_END    0xffffU /* 65535 */

#if (USE_LOW_POWER == 1)
#define COM_TIMER_INACTIVITY_MS 10000U /* in ms */
#endif /* USE_LOW_POWER == 1 */

/* Private typedef -----------------------------------------------------------*/
typedef char CSIP_CHAR_t; /* used in stdio.h and string.h service call */

/* Message exchange between callback and sockets */
typedef uint16_t com_socket_msg_type_t;
#define COM_SOCKET_MSG        (com_socket_msg_type_t)1    /* MSG is SOCKET type       */
#define COM_PING_MSG          (com_socket_msg_type_t)2    /* MSG is PING type         */

typedef uint16_t com_socket_msg_id_t;
#define COM_DATA_RCV          (com_socket_msg_id_t)1      /* MSG id is DATA_RCV       */
#define COM_CLOSING_RCV       (com_socket_msg_id_t)2      /* MSG id is CLOSING_RCV    */

/* Message Description: com_socket_msg_t
com_socket_msg_type_t type (uint16_t)
com_socket_msg_id_t   id   (uint16_t)
*/
typedef uint32_t com_socket_msg_t;

/* Socket State */
typedef enum
{
  COM_SOCKET_INVALID = 0,
  COM_SOCKET_CREATING,
  COM_SOCKET_CREATED,
  COM_SOCKET_CONNECTED,
  COM_SOCKET_SENDING,
  COM_SOCKET_WAITING,
  COM_SOCKET_CLOSING
} com_socket_state_t;

/* Socket descriptor data structure */
typedef struct _socket_desc_t
{
  com_socket_state_t    state;       /* socket state            */
  bool                  closing;     /* close recv from remote  */
  uint8_t               type;        /* Socket Type TCP/UDP/RAW */
  int32_t               error;       /* last command status     */
  int32_t               id;          /* identifier              */
  uint16_t              local_port;  /* local port              */
  uint16_t              remote_port; /* remote port             */
  com_ip_addr_t         remote_addr; /* remote addr             */
  uint32_t              snd_timeout; /* timeout for send cmd    */
  uint32_t              rcv_timeout; /* timeout for receive cmd */
  osMessageQId          queue;       /* message queue for URC   */
#if (USE_COM_PING == 1)
  com_ping_rsp_t        *p_ping_rsp; /* pointer on ping rsp     */
#endif /* USE_COM_PING == 1 */
  struct _socket_desc_t *p_next;     /* chained list            */
} socket_desc_t;

typedef struct
{
  CS_IPaddrType_t ip_type; /* possible values: IPv4 or IPv6 - only IPv4 supported */
  /* In cellular_service socket_configure_remote()
     memcpy from char *addr to addr[] without knowing the length
     and by using strlen(char *addr) so ip_value must contain /0 */
  /* IPv4 : xxx.xxx.xxx.xxx=15+/0*/
  /* IPv6 : xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx = 39+/0*/
  uint8_t         ip_value[40];
  uint16_t        port;
} socket_addr_t;

#if (USE_LOW_POWER == 1)
/* Timer State */
typedef enum
{
  COM_TIMER_INVALID = 0, /* Timer not created         */
  COM_TIMER_IDLE,        /* Timer created not started */
  COM_TIMER_RUN          /* Timer created and started */
} com_timer_state_t;
#endif /* USE_LOW_POWER == 1 */

/* Private macros ------------------------------------------------------------*/
#define COM_MIN(a,b) (((a)<(b)) ? (a) : (b))

/* Set socket error - Used a lot of time in code */
#define SOCKET_SET_ERROR(socket, val) do { if ((socket) != NULL) { (socket)->error = (val); } } while(0)

/* Set / Get Socket message */
#define SET_SOCKET_MSG_TYPE(msg, type) ((msg) = ((msg)&0xFFFF0000U) | (uint32_t)(type))
#define SET_SOCKET_MSG_ID(msg, id)     ((msg) = ((msg)&0x0000FFFFU) | (((uint32_t)(id))<<16))
#define GET_SOCKET_MSG_TYPE(msg)       ((com_socket_msg_type_t)((msg)&0x0000FFFFU))
#define GET_SOCKET_MSG_ID(msg)         ((com_socket_msg_id_t)(((msg)&0xFFFF0000U)>>16))

/* Private variables ---------------------------------------------------------*/

/* Mutex to protect access to: socket descriptor lists */
static osMutexId ComSocketsMutexHandle;

#if defined (COM_SOCKETS_MODEM_NUMBER)
/* Static configuration: sockets are an array and initialization is done at com_sockets() init */
#define COM_SOCKETS_IP_MODEM_NUMBER       COM_SOCKETS_MODEM_NUMBER
#else /* !defined (COM_SOCKETS_MODEM_NUMBER) */
/* Dynamic configuration: first socket always statically created and new sockets can be created */
#define COM_SOCKETS_IP_MODEM_NUMBER       1U
#endif /* !defined (COM_SOCKETS_MODEM_NUMBER) */
static socket_desc_t socket_desc_list[COM_SOCKETS_IP_MODEM_NUMBER];

#if (USE_COM_PING == 1)
static int32_t ping_socket_id; /* Ping socket id */
static socket_desc_t socket_ping_desc; /* Ping socket desc is static */
#endif /* USE_COM_PING  == 1 */

/* Network status is managed through Datacache */
static bool com_sockets_network_is_up;

#if (USE_LOW_POWER == 1)
/* Timer to check inactivity on socket and maybe to go in data idle mode */
static osTimerId ComTimerInactivityId;
/* Timer state */
static com_timer_state_t com_timer_inactivity_state;
/* If com_nb_wake_up <= 1 inactivity timer can be activated */
static uint8_t com_nb_wake_up;
/* Mutex to protect access to: com_timer_inactivity_state, com_nb_wake_up (several applications and datacache) */
static osMutexId ComTimerInactivityMutexHandle;
#endif /* USE_LOW_POWER == 1 */

#if (UDP_SERVICE_SUPPORTED == 1U)
/* Local port allocated - used when bind(local_port = 0U) */
static uint16_t com_local_port; /* a value in range [COM_LOCAL_PORT_BEGIN, COM_LOCAL_PORT_BEGIN] */
#endif /* UDP_SERVICE_SUPPORTED == 1U */

/* Global variables ----------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
/*** BEGIN Callback prototype ***/
/* Callback called by AT when data are received */
static void com_ip_modem_data_received_cb(socket_handle_t sock);
/* Callback called by AT when closing is received */
static void com_ip_modem_closing_cb(socket_handle_t sock);

/* Callback called by Datacache - used to know Network status */
static void com_socket_datacache_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data);

#if (USE_COM_PING == 1)
/* Callback called by AT when Ping rsp is received */
static void com_ip_modem_ping_rsp_cb(CS_Ping_response_t ping_rsp);
#endif /* USE_COM_PING == 1 */

#if (USE_LOW_POWER == 1)
/* Callback called When Timer Inactivity is raised */
static void com_ip_modem_timer_inactivity_cb(void *p_argument);
#endif /* USE_LOW_POWER == 1 */
/*** END Callback prototype ***/

/* Initialize a socket descriptor */
static void com_ip_modem_init_socket_desc(socket_desc_t *p_socket_desc);
/* Create a static socket descriptor */
static bool com_ip_modem_create_static_socket_desc(socket_desc_t *p_socket_desc);
#if !defined (COM_SOCKETS_MODEM_NUMBER)
/* Create a dynamic socket descriptor */
static socket_desc_t *com_ip_modem_create_dynamic_socket_desc(void);
#endif /* !defined (COM_SOCKETS_MODEM_NUMBER) */
/* Provide a free socket descriptor - if needed and possible - create it */
static socket_desc_t *com_ip_modem_provide_socket_desc(socket_desc_t *p_socket_desc_list);
/* Invalid/Reinitialize a socket descriptor */
static void com_ip_modem_invalid_socket_desc(int32_t sock, socket_desc_t *p_socket_desc_list);
/* Find a socket descriptor */
static socket_desc_t *com_ip_modem_find_socket(int32_t sock, socket_desc_t *p_socket_desc_list);

/* Empty queue from all messages */
static void com_ip_modem_empty_queue(osMessageQId queue);

/*** BEGIN Conversion IP address functions ***/
static bool com_translate_ip_address(const com_sockaddr_t *p_addr, int32_t addrlen, socket_addr_t *p_socket_addr);
static bool com_convert_IPString_to_sockaddr(uint16_t ipaddr_port, com_char_t *p_ipaddr_str,
                                             com_sockaddr_t *p_sockaddr);
static void com_convert_ipaddr_port_to_sockaddr(const com_ip_addr_t *p_ip_addr, uint16_t port,
                                                com_sockaddr_in_t *p_sockaddr_in);
static void com_convert_sockaddr_to_ipaddr_port(const com_sockaddr_in_t *p_sockaddr_in,
                                                com_ip_addr_t *p_ip_addr, uint16_t *p_port);

/*** END Conversion IP address functions ***/

/* Network status is managed through Datacache */
static bool com_ip_modem_is_network_up(void);

#if (UDP_SERVICE_SUPPORTED == 1U)
/* Provide a free local port */
static uint16_t com_ip_modem_new_local_port(void);

/* Establish a UDP service (sendto/recvfrom) socket */
static int32_t com_ip_modem_connect_udp_service(socket_desc_t *p_socket_desc);
#endif /* UDP_SERVICE_SUPPORTED == 1U */

/* Request wakeup */
static void com_ip_modem_wakeup_request(void);
/* Request idle mode */
static void com_ip_modem_idlemode_request(bool immediate);
#if (USE_LOW_POWER == 1)
static bool com_ip_modem_are_all_sockets_invalid(void);
#endif /* USE_LOW_POWER == 1U */

/* Private function Definition -----------------------------------------------*/
/**
  * @brief  Initialize a socket descriptor
  * @note   socket queue is created the first time then it is reused
  * @param  p_socket_desc - socket descriptor to initialize
  * @retval -
  */
static void com_ip_modem_init_socket_desc(socket_desc_t *p_socket_desc)
{
  p_socket_desc->state            = COM_SOCKET_INVALID;
  p_socket_desc->closing          = false;
  p_socket_desc->id               = COM_SOCKET_INVALID_ID;
  p_socket_desc->local_port       = 0U;
  p_socket_desc->remote_port      = 0U;
  p_socket_desc->remote_addr.addr = 0U;
  (void)memset((void *) & (p_socket_desc->remote_addr), 0, sizeof(p_socket_desc->remote_addr));
  p_socket_desc->rcv_timeout      = RTOSAL_WAIT_FOREVER; /* default value, updated with setsockopt COM_SO_RCVTIMEO */
  p_socket_desc->snd_timeout      = RTOSAL_WAIT_FOREVER; /* default value, updated with setsockopt COM_SO_SNDTIMEO */
  p_socket_desc->error            = COM_SOCKETS_ERR_OK;
  /* p_socket_desc->p_next is not re-initialized - element is let in the list at its place */
  /* p_socket_desc->queue is not re-initialized  - queue is reused */
}

/**
  * @brief  Create and initialize a static socket descriptor
  * @note   Initialize and allocate (e.g queue) socket fields
  * @param  p_socket_desc - socket descriptor pointer to create
  * @retval bool false/true - creation/initialization NOK / OK
  */
static bool com_ip_modem_create_static_socket_desc(socket_desc_t *p_socket_desc)
{
  bool result = false;

  p_socket_desc->queue = rtosalMessageQueueNew((const rtosal_char_t *)"COMSOCKIP_QUE_SOCKET", 4U);
  if (p_socket_desc->queue != NULL)
  {
    /* Creation is ok, initialize socket description fields */
    p_socket_desc->p_next = NULL;
    com_ip_modem_init_socket_desc(p_socket_desc);
    result = true;
  }

  return (result);
}

#if !defined (COM_SOCKETS_MODEM_NUMBER)
/**
  * @brief  Create and initialize a dynamic socket descriptor
  * @note   Allocate a new socket_desc_t, initialize and allocate (e.g queue) socket fields
  * @param  -
  * @retval NULL or socket_desc_t - creation/initialization failed / p_socket_desc allocated socket
  */
static socket_desc_t *com_ip_modem_create_dynamic_socket_desc(void)
{
  socket_desc_t *p_socket_desc_result;

  /* Allocation */
  p_socket_desc_result = (socket_desc_t *)RTOSAL_MALLOC(sizeof(socket_desc_t));

  if (p_socket_desc_result != NULL)
  {
    if (com_ip_modem_create_static_socket_desc(p_socket_desc_result) == false)
    {
      /* Need to cleanup memory */
      RTOSAL_FREE((void *)p_socket_desc_result);
      p_socket_desc_result = NULL;
    }
  }

  return (p_socket_desc_result);
}
#endif /* !defined (COM_SOCKETS_MODEM_NUMBER) */


/**
  * @brief  Provide a socket descriptor - if needed and possible - create it
  * @note   Search the first empty place in the chained socket list
  *         if no empty place found and configure in dynamic mode create a new socket
  *         and add the new socket at the end of the chained list
  * @param  p_socket_desc_list - pointer to socket descriptor list to use
  * @retval NULL (if not enough memory/no more socket available) - socket_desc_t
  */
static socket_desc_t *com_ip_modem_provide_socket_desc(socket_desc_t *p_socket_desc_list)
{
  /* Chained list will be changed */
  (void)rtosalMutexAcquire(ComSocketsMutexHandle, RTOSAL_WAIT_FOREVER);

  socket_desc_t *p_socket_desc;

  p_socket_desc = p_socket_desc_list;

  /* Scan the socket descriptor list to find an empty place */
  while ((p_socket_desc->state != COM_SOCKET_INVALID) && (p_socket_desc->p_next != NULL))
  {
    p_socket_desc = p_socket_desc->p_next; /* Check next descriptor */
  }
  /* Find an empty/invalid socket ? */
  if (p_socket_desc->state == COM_SOCKET_INVALID)
  {
    /* Find an empty socket - Use it */
    PRINT_DBG("Old socket reused")
    p_socket_desc->state = COM_SOCKET_CREATING;
  }
  else
  {
    /* No empty/invalid socket - Create a new one ? */
    /* Is configuration in a static or dynamic socket creation ? */
#if defined (COM_SOCKETS_MODEM_NUMBER)
    /* Static configuration: not authorized to create dynamically a new socket */
    p_socket_desc = NULL; /* No empty socket */
    PRINT_INFO("Create new socket NOK: max socket number config(%d) and all sockets in use!", COM_SOCKETS_MODEM_NUMBER)
#else /* !defined (COM_SOCKETS_MODEM_NUMBER) */
    /* Dynamic configuration: try to create a new socket */
    /* No empty socket, save the reference to the last socket descriptor of the chained list to attach the new one */
    socket_desc_t *p_socket_desc_previous = p_socket_desc;
    /* Create new socket descriptor */
    p_socket_desc = com_ip_modem_create_dynamic_socket_desc();
    if (p_socket_desc != NULL)
    {
      PRINT_DBG("New socket desc created")
      /* Before to attach this new socket to the descriptor list finalize its initialization */
      p_socket_desc->state = COM_SOCKET_CREATING;
      /* Initialization is finalized, socket can be attached to the list */
      p_socket_desc_previous->p_next = p_socket_desc;
    }
    else
    {
      PRINT_INFO("Create new socket NOK: no memory!")
    }
#endif /* defined (COM_SOCKETS_MODEM_NUMBER) */
  }

  (void)rtosalMutexRelease(ComSocketsMutexHandle);

  /* If provide == false then p_socket_desc = NULL */
  return (p_socket_desc);
}

/**
  * @brief  Invalid a socket descriptor
  * @note   Search the socket descriptor and reinitialze it to unused
  * @param  sock  - socket id
  * @param  p_socket_desc_list - pointer to socket descriptor list to use
  * @retval -
  */
static void com_ip_modem_invalid_socket_desc(int32_t sock, socket_desc_t *p_socket_desc_list)
{
  /* Chained list will be changed */
  (void)rtosalMutexAcquire(ComSocketsMutexHandle, RTOSAL_WAIT_FOREVER);

  bool found = false;
  socket_desc_t *p_socket_desc;

  p_socket_desc = p_socket_desc_list;

  /* Search the socket descriptor */
  while ((p_socket_desc != NULL) && (found != true))
  {
    if (p_socket_desc->id == sock)
    {
      /* Socket descriptor is found */
      found = true;
    }
    else
    {
      /* Not the searched one ... Next */
      p_socket_desc = p_socket_desc->p_next;
    }
  }
  if (found == true)
  {
    /* Always keep a created socket */
    com_ip_modem_init_socket_desc(p_socket_desc);
  }

  (void)rtosalMutexRelease(ComSocketsMutexHandle);
}

/**
  * @brief  Find a socket descriptor according to its socket id
  * @param  sock  - socket id
  * @param  p_socket_desc_list - pointer to socket descriptor list to use
  * @retval NULL (not find) or socket_desc_t (find)
  */
static socket_desc_t *com_ip_modem_find_socket(int32_t sock, socket_desc_t *p_socket_desc_list)
{
  bool found = false;
  socket_desc_t *p_socket_desc;

  p_socket_desc = p_socket_desc_list;

  if (sock >= 0)
  {
    /* Search the socket descriptor */
    while ((p_socket_desc != NULL) && (found != true))
    {
      if (p_socket_desc->id == sock)
      {
        /* Socket descriptor is found */
        found = true;
      }
      else
      {
        /* Not the searched one ... Next */
        p_socket_desc = p_socket_desc->p_next;
      }
    }
  }

  if (found == false)
  {
    p_socket_desc = NULL;
  }

  /* If found == false then p_socket_desc = NULL */
  return (p_socket_desc);
}

/**
  * @brief  Empty queue from potential messages
  * @param  queue - queue to empty
  * @retval -
  */
static void com_ip_modem_empty_queue(osMessageQId queue)
{
  com_socket_msg_t msg_queue;

  do
  {
    msg_queue = 0U; /* if msg_queue = 0U after MessageQueueGet() then the queue is empty */
    /* Read the queue to suppress the msg from it */
    (void)rtosalMessageQueueGet(queue, &msg_queue, 0U); /* timeout = 0U */
    if (msg_queue != 0U)
    {
      /* Message available in the queue */
      PRINT_DBG("rcv cleanup MSGqueue")
    }
  } while (msg_queue != 0U);
}

#if (USE_LOW_POWER == 1)
/**
  * @brief  Are all sockets in invalid state
  * @note   Check if all sockets are in invalid state
  * @param  -
  * @retval bool - false/true - at least one socket is opened/no socket are opened
  */
static bool com_ip_modem_are_all_sockets_invalid(void)
{
  bool result = true; /* false : at least one socket is still open, true : all sockets are in invalid state */
  socket_desc_t *p_socket_desc;

  p_socket_desc = &socket_desc_list[0];
  /* Check one by one socket descriptor list */
  while ((p_socket_desc != NULL) && (result != false))
  {
    if (p_socket_desc->id > COM_SOCKET_INVALID_ID)
    {
      result = false; /* at least one socket is not in invalid state, no need to continue the check */
    }
    else
    {
      /* The socket is invalid state, check the next one */
      p_socket_desc = p_socket_desc->p_next;
    }
  }

#if (USE_COM_PING == 1)
  p_socket_desc = &socket_ping_desc;
  /* Check one by one socket descriptor list */
  while ((p_socket_desc != NULL) && (result != false))
  {
    if (p_socket_desc->id > COM_SOCKET_INVALID_ID)
    {
      result = false; /* at least one socket is not in invalid state, no need to continue the check */
    }
    else
    {
      /* The socket is invalid state, check the next one */
      p_socket_desc = p_socket_desc->p_next;
    }
  }
#endif /* USE_COM_PING == 1 */

  return (result);
}
#endif /* USE_LOW_POWER == 1 */

/**
  * @brief  Translate a com_sockaddr_t to a socket_addr_t
  * @param  p_addr - pointer on address in com socket format
  * @note   address is uint32_t 0xDDCCBBAA and port is COM_HTONS
  * @param  addrlen - com socket address length
  * @note   must be equal sizeof(com_sockaddr_in_t)
  * @param  p_socket_addr - pointer on socket address AT format
  * @note   address is AAA.BBB.CCC.DDD and port is COM_NTOHS
  * @retval bool - false/true conversion ok/nok
  */
static bool com_translate_ip_address(const com_sockaddr_t *p_addr, int32_t addrlen, socket_addr_t *p_socket_addr)
{
  bool result = false;
  const com_sockaddr_in_t *p_sockaddr_in;

  if (addrlen == (int32_t)sizeof(com_sockaddr_in_t))
  {
    p_sockaddr_in = (const com_sockaddr_in_t *)p_addr;

    if ((p_addr != NULL) && (p_socket_addr != NULL))
    {
      if (p_addr->sa_family == (uint8_t)COM_AF_INET)
      {
        p_socket_addr->ip_type = CS_IPAT_IPV4;
        if (p_sockaddr_in->sin_addr.s_addr == COM_INADDR_ANY)
        {
          (void)memcpy(&(p_socket_addr->ip_value[0]), "0.0.0.0", strlen("0.0.0.0"));
        }
        else
        {
          com_ip_addr_t com_ip_addr;
          com_ip_addr.addr = ((const com_sockaddr_in_t *)p_addr)->sin_addr.s_addr;
          (void)sprintf((CSIP_CHAR_t *)p_socket_addr->ip_value, "%u.%u.%u.%u",
                        COM_IP4_ADDR1_VAL(com_ip_addr), COM_IP4_ADDR2_VAL(com_ip_addr),
                        COM_IP4_ADDR3_VAL(com_ip_addr), COM_IP4_ADDR4_VAL(com_ip_addr));
        }
        p_socket_addr->port = COM_NTOHS(((const com_sockaddr_in_t *)p_addr)->sin_port);
        result = true;
      }
      /* else if (addr->sa_family == COM_AF_INET6) */
      /* or any other value */
      /* not supported */
    }
  }

  return (result);
}

/**
  * @brief  Translate a port/address string to com_sockaddr_t
  * @param  ipaddr_port  - ipaddr port
  * @param  p_ipaddr_str - pointer on address in string format
  * @note   "xxx.xxx.xxx.xxx" or xxx.xxx.xxx.xxx
  * @param  p_sockaddr   - pointer on address in com socket format
  * @note   address is uint32_t 0xDDCCBBAA and port is COM_HTONS
  * @retval bool - false/true conversion ok/nok
  */
static bool com_convert_IPString_to_sockaddr(uint16_t ipaddr_port, com_char_t *p_ipaddr_str, com_sockaddr_t *p_sockaddr)
{
  bool result = false;
  int32_t  count; /* To check if p_ipaddr_str contains an addr xxx.xxx.xxx.xxx */
  uint8_t  begin; /* To start the copy skipping potential '"' at begin/end of p_ipaddr_str */
  uint32_t ip_addr_tmp[4];
  com_ip4_addr_t ip4_addr_tmp;

  /* In a static function no need to retest sockaddr != NULL
     But let it because this function will become an user library */
  /* if (sockaddr != NULL) */
  {
    begin = (p_ipaddr_str[0] == ((uint8_t)'"')) ? 1U : 0U;

    (void)memset(p_sockaddr, 0, sizeof(com_sockaddr_t));

    count = sscanf((CSIP_CHAR_t *)(&(p_ipaddr_str[begin])), "%03lu.%03lu.%03lu.%03lu",
                   &ip_addr_tmp[0], &ip_addr_tmp[1], &ip_addr_tmp[2], &ip_addr_tmp[3]);

    if (count == 4)
    {
      if ((ip_addr_tmp[0] <= 255U) && (ip_addr_tmp[1] <= 255U) && (ip_addr_tmp[2] <= 255U) && (ip_addr_tmp[3] <= 255U))
      {
        p_sockaddr->sa_family = (uint8_t)COM_AF_INET;
        p_sockaddr->sa_len    = (uint8_t)sizeof(com_sockaddr_in_t);
        ((com_sockaddr_in_t *)p_sockaddr)->sin_port = COM_HTONS(ipaddr_port);
        COM_IP4_ADDR(&ip4_addr_tmp, ip_addr_tmp[0], ip_addr_tmp[1], ip_addr_tmp[2], ip_addr_tmp[3]);
        ((com_sockaddr_in_t *)p_sockaddr)->sin_addr.s_addr = ip4_addr_tmp.addr;
        result = true;
      }
    }
  }

  return (result);
}

/**
  * @brief  Translate a port/address uint32_t to com_sockaddr_in_t
  * @param  p_ip_addr - pointer on address in uint32_t format 0xDDCCBBAA
  * @param  port - port
  * @param  p_sockaddr_in - pointer on address in com socket format
  * @note   address is uint32_t 0xDDCCBBCCAA and port is COM_HTONS
  * @retval -
  */
static void com_convert_ipaddr_port_to_sockaddr(const com_ip_addr_t *p_ip_addr, uint16_t port,
                                                com_sockaddr_in_t *p_sockaddr_in)
{
  p_sockaddr_in->sin_len         = (uint8_t)sizeof(com_sockaddr_in_t);
  p_sockaddr_in->sin_family      = COM_AF_INET;
  p_sockaddr_in->sin_addr.s_addr = p_ip_addr->addr;
  p_sockaddr_in->sin_port        = COM_HTONS(port);
  (void)memset(p_sockaddr_in->sin_zero, 0, COM_SIN_ZERO_LEN);
}

/**
  * @brief  Translate com_sockaddr_in_t to a port/address uint32_t
  * @param  p_sockaddr_in - pointer on address in com socket format
  * @note   address is uint32_t 0xDDCCDDAA and port is COM_HTONS
  * @param  p_ip_addr - pointer on address in uint32_t format 0xDDCCBBAA
  * @param  p_port - pointer on port
  * @note   port is COM_NTOHS
  * @retval -
  */
static void com_convert_sockaddr_to_ipaddr_port(const com_sockaddr_in_t *p_sockaddr_in,
                                                com_ip_addr_t *p_ip_addr, uint16_t *p_port)
{
  p_ip_addr->addr = p_sockaddr_in->sin_addr.s_addr;
  *p_port = COM_NTOHS(p_sockaddr_in->sin_port);
}

/**
  * @brief  Provide Network status
  * @param  -
  * @retval true/false network is up/down
  */
static bool com_ip_modem_is_network_up(void)
{
  return (com_sockets_network_is_up);
}

#if (UDP_SERVICE_SUPPORTED == 1U)
/**
  * @brief  Allocate a new local port value
  * @retval new local port value in [COM_LOCAL_PORT_BEGIN, COM_LOCAL_PORT_END]
  *         or 0U if impossible to find a free port
  */
static uint16_t com_ip_modem_new_local_port(void)
{
  bool local_port_ok = false;
  bool found; /* false: local port is unused, true: local port is already used */
  uint16_t iter = 0U;
  uint16_t result;
  socket_desc_t *p_socket_desc;

  while ((local_port_ok != true) && (iter < (COM_LOCAL_PORT_END - COM_LOCAL_PORT_BEGIN)))
  {
    /* Test the next local port value */
    com_local_port++;
    iter++;
    if (com_local_port == COM_LOCAL_PORT_END)
    {
      com_local_port = COM_LOCAL_PORT_BEGIN;
    }

    /* Check if com_local_port is not already used by a socket */
    p_socket_desc = &socket_desc_list[0];
    found = false; /* local port not already used */

    /* Check all sockets in the chained list */
    while ((p_socket_desc != NULL) && (found != true))
    {
      if (p_socket_desc->local_port == com_local_port)
      {
        /* Local port already used - no need to continue the check for this local port value */
        found = true;
      }
      else
      {
        p_socket_desc = p_socket_desc->p_next;
      }
    }

    if (found == false)
    {
      /* Local port is unused */
      local_port_ok = true;
    }
    /* Continue to search a free value */
  }
  if (local_port_ok != true)
  {
    result = 0U;
  }
  else
  {
    result = com_local_port;
  }

  return (result);
}

/**
  * @brief  Establish a UDP service(sendto/recvfrom) socket
  * @note   Regarding the socket state and its parameters
  *         Allocate a local port / Send bind / Send modem connect to use sendto/recvfrom services
  * @param  p_socket_desc - pointer on socket descriptor
  * @note   socket descriptor (local port, state)
  * @retval result
  */
static int32_t com_ip_modem_connect_udp_service(socket_desc_t *p_socket_desc)
{
  int32_t result = COM_SOCKETS_ERR_STATE;

  if (p_socket_desc->state == COM_SOCKET_CREATED)
  {
    /* Bind and Connect udp service must be done */
    result = COM_SOCKETS_ERR_OK;

    /* Find a local port ? */
    if (p_socket_desc->local_port == 0U)
    {
      p_socket_desc->local_port = com_ip_modem_new_local_port();
      if (p_socket_desc->local_port == 0U)
      {
        /* No local port available */
        result = COM_SOCKETS_ERR_LOCKED;
      }
    }

    /* Connect must be done with specific parameter*/
    if (result == COM_SOCKETS_ERR_OK)
    {
      result = COM_SOCKETS_ERR_GENERAL;

      com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */

      /* Bind */
      if (osCDS_socket_bind(p_socket_desc->id, p_socket_desc->local_port) == CS_OK)
      {
        PRINT_INFO("socket internal bind ok")
        /* Connect UDP service */
        if (osCDS_socket_connect(p_socket_desc->id, CS_IPAT_IPV4, CONFIG_MODEM_UDP_SERVICE_CONNECT_IP, 0)
            == CS_OK)
        {
          result = COM_SOCKETS_ERR_OK;
          PRINT_INFO("socket internal connect ok")
          p_socket_desc->state = COM_SOCKET_CONNECTED;
        }
        else
        {
          PRINT_ERR("socket internal connect NOK at low level")
        }
      }
      else
      {
        PRINT_ERR("socket internal bind NOK at low level")
      }

      com_ip_modem_idlemode_request(false);

    }
  }
  else if (p_socket_desc->state == COM_SOCKET_CONNECTED)
  {
    /* Already connected - nothing to do */
    result = COM_SOCKETS_ERR_OK;
  }
  else
  {
    PRINT_ERR("socket internal connect - err state")
  }

  return (result);
}
#endif /* UDP_SERVICE_SUPPORTED == 1U */

/**
  * @brief  Request Idle mode
  * @note   -
  * @param  immediate - false/true
  * @retval -
  */
static void com_ip_modem_idlemode_request(bool immediate)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalMutexAcquire(ComTimerInactivityMutexHandle, RTOSAL_WAIT_FOREVER);

  if (com_nb_wake_up <= 1U) /* only one socket in progress */
  {
    if (immediate == true)
    {
      /* Are all sockets closed ? If so, don't arm the timer, immediate request to go in idle */
      if (com_ip_modem_are_all_sockets_invalid() == true)
      {
        (void)rtosalTimerStop(ComTimerInactivityId);
        com_timer_inactivity_state = COM_TIMER_IDLE;
        if (com_ip_modem_is_network_up() == true) /* If network is up IdleMode can be requested */
        {
          PRINT_INFO("Inactivity: All sockets closed: Timer stopped and IdleMode requested because network is up")
          if (CSP_DataIdle() == CS_OK)
          {
            PRINT_INFO("Inactivity: IdleMode request OK")
          }
          else
          {
            /* CSP_DataIdle may be NOK because CSP already in Idle */
            PRINT_INFO("Inactivity: IdleMode request NOK")
          }
        }
        else
        {
          PRINT_INFO("Inactivity: All sockets closed: Timer stopped but IdleMode NOT requested because network down")
        }
      }
      else /* All sockets not closed. Arm the timer */
      {
        /* Start or Restart timer */
        (void)rtosalTimerStart(ComTimerInactivityId, COM_TIMER_INACTIVITY_MS);
        com_timer_inactivity_state = COM_TIMER_RUN;
        PRINT_INFO("Inactivity: last command finished - Timer re/started")
      }
    }
    else
    {
      /* Start or Restart timer */
      (void)rtosalTimerStart(ComTimerInactivityId, COM_TIMER_INACTIVITY_MS);
      com_timer_inactivity_state = COM_TIMER_RUN;
      PRINT_INFO("Inactivity: last command finished - Timer re/started")
    }
    com_nb_wake_up = 0U;
  }
  else /* at least one socket in transaction ping/send/recv... and another finished its action */
  {
    /* Improvement : do next treatment only if all sockets in INVALID/CREATING/CREATED state ? */
    /* Start or Restart timer */
    (void)rtosalTimerStart(ComTimerInactivityId, COM_TIMER_INACTIVITY_MS);
    com_timer_inactivity_state = COM_TIMER_RUN;
    PRINT_INFO("Inactivity: one command finished - Timer re/started")
    com_nb_wake_up --;
  }
  (void)rtosalMutexRelease(ComTimerInactivityMutexHandle);

#else /* USE_LOW_POWER == 0 */
  UNUSED(immediate);
  __NOP();
#endif /* USE_LOW_POWER == 1 */
}

/**
  * @brief  Request Wake-Up
  * @note   -
  * @param  -
  * @retval -
  */
static void com_ip_modem_wakeup_request(void)
{
#if (USE_LOW_POWER == 1)
  (void)rtosalMutexAcquire(ComTimerInactivityMutexHandle, RTOSAL_WAIT_FOREVER);

  com_timer_inactivity_state = COM_TIMER_IDLE;
  (void)rtosalTimerStop(ComTimerInactivityId);
  com_nb_wake_up++;
  PRINT_INFO("Inactivity: WakeUp requested - Timer stopped")
  if (CSP_DataWakeup(HOST_WAKEUP) == CS_OK)
  {
    PRINT_INFO("Inactivity: WakeUp request OK")
  }
  else
  {
    /* CSP_DataWakeUp may be NOK because CSP already WakeUp */
    PRINT_INFO("Inactivity: WakeUp request NOK")
  }

  (void)rtosalMutexRelease(ComTimerInactivityMutexHandle);
#else /* USE_LOW_POWER == 0 */
  __NOP();
#endif /* USE_LOW_POWER == 1 */
}

/**
  * @brief  Callback called when URC data received raised
  * @note   Managed URC data received
  * @param  sock - socket handle
  * @note   -
  * @retval -
  */
static void com_ip_modem_data_received_cb(socket_handle_t sock)
{
  com_socket_msg_type_t msg_type = COM_SOCKET_MSG;
  com_socket_msg_id_t   msg_id   = COM_DATA_RCV;
  com_socket_msg_t msg_queue = 0U;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if (p_socket_desc != NULL)
  {
    if (p_socket_desc->closing != true)
    {
      if (p_socket_desc->state == COM_SOCKET_WAITING)
      {
        PRINT_INFO("cb socket %ld data ready called: waiting", p_socket_desc->id)
        SET_SOCKET_MSG_TYPE(msg_queue, msg_type);
        SET_SOCKET_MSG_ID(msg_queue, msg_id);
        PRINT_DBG("cb socket %ld MSGput %lu queue %p", p_socket_desc->id, msg_queue, p_socket_desc->queue)
        (void)rtosalMessageQueuePut(p_socket_desc->queue, msg_queue, 0U);
      }
      else
      {
        PRINT_INFO("cb socket data ready called: socket_state:%i NOK", p_socket_desc->state)
      }
    }
    else
    {
      PRINT_ERR("cb socket data ready called: socket is closing")
    }
  }
  else
  {
    PRINT_ERR("cb socket data ready called: unknown socket")
  }
}

/**
  * @brief  Callback called when URC socket closing raised
  * @note   Managed URC socket closing
  * @param  sock - socket handle
  * @note   -
  * @retval -
  */
static void com_ip_modem_closing_cb(socket_handle_t sock)
{
  com_socket_msg_type_t msg_type = COM_SOCKET_MSG;
  com_socket_msg_id_t   msg_id   = COM_CLOSING_RCV;
  com_socket_msg_t msg_queue = 0U;
  socket_desc_t *p_socket_desc;

  PRINT_DBG("callback socket closing called")

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if (p_socket_desc != NULL)
  {
    PRINT_INFO("cb socket closing called: close rqt")
    if (p_socket_desc->closing == false)
    {
      p_socket_desc->closing = true;
      PRINT_INFO("cb socket closing: close rqt")
    }
    if (p_socket_desc->state == COM_SOCKET_WAITING)
    {
      PRINT_ERR("!!! cb socket %ld closing called: data_expected !!!", p_socket_desc->id)
      SET_SOCKET_MSG_TYPE(msg_queue, msg_type);
      SET_SOCKET_MSG_ID(msg_queue, msg_id);
      PRINT_DBG("cb socket %ld MSGput %lu queue %p", p_socket_desc->id, msg_queue, p_socket_desc->queue)
      (void)rtosalMessageQueuePut(p_socket_desc->queue, msg_queue, 0U);
    }
  }
  else
  {
    PRINT_ERR("cb socket closing called: unknown socket")
  }
}

/**
  * @brief  Callback called when a value in datacache changed
  * @note   Managed datacache value changed
  * @param  dc_event_id - value changed
  * @note   -
  * @param  p_private_gui_data - value provided at service subscription
  * @note   Unused
  * @retval -
  */
static void com_socket_datacache_cb(dc_com_event_id_t dc_event_id, const void *p_private_gui_data)
{
  UNUSED(p_private_gui_data);

  /** If using DC_CELLULAR_INFO in case of LowPower com_socket_datacache_cb to update LowPower state
    * but soft is still in function com_ip_modem_idlemode_request so ComTimerInactivityMutexHandle is still acquire
    * possible to be blocked on: else (void)rtosalMutexAcquire(ComTimerInactivityMutexHandle, RTOSAL_WAIT_FOREVER)
    */
  if (dc_event_id == DC_CELLULAR_NIFMAN_INFO)
  {
    dc_nifman_info_t dc_nifman_rt_info;

    if (dc_com_read(&dc_com_db, DC_CELLULAR_NIFMAN_INFO, (void *)&dc_nifman_rt_info, sizeof(dc_nifman_rt_info))
        == DC_COM_OK)
    {
      /* Is Data ready or not ? */
      if (dc_nifman_rt_info.rt_state == DC_SERVICE_ON)
      {
        if (com_ip_modem_is_network_up() == false)
        {
          com_sockets_network_is_up = true;
          com_sockets_statistic_update(COM_SOCKET_STAT_NWK_UP);
#if (USE_LOW_POWER == 1)
          (void)rtosalMutexAcquire(ComTimerInactivityMutexHandle, RTOSAL_WAIT_FOREVER);
          com_timer_inactivity_state = COM_TIMER_RUN;
          /* Start or Restart timer */
          (void)rtosalTimerStart(ComTimerInactivityId, COM_TIMER_INACTIVITY_MS);
          PRINT_INFO("Inactivity: Network on - Timer started")
          (void)rtosalMutexRelease(ComTimerInactivityMutexHandle);
#endif /* USE_LOW_POWER == 1 */
        }
      }
      else
      {
        if (com_ip_modem_is_network_up() == true)
        {
          com_sockets_network_is_up = false;
          com_sockets_statistic_update(COM_SOCKET_STAT_NWK_DWN);
#if (USE_LOW_POWER == 1)
          (void)rtosalMutexAcquire(ComTimerInactivityMutexHandle, RTOSAL_WAIT_FOREVER);
          com_timer_inactivity_state = COM_TIMER_IDLE;
          (void)rtosalTimerStop(ComTimerInactivityId);
          PRINT_INFO("Inactivity: Network off - Timer stopped")
          (void)rtosalMutexRelease(ComTimerInactivityMutexHandle);
#endif /* USE_LOW_POWER == 1 */
        }
      }
    }
  }
  else
  {
    /* Nothing to do */
  }
}

#if (USE_COM_PING == 1)
/**
  * @brief Ping response callback
  * @param  ping_rsp
  * @note   ping data response
  * @retval -
  */
static void com_ip_modem_ping_rsp_cb(CS_Ping_response_t ping_rsp)
{
  bool treated = false;
  com_socket_msg_type_t msg_type = COM_PING_MSG;
  com_socket_msg_id_t   msg_id   = COM_DATA_RCV;
  com_socket_msg_t msg_queue = 0U;
  socket_desc_t *p_socket_desc;

  PRINT_DBG("callback ping response")
  PRINT_DBG("Ping rsp status:%d index:%d final:%d time:%ld",
            ping_rsp.ping_status, ping_rsp.index, ping_rsp.is_final_report, ping_rsp.time)

  p_socket_desc = NULL;

  /* Avoid treatment if Ping Id is unknown - Should not happen */
  if (ping_socket_id != COM_SOCKET_INVALID_ID)
  {
    p_socket_desc = com_ip_modem_find_socket(ping_socket_id, &socket_ping_desc);

    if ((p_socket_desc != NULL) && (p_socket_desc->closing == false) && (p_socket_desc->state == COM_SOCKET_WAITING))
    {
      treated = true;

      if (ping_rsp.ping_status != CS_OK)
      {
        p_socket_desc->p_ping_rsp->status = COM_SOCKETS_ERR_GENERAL;
        p_socket_desc->p_ping_rsp->time   = 0U;
        p_socket_desc->p_ping_rsp->size   = 0U;
        p_socket_desc->p_ping_rsp->ttl    = 0U;
        PRINT_INFO("callback ping data ready: error rcv - exit")
        SET_SOCKET_MSG_TYPE(msg_queue, msg_type);
        SET_SOCKET_MSG_ID(msg_queue, msg_id);
        (void)rtosalMessageQueuePut(p_socket_desc->queue, msg_queue, 0U);
      }
      else
      {
        if (ping_rsp.index == 1U)
        {
          if (ping_rsp.is_final_report == CS_FALSE)
          {
            /* Save the data wait final report to send event */
            PRINT_INFO("callback ping data ready: rsp rcv - wait final report")
            p_socket_desc->p_ping_rsp->status = COM_SOCKETS_ERR_OK;
            p_socket_desc->p_ping_rsp->time   = ping_rsp.time;
            p_socket_desc->p_ping_rsp->size   = ping_rsp.ping_size;
            p_socket_desc->p_ping_rsp->ttl    = ping_rsp.ttl;
            SET_SOCKET_MSG_TYPE(msg_queue, msg_type);
            SET_SOCKET_MSG_ID(msg_queue, msg_id);
            (void)rtosalMessageQueuePut(p_socket_desc->queue, msg_queue, 0U);
          }
          else
          {
            /* index == 1U and final report == true => error */
            PRINT_INFO("callback ping data ready: index=1 and final report=true - exit")
            msg_id = COM_CLOSING_RCV;
            SET_SOCKET_MSG_TYPE(msg_queue, msg_type);
            SET_SOCKET_MSG_ID(msg_queue, msg_id);
            (void)rtosalMessageQueuePut(p_socket_desc->queue, msg_queue, 0U);
          }
        }
        else
        {
          /* Must wait final report */
          if (ping_rsp.is_final_report == CS_TRUE)
          {
            PRINT_INFO("callback ping data ready: final report rcv")
            msg_id = COM_CLOSING_RCV;
            SET_SOCKET_MSG_TYPE(msg_queue, msg_type);
            SET_SOCKET_MSG_ID(msg_queue, msg_id);
            (void)rtosalMessageQueuePut(p_socket_desc->queue, msg_queue, 0U);
          }
          else
          {
            /* we receive more than one response */
            SET_SOCKET_MSG_TYPE(msg_queue, msg_type);
            SET_SOCKET_MSG_ID(msg_queue, msg_id);
            (void)rtosalMessageQueuePut(p_socket_desc->queue, msg_queue, 0U);
          }
        }
      }
    }
  }

  if (treated == false)
  {
    PRINT_INFO("!!! PURGE callback ping data ready - index %d !!!", ping_rsp.index)
    if (p_socket_desc == NULL)
    {
      PRINT_ERR("Ping Id unknown or no Ping in progress")
    }
    else
    {
      if (p_socket_desc->closing == true)
      {
        PRINT_ERR("callback ping data ready: ping is closing")
      }
      if (p_socket_desc->state != COM_SOCKET_WAITING)
      {
        PRINT_DBG("callback ping data ready: ping state:%d index:%d final report:%u NOK",
                  p_socket_desc->state, ping_rsp.index, ping_rsp.is_final_report)
      }
    }
  }
}

#endif /* USE_COM_PING == 1 */

#if (USE_LOW_POWER == 1)
/**
  * @brief  Callback called when Inactivity Timer raised
  * @note   Managed Inactivity Timer
  * @param  p_argument - UNUSED
  * @note   -
  * @retval -
  */
static void com_ip_modem_timer_inactivity_cb(void *p_argument)
{
  PRINT_DBG("callback socket inactitvity timer called")
  UNUSED(p_argument);
  PRINT_INFO("Inactivity: Inactivity Timer: Timer cb interaction %d", com_nb_wake_up)

  /* Improvement: do next treatment also if a Ping session is opened and not closed ? */
  if ((com_timer_inactivity_state == COM_TIMER_RUN) && (com_nb_wake_up == 0U))
  {
    com_timer_inactivity_state = COM_TIMER_IDLE;
    if (com_ip_modem_is_network_up() == true) /* If network is up IdleMode can be requested */
    {
      PRINT_INFO("Inactivity: Inactivity Timer: IdleMode requested because network is up")

      if (CSP_DataIdle() == CS_OK)
      {
        PRINT_INFO("Inactivity: Inactivity Timer: IdleMode request OK")
      }
      else
      {
        /* CSP_DataIdle may be NOK because CSP already in Idle */
        PRINT_INFO("Inactivity: Inactivity Timer: IdleMode request NOK")
      }
    }
    else
    {
      PRINT_INFO("Inactivity: Inactivity Timer: IdleMode NOT requested because network is down")
    }
  }
  else
  {
    PRINT_INFO("Inactivity: Inactivity Timer: Timer cb called but timer state or interaction: %d NOK", com_nb_wake_up)
  }
}
#endif /* USE_LOW_POWER == 1 */

/* Functions Definition ------------------------------------------------------*/

/*** Socket management ********************************************************/

/**
  * @brief  Socket handle creation
  * @note   Create a communication endpoint called socket
  *         only TCP/UDP IPv4 client mode supported
  * @param  family   - address family
  * @note   only AF_INET supported
  * @param  type     - connection type
  * @note   only SOCK_STREAM or SOCK_DGRAM supported
  * @param  protocol - protocol type
  * @note   only IPPROTO_TCP or IPPROTO_UDP supported
  * @retval int32_t  - socket handle or error value
  */
int32_t com_socket_ip_modem(int32_t family, int32_t type, int32_t protocol)
{
  int32_t sock   = COM_SOCKET_INVALID_ID;
  int32_t result = COM_SOCKETS_ERR_OK;

  CS_IPaddrType_t IPaddrType;               /* Depends on argument value */
  CS_TransportProtocol_t TransportProtocol; /* Depends on argument value */
  CS_PDN_conf_id_t PDN_conf_id = CS_PDN_CONFIG_DEFAULT; /* Fixed value   */

  /* IPaddrType */
  if (family == COM_AF_INET)
  {
    /* address family IPv4 */
    IPaddrType = CS_IPAT_IPV4;
  }
  else if (family == COM_AF_INET6) /* Not yet supported */
  {
    /* address family IPv6 */
    IPaddrType = CS_IPAT_IPV6; /* To avoid a warning */
    result = COM_SOCKETS_ERR_UNSUPPORTED;
  }
  else
  {
    IPaddrType = CS_IPAT_INVALID; /* To avoid a warning */
    result = COM_SOCKETS_ERR_PARAMETER;
  }

  /* TransportProtocol */
  if ((type == COM_SOCK_STREAM) && ((protocol == COM_IPPROTO_TCP) || (protocol == COM_IPPROTO_IP)))
  {
    /* IPPROTO_TCP = must be used with SOCK_STREAM */
    TransportProtocol = CS_TCP_PROTOCOL;
  }
  else if ((type == COM_SOCK_DGRAM) && ((protocol == COM_IPPROTO_UDP) || (protocol == COM_IPPROTO_IP)))
  {
    /* IPPROTO_UDP = must be used with SOCK_DGRAM */
    TransportProtocol = CS_UDP_PROTOCOL;
  }
  else
  {
    TransportProtocol = CS_TCP_PROTOCOL; /* To avoid a warning */
    result = COM_SOCKETS_ERR_UNSUPPORTED;
  }

  if (result == COM_SOCKETS_ERR_OK)
  {
    result = COM_SOCKETS_ERR_GENERAL;
    PRINT_DBG("socket create request")
    com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
    sock = osCDS_socket_create(IPaddrType, TransportProtocol, PDN_conf_id);

    if (sock != CS_INVALID_SOCKET_HANDLE)
    {
      socket_desc_t *p_socket_desc;
      PRINT_INFO("create socket ok low level")

      /* Need to create a new p_socket_desc ? */
      p_socket_desc = com_ip_modem_provide_socket_desc(&socket_desc_list[0]);
      if (p_socket_desc == NULL)
      {
        result = COM_SOCKETS_ERR_NOMEMORY;
        PRINT_ERR("create socket NOK no memory")
        /* Socket descriptor is not existing in COM
          must close directly the socket and not call com_close */
        if (osCDS_socket_close(sock, 0U) == CS_OK)
        {
          PRINT_INFO("close socket ok low level")
        }
        else
        {
          PRINT_ERR("close socket NOK low level")
        }
      }
      else
      {
        /* Update socket descriptor */
        p_socket_desc->id    = sock;
        p_socket_desc->type  = (uint8_t)type;
        p_socket_desc->state = COM_SOCKET_CREATED;

        if (osCDS_socket_set_callbacks(sock, com_ip_modem_data_received_cb, NULL, com_ip_modem_closing_cb)
            == CS_OK)
        {
          result = COM_SOCKETS_ERR_OK;
        }
        else
        {
          PRINT_ERR("rqt close socket issue at creation")
          if (com_closesocket_ip_modem(sock) == COM_SOCKETS_ERR_OK)
          {
            PRINT_INFO("close socket ok low level")
          }
          else
          {
            PRINT_ERR("close socket NOK low level")
          }
        }
      }
    }
    else
    {
      PRINT_ERR("create socket NOK low level")
    }
    com_ip_modem_idlemode_request(false);
    /* Stat only socket whose parameters are supported */
    com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? COM_SOCKET_STAT_CRE_OK : COM_SOCKET_STAT_CRE_NOK);
  }
  else
  {
    PRINT_ERR("create socket NOK parameter NOK")
  }

  /* result == COM_SOCKETS_ERR_OK return socket handle */
  /* result != COM_SOCKETS_ERR_OK socket not created,
     no need to call SOCKET_SET_ERROR */
  return ((result == COM_SOCKETS_ERR_OK) ? sock : result);
}


/**
  * @brief  Socket option set
  * @note   Set option for the socket
  * @note   only send or receive timeout supported
  * @param  sock      - socket handle obtained with com_socket
  * @param  level     - level at which the option is defined
  * @note   only COM_SOL_SOCKET supported
  * @param  optname   - option name for which the value is to be set
  * @note
  *         - COM_SO_SNDTIMEO : OK but value not used because there is already
  *                             a tempo at low level - risk of conflict
  *         - COM_SO_RCVTIMEO : OK
  *         - any other value is rejected
  * @param  optval    - pointer to the buffer containing the option value
  * @note   COM_SO_SNDTIMEO and COM_SO_RCVTIMEO : unit is ms
  * @param  optlen    - size of the buffer containing the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_setsockopt_ip_modem(int32_t sock, int32_t level, int32_t optname, const void *optval, int32_t optlen)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if (p_socket_desc != NULL)
  {
    if ((optval != NULL) && (optlen > 0))
    {
      com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
      if (level == COM_SOL_SOCKET)
      {
        switch (optname)
        {
          /* Send Timeout */
          case COM_SO_SNDTIMEO :
            if ((uint32_t)optlen <= sizeof(p_socket_desc->rcv_timeout))
            {
              /* A tempo already exists at low level and cannot be redefined */
              /* Ok to accept value setting but :
                 value will not be used due to conflict risk
                 if tempo differ from low level tempo value */
              p_socket_desc->snd_timeout = *(const uint32_t *)optval;
              result = COM_SOCKETS_ERR_OK;
            }
            break;

          /* Receive Timeout */
          case COM_SO_RCVTIMEO :
            if ((uint32_t)optlen <= sizeof(p_socket_desc->rcv_timeout))
            {
              /* A tempo already exists at low level and cannot be redefined */
              /* Ok to accept value setting but :
                 if tempo value is shorter and data are received after
                 then if socket is not closing data still available in the modem
                 and can still be read if modem manage this feature
                 if tempo value is bigger and data are received before
                 then data will be send to application */
              p_socket_desc->rcv_timeout = *(const uint32_t *)optval;
              result = COM_SOCKETS_ERR_OK;
            }
            break;

          /* Error */
          case COM_SO_ERROR :
            /* Set for this option NOK */
            __NOP(); /* result already set to COM_SOCKETS_ERR_PARAMETER */
            break;

          default :
            /* Other options NOT YET SUPPORTED */
            __NOP(); /* result already set to COM_SOCKETS_ERR_PARAMETER */
            break;
        }
      }
      else
      {
        /* Other level than SOL_SOCKET NOT YET SUPPORTED */
        __NOP(); /* result already set to COM_SOCKETS_ERR_PARAMETER */
      }
      com_ip_modem_idlemode_request(false);
    }
  }
  else
  {
    result = COM_SOCKETS_ERR_DESCRIPTOR;
  }

  SOCKET_SET_ERROR(p_socket_desc, result);

  return (result);
}


/**
  * @brief  Socket option get
  * @note   Get option for a socket
  * @note   only send timeout, receive timeout, last error supported
  * @param  sock      - socket handle obtained with com_socket
  * @param  level     - level at which option is defined
  * @note   only COM_SOL_SOCKET supported
  * @param  optname   - option name for which the value is requested
  * @note
  *         - COM_SO_SNDTIMEO, COM_SO_RCVTIMEO, COM_SO_ERROR supported
  *         - any other value is rejected
  * @param  optval    - pointer to the buffer that will contain the option value
  * @note   COM_SO_SNDTIMEO, COM_SO_RCVTIMEO: in ms for timeout (uint32_t)
  *         COM_SO_ERROR : result of last operation (int32_t)
  * @param  optlen    - size of the buffer that will contain the option value
  * @note   must be sizeof(x32_t)
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockopt_ip_modem(int32_t sock, int32_t level, int32_t optname, void *optval, int32_t *optlen)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if (p_socket_desc != NULL)
  {
    if ((optval != NULL) && (optlen != NULL))
    {
      com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
      if (level == COM_SOL_SOCKET)
      {
        switch (optname)
        {
          /* Send Timeout */
          case COM_SO_SNDTIMEO :
            /* Force optval to be on uint32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(uint32_t))
            {
              *(uint32_t *)optval = p_socket_desc->snd_timeout;
              result = COM_SOCKETS_ERR_OK;
            }
            break;

          /* Receive Timeout */
          case COM_SO_RCVTIMEO :
            /* Force optval to be on uint32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(uint32_t))
            {
              *(uint32_t *)optval = p_socket_desc->rcv_timeout;
              result = COM_SOCKETS_ERR_OK;
            }
            break;

          /* Error */
          case COM_SO_ERROR :
            /* Force optval to be on int32_t to be compliant with lwip */
            if ((uint32_t)*optlen == sizeof(int32_t))
            {
              /* Get socket error taking into account COM_SOCKETS_ERRNO_COMPAT define */
              *(int32_t *)optval   = com_sockets_err_to_errno((com_sockets_err_t)(p_socket_desc->error));
              /* A read of last ERROR reset the socket error */
              p_socket_desc->error = COM_SOCKETS_ERR_OK;
              result = COM_SOCKETS_ERR_OK;
            }
            break;

          default :
            /* Other options NOT YET SUPPORTED */
            __NOP(); /* result already set to COM_SOCKETS_ERR_PARAMETER */
            break;
        }
      }
      else
      {
        /* Other level than SOL_SOCKET NOT YET SUPPORTED */
        __NOP(); /* result already set to COM_SOCKETS_ERR_PARAMETER */
      }
      com_ip_modem_idlemode_request(false);
    }
  }
  else
  {
    result = COM_SOCKETS_ERR_DESCRIPTOR;
  }

  SOCKET_SET_ERROR(p_socket_desc, result);

  return (result);
}


/**
  * @brief  Socket bind
  * @note   Assign a local address and port to a socket
  * @param  sock      - socket handle obtained with com_socket
  * @param  addr      - local IP address and port
  * @note   only port value field is used as local port,
  *         but whole addr parameter must be "valid"
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_bind_ip_modem(int32_t sock, const com_sockaddr_t *addr, int32_t addrlen)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  socket_addr_t socket_addr;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if (p_socket_desc != NULL)
  {
    com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
    /* Bind supported only in Created state */
    if (p_socket_desc->state == COM_SOCKET_CREATED)
    {
      if (com_translate_ip_address(addr, addrlen, &socket_addr) == true)
      {
        result = COM_SOCKETS_ERR_GENERAL;
        PRINT_DBG("socket bind request")

        if (osCDS_socket_bind(sock, socket_addr.port) == CS_OK)
        {
          PRINT_INFO("socket bind ok low level")
          result = COM_SOCKETS_ERR_OK;
          p_socket_desc->local_port = socket_addr.port;
        }
      }
      else
      {
        PRINT_ERR("socket bind NOK translate IP NOK")
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_STATE;
      PRINT_ERR("socket bind NOK state invalid")
    }
    com_ip_modem_idlemode_request(false);
  }

  SOCKET_SET_ERROR(p_socket_desc, result);

  return (result);
}


/**
  * @brief  Socket listen
  * @note   Set socket in listening mode
  *         NOT YET SUPPORTED
  * @param  sock      - socket handle obtained with com_socket
  * @param  backlog   - number of connection requests that can be queued
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_listen_ip_modem(int32_t sock, int32_t backlog)
{
  UNUSED(sock);
  UNUSED(backlog);
  return (COM_SOCKETS_ERR_UNSUPPORTED);
}


/**
  * @brief  Socket accept
  * @note   Accept a connect request for a listening socket
  *         NOT YET SUPPORTED
  * @param  sock      - socket handle obtained with com_socket
  * @param  addr      - IP address and port number of the accepted connection
  * @param  addrlen   - addr length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_accept_ip_modem(int32_t sock, com_sockaddr_t *addr, int32_t *addrlen)
{
  UNUSED(sock);
  UNUSED(addr);
  UNUSED(addrlen);
  return (COM_SOCKETS_ERR_UNSUPPORTED);
}


/**
  * @brief  Socket connect
  * @note   Connect socket to a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @param  addr      - remote IP address and port
  * @note   only an IPv4 address is supported
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_connect_ip_modem(int32_t sock, const com_sockaddr_t *addr, int32_t addrlen)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  socket_addr_t socket_addr;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  /* Check parameters validity */
  if (p_socket_desc != NULL)
  {
    if (com_translate_ip_address(addr, addrlen, &socket_addr) == true)
    {
      /* Parameters are valid */
      result = COM_SOCKETS_ERR_OK;
    }
  }

  /* If parameters are valid continue the treatment */
  if (result == COM_SOCKETS_ERR_OK)
  {
    if (p_socket_desc->type == (uint8_t)COM_SOCK_STREAM)
    {
      if (p_socket_desc->state == COM_SOCKET_CREATED)
      {
        /* Check Network status */
        if (com_ip_modem_is_network_up() == true)
        {
          com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
          if (osCDS_socket_connect(p_socket_desc->id, socket_addr.ip_type, &socket_addr.ip_value[0], socket_addr.port)
              == CS_OK)
          {
            /* result already set to the correct value COM_SOCKETS_ERR_OK */
            PRINT_INFO("socket connect ok")
            p_socket_desc->state = COM_SOCKET_CONNECTED;
          }
          else
          {
            result = COM_SOCKETS_ERR_GENERAL;
            PRINT_ERR("socket connect NOK at low level")
          }
          com_ip_modem_idlemode_request(false);
        }
        else
        {
          result = COM_SOCKETS_ERR_NONETWORK;
          PRINT_ERR("socket connect NOK no network")
        }
      }
      else
      {
        result = COM_SOCKETS_ERR_STATE;
        PRINT_ERR("socket connect NOK err state")
      }
    }
    else /* p_socket_desc->type == (uint8_t)COM_SOCK_DGRAM */
    {
#if (UDP_SERVICE_SUPPORTED == 0U)
      /* even if CONNECTED let MODEM decide if it is supported
         to update internal configuration */
      /* for DGRAM no need to check network status
         because connection is internal */
      if ((p_socket_desc->state == COM_SOCKET_CREATED) || (p_socket_desc->state == COM_SOCKET_CONNECTED))
      {
        com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
        if (osCDS_socket_connect(p_socket_desc->id, socket_addr.ip_type, &socket_addr.ip_value[0], socket_addr.port)
            == CS_OK)
        {
          /* result already set to the correct value COM_SOCKETS_ERR_OK */
          /* result = COM_SOCKETS_ERR_OK; */
          PRINT_INFO("socket connect ok")
          p_socket_desc->state = COM_SOCKET_CONNECTED;
        }
        else
        {
          result = COM_SOCKETS_ERR_GENERAL;
          PRINT_ERR("socket connect NOK at low level")
        }
        com_ip_modem_idlemode_request(false);
      }
      else
      {
        result = COM_SOCKETS_ERR_STATE;
        PRINT_ERR("socket connect NOK err state")
      }
#else /* UDP_SERVICES_SUPPORTED == 1U */
      /* A specific udp service connection is done
         in order to be able to use sendto/recvfrom services */
      result = com_ip_modem_connect_udp_service(p_socket_desc);
#endif /* UDP_SERVICES_SUPPORTED == 0U */
    }
  }
  if (p_socket_desc != NULL)
  {
    /* if com_translate_ip_address == FALSE, result already set to COM_SOCKETS_ERR_PARAMETER */
    com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? COM_SOCKET_STAT_CNT_OK : COM_SOCKET_STAT_CNT_NOK);
    SOCKET_SET_ERROR(p_socket_desc, result);
  }

  if (result == COM_SOCKETS_ERR_OK)
  {
    /* Save remote addr - port */
    com_ip_addr_t remote_addr;
    uint16_t remote_port;

    com_convert_sockaddr_to_ipaddr_port((const com_sockaddr_in_t *)addr, &remote_addr, &remote_port);
    p_socket_desc->remote_addr.addr = remote_addr.addr;
    p_socket_desc->remote_port = remote_port;
  }

  return (result);
}


/**
  * @brief  Socket send data
  * @note   Send data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to send
  * @note   see below
  * @param  len       - length of the data to send (in bytes)
  * @note   see below
  * @param  flags     - options
  * @note
  *         - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         if len of buffer to send > interface between COM and low level.
  *          The maximum of interface will be send (only one send)
  *         - if flags = COM_MSG_WAIT, application accept to wait
  *         if len of buffer to send > interface between COM and low level.
  *          COM will fragment the buffer according to the interface (multiple sends)
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_send_ip_modem(int32_t sock, const com_char_t *buf, int32_t len, int32_t flags)
{
  bool is_network_up;
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if ((p_socket_desc != NULL) && (buf != NULL) && (len > 0))
  {
    if (p_socket_desc->state == COM_SOCKET_CONNECTED)
    {
      /* closing maybe received, refuse to send data */
      if (p_socket_desc->closing == false)
      {
        /* network maybe down, refuse to send data */
        if (com_ip_modem_is_network_up() == false)
        {
          result = COM_SOCKETS_ERR_NONETWORK;
          PRINT_ERR("snd data NOK no network")
        }
        else
        {
          com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */

#if (UDP_SERVICE_SUPPORTED == 1)
          /* if UDP_SERVICE is supported and socket is using UDP protocol (socket type = DGRAM),
           * Connect already done by Appli => send() may be changed to sendto()
           * or Connect by COM to use sendto()/recvfrom() services => send() must be changed to sendto()
           * else send() service must be used whatever the socket type */
          if (p_socket_desc->type == (uint8_t)COM_SOCK_DGRAM)
          {
            result = com_sendto_ip_modem(sock, buf, len, flags, NULL, 0);
          }
          else
#endif /* UDP_SERVICE_SUPPORTED == 1 */
          {
            uint32_t length_to_send;
            uint32_t length_send;

            result = COM_SOCKETS_ERR_GENERAL;
            length_send = 0U;
            p_socket_desc->state = COM_SOCKET_SENDING;

            if (flags == COM_MSG_DONTWAIT)
            {
              length_to_send = COM_MIN((uint32_t)len, COM_MODEM_MAX_TX_DATA_SIZE);
              if (osCDS_socket_send(p_socket_desc->id, buf, length_to_send) == CS_OK)
              {
                length_send = length_to_send;
                result = (int32_t)length_send;
                PRINT_INFO("snd data DONTWAIT ok")
              }
              else
              {
                PRINT_ERR("snd data DONTWAIT NOK at low level")
              }
              p_socket_desc->state = COM_SOCKET_CONNECTED;
            }
            else
            {
              is_network_up = com_ip_modem_is_network_up();
              /* Send all data of a big buffer - Whatever the size */
              while ((length_send != (uint32_t)len)
                     && (p_socket_desc->closing == false)
                     && (is_network_up == true)
                     && (p_socket_desc->state == COM_SOCKET_SENDING))
              {
                length_to_send = COM_MIN((((uint32_t)len) - length_send), COM_MODEM_MAX_TX_DATA_SIZE);
                com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
                /* A tempo is already managed at low-level */
                if (osCDS_socket_send(p_socket_desc->id, buf + length_send, length_to_send) == CS_OK)
                {
                  length_send += length_to_send;
                  PRINT_INFO("snd data ok")
                  /* Update Network status */
                  is_network_up = com_ip_modem_is_network_up();
                }
                else
                {
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  PRINT_ERR("snd data NOK at low level")
                }
                com_ip_modem_idlemode_request(false);
              }
              p_socket_desc->state = COM_SOCKET_CONNECTED;
              result = (int32_t)length_send;
            }
            /* If no data send and socket is closing : force ERR_CLOSING */
            if ((length_send == 0U) && (p_socket_desc->closing == true))
            {
              result = COM_SOCKETS_ERR_CLOSING;
              PRINT_INFO("no data send and socket closing force result")
            }
          }
          com_ip_modem_idlemode_request(false);
        }
      }
      else
      {
        PRINT_ERR("snd data NOK socket closing")
        result = COM_SOCKETS_ERR_CLOSING;
      }
    }
    else
    {
      PRINT_ERR("snd data NOK err state")
      if (p_socket_desc->state < COM_SOCKET_CONNECTED)
      {
        result = COM_SOCKETS_ERR_STATE;
      }
      else
      {
        result = (p_socket_desc->state == COM_SOCKET_CLOSING) ? COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
      }
    }

    /* Update statistic counter */
    if (p_socket_desc->type == (uint8_t)COM_SOCK_STREAM)
    {
      com_sockets_statistic_update((result >= 0) ? COM_SOCKET_STAT_SND_OK : COM_SOCKET_STAT_SND_NOK);
    }
    else
    {
      /* Do not count twice: sendto() call send() and sendto() will update statistic counter */
#if (UDP_SERVICE_SUPPORTED == 0U)
      com_sockets_statistic_update((result >= 0) ? COM_SOCKET_STAT_SND_OK : COM_SOCKET_STAT_SND_NOK);
#else /* UDP_SERVICE_SUPPORTED == 1U */
      /* Statistic updated by sendto() */
      __NOP();
#endif /* UDP_SERVICE_SUPPORTED == 0U */
    }
  }

  if (result > 0)
  {
    SOCKET_SET_ERROR(p_socket_desc, COM_SOCKETS_ERR_OK);
  }
  else
  {
    SOCKET_SET_ERROR(p_socket_desc, result);
  }

  return (result);
}


/**
  * @brief  Socket send to data
  * @note   Send data to a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to send
  * @note   see below
  * @param  len       - length of the data to send (in bytes)
  * @note   see below
  * @param  flags     - options
  * @note
  *         - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         if len of buffer to send > interface between COM and low level.
  *          The maximum of interface will be send (only one send)
  *         - if flags = COM_MSG_WAIT, application accept to wait
  *         if len of buffer to send > interface between COM and low level.
  *          COM will fragment the buffer according to the interface (multiple sends)
  * @param  to        - remote IP address and port
  * @note   only an IPv4 address is supported
  * @param  tolen     - remote IP length
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_sendto_ip_modem(int32_t sock,
                            const com_char_t *buf, int32_t len,
                            int32_t flags,
                            const com_sockaddr_t *to, int32_t tolen)
{
#if (UDP_SERVICE_SUPPORTED == 0U)
  UNUSED(to);    /* parameter is unused */
  UNUSED(tolen); /* parameter is unused */
#endif /* UDP_SERVICE_SUPPORTED == 0U */
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if ((p_socket_desc != NULL) && (buf != NULL) && (len > 0))
  {
    if (p_socket_desc->type == (uint8_t)COM_SOCK_STREAM)
    {
      /* sendto service may be called and it is changed to send service */
      result = com_send_ip_modem(sock, buf, len, flags);
    }
    else /* p_socket_desc->type == (uint8_t)COM_SOCK_DGRAM */
    {
      /* If Modem doesn't support sendto rather than to test:
         if connect already done by appli
         and if IPaddress in sendto equal to IPaddress in connect
         decision is to return an error
      */
#if (UDP_SERVICE_SUPPORTED == 0U)
      {
        result = COM_SOCKETS_ERR_UNSUPPORTED;
      }
#else /* UDP_SERVICE_SUPPORTED == 1U */
      {
        bool is_network_up;
        socket_addr_t socket_addr;

        /* Check remote addr is valid */
        if ((to != NULL) && (tolen != 0))
        {
          if (com_translate_ip_address(to, tolen, &socket_addr) == true)
          {
            result = COM_SOCKETS_ERR_OK;
          }
          /* else result = COM_SOCKETS_ERR_PARAMETER */
        }
        /* No address provided by connect previously done */
        /* a send translate to sendto */
        /* Use IPaddress of connect */
        else if ((to == NULL) && (tolen == 0) && (p_socket_desc->remote_addr.addr != 0U))
        {
          com_sockaddr_in_t sockaddr_in;
          com_ip_addr_t remote_addr;
          uint16_t remote_port;

          remote_addr.addr = p_socket_desc->remote_addr.addr;
          remote_port = p_socket_desc->remote_port;
          com_convert_ipaddr_port_to_sockaddr(&remote_addr, remote_port, &sockaddr_in);

          if (com_translate_ip_address((com_sockaddr_t *)&sockaddr_in, (int32_t)sizeof(sockaddr_in), &socket_addr)
              == true)
          {
            result = COM_SOCKETS_ERR_OK;
          }
          else
          {
            /* else result = COM_SOCKETS_ERR_PARAMETER */
          }
        }
        else
        {
          /* else result = COM_SOCKETS_ERR_PARAMETER */
        }

        if (result == COM_SOCKETS_ERR_OK)
        {
          /* If socket state == CREATED implicit bind and connect UDP service must be done */
          /* Without updating internal parameters
             => com_ip_modem_connect must not be called */
          result = com_ip_modem_connect_udp_service(p_socket_desc);

          /* closing maybe received, refuse to send data */
          if ((result == COM_SOCKETS_ERR_OK)
              && (p_socket_desc->closing == false)
              && (p_socket_desc->state == COM_SOCKET_CONNECTED))
          {
            /* network maybe down, refuse to send data */
            if (com_ip_modem_is_network_up() == false)
            {
              result = COM_SOCKETS_ERR_NONETWORK;
              PRINT_ERR("sndto data NOK no network")
            }
            else
            {
              uint32_t length_to_send;
              uint32_t length_send;

              result = COM_SOCKETS_ERR_GENERAL;
              length_send = 0U;
              p_socket_desc->state = COM_SOCKET_SENDING;

              com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */

              if (flags == COM_MSG_DONTWAIT)
              {
                length_to_send = COM_MIN((uint32_t)len, COM_MODEM_MAX_TX_DATA_SIZE);

                if (osCDS_socket_sendto(p_socket_desc->id, buf, length_to_send,
                                        socket_addr.ip_type, socket_addr.ip_value, socket_addr.port)
                    == CS_OK)
                {
                  length_send = length_to_send;
                  result = (int32_t)length_send;
                  PRINT_INFO("sndto data DONTWAIT ok")
                }
                else
                {
                  PRINT_ERR("sndto data DONTWAIT NOK at low level")
                }
                p_socket_desc->state = COM_SOCKET_CONNECTED;
              }
              else
              {
                is_network_up = com_ip_modem_is_network_up();
                /* Send all data of a big buffer - Whatever the size */
                while ((length_send != (uint32_t)len)
                       && (p_socket_desc->closing == false)
                       && (is_network_up == true)
                       && (p_socket_desc->state == COM_SOCKET_SENDING))
                {
                  length_to_send = COM_MIN((((uint32_t)len) - length_send), COM_MODEM_MAX_TX_DATA_SIZE);
                  com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
                  /* A tempo is already managed at low-level */
                  if (osCDS_socket_sendto(p_socket_desc->id, (buf + length_send), length_to_send,
                                          socket_addr.ip_type, socket_addr.ip_value, socket_addr.port)
                      == CS_OK)
                  {
                    length_send += length_to_send;
                    PRINT_INFO("sndto data ok")
                    /* Update Network status */
                    is_network_up = com_ip_modem_is_network_up();
                  }
                  else
                  {
                    p_socket_desc->state = COM_SOCKET_CONNECTED;
                    PRINT_ERR("sndto data NOK at low level")
                  }
                  com_ip_modem_idlemode_request(false);
                }
                p_socket_desc->state = COM_SOCKET_CONNECTED;
                result = (int32_t)length_send;
              }
              /* If no data send and socket is closing : force ERR_CLOSING */
              if ((length_send == 0U) && (p_socket_desc->closing == true))
              {
                result = COM_SOCKETS_ERR_CLOSING;
                PRINT_INFO("no data send and socket closing force result")
              }
              com_ip_modem_idlemode_request(false);
            }
          }
          else
          {
            if (p_socket_desc->closing == true)
            {
              PRINT_ERR("sndto data NOK socket closing")
              result = COM_SOCKETS_ERR_CLOSING;
            }
            else
            {
              /* else result already updated com_ip_modem_connect_udp_service */
            }
          }

          com_sockets_statistic_update((result >= 0) ? COM_SOCKET_STAT_SND_OK : COM_SOCKET_STAT_SND_NOK);
        }
        else
        {
          /* result = COM_SOCKETS_ERR_PARAMETER */
        }
      }
#endif /* UDP_SERVICE_SUPPORTED == 0U */
    }
  }

  if (result > 0)
  {
    SOCKET_SET_ERROR(p_socket_desc, COM_SOCKETS_ERR_OK);
  }
  else
  {
    SOCKET_SET_ERROR(p_socket_desc, result);
  }

  return (result);
}


/**
  * @brief  Socket receive data
  * @note   Receive data on already connected socket
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to store the data to
  * @note   see below
  * @param  len       - size of application data buffer (in bytes)
  * @note   even if len > interface between COM and low level
  *         a maximum of the interface capacity can be received
  *         at each function call
  * @param  flags     - options
  * @note
  *         - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         until data are available at low level
  *         - if flags = COM_MSG_WAIT, application accept to wait
  *         until data are available at low level with respect of potential
  *         timeout COM_SO_RCVTIMEO setting
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recv_ip_modem(int32_t sock, com_char_t *buf, int32_t len, int32_t flags)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  int32_t len_rcv = 0;
  com_socket_msg_t msg_queue;
  rtosalStatus status_queue;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if ((p_socket_desc != NULL) && (buf != NULL) && (len > 0))
  {
    /* Closing maybe received or Network maybe done
       but still some data to read */
    if (p_socket_desc->state == COM_SOCKET_CONNECTED)
    {
      uint32_t length_to_read;
      length_to_read = COM_MIN((uint32_t)len, COM_MODEM_MAX_RX_DATA_SIZE);
      p_socket_desc->state = COM_SOCKET_WAITING;

      com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */

      /* Empty the queue from possible messages */
      com_ip_modem_empty_queue(p_socket_desc->queue);

      if (flags == COM_MSG_DONTWAIT)
      {

        /* Application don't want to wait if there is no data available */
        len_rcv = osCDS_socket_receive(p_socket_desc->id, buf, length_to_read);
        result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
        p_socket_desc->state = COM_SOCKET_CONNECTED;
        PRINT_DBG("rcv data DONTWAIT")
      }
      else
      {
        /* Maybe still some data available
           because application don't read all data with previous calls */
        PRINT_DBG("rcv data waiting")
        len_rcv = osCDS_socket_receive(p_socket_desc->id, buf, length_to_read);
        PRINT_DBG("rcv data waiting exit")

        if (len_rcv == 0)
        {
          /* Waiting for Distant response or Closure Socket or Timeout */
          msg_queue = 0U;
          status_queue = rtosalMessageQueueGet(p_socket_desc->queue, &msg_queue, p_socket_desc->rcv_timeout);
          /* For Timeout returned result :
           * osEventTimeout in case cmsisV1
           * osErrorTimeoutResource = osErrorTimeout in case cmsisV2
           * osErrorTimeoutResource defined for cmsisV1 and V2 so better than osErrorTimeout */
          if ((status_queue == (rtosalStatus)osEventTimeout) || (status_queue == (rtosalStatus)osErrorTimeoutResource))
          {
            result = COM_SOCKETS_ERR_TIMEOUT;
            p_socket_desc->state = COM_SOCKET_CONNECTED;
            PRINT_INFO("rcv data exit timeout")
          }
          else
          {
            /* A message is in the queue and it it a SOCKET one */
            com_socket_msg_type_t msg_type = GET_SOCKET_MSG_TYPE(msg_queue);
            com_socket_msg_type_t msg_id   = GET_SOCKET_MSG_ID(msg_queue);
            if ((msg_queue != 0U) && (msg_type == COM_SOCKET_MSG))
            {
              switch (msg_id)
              {
                case COM_DATA_RCV :
                {
                  len_rcv = osCDS_socket_receive(p_socket_desc->id, buf, length_to_read);
                  result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  if (len_rcv == 0)
                  {
                    PRINT_DBG("rcv data exit with no data")
                  }
                  PRINT_INFO("rcv data exit with data")
                  break;
                }
                case COM_CLOSING_RCV :
                {
                  result = COM_SOCKETS_ERR_CLOSING;
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  PRINT_INFO("rcv data exit socket closing")
                  break;
                }
                default :
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  PRINT_ERR("rcv data exit NOK impossible case")
                  break;
                }
              }
            }
            else
            {
              /* Error or empty queue */
              result = COM_SOCKETS_ERR_GENERAL;
              p_socket_desc->state = COM_SOCKET_CONNECTED;
              PRINT_ERR("rcv data msg NOK or empty queue")
            }
          }
        }
        else
        {
          result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
          p_socket_desc->state = COM_SOCKET_CONNECTED;
          PRINT_INFO("rcv data exit data available or err low level")
        }
      }

      /* Empty the queue from possible messages */
      com_ip_modem_empty_queue(p_socket_desc->queue);
      /* If no data received and socket is closing : force ERR_CLOSING */
      if ((len_rcv == 0) && (p_socket_desc->closing == true))
      {
        result = COM_SOCKETS_ERR_CLOSING;
        p_socket_desc->state = COM_SOCKET_CONNECTED;
        PRINT_INFO("no data received and socket closing force result")
      }

      com_ip_modem_idlemode_request(false);
    }
    else
    {
      PRINT_ERR("rcv data NOK err state")
      if (p_socket_desc->state < COM_SOCKET_CONNECTED)
      {
        result = COM_SOCKETS_ERR_STATE;
      }
      else
      {
        result = (p_socket_desc->state == COM_SOCKET_CLOSING) ? COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
      }
    }

    com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? COM_SOCKET_STAT_RCV_OK : COM_SOCKET_STAT_RCV_NOK);
  }

  SOCKET_SET_ERROR(p_socket_desc, result);

  return ((result == COM_SOCKETS_ERR_OK) ? len_rcv : result);
}


/**
  * @brief  Socket receive from data
  * @note   Receive data from a remote host
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to store the data to
  * @note   see below
  * @param  len       - size of application data buffer (in bytes)
  * @note   even if len > interface between COM and low level
  *         a maximum of the interface capacity can be received
  *         at each function call
  * @param  flags     - options
  * @note
  *         - if flags = COM_MSG_DONTWAIT, application request to not wait
  *         until data are available at low level
  *         - if flags = COM_MSG_WAIT, application accept to wait
  *         until data are available at low level with respect of potential
  *         timeout COM_SO_RCVTIMEO setting
  * @param  from      - remote IP address and port
  * @note   only an IPv4 address is supported
  *         if information is reported by the modem
  *         elsif value 0 is returned as remote IP address and port
  * @param  fromlen   - remote IP length
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recvfrom_ip_modem(int32_t sock, com_char_t *buf, int32_t len, int32_t flags,
                              com_sockaddr_t *from, int32_t *fromlen)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  int32_t len_rcv = 0;
  socket_desc_t *p_socket_desc;
  CS_CHAR_t     ip_addr_value[40];
  uint16_t      ip_remote_port = 0U;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);
  (void)strncpy((CSIP_CHAR_t *)&ip_addr_value[0], (const CSIP_CHAR_t *)"0.0.0.0", sizeof(ip_addr_value));
  if ((p_socket_desc != NULL) && (buf != NULL) && (len > 0))
  {
    if (p_socket_desc->type == (uint8_t)COM_SOCK_STREAM)
    {
      /* recvfrom service may be called and it is changed to recv service */
      result = com_recv_ip_modem(sock, buf, len, flags);
      /* If data received set remote addr parameter to the connected addr */
      if ((result > 0) && (from != NULL) && (fromlen != NULL) && (*fromlen >= (int32_t)sizeof(com_sockaddr_in_t)))
      {
        com_ip_addr_t remote_addr;
        uint16_t remote_port;

        remote_addr.addr = p_socket_desc->remote_addr.addr;
        remote_port = p_socket_desc->remote_port;
        com_convert_ipaddr_port_to_sockaddr(&remote_addr, remote_port, (com_sockaddr_in_t *)from);
        *fromlen = (int32_t)sizeof(com_sockaddr_in_t);
      }
    }
    else /* p_socket_desc->type == (uint8_t)COM_SOCK_DGRAM */
    {
      /* If Modem doesn't support recvfrom rather than to test:
         if connect already done by appli
         decision is to return an error
      */
#if (UDP_SERVICE_SUPPORTED == 0U)
      {
        result = COM_SOCKETS_ERR_UNSUPPORTED;
      }
#else /* UDP_SERVICE_SUPPORTED == 1U */
      {
        rtosalStatus status_queue;
        com_socket_msg_t msg_queue;
        CS_IPaddrType_t ip_addr_type = CS_IPAT_INVALID;

        com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */

        /* If socket state == CREATED implicit bind and connect must be done */
        /* Without updating internal parameters
           => com_ip_modem_connect must not be called */
        result = com_ip_modem_connect_udp_service(p_socket_desc);

        /* closing maybe received, refuse to send data */
        if ((result == COM_SOCKETS_ERR_OK) && (p_socket_desc->state == COM_SOCKET_CONNECTED))
        {
          uint32_t length_to_read;
          length_to_read = COM_MIN((uint32_t)len, COM_MODEM_MAX_RX_DATA_SIZE);
          p_socket_desc->state = COM_SOCKET_WAITING;

          /* Empty the queue from possible messages */
          com_ip_modem_empty_queue(p_socket_desc->queue);

          if (flags == COM_MSG_DONTWAIT)
          {
            /* Application don't want to wait if there is no data available */
            len_rcv = osCDS_socket_receivefrom(p_socket_desc->id, buf, length_to_read,
                                               &ip_addr_type, &ip_addr_value[0], &ip_remote_port);
            result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
            p_socket_desc->state = COM_SOCKET_CONNECTED;
            PRINT_DBG("rcvfrom data DONTWAIT")
          }
          else
          {
            /* Maybe still some data available
               because application don't read all data with previous calls */
            PRINT_DBG("rcvfrom data waiting")
            len_rcv = osCDS_socket_receivefrom(p_socket_desc->id, buf, length_to_read,
                                               &ip_addr_type, &ip_addr_value[0], &ip_remote_port);
            PRINT_DBG("rcvfrom data waiting exit")

            if (len_rcv == 0)
            {
              /* Waiting for Distant response or Closure Socket or Timeout */
              PRINT_DBG("rcvfrom data waiting on MSGqueue")
              msg_queue = 0U;
              status_queue = rtosalMessageQueueGet(p_socket_desc->queue, &msg_queue, p_socket_desc->rcv_timeout);
              PRINT_DBG("rcvfrom data exit from MSGqueue")
              /* For Timeout returned result :
               * osEventTimeout in case cmsisV1
               * osErrorTimeoutResource = osErrorTimeout in case cmsisV2
               * osErrorTimeoutResource defined for cmsisV1 and V2 so better than osErrorTimeout */
              if ((status_queue == (rtosalStatus)osEventTimeout)
                  || (status_queue == (rtosalStatus)osErrorTimeoutResource))
              {
                result = COM_SOCKETS_ERR_TIMEOUT;
                p_socket_desc->state = COM_SOCKET_CONNECTED;
                PRINT_INFO("rcvfrom data exit timeout")
              }
              else
              {
                /* A message is in the queue and it it a SOCKET one */
                com_socket_msg_type_t msg_type = GET_SOCKET_MSG_TYPE(msg_queue);
                com_socket_msg_type_t msg_id   = GET_SOCKET_MSG_ID(msg_queue);
                if ((msg_queue != 0U) && (msg_type == COM_SOCKET_MSG))
                {
                  switch (msg_id)
                  {
                    case COM_DATA_RCV :
                    {
                      len_rcv = osCDS_socket_receivefrom(p_socket_desc->id, buf, length_to_read,
                                                         &ip_addr_type, &ip_addr_value[0], &ip_remote_port);
                      result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
                      p_socket_desc->state = COM_SOCKET_CONNECTED;
                      if (len_rcv == 0)
                      {
                        PRINT_DBG("rcvfrom data exit with no data")
                      }
                      PRINT_INFO("rcvfrom data exit with data")
                      break;
                    }
                    case COM_CLOSING_RCV :
                    {
                      result = COM_SOCKETS_ERR_CLOSING;
                      p_socket_desc->state = COM_SOCKET_CONNECTED;
                      PRINT_INFO("rcvfrom data exit socket closing")
                      break;
                    }
                    default :
                    {
                      /* Impossible case */
                      result = COM_SOCKETS_ERR_GENERAL;
                      p_socket_desc->state = COM_SOCKET_CONNECTED;
                      PRINT_ERR("rcvfrom data exit NOK impossible case")
                      break;
                    }
                  }
                }
                else
                {
                  /* Error or empty queue */
                  result = COM_SOCKETS_ERR_GENERAL;
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  PRINT_ERR("rcvfrom data msg NOK or empty queue")
                }
              }
            }
            else
            {
              result = (len_rcv < 0) ? COM_SOCKETS_ERR_GENERAL : COM_SOCKETS_ERR_OK;
              p_socket_desc->state = COM_SOCKET_CONNECTED;
              PRINT_INFO("rcvfrom data exit data available or err low level")
            }
          }

          /* Empty the queue from possible messages */
          com_ip_modem_empty_queue(p_socket_desc->queue);

          /* If no data received and socket is closing : force ERR_CLOSING */
          if ((len_rcv == 0) && (p_socket_desc->closing == true))
          {
            result = COM_SOCKETS_ERR_CLOSING;
            p_socket_desc->state = COM_SOCKET_CONNECTED;
            PRINT_INFO("force result socket closing")
          }
        }
        else
        {
          result = (p_socket_desc->state == COM_SOCKET_CLOSING) ? COM_SOCKETS_ERR_CLOSING : COM_SOCKETS_ERR_INPROGRESS;
        }
        com_ip_modem_idlemode_request(false);

        com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? COM_SOCKET_STAT_RCV_OK : COM_SOCKET_STAT_RCV_NOK);
      }
#endif /* UDP_SERVICE_SUPPORTED == 0U */
    }

    /* Update output from and fromlen parameters */
    if ((len_rcv > 0) && (from != NULL) && (fromlen != NULL) && (*fromlen >= (int32_t)sizeof(com_sockaddr_in_t)))
    {
      if (com_convert_IPString_to_sockaddr(ip_remote_port, (com_char_t *)(&ip_addr_value[0]), from) == true)
      {
        *fromlen = (int32_t)sizeof(com_sockaddr_in_t);
      }
      else
      {
        *fromlen = 0;
      }
    }
  }

  SOCKET_SET_ERROR(p_socket_desc, result);

  return ((result == COM_SOCKETS_ERR_OK) ? len_rcv : result);
}


/**
  * @brief  Socket close
  * @note   Close a socket and release socket handle
  *         For an opened socket as long as socket close is in error value
  *         socket must be considered as not closed and handle as not released
  * @param  sock      - socket handle obtained with com_socket
  * @retval int32_t   - ok or error value
  */
int32_t com_closesocket_ip_modem(int32_t sock)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(sock, &socket_desc_list[0]);

  if (p_socket_desc != NULL)
  {
    /* If socket is currently under process refused to close it */
    if ((p_socket_desc->state == COM_SOCKET_SENDING) || (p_socket_desc->state == COM_SOCKET_WAITING))
    {
      PRINT_ERR("close socket NOK err state")
      result = COM_SOCKETS_ERR_INPROGRESS;
    }
    else
    {
      result = COM_SOCKETS_ERR_GENERAL;
      com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
      if (osCDS_socket_close(sock, 0U) == CS_OK)
      {
        com_ip_modem_invalid_socket_desc(sock, &socket_desc_list[0]);
        result = COM_SOCKETS_ERR_OK;
        PRINT_INFO("close socket ok")
      }
      else
      {
        PRINT_INFO("close socket NOK low level")
      }
      com_ip_modem_idlemode_request(true);
    }
    com_sockets_statistic_update((result == COM_SOCKETS_ERR_OK) ? COM_SOCKET_STAT_CLS_OK : COM_SOCKET_STAT_CLS_NOK);
  }

  return (result);
}


/*** Other functionalities ****************************************************/

/**
  * @brief  Get host IP from host name
  * @note   Retrieve host IP address from host name
  *         DNS resolver is a fix value in the module
  *         only a primary DNS is used
  * @param  name      - host name
  * @param  addr      - host IP corresponding to host name
  * @note   only IPv4 address is managed
  * @retval int32_t   - ok or error value
  */
int32_t com_gethostbyname_ip_modem(const com_char_t *name, com_sockaddr_t   *addr)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  CS_PDN_conf_id_t PDN_conf_id = CS_PDN_CONFIG_DEFAULT;
  CS_DnsReq_t  dns_req;
  CS_DnsResp_t dns_resp;

  if ((name != NULL) && (addr != NULL))
  {
    if (strlen((const CSIP_CHAR_t *)name) <= sizeof(dns_req.host_name))
    {
      (void)strncpy((CSIP_CHAR_t *)&dns_req.host_name[0], (const CSIP_CHAR_t *)name, sizeof(dns_req.host_name));
      result = COM_SOCKETS_ERR_GENERAL;
      com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */
      if (osCDS_dns_request(PDN_conf_id, &dns_req, &dns_resp) == CS_OK)
      {
        PRINT_INFO("DNS resolution OK - Remote: %s IP: %s", name, dns_resp.host_addr)
        if (com_convert_IPString_to_sockaddr(0U, (com_char_t *)&dns_resp.host_addr[0], addr) == true)
        {
          PRINT_DBG("DNS conversion OK")
          result = COM_SOCKETS_ERR_OK;
        }
        else
        {
          PRINT_ERR("DNS conversion NOK")
        }
      }
      else
      {
        PRINT_ERR("DNS resolution NOK for %s", name)
      }
      com_ip_modem_idlemode_request(false);
    }
  }

  return (result);
}


/**
  * @brief  Get peer name
  * @note   Retrieve IP address and port number
  *         NOT YET SUPPORTED
  * @param  sock      - socket handle obtained with com_socket
  * @param  name      - IP address and port number of the peer
  * @param  namelen   - name length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_getpeername_ip_modem(int32_t sock, com_sockaddr_t *name, int32_t *namelen)
{
  UNUSED(sock);
  UNUSED(name);
  UNUSED(namelen);
  return (COM_SOCKETS_ERR_UNSUPPORTED);
}


/**
  * @brief  Get sock name
  * @note   Retrieve local IP address and port number
  *         NOT YET SUPPORTED
  * @param  sock      - socket handle obtained with com_socket
  * @param  name      - IP address and port number
  * @param  namelen   - name length
  * @retval int32_t   - COM_SOCKETS_ERR_UNSUPPORTED
  */
int32_t com_getsockname_ip_modem(int32_t sock, com_sockaddr_t *name, int32_t *namelen)
{
  UNUSED(sock);
  UNUSED(name);
  UNUSED(namelen);
  return (COM_SOCKETS_ERR_UNSUPPORTED);
}


/*** Ping functionalities *****************************************************/

#if (USE_COM_PING == 1)
/**
  * @brief  Ping handle creation
  * @note   Create a ping session
  * @param  -
  * @retval int32_t  - ping handle or error value
  */
int32_t com_ping_ip_modem(void)
{
  int32_t result;
  socket_desc_t *p_socket_desc;

  /* Need to create a new p_socket_desc ? */
  p_socket_desc = com_ip_modem_provide_socket_desc(&socket_ping_desc);
  if (p_socket_desc != NULL)
  {
    /* Update socket descriptor */
    p_socket_desc->id    = 0x01;
    p_socket_desc->state = COM_SOCKET_CREATED;
    result = COM_SOCKETS_ERR_OK;
    com_ip_modem_wakeup_request(); /* to avoid to be stopped by a close socket */
  }
  else
  {
    result = COM_SOCKETS_ERR_NOMEMORY;
    PRINT_ERR("create ping NOK no memory")
    /* Socket descriptor is not existing in COM and nothing to do at low level */
  }

  return ((result == COM_SOCKETS_ERR_OK) ? p_socket_desc->id : result);
}


/**
  * @brief  Ping process request
  * @note   Process a ping
  * @param  ping     - ping handle obtained with com_ping
  * @param  addr     - remote IP address and port
  * @param  addrlen  - addr length
  * @param  timeout  - timeout for ping response (in sec)
  * @param  rsp      - ping response
  * @retval int32_t  - ok or error value
  */
int32_t com_ping_process_ip_modem(int32_t ping, const com_sockaddr_t *addr, int32_t addrlen,
                                  uint8_t timeout, com_ping_rsp_t *rsp)
{
  int32_t  result = COM_SOCKETS_ERR_PARAMETER;
  uint32_t timeout_ms;
  socket_desc_t *p_socket_desc;
  socket_addr_t  socket_addr;
  CS_Ping_params_t ping_params;
  CS_PDN_conf_id_t PDN_conf_id;
  rtosalStatus status_queue;
  com_socket_msg_t msg_queue;

  p_socket_desc = com_ip_modem_find_socket(ping, &socket_ping_desc);

  /* Check parameters validity and context */
  if (p_socket_desc != NULL)
  {
    /* No need each time to close the connection */
    if ((p_socket_desc->state == COM_SOCKET_CREATED) || (p_socket_desc->state == COM_SOCKET_CONNECTED))
    {
      if ((timeout != 0U) && (rsp != NULL) && (addr != NULL))
      {
        if (com_translate_ip_address(addr, addrlen, &socket_addr) == true)
        {
          /* Parameters are valid */
          result = COM_SOCKETS_ERR_OK;
        }
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_STATE;
      PRINT_ERR("ping send NOK state invalid")
    }
  }

  /* If parameters are valid continue the treatment */
  if (result == COM_SOCKETS_ERR_OK)
  {
    /* Check Network status */
    if (com_ip_modem_is_network_up() == true)
    {
      /* Because URC can be received before Reply and
         in URC not possible to distinguish several Ping in parallel
         authorized only one Ping at a time */
      (void)rtosalMutexAcquire(ComSocketsMutexHandle, RTOSAL_WAIT_FOREVER);
      /* Is a Ping already in progress ? */
      if (ping_socket_id != COM_SOCKET_INVALID_ID)
      {
        /* a Ping is already in progress */
        result = COM_SOCKETS_ERR_INPROGRESS;
      }
      else
      {
        /* No ping in progress => assign id to ping_socket_id */
        ping_socket_id = p_socket_desc->id;
        /* result already set to the correct value COM_SOCKETS_ERR_OK */
      }
      (void)rtosalMutexRelease(ComSocketsMutexHandle);

      if (result == COM_SOCKETS_ERR_OK)
      {
        PDN_conf_id = CS_PDN_CONFIG_DEFAULT;
        ping_params.timeout = timeout;
        ping_params.pingnum = 1U;
        (void)strncpy((CSIP_CHAR_t *)&ping_params.host_addr[0], (const CSIP_CHAR_t *)&socket_addr.ip_value[0],
                      sizeof(ping_params.host_addr));
        p_socket_desc->p_ping_rsp = rsp;

        com_ip_modem_wakeup_request(); /* Before to interact with the modem, wakeup it */

        /* Empty the queue from possible messages */
        com_ip_modem_empty_queue(p_socket_desc->queue);

        /* In order to receive response whatever the moment URC is received
           do not go under SENDING then WAITING state, set the state in WAITING directly */
        p_socket_desc->state = COM_SOCKET_WAITING;

        /* Case 1) URC received before Reply
           Case 2) Reply received before URC */
        if (osCDS_ping(PDN_conf_id, &ping_params, com_ip_modem_ping_rsp_cb) == CS_OK)
        {
          /* Case 1) URC already available in the queue -> timeout not apply */
          /* Case 2) URC are still expected -> timeout apply */
#if (PING_URC_RECEIVED_AFTER_REPLY == 1U)
          timeout_ms = (uint32_t)timeout * 1000U;
#else /* PING_URC_RECEIVED_AFTER_REPLY == 0U */
          timeout_ms = 0U;
#endif /* PING_URC_RECEIVED_AFTER_REPLY == 1U */

          msg_queue = 0U;
          status_queue = rtosalMessageQueueGet(p_socket_desc->queue, &msg_queue, timeout_ms);
          /* For Timeout returned result :
           * osEventTimeout in case cmsisV1
           * osErrorTimeoutResource = osErrorTimeout in case cmsisV2
           * osErrorTimeoutResource defined for cmsisV1 and V2 so better than osErrorTimeout */
          if ((status_queue == osEventTimeout) || (status_queue == osErrorTimeoutResource))
          {
            /* Case 1) Impossible case or URC lost */
            /* Case 2) No URC available */
            result = COM_SOCKETS_ERR_TIMEOUT;
            p_socket_desc->state = COM_SOCKET_CONNECTED;
            PRINT_INFO("ping exit timeout")
          }
          else
          {
            /* A message is in the queue and it it a PING one */
            com_socket_msg_type_t msg_type = GET_SOCKET_MSG_TYPE(msg_queue);
            com_socket_msg_type_t msg_id   = GET_SOCKET_MSG_ID(msg_queue);
            if ((msg_queue != 0U) && (msg_type == COM_PING_MSG))
            {
              switch (msg_id)
              {
                case COM_DATA_RCV :
                {
                  bool wait;
                  /* result already set to the correct value COM_SOCKETS_ERR_OK */
                  /* Sometimes rather than to receive only one report, 255 reports are received
                     Wait final report - in order to not reject next ping */
#if (PING_URC_RECEIVED_AFTER_REPLY == 1U)
                  timeout_ms = (uint32_t)COM_MIN(timeout, 5U) * 1000U;
#else /* PING_URC_RECEIVED_AFTER_REPLY == 0U */
                  timeout_ms = 0U; /* reply is send so final report must be available */
#endif /* PING_URC_RECEIVED_AFTER_REPLY == 1U */
                  wait = true;
                  while (wait == true)
                  {
                    msg_queue = 0U;
                    status_queue = rtosalMessageQueueGet(p_socket_desc->queue, &msg_queue, timeout_ms);
                    /* For Timeout returned result :
                     * osEventTimeout in case cmsisV1
                     * osErrorTimeoutResource = osErrorTimeout in case cmsisV2
                     * osErrorTimeoutResource defined for cmsisV1 and V2 so better than osErrorTimeout */
                    if ((status_queue == osEventTimeout) || (status_queue == osErrorTimeoutResource))
                    {
                      wait = false;
                    }
                    else
                    {
                      msg_type = GET_SOCKET_MSG_TYPE(msg_queue);
                      msg_id   = GET_SOCKET_MSG_ID(msg_queue);
                      if ((msg_queue != 0U) && (msg_type == COM_PING_MSG) && (msg_id == COM_CLOSING_RCV))
                      {
                        wait = false;
                      }
                    }
                  }
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  break;
                }
                case COM_CLOSING_RCV :
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  PRINT_ERR("rcv data exit NOK closing case")
                  break;
                }
                default :
                {
                  /* Impossible case */
                  result = COM_SOCKETS_ERR_GENERAL;
                  p_socket_desc->state = COM_SOCKET_CONNECTED;
                  PRINT_ERR("rcv data exit NOK impossible case")
                  break;
                }
              }
            }
            else
            {
              /* Error or empty queue */
              result = COM_SOCKETS_ERR_GENERAL;
              p_socket_desc->state = COM_SOCKET_CONNECTED;
              PRINT_ERR("ping data msg NOK or empty queue")
            }
          }
        }
        else
        {
          result = COM_SOCKETS_ERR_GENERAL;
          p_socket_desc->state = COM_SOCKET_CONNECTED;
          PRINT_ERR("ping send NOK at low level")
        }
        com_ip_modem_idlemode_request(false);

        /* Empty the queue from possible messages */
        com_ip_modem_empty_queue(p_socket_desc->queue);

        /* Release Ping */
        ping_socket_id = COM_SOCKET_INVALID_ID;
      }
    }
    else
    {
      result = COM_SOCKETS_ERR_NONETWORK;
      PRINT_ERR("ping send NOK no network")
    }
  }

  return (result);
}


/**
  * @brief  Ping close
  * @note   Close a ping session and release ping handle
  * @param  ping      - ping handle obtained with com_ping
  * @retval int32_t   - ok or error value
  */
int32_t com_closeping_ip_modem(int32_t ping)
{
  int32_t result = COM_SOCKETS_ERR_PARAMETER;
  const socket_desc_t *p_socket_desc;

  p_socket_desc = com_ip_modem_find_socket(ping, &socket_ping_desc);

  if (p_socket_desc != NULL)
  {
    /* If socket is currently under process refused to close it */
    if ((p_socket_desc->state == COM_SOCKET_SENDING) || (p_socket_desc->state == COM_SOCKET_WAITING))
    {
      PRINT_ERR("close ping NOK err state")
      result = COM_SOCKETS_ERR_INPROGRESS;
    }
    else
    {
      com_ip_modem_invalid_socket_desc(ping, &socket_ping_desc);
      result = COM_SOCKETS_ERR_OK;
      PRINT_INFO("close ping ok")
      com_ip_modem_idlemode_request(true); /* same behavior than all sockets closed */
    }
  }

  return (result);
}

#endif /* USE_COM_PING == 1 */


/*** Component Initialization/Start *******************************************/
/*** Used by com_sockets module - Not an User Interface ***********************/

/**
  * @brief  Component initialization
  * @note   must be called only one time and
  *         before using any other functions of com_*
  * @param  -
  * @retval bool - true/false - init ok/nok
  */
bool com_init_ip_modem(void)
{
  bool result = false;
  uint8_t i = 0U;

  /* Initialize Network status */
  com_sockets_network_is_up = false; /* Network status update by Datacache see com_socket_datacache_cb() */

#if (USE_COM_PING == 1)
  ping_socket_id = COM_SOCKET_INVALID_ID;
#endif /* USE_COM_PING == 1 */

#if (UDP_SERVICE_SUPPORTED == 1U)
  com_local_port = 0U; /* com_start_ip in charge to initialize it to a random value */
#endif /* UDP_SERVICE_SUPPORTED == 1U */

  /* Initialize Mutex to protect socket descriptor list access */
  ComSocketsMutexHandle = rtosalMutexNew((const rtosal_char_t *)"COMSOCKIP_MUT_SOCKET_LIST");
  if (ComSocketsMutexHandle != NULL)
  {
    result = true;
    /* First element of the list is always created */
    /* Initialize the whole socket descriptor list */
    while ((i < COM_SOCKETS_IP_MODEM_NUMBER) && (result == true))
    {
      /* Create and initialize socket_desc items */
      if (com_ip_modem_create_static_socket_desc(&socket_desc_list[i]) == true)
      {
#if (COM_SOCKETS_IP_MODEM_NUMBER == 1U)
        __NOP(); /* Item is correctly initialized */
#else  /* COM_SOCKETS_IP_MODEM_NUMBER > 1U */
        if (i > 0U)
        {
          /* Creation of the element is OK, update field p_next of the previous element */
          socket_desc_list[i - 1U].p_next = &socket_desc_list[i];
        }
#endif /* COM_SOCKETS_IP_MODEM_NUMBER == 1U */
        i++; /* Point on next potential items of the static list */
      }
      else
      {
        /* Error in creation of a descriptor, stop the treatment */
        result = false;
        PRINT_ERR("Modem socket %d creation NOK", i)
      }
    }
  }

#if (USE_COM_PING == 1)
  /* Continue initialization ? */
  if (result == true)
  {
    /* Initialize the whole socket descriptor list */
    /* Create and initialize socket_desc items */
    if (com_ip_modem_create_static_socket_desc(&socket_ping_desc) == false)
    {
      /* Error in creation of a descriptor, stop the treatment */
      result = false;
      PRINT_ERR("Ping socket creation NOK")
    }
  }
#endif /* USE_COM_PING == 1 */

#if (USE_LOW_POWER == 1)
  /* Default initialization */
  ComTimerInactivityId = NULL;
  ComTimerInactivityMutexHandle = NULL;
  com_timer_inactivity_state = COM_TIMER_INVALID;
  com_nb_wake_up = 0U;

  /* Continue initialization ? */
  if (result == true)
  {
    result = false;
    /* Initialize Timer inactivity and its Mutex to check inactivity on socket */
    ComTimerInactivityId = rtosalTimerNew((const rtosal_char_t *)"COMSOCKIP_TIM_INACTIVITY",
                                          (os_ptimer)com_ip_modem_timer_inactivity_cb, osTimerOnce, NULL);
    if (ComTimerInactivityId != NULL)
    {
      ComTimerInactivityMutexHandle = rtosalMutexNew((const rtosal_char_t *)"COMSOCKIP_MUT_INACTIVITY");
      if (ComTimerInactivityMutexHandle != NULL)
      {
        /* Timer inactivivity initialized correctly */
        com_timer_inactivity_state = COM_TIMER_IDLE;
        result = true;
      }
      else
      {
        /* Cleanup */
        (void)rtosalTimerDelete(ComTimerInactivityId);
        ComTimerInactivityId = NULL;
        /* result already set to false */
      }
    }
    /* else result already set to false */
    if (result == false)
    {
      PRINT_ERR("Low power initialization NOK")
    }
  }
#endif /* USE_LOW_POWER == 1 */

  return (result);
}


/**
  * @brief  Component start
  * @note   must be called only one time but
  *         after com_init and dc_start
  *         and before using any other functions of com_*
  * @param  -
  * @retval -
  */
void com_start_ip_modem(void)
{
  /* Datacache registration for netwok on/off status */
  (void)dc_com_core_register_gen_event_cb(&dc_com_db, com_socket_datacache_cb, (void *)NULL);

#if (UDP_SERVICE_SUPPORTED == 1U)
  uint32_t random;

  /* Initialize local port to a random value */
#if defined(TFM_PSA_API)
  /* Decision to not call psa/crypto.h service */
  random = (uint32_t)rand();
#elif defined(RNG_HANDLE)
  if (HAL_RNG_GenerateRandomNumber(&RNG_HANDLE, &random) != HAL_OK)
  {
    /* Maybe a temporary error - try a 2nd time */
    if (HAL_RNG_GenerateRandomNumber(&RNG_HANDLE, &random) != HAL_OK)
    {
      /* Backup solution */
      PRINT_INFO("Backup solution used for random number generation")
      random = (uint32_t)rand();
    }
  }
#else /* !defined(TFM_PSA_API) && !defined(RNG_HANDLE) */
  random = (uint32_t)rand();
#endif /* TFM_PSA_API */
  random = random & (uint32_t)0xFFFF;
  random = random & ~COM_LOCAL_PORT_BEGIN;
  random = random + COM_LOCAL_PORT_BEGIN;
  com_local_port = (uint16_t)(random);
#endif /* UDP_SERVICE_SUPPORTED == 1U */
}

#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_MODEM */

#endif /* USE_COM_SOCKETS == 1 */

