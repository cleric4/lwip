//******************************************************************************
// LWPORT_CONFIG_H
// DESCRIPTION:
// Конфигурационные макросы для ПОРТА lwIP под scmRTOS.
//
//
//******************************************************************************

#ifndef LWPORT_CONFIG_H
#define LWPORT_CONFIG_H

#include <stdlib.h>
// Макрос scmRTOS_SYSTEM_TICKS_ENABLE в scmRTOS_config должен быть равен 1.
// Макрос LWIP_COMPAT_MUTEX в lwipopt.h должен быть равен 0.
//------------------------------------------------------------------------------
// T Y P E S    and    D E F I N I T I O N S
//------------------------------------------------------------------------------
// Размер пула мьютексов.
#define ARCH_MUTEX_POOL_SIZE    8

// Размер пула семафоров.
#define ARCH_SEM_POOL_SIZE      16
                                     
// Размер пула очередей.
#define ARCH_QUEUE_POOL_SIZE    16
// Длина очереди (от 1 и более, 8 вполне достаточно).
#define ARCH_QUEUE_SIZE         8
// Количество приемных буферов MAC
#define ETH_RXBUFNB        8
// Количество передающих буферов MAC
#define ETH_TXBUFNB        2

// lwIP использует один поток для своей работы.
// В коде приложения необходимо определить typedef OS::process<OS::prX, XXX>  lwip_proc;
// Значения приоритета и размера стека из lwipopt.h игнорируются.
//#define TCPIP_THREAD_NAME       "LwIP"

// Обработка принятых пакетов происходит в отдельном потоке.
// В коде приложения необходимо определить typedef OS::process<OS::prX, XXX>  mac_proc;

// Файл, где определен типы задачи для lwIP и MAC.
//#ifdef __cplusplus
//#include    <tasks.h>
//extern "C"
//#endif
//uint32_t sys_now();
#define LWIP_RAND   sys_now

#endif /* LWPORT_CONFIG_H */

