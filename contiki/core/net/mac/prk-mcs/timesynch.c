#include "core/net/mac/prk-mcs/prkmcs.h"


static uint16_t time_synch_no;
PROCESS(heart_beat_process, "Heart beat process");
void time_synch_init()
{
	time_synch_no = 0;
}

/*----------------------- Send time synch packet ----------------------------*/
void time_synch_send()
{
	SetPower(RF231_TX_PWR_MAX);

	memset(rf231_tx_buffer, 0, RF231_MAX_FRAME_LENGTH);
	uint8_t *buf_ptr = rf231_tx_buffer;

	uint8_t data_type = TIME_SYNCH_BEACON;
	buf_ptr += FCF;
	memcpy(buf_ptr, &data_type, sizeof(uint8_t));
	buf_ptr += sizeof(uint8_t);
	rtimer_clock_t network_time = RTIMER_NOW();
	memcpy(buf_ptr, &network_time, sizeof(rtimer_clock_t));
	buf_ptr += sizeof(rtimer_clock_t);
	memcpy(buf_ptr, &time_synch_no, sizeof(uint16_t));

	rf231_tx_buffer_size = FCF + TIME_SYNCH_BEACON_LENGTH + CHECKSUM_LEN;

	rf231_send();

	if (!prkmcs_is_synchronized && time_synch_no >= 5)
	{
		prkmcs_is_synchronized = 1;
	}

	log_debug("TS packet %u", time_synch_no);
	++time_synch_no;	
}
process_event_t	heart_beat;
PROCESS_THREAD(heart_beat_process, ev, data)
{
	PROCESS_BEGIN();
	while (1) 
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == heart_beat);
		log_debug("Mote %u works fine", node_addr);
	}
	PROCESS_END();
}