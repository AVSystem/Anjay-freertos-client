/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Simon Goldschmidt
 *
 */
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

#include "plf_config.h"
/* PPP SUPPORT */
#define PPP_SUPPORT       1
#define PPPOS_SUPPORT     1
#define PPP_SERVER        1
#define PPP_DEBUG         0

#define LWIP_ARP          0
#define PAP_SUPPORT       1

#define PPP_NOTIFY_PHASE  1
#define MEMP_NUM_PPP_PCB  2

#define LWIP_COMPAT_MUTEX_ALLOWED  1

#define LWIP_DNS   1
#define LWIP_SO_SNDTIMEO   1
#define LWIP_SO_RCVTIMEO   1
#define LWIP_SO_SNDRCVTIMEO_NONSTANDARD   1
#define LWIP_SO_RCVRCVTIMEO_NONSTANDARD   1

#define LWIP_TCP_KEEPALIVE                  1   /* Keep the TCP link active. Important for MQTT/TLS */
#define LWIP_RANDOMIZE_INITIAL_LOCAL_PORTS  1   /* Prevent the same port to be used after reset.
                                                   Otherwise, the remote host may be confused if the
                                                   port was not explicitly closed before the reset. */

/* #define PPPDEBUG */


#define ETHARP_TRUST_IP_MAC     0
#define IP_REASSEMBLY           0
#define IP_FRAG                 0
#define ARP_QUEUEING            0
#define TCP_LISTEN_BACKLOG      1

/**
  * NO_SYS==1: Provides VERY minimal functionality. Otherwise,
  * use lwIP facilities.
  */
#define NO_SYS                  0

/**
  * Set this to 1 if you want to free PBUF_RAM pbufs (or call mem_free()) from
  * interrupt context (or another context that doesn't allow waiting for a
  * semaphore).
  * If set to 1, mem_malloc will be protected by a semaphore and SYS_ARCH_PROTECT,
  * while mem_free will only use SYS_ARCH_PROTECT. mem_malloc SYS_ARCH_UNPROTECTs
  * with each loop so that mem_free can run.
  *
  * ATTENTION: As you can see from the above description, this leads to dis-/
  * enabling interrupts often, which can be slow! Also, on low memory, mem_malloc
  * can need longer.
  *
  * If you don't want that, at least for NO_SYS=0, you can still use the following
  * functions to enqueue a deallocation call which then runs in the tcpip_thread
  * context:
  * - pbuf_free_callback(p);
  * - mem_free_callback(m);
  */
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1

/**
  * LWIP_TCPIP_CORE_LOCKING_INPUT: when LWIP_TCPIP_CORE_LOCKING is enabled,
  * this lets tcpip_input() grab the mutex for input packets as well,
  * instead of allocating a message and passing it to tcpip_thread.
  *
  * ATTENTION: this does not work when tcpip_input() is called from
  * interrupt context!
  */
#define LWIP_TCPIP_CORE_LOCKING_INPUT   1

/**
  * LWIP_TCPIP_CORE_LOCKING
  * Creates a global mutex that is held during TCPIP thread operations.
  * Can be locked by client code to perform lwIP operations without changing
  * into TCPIP thread using callbacks. See LOCK_TCPIP_CORE() and
  * UNLOCK_TCPIP_CORE().
  * Your system should provide mutexes supporting priority inversion to use this.
  */
#define LWIP_TCPIP_CORE_LOCKING         1

/**
  * SYS_LIGHTWEIGHT_PROT==1: enable inter-task protection (and task-vs-interrupt
  * protection) for certain critical regions during buffer allocation, deallocation
  * and memory allocation and deallocation.
  * ATTENTION: This is required when using lwIP from more than one context! If
  * you disable this, you must be sure what you are doing!
  */
#define SYS_LIGHTWEIGHT_PROT            1


/* ---------- Memory options ---------- */
/* MEM_ALIGNMENT: should be set to the alignment of the CPU for which
   lwIP is compiled. 4 byte alignment -> define MEM_ALIGNMENT to 4, 2
   byte alignment -> define MEM_ALIGNMENT to 2. */
#define MEM_ALIGNMENT           4

/* MEM_SIZE: the size of the heap memory. If the application will send
a lot of data that needs to be copied, this should be set high. */
/* #define MEM_SIZE                (15 *1024) */
#define MEM_SIZE                (10 *1024)

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#define MEMP_NUM_PBUF           10
/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#define MEMP_NUM_UDP_PCB        2 /* needed for DNS */
/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#define MEMP_NUM_TCP_PCB        2 /* needed for HTTP Client */
/**
  * MEMP_NUM_RAW_PCB: Number of raw connection PCBs
  * (requires the LWIP_RAW option)
  */
