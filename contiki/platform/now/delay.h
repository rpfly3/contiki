#ifndef _DWTDELAY_H_
#define _DWTDELAY_H_

#define MY_MCU_SYSCLK           (8000000)

#include "stm32f10x.h"

void dwt_init(uint32_t sys_clk);

void delay_us(uint32_t uSec);

#define delay_ms(mSec)    delay_us( mSec*1000 )

void wait_us(uint32_t uSec);
#endif // _DWTDELAY_H_
