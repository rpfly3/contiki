#include "stm32f10x.h"
#include "contiki.h"
#include <stdio.h>
#include "sys/etimer.h"
#include "sys/clock.h"
#include "core/dev/watchdog.h"

extern process_event_t heart_beat;
PROCESS_NAME(heart_beat_process);

/* clock_time_t is defined in platform.h */
static volatile clock_time_t current_clock = 0;
static volatile uint32_t current_seconds = 0;
static volatile uint32_t second_countdown = CLOCK_SECOND;

void SysTick_Handler(void)
{
    ++current_clock;
	--second_countdown;
    if(etimer_pending() && etimer_next_expiration_time() <= current_clock) {
        etimer_request_poll();
    }
    if(second_countdown == 0) {
        ++current_seconds;
        second_countdown = CLOCK_SECOND;
		// feed watchdog
	    watchdog_periodic();

	    if (current_seconds % 5 == 0)
	    {
		    process_post(&heart_beat_process, heart_beat, NULL);
	    }
    }
}

void clock_init(void) {
	/**************************
	 * SystemFrequency / 1000		1ms
	 * SystemFrequency / 100000		10us
	 * SystemFrequency / 1000000	1us
	 *************************/
	/* after testing, 10us is the best for this platform */
	if (SysTick_Config(MCK / 100000)) { // 10 us
        /* Capture error */
        while(1);
    }
	NVIC_SetPriority(SysTick_IRQn, 0);  
	watchdog_start();
}

clock_time_t clock_time(void) {
    return current_clock;
}

uint32_t clock_seconds(void) {
    return current_seconds;
}

void clock_set_seconds(uint32_t sec) {
    current_seconds = sec;
}

void clock_wait(clock_time_t t) {
    // it's meaningless to implement this function
}

void clock_delay_usec(uint16_t dt) {
	// use register reading to implement us delay in other functions
}
