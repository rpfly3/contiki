#ifndef __TIMESYNCH_H__
#define __TIMESYNCH_H__

#define TIME_SYNCH_BEACON_LENGTH (sizeof(uint8_t) + sizeof(rtimer_clock_t) + sizeof(uint16_t))

void time_synch_init();
void time_synch_send();
#endif /* __TIMESYNCH_H__ */
