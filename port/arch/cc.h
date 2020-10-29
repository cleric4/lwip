/*******
 * stm32h750 compiler and processor header to lwIP
 * device: for model_c
 * date: 25.08.2020
 * documentations: lwIP Wiki, sborschch port, hnkr project (HAL)
 *******/
 
#ifndef __CC_H__
#define __CC_H__
 
#include    <lwipopts.h> 
 
#define MEM_ALIGNMENT           4

/** LWIP_TIMEVAL_PRIVATE: if you want to use the struct timeval provided
 * by your system, set this to 0 and include <sys/time.h> in cc.h */
#define LWIP_TIMEVAL_PRIVATE    0   // avoid conflict with system headers
#include    <sys/time.h>

/* define compiler specific symbols */
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_STRUCT      __attribute__ ((__packed__))
#define PACK_STRUCT_END
#define PACK_STRUCT_FIELD(x)    x

#ifdef   LWIP_DEBUG
#define LWIP_PLATFORM_DIAG(x) do { diag_message x; } while(0)
#else
#define LWIP_PLATFORM_DIAG(...)
#endif
/*
#ifndef LWIP_NOASSERT
#define LWIP_PLATFORM_ASSERT(x) assertion_failed(x, __FILE__, __LINE__)
#else
#define LWIP_PLATFORM_ASSERT(...)
#endif*/

#define LWIP_PLATFORM_BYTESWAP  1
#define LWIP_PLATFORM_NTOHL(x)  __REV(x)
#define LWIP_PLATFORM_HTONL(x)  __REV(x)
#define LWIP_PLATFORM_HTONS(x)  __REV16(x)
#define LWIP_PLATFORM_NTOHS(x)  __REV16(x)

#endif /* __CC_H__ */

#ifndef __CPU_H__
#define __CPU_H__

#ifndef BYTE_ORDER
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#endif /* __CPU_H__ */
