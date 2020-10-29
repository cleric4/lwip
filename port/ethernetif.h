/*******
 * stm32h750 lwIP to ethernet interface
 * device: for model_c
 * date: 03.09.2020
 * documentations: lwIP Wiki
 *******/


#include <lwip/netif.h>
#include <lwip/err.h>

extern netif NetIF;
err_t ethernetif_init(netif * netif);