#define MEMP_NUM_RAW_PCB                2 /* needed for Ping */
/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#define MEMP_NUM_TCP_PCB_LISTEN 0 /* No TCP PCB Server */
/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#define MEMP_NUM_TCP_SEG        12
/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active
   timeouts. */
#define MEMP_NUM_SYS_TIMEOUT    10


/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */
#define PBUF_POOL_SIZE          12

/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#define PBUF_POOL_BUFSIZE       512


/* ---------- TCP options ---------- */
#define LWIP_TCP                1
#define TCP_TTL                 255

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ         0

/* TCP Maximum segment size. */
#define TCP_MSS                 (1500 - 40)  /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF             (4*TCP_MSS)

/*  TCP_SND_QUEUELEN: TCP sender buffer space (pbufs). This must be at least
  as much as (2 * TCP_SND_BUF/TCP_MSS) for things to work. */

#define TCP_SND_QUEUELEN        (2* TCP_SND_BUF/TCP_MSS)

/* TCP receive window. */
#define TCP_WND                 (2*TCP_MSS)


#define TCP_MSL 6000UL /* The maximum segment lifetime in milliseconds */

/* ---------- ICMP options ---------- */
#define LWIP_SO_RCVTIMEO                1 /* ICPM PING */
#define LWIP_ICMP                       1
#define LWIP_RAW                        1 /* PING changed to 1 */
#define DEFAULT_RAW_RECVMBOX_SIZE       3 /* for ICMP PING */


/* ---------- DHCP options ---------- */
/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces. DHCP is not implemented in lwIP 0.5.1, however, so
   turning this on does currently not work. */
#define LWIP_DHCP               0


/* ---------- UDP options ---------- */
#define LWIP_UDP                1  /* needed for DNS */
#define UDP_TTL                 255


/* ---------- Statistics options ---------- */
#define LWIP_STATS 0
/* #define LWIP_PROVIDE_ERRNO */

/* ---------- link callback options ---------- */
/* LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
  * whenever the link changes (i.e., link down)
  */
#define LWIP_NETIF_LINK_CALLBACK        1

/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
  */

/*
The STM32F4x7 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 - To use this feature let the following define uncommented.
 - To disable it and process by CPU comment the  the checksum.
  */
#define CHECKSUM_BY_HARDWARE           0


#if (CHECKSUM_BY_HARDWARE == 1)
/* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
#define CHECKSUM_GEN_IP                 0
/* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
#define CHECKSUM_GEN_UDP                0
/* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
#define CHECKSUM_GEN_TCP                0
/* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
#define CHECKSUM_CHECK_IP               0
/* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
#define CHECKSUM_CHECK_UDP              0
/* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
#define CHECKSUM_CHECK_TCP              0
/* CHECKSUM_CHECK_ICMP==0: Check checksums by hardware for incoming ICMP packets.*/
#define CHECKSUM_GEN_ICMP               0
#else
/* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
#define CHECKSUM_GEN_IP                 1
/* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
#define CHECKSUM_GEN_UDP                1
/* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
#define CHECKSUM_GEN_TCP                1
/* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
#define CHECKSUM_CHECK_IP               1
/* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
#define CHECKSUM_CHECK_UDP              1
/* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
#define CHECKSUM_CHECK_TCP              1
/* CHECKSUM_CHECK_ICMP==1: Check checksums by hardware for incoming ICMP packets.*/
#define CHECKSUM_GEN_ICMP               1
#endif /* CHECKSUM_BY_HARDWARE */


/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
  */
/**
  * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
  */
/* Change next define to support socket interface */
#define LWIP_NETCONN                    1

/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
  */
/**
  * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
  */
/* Change next define to support socket interface */
#define LWIP_SOCKET                     1

/*
   -----------------------------------
   ---------- DEBUG options ----------
   -----------------------------------
  */

/* #define LWIP_DEBUG */

/*
   ---------------------------------
   ---------- OS options ----------
   ---------------------------------
  */

#define TCPIP_THREAD_NAME              "TCP/IP"
#define TCPIP_THREAD_STACKSIZE          TCPIP_THREAD_STACK_SIZE
#define TCPIP_MBOX_SIZE                 10
#define DEFAULT_UDP_RECVMBOX_SIZE       16
#define DEFAULT_TCP_RECVMBOX_SIZE       16
#define DEFAULT_ACCEPTMBOX_SIZE         16
#define DEFAULT_THREAD_STACKSIZE        500
#define LWIP_COMPAT_MUTEX               1

/* If your port already typedef's socklen_t, define SOCKLEN_T_DEFINED
   to prevent this code from redefining it. */
#if !defined(socklen_t) && !defined(SOCKLEN_T_DEFINED)
typedef int32_t socklen_t;
#define SOCKLEN_T_DEFINED (1)
#endif /* !defined(socklen_t) && !defined(SOCKLEN_T_DEFINED) */


#endif /* __LWIPOPTS_H__ */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
