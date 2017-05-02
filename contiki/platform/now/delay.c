#include "delay.h"

// 0xE000EDFC DEMCR RW Debug Exception and Monitor Control Register.
#define DEMCR           ( *(unsigned int *)0xE000EDFC )
#define TRCENA          ( 0x01 << 24)

// 0xE0001000 DWT_CTRL RW The Debug Watchpoint and Trace (DWT) unit
#define DWT_CTRL        ( *(unsigned int *)0xE0001000 )
#define CYCCNTENA       ( 0x01 << 0 ) 
// 0xE0001004 DWT_CYCCNT RW Cycle Count register, 
#define DWT_CYCCNT      ( *(unsigned int *)0xE0001004)

static uint32_t SYSCLK = 0;

void dwt_init(uint32_t sys_clk)
{
  DEMCR |= TRCENA;
  DWT_CTRL |= CYCCNTENA;
  
  SYSCLK = sys_clk;
}

/* Note that uSec cannot be 0 */
void delay_us(uint32_t uSec)
{
	uint32_t ticks_start, ticks_end, ticks_delay;
  
	ticks_start = DWT_CYCCNT;

	if ( !SYSCLK )
		dwt_init( MY_MCU_SYSCLK );
	
	ticks_delay = ( uSec * (SYSCLK / (1000 * 1000) ) );
  
	ticks_end = ticks_start + ticks_delay;
  
	if ( ticks_end > ticks_start ) {
		while( DWT_CYCCNT < ticks_end );
	} else {
		while( DWT_CYCCNT >= ticks_end );
		while( DWT_CYCCNT < ticks_end );
	}
}
/* this is much less precise than delay_us */
void wait_us(uint32_t uSec)
{
	for (uint32_t i = 0; i < uSec; i++)
	{
		asm("nop");
	}
}
