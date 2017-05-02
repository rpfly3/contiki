#ifndef PLATFORM_CONF_H__
#define PLATFORM_CONF_H__

#include <stdint.h>
/* Platform specific includes */
#include "platform/now/delay.h"
#include "platform/now/usart1.h"
#include "platform/now/spi1.h"
#include "platform/now/rf231-arch.h"
#include "platform/now/rf231.h"
#include "debug.h"

/* Contiki related setting */
#define LINKADDR_CONF_SIZE 1

//typedef unsigned int clock_time_t;
/*  To record very long time, we use uint64_t as the clock_time_t;
 *  64_bit type is supported by GCC for all 32-bit arm cpus;
 *  https://answers.launchpad.net/gcc-arm-embedded/+question/281021
 */
/* but later I find there is an issue on printf when using uint64_t,
 * and uint32_t is large enough for my application.
 */
typedef uint32_t clock_time_t;

/* To override the definition of rtimer_clock_t in rtimer.h*/
#ifndef RTIMER_CLOCK_LT
typedef uint32_t rtimer_clock_t;
#define RTIMER_CLOCK_LT(a,b)     ((signed short)((a)-(b)) < 0)
#endif /* RTIMER_CLOCK_LT */

typedef unsigned int uip_stats_t;


/* the low-level radio driver */
#define NETSTACK_CONF_RADIO rf231_driver

#endif

