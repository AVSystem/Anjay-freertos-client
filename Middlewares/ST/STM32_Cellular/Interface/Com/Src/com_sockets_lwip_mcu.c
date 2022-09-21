/**
  ******************************************************************************
  * @file    com_sockets_lwip_mcu.c
  * @author  MCD Application Team
  * @brief   This file implements Socket LwIP on MCU side
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
#include "com_sockets_lwip_mcu.h"

#if (USE_COM_SOCKETS == 1)

#if (USE_SOCKETS_TYPE == USE_SOCKETS_LWIP)
#include "rtosal.h"

#include "com_sockets_net_compat.h"
#include "com_trace.h"

/* LwIP is a Third Party so MISRAC messages linked to it are ignored */
/*cstat -MISRAC2012-* */
#include "lwip/sockets.h"
#include "lwip/api.h"     /* for netconn_gethostbyname */
#include "lwip/tcpip.h"   /* for tcp_init */
/* ICMP include for Ping */
#include "lwip/icmp.h"
#include "lwip/inet_chksum.h"
#include "lwip/ip4.h"
/*cstat +MISRAC2012-* */

/* Private defines -----------------------------------------------------------*/
#if (USE_COM_PING == 1)
#define COM_PING_DATA_SIZE          32U
#define COM_PING_ID             0x1234U
#define COM_PING_MUTEX_TIMEOUT   10000U /* in ms : 1 sec. */
#define COM_PING_RSP_LEN_MAX        80U /* in bytes */
#endif /* USE_COM_PING == 1 */

/* Private typedef -----------------------------------------------------------*/
typedef char COM_SOCKETS_IP_CHAR_t; /* used in lwip service call */

/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Mutex to protect Ping process - only one Ping at a time is authorized */
/* only one Ping to avoid to many buffer allocation to send/receive message */
#if (USE_COM_PING == 1)
static osMutexId ComPingMutexHandle;

static struct icmp_echo_hdr *ping_snd;
static com_char_t *ping_rcv;
static uint8_t ping_seqno;
#endif /* USE_COM_PING == 1 */

/* Global variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Functions Definition ------------------------------------------------------*/


/*** Socket management ********************************************************/

/**
  * @brief  Socket handle creation
  * @note   Create a communication endpoint called socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  family   - address family
  * @param  type     - connection type
  * @param  protocol - protocol type
  * @retval int32_t  - socket handle or error value
  */
int32_t com_socket_lwip_mcu(int32_t family, int32_t type, int32_t protocol)
{
  return (lwip_socket(family, type, protocol));
}


/**
  * @brief  Socket option set
  * @note   Set option for the socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  level     - level at which the option is defined
  * @param  optname   - option name for which the value is to be set
  * @param  optval    - pointer to the buffer containing the option value
  * @param  optlen    - size of the buffer containing the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_setsockopt_lwip_mcu(int32_t sock, int32_t level, int32_t optname,
                                const void *optval, int32_t optlen)
{
  return (lwip_setsockopt(sock, level, optname, optval, optlen));
}


/**
  * @brief  Socket option get
  * @note   Get option for a socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  level     - level at which option is defined
  * @param  optname   - option name for which the value is requested
  * @param  optval    - pointer to the buffer that will contain the option value
  * @param  optlen    - size of the buffer that will contain the option value
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockopt_lwip_mcu(int32_t sock, int32_t level, int32_t optname,
                                void *optval, int32_t *optlen)
{
  return (lwip_getsockopt(sock, level, optname, optval, optlen));
}


/**
  * @brief  Socket bind
  * @note   Assign a local address and port to a socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  addr      - local IP address and port
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_bind_lwip_mcu(int32_t sock,
                          const com_sockaddr_t *addr, int32_t addrlen)
{
  return (lwip_bind(sock, (const struct sockaddr *)addr, addrlen));
}


/**
  * @brief  Socket listen
  * @note   Set socket in listening mode
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  backlog   - number of connection requests that can be queued
  * @retval int32_t   - ok or error value
  */
int32_t com_listen_lwip_mcu(int32_t sock,
                            int32_t backlog)
{
  return (lwip_listen(sock, backlog));
}


/**
  * @brief  Socket accept
  * @note   Accept a connect request for a listening socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  addr      - IP address and port number of the accepted connection
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_accept_lwip_mcu(int32_t sock,
                            com_sockaddr_t *addr, int32_t *addrlen)
{
  return (lwip_accept(sock, (struct sockaddr *)addr, addrlen));
}


/**
  * @brief  Socket connect
  * @note   Connect socket to a remote host
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  addr      - remote IP address and port
  * @param  addrlen   - addr length
  * @retval int32_t   - ok or error value
  */
