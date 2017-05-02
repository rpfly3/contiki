#ifndef __RTIMER_ARCH_H__
#define __RTIMER_ARCH_H__

#include "sys/rtimer.h"
#include "contiki-conf.h"


#define RTIMER_ARCH_SECOND 1000000

void rtimer_arch_init(void);
rtimer_clock_t rtimer_arch_now();
void rtimer_arch_schedule(rtimer_clock_t t);
void network_time_set(rtimer_clock_t network_time);
void TIM3_irs(void);
#endif
