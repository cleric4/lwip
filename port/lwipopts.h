/**
  ******************************************************************************
  * @file    lwipopts.h
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    11/20/2009
  * @brief   lwIP Options Configuration.
  *          This file is based on Utilities\lwip-1.3.1\src\include\lwip\opt.h 
  *          and contains the lwIP configuration for the STM32F107 demonstration.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__
#ifdef NDEBUG
#ifndef LWIP_NOASSERT
#define LWIP_NOASSERT
#endif
#endif

/**
 * SYS_LIGHTWEIGHT_PROT==1: if you want inter-task protection for certain
 * critical regions during buffer allocation, deallocation and memory
 * allocation and deallocation.
 */
#define SYS_LIGHTWEIGHT_PROT        1

/**
 * NO_SYS==1: Provides VERY minimal functionality. Otherwise,
 * use lwIP facilities.
 */
#define NO_SYS                      0

/**
 * LWIP_NETIF_LINK_CALLBACK==1: Support a callback function from an interface
 * whenever the link changes (i.e., link down)
 */
#define LWIP_NETIF_LINK_CALLBACK    1

// port has it's own mutexes
#define LWIP_COMPAT_MUTEX           0

/* ---------- Memory options ---------- */
// do not use lwIP memory management
#define MEM_LIBC_MALLOC             1
#define MEMP_MEM_MALLOC             1

//#define ETH_PAD_SIZE                2

/* MEMP_NUM_PBUF: the number of memp struct pbufs. If the application
   sends a lot of data out of ROM (or other static memory), this
   should be set high. */
#define MEMP_NUM_PBUF               10

/* MEMP_NUM_SYS_TIMEOUT: the number of simulateously active
   timeouts. */
#define MEMP_NUM_SYS_TIMEOUT        20

/* ---------- Pbuf options ---------- */
/* PBUF_POOL_SIZE: the number of buffers in the pbuf pool. */

//############################################
#define PBUF_POOL_SIZE              10
//############################################
/* PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. */
#define PBUF_POOL_BUFSIZE           1500

/* ---------- ICMP options ---------- */
#define LWIP_ICMP                   1

/* ---------- TCP options ---------- */
#define LWIP_TCP                    1
#define TCP_TTL                     255

/* MEMP_NUM_TCP_PCB: the number of simulatenously active TCP
   connections. */
#define MEMP_NUM_TCP_PCB            34  // 32 RRP-GPRS channels + 2 telnets

/* MEMP_NUM_TCP_PCB_LISTEN: the number of listening TCP
   connections. */
#define MEMP_NUM_TCP_PCB_LISTEN     2   // telnet + RRP-GPRS

/* MEMP_NUM_TCP_SEG: the number of simultaneously queued TCP
   segments. */
#define MEMP_NUM_TCP_SEG            12

/* Controls if TCP should queue segments that arrive out of
   order. Define to 0 if your device is low on memory. */
#define TCP_QUEUE_OOSEQ             1

/* TCP Maximum segment size. */
#define TCP_MSS                     (1500 - 40)   /* TCP_MSS = (Ethernet MTU - IP header size - TCP header size) */

/* TCP sender buffer space (bytes). */
#define TCP_SND_BUF                 (2 * TCP_MSS)

/* TCP sender buffer space (pbufs). This must be at least = 2 *
   TCP_SND_BUF/TCP_MSS for things to work. */
#define TCP_SND_QUEUELEN            (6 * TCP_SND_BUF)/TCP_MSS

/* TCP receive window. */
#define TCP_WND                     (2 * TCP_MSS)

/**
 * LWIP_TCP_KEEPALIVE==1: Enable TCP_KEEPIDLE, TCP_KEEPINTVL and TCP_KEEPCNT
 * options processing. Note that TCP_KEEPIDLE and TCP_KEEPINTVL have to be set
 * in seconds. (does not require sockets.c, and will affect tcp.c)
 * LWIP_TCP_KEEPALIVE==0: use default values
 */
#define LWIP_TCP_KEEPALIVE          0

/* Keepalive values, compliant with RFC 1122. Don't change this unless you know what you're doing */
// Default idle time before first KEEPALIVE, milliseconds. 7200000UL, e.g 2 hours if not defined
#define TCP_KEEPIDLE_DEFAULT        (50 * 1000UL)

// Default time between KEEPALIVE probes in milliseconds, 75000UL, e.g. 75 seconds if not defined
#define TCP_KEEPINTVL_DEFAULT       (2 * 1000UL)

// Default number of KEEPALIVE fails before socket close, 9U if not defined
#define TCP_KEEPCNT_DEFAULT         5U

/* ---------- UDP options ---------- */
#define LWIP_UDP                    1

/* MEMP_NUM_UDP_PCB: the number of UDP protocol control blocks. One
   per active UDP "connection". */
#define MEMP_NUM_UDP_PCB            2   // RRP receiver + DHCP

#define UDP_TTL                     255


/* ---------- DHCP options ---------- */
/* Define LWIP_DHCP to 1 if you want DHCP configuration of
   interfaces.  */
#define LWIP_DHCP                   1