int32_t com_connect_lwip_mcu(int32_t sock,
                             const com_sockaddr_t *addr, int32_t addrlen)
{
  return (lwip_connect(sock, (const struct sockaddr *)addr, addrlen));
}


/**
  * @brief  Socket send data
  * @note   Send data on already connected socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to send
  * @param  len       - length of the data to send (in bytes)
  * @param  flags     - options
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_send_lwip_mcu(int32_t sock,
                          const com_char_t *buf, int32_t len,
                          int32_t flags)
{
  return (lwip_send(sock, buf, (size_t)len, flags));
}


/**
  * @brief  Socket send to data
  * @note   Send data to a remote host
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to send
  * @param  len       - length of the data to send (in bytes)
  * @param  flags     - options
  * @param  to        - remote IP address and port number
  * @param  tolen     - remote IP length
  * @retval int32_t   - number of bytes sent or error value
  */
int32_t com_sendto_lwip_mcu(int32_t sock,
                            const com_char_t *buf, int32_t len,
                            int32_t flags,
                            const com_sockaddr_t *to, int32_t tolen)
{
  return (lwip_sendto(sock, buf, (size_t)len, flags, (const struct sockaddr *)to, tolen));
}


/**
  * @brief  Socket receive data
  * @note   Receive data on already connected socket
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to store the data to
  * @param  len       - size of application data buffer (in bytes)
  * @param  flags     - options
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recv_lwip_mcu(int32_t sock,
                          com_char_t *buf, int32_t len,
                          int32_t flags)
{
  return (lwip_recv(sock, buf, (size_t)len, flags));
}


/**
  * @brief  Socket receive from data
  * @note   Receive data from a remote host
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  buf       - pointer to application data buffer to store the data to
  * @param  len       - size of application data buffer (in bytes)
  * @param  flags     - options
  * @param  from      - remote IP address and port number
  * @param  fromlen   - remote IP length
  * @retval int32_t   - number of bytes received or error value
  */
int32_t com_recvfrom_lwip_mcu(int32_t sock,
                              com_char_t *buf, int32_t len,
                              int32_t flags,
                              com_sockaddr_t *from, int32_t *fromlen)
{
  return (lwip_recvfrom(sock, buf, (size_t)len, flags, (struct sockaddr *)from, fromlen));
}


/**
  * @brief  Socket close
  * @note   Close a socket and release socket handle
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @retval int32_t   - ok or error value
  */
int32_t com_closesocket_lwip_mcu(int32_t sock)
{
  return (lwip_close(sock));
}


/*** Other functionalities ****************************************************/

/**
  * @brief  Get host IP from host name
  * @note   Retrieve host IP address from host name
  *         Restrictions, if any, are linked to LwIP module used
  * @param  name      - host name
  * @param  addr      - host IP corresponding to host name
  * @retval int32_t   - ok or error value
  */
int32_t com_gethostbyname_lwip_mcu(const com_char_t *name,
                                   com_sockaddr_t   *addr)
{
  int32_t result = (int32_t)ERR_ARG;

  if (addr != NULL) /* parameter name is checked by netconn_gethostbyname() */
  {
    com_ip_addr_t ip_addr;

    result = netconn_gethostbyname((const COM_SOCKETS_IP_CHAR_t *)name, &ip_addr);

    if (result == 0)
    {
      /* Format output addr parameter */
      (void)memset(addr, 0, sizeof(com_sockaddr_t));
      addr->sa_family = (uint8_t)COM_AF_INET;
      addr->sa_len    = (uint8_t)sizeof(com_sockaddr_in_t);
      /* Set sin_port is done by memset */
      /* ((com_sockaddr_in_t *)addr)->sin_port = 0U; */
      ((com_sockaddr_in_t *)addr)->sin_addr.s_addr = ip_addr.addr;
    }
  }

  return (result);
}


/**
  * @brief  Get peer name
  * @note   Retrieve IP address and port number
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  name      - IP address and port number of the peer
  * @param  namelen   - name length
  * @retval int32_t   - ok or error value
  */
int32_t com_getpeername_lwip_mcu(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen)
{
  return (lwip_getpeername(sock, (struct sockaddr *)name, namelen));
}


/**
  * @brief  Get sock name
  * @note   Retrieve local IP address and port number
  *         Restrictions, if any, are linked to LwIP module used
  * @param  sock      - socket handle obtained with com_socket
  * @param  name      - IP address and port number
  * @param  namelen   - name length
  * @retval int32_t   - ok or error value
  */
int32_t com_getsockname_lwip_mcu(int32_t sock,
                                 com_sockaddr_t *name, int32_t *namelen)
{
  return (lwip_getsockname(sock, (struct sockaddr *)name, namelen));
}


/*** Ping functionalities *****************************************************/

#if (USE_COM_PING == 1)
/**
  * @brief  Ping handle creation
  * @note   Create a ping session
  * @param  -
  * @retval int32_t  - ping handle or error value
  */
int32_t com_ping_lwip_mcu(void)
{
  int32_t result;

  /* Only one Ping session at a time */
  if (rtosalMutexAcquire(ComPingMutexHandle, COM_PING_MUTEX_TIMEOUT) == osOK)
  {
    result = com_socket_lwip_mcu(COM_AF_INET, COM_SOCK_RAW, COM_IPPROTO_ICMP);
    /* Possible to reset ping_seqno to 0 because only one Ping at a time */
    ping_seqno = 0U;
  }
  else
  {
    result = (int32_t)ERR_WOULDBLOCK;
  }

  return (result);
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
int32_t com_ping_process_lwip_mcu(int32_t ping,
                                  const com_sockaddr_t *addr, int32_t addrlen,
                                  uint8_t timeout, com_ping_rsp_t *rsp)
{
  int32_t result;
  uint32_t ping_size;
  /* Manage the send to the distant */
  uint32_t timeout_ms;
  uint32_t ping_time;
  /* Manage the response from the distant */
  int32_t fromlen;
  com_sockaddr_in_t from;
  struct ip_hdr *p_ip_hdr;
  struct icmp_echo_hdr *p_icmp_echo;

  /* Check input parameters and memory availability */
  if ((ping < 0) || (rsp == NULL) || (addr == NULL))
  {
    result = (int32_t)ERR_ARG;
  }
  else if ((ping_snd == NULL) || (ping_rcv == NULL))
  {
    result = (int32_t)ERR_MEM;
  }
  else
  {
    result = (int32_t)ERR_OK;
  }

  /* if all is OK, do the treatment: set timeout, send trame, receive trame */
  if (result == (int32_t)ERR_OK)
  {
    rsp->time = 0U;
    rsp->size = 0U;
    rsp->ttl  = 0U;
    timeout_ms = (uint32_t)timeout * 1000U;

    /* Set timeout */
    result = com_setsockopt_lwip_mcu(ping, COM_SOL_SOCKET, COM_SO_RCVTIMEO, &timeout_ms, (int32_t)sizeof(timeout_ms));
    if (result == (int32_t)ERR_OK)
    {
      PRINT_DBG("ping setsockopt OK")
      /* Format icmp message to send */
      ICMPH_TYPE_SET(ping_snd, ICMP_ECHO);
      ICMPH_CODE_SET(ping_snd, 0);
      ping_snd->chksum = 0U;
      ping_snd->id     = COM_PING_ID;
      ping_seqno++;
      ping_snd->seqno  = ping_seqno;
      ping_size = sizeof(struct icmp_echo_hdr) + COM_PING_DATA_SIZE;

      /* Fill the additional data buffer with some data */
      for (uint8_t i = (uint8_t)sizeof(struct icmp_echo_hdr); i < COM_PING_DATA_SIZE; i++)
      {
        ((COM_SOCKETS_IP_CHAR_t *)ping_snd)[i] = (COM_SOCKETS_IP_CHAR_t)i;
      }

      ping_snd->chksum = inet_chksum(ping_snd, (uint16_t)sizeof(struct icmp_echo_hdr) + (uint16_t)COM_PING_DATA_SIZE);

      /* Send the trame */
      result = com_sendto_lwip_mcu(ping, (const com_char_t *)ping_snd, (int32_t)ping_size, 0,
                                   (const com_sockaddr_t *)addr, addrlen);
      ping_time = sys_now();
      if (result == (int32_t)ping_size)
      {
        PRINT_DBG("ping send OK")
        /* Send is OK, wait response from remote */
        (void)memset((void *)&from, 0, sizeof(from));
        fromlen = (int32_t)sizeof(from);
        result = com_recvfrom_lwip_mcu(ping, (uint8_t *)ping_rcv, (int32_t)COM_PING_RSP_LEN_MAX, 0,
                                       (com_sockaddr_t *)&from, &fromlen);

        rsp->time = sys_now() - ping_time;
        rsp->size = COM_PING_DATA_SIZE;
        /* Data received ? */
        if (result >= 0) /* No error */
        {
          /* Is answer received complete ? */
          if ((uint32_t)result >= (sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
          {
            PRINT_DBG("ping receive OK")
            result = (int32_t)ERR_OK;
          }
          else
          {
            /* Incomplete answer, ensure an error is returned */
            result = (int32_t)ERR_IF;
          }
        }
        /* else result < 0, receive is NOK, result contains the error */
      }
      else if (result >= 0) /* case incomplete send */
      {
        /* Ensure an error is returned */
        result = (int32_t)ERR_IF;
      }
      else
      {
        __NOP(); /* send NOK, result contains the error */
      }
    }
    /* else setsockopt NOK, result contains the error */
  }

  /* Analyse the received trame and format the response */
  if (result == (int32_t)ERR_OK)
  {
    p_ip_hdr = (struct ip_hdr *)ping_rcv;
    /* For next line, use of IPH_HL() or IPH_HL_BYTES() make MISRA errors */
    p_icmp_echo = (struct icmp_echo_hdr *)(ping_rcv + (((p_ip_hdr->_v_hl) & 0x0FU) * 4U));
    if ((p_icmp_echo->id == COM_PING_ID) && (p_icmp_echo->seqno == ping_seqno))
    {
      if (ICMPH_TYPE(p_icmp_echo) == (uint8_t)ICMP_ER)
      {
        PRINT_DBG("ping icmp echo reply OK")
        /* result already set to ERR_OK */
        rsp->status = (int32_t)ERR_OK;
        rsp->ttl = IPH_TTL(p_ip_hdr);
      }
      else
      {
        /* Ensure an error is returned */
        result = (int32_t)ERR_IF;
        rsp->status = (int32_t)ICMPH_TYPE(p_icmp_echo);
      }
    }
    else
    {
      /* Ensure an error is returned */
      result = (int32_t)ERR_IF;
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
int32_t com_closeping_lwip_mcu(int32_t ping)
{
  int32_t result;

  result = com_closesocket_lwip_mcu(ping);
  if (result == (int32_t)ERR_OK)
  {
    (void)rtosalMutexRelease(ComPingMutexHandle);
    ping_seqno = 0U;
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
  *         Restrictions, if any, are linked to LwIP module used
  * @param  -
  * @retval bool      - true/false init ok/nok
  */
bool com_init_lwip_mcu(void)
{
  bool result = true;

  /* No way to know if tcp_init is ok or not */
  tcpip_init(NULL, NULL);

#if (USE_COM_PING == 1)
  /* Initialization Ping variables */
  ping_seqno = 0U;
  /* ping_snd always initialize with mem_malloc() call */
  ping_rcv = NULL;
  ComPingMutexHandle = NULL;

  /* Allocation Ping send buffer - if error ping_snd is set to NULL by mem_malloc() */
  ping_snd = (struct icmp_echo_hdr *)mem_malloc((mem_size_t)(sizeof(struct icmp_echo_hdr) + COM_PING_DATA_SIZE));

  /* No need to continue memory allocation in case of error */
  if (ping_snd != NULL)
  {
    /* Allocation Ping receive buffer  - if error ping_rcv is set to NULL by mem_malloc() */
    ping_rcv = (uint8_t *)mem_malloc((mem_size_t)COM_PING_RSP_LEN_MAX);

    if (ping_rcv != NULL)
    {
      /* Create Mutex to protect Ping session */
      ComPingMutexHandle = rtosalMutexNew((const rtosal_char_t *)"COMSOCKLWIP_MUT_PING_DESC");
    }
  }

  /* Is Ping initialization completely ok ? */
  if (ComPingMutexHandle == NULL)
  {
    /* Something goes wrong during Ping initialization - Clean memory */
    result = false;

    /* Clean-up memory */
    if (ping_snd != NULL) /* to avoid an error debug trace in LwIP */
    {
      mem_free(ping_snd);
      ping_snd = NULL; /* to ensure correct variable value in com */
    }
    if (ping_rcv != NULL) /* to avoid an error debug trace in LwIP */
    {
      mem_free(ping_rcv); /* to ensure correct variable value in com */
      ping_rcv = NULL;
    }
  }
  /* else result already set to true */
#endif /* USE_COM_PING == 1 */

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
void com_start_lwip_mcu(void)
{
  /* Nothing to do */
  __NOP();
}

#endif /* USE_SOCKETS_TYPE == USE_SOCKETS_LWIP */

#endif /* USE_COM_SOCKETS == 1 */