/**
 * LWIP_DHCP_CHECK_LINK_UP==1: dhcp_start() only really starts if the netif has
 * NETIF_FLAG_LINK_UP set in its flags. As this is only an optimization and
 * netif drivers might not set this flag, the default is off. If enabled,
 * netif_set_link_up() must be called to continue dhcp starting.
 */
#define LWIP_DHCP_CHECK_LINK_UP     1

/* ---------- DNS options ---------- */
#define LWIP_DNS                    0
#define DNS_MAX_SERVERS             2
/** DNS maximum host name length supported in the name table. */
#define DNS_MAX_NAME_LENGTH         64
/** DNS maximum number of entries to maintain locally. */
#define DNS_TABLE_SIZE              1

#define LWIP_NETIF_HOSTNAME         1


#if (defined(LWIP_DHCP) && LWIP_DHCP == 1) \
  ||(defined(LWIP_DNS) && LWIP_DNS == 1)

#if !defined(LWIP_UDP) || LWIP_UDP == 0
#undef  LWIP_UDP
#define LWIP_UDP                    1
#endif

#if !defined(MEMP_NUM_UDP_PCB) || (MEMP_NUM_UDP_PCB == 0) 
#define MEMP_NUM_UDP_PCB            1
#endif

#endif

/**
 * MEMP_NUM_NETCONN: the number of struct netconns, number of sockets
 */
#define MEMP_NUM_NETCONN            (MEMP_NUM_UDP_PCB + MEMP_NUM_TCP_PCB + MEMP_NUM_TCP_PCB_LISTEN)

/* ---------- Statistics options ---------- */
#define LWIP_STATS                  0
#define LWIP_PROVIDE_ERRNO          1

/*
   ----------------------------------------------
   ---------- Sequential layer options ----------
   ----------------------------------------------
*/
/**
 * LWIP_NETCONN==1: Enable Netconn API (require to use api_lib.c)
 */
#define LWIP_NETCONN                1


/*
   ------------------------------------
   ---------- Socket options ----------
   ------------------------------------
*/
/**
 * LWIP_SOCKET==1: Enable Socket API (require to use sockets.c)
 */
#define LWIP_SOCKET                 1
#define LWIP_NETIF_API              1
#define LWIP_COMPAT_SOCKETS         0   // avoid #defines conflict with function names

#define LWIP_CHECKSUM_CTRL_PER_NETIF    1

/*
   --------------------------------------
   ---------- Checksum options ----------
   --------------------------------------
*/

/* 
The STM32 allows computing and verifying the IP, UDP, TCP and ICMP checksums by hardware:
 - To use this feature let the following define uncommented.
 - To disable it and process by CPU comment the  the checksum.
*/
//#define CHECKSUM_BY_HARDWARE        

#ifdef CHECKSUM_BY_HARDWARE
  /* CHECKSUM_GEN_IP==0: Generate checksums by hardware for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP           0
  /* CHECKSUM_GEN_UDP==0: Generate checksums by hardware for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP          0
  /* CHECKSUM_GEN_TCP==0: Generate checksums by hardware for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP          0 
  /* CHECKSUM_GEN_ICMP==0: Generate checksums by hardware for outgoing ICMP packets.*/
  #define CHECKSUM_GEN_ICMP         0
  /* CHECKSUM_CHECK_IP==0: Check checksums by hardware for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP         0
  /* CHECKSUM_CHECK_UDP==0: Check checksums by hardware for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP        0
  /* CHECKSUM_CHECK_TCP==0: Check checksums by hardware for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP        0
  /* CHECKSUM_CHECK_ICMP==0: Check checksums by hardware for incoming ICMP packets.*/
  #define CHECKSUM_CHECK_ICMP       0
  /* CHECKSUM_CHECK_ICMP6==1: Check checksums in software for incoming ICMPv6 packets*/
  #define CHECKSUM_CHECK_ICMP6      0
#else
  /* CHECKSUM_GEN_IP==1: Generate checksums in software for outgoing IP packets.*/
  #define CHECKSUM_GEN_IP           1
  /* CHECKSUM_GEN_UDP==1: Generate checksums in software for outgoing UDP packets.*/
  #define CHECKSUM_GEN_UDP          1
  /* CHECKSUM_GEN_TCP==1: Generate checksums in software for outgoing TCP packets.*/
  #define CHECKSUM_GEN_TCP          1
  /* CHECKSUM_GEN_ICMP==1: Generate checksums in software for outgoing ICMP packets.*/
  #define CHECKSUM_GEN_ICMP         1
  /* CHECKSUM_CHECK_IP==1: Check checksums in software for incoming IP packets.*/
  #define CHECKSUM_CHECK_IP         1
  /* CHECKSUM_CHECK_UDP==1: Check checksums in software for incoming UDP packets.*/
  #define CHECKSUM_CHECK_UDP        1
  /* CHECKSUM_CHECK_TCP==1: Check checksums in software for incoming TCP packets.*/
  #define CHECKSUM_CHECK_TCP        1
  /* CHECKSUM_CHECK_ICMP==1: Check checksums in software for incoming ICMP packets.*/
  #define CHECKSUM_CHECK_ICMP       1
  /* CHECKSUM_CHECK_ICMP6==1: Check checksums in software for incoming ICMPv6 packets*/
  #define CHECKSUM_CHECK_ICMP6      1
#endif

#endif /* __LWIPOPTS_H__ */

/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/
