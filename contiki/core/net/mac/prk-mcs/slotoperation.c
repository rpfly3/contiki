#include "core/net/mac/prk-mcs/prkmcs.h"

static struct ctimer send_timer;

struct asn_t current_asn; //absolute slot number
volatile rtimer_clock_t current_slot_start;
uint8_t building_signalmap = 1;
uint8_t is_receiver, my_link_index, prkmcs_is_synchronized;
uint8_t control_channel, data_channel;

static void signalmap_slot_operation(struct rtimer *st, void *ptr);
static void prkmcs_slot_operation(struct rtimer *st, void *ptr);
static void synch_slot_operation(struct rtimer *st, void *ptr);

/* schedule a wakeup at a specified offset from a reference time */
static uint8_t prkmcs_schedule_slot_operation(struct rtimer *slot_timer, rtimer_clock_t ref_time, rtimer_clock_t offset)
{
	rtimer_clock_t now = RTIMER_NOW();

	if (ref_time + offset - SLOT_GUARD <= now)
	{
		log_info("Miss slot %ld", current_asn.ls4b);
		return 0;
	}

	if (node_addr != BASE_STATION_ID)
	{
		if (building_signalmap && current_asn.ls4b <= BUILD_SIGNALMAP_PERIOD)
		{
			//schedule slot operation for building signal map
			rtimer_set(slot_timer, current_slot_start, 0, (void(*)(struct rtimer*, void *))signalmap_slot_operation, NULL);
		}
		else
		{
			//schedule slot operation for transceiving motes
			rtimer_set(slot_timer, current_slot_start, 0, (void(*)(struct rtimer*, void *))prkmcs_slot_operation, NULL);
		}
	}
	else
	{
		//schedule slot operation for time synch motes
		rtimer_set(slot_timer, current_slot_start, 0, (void(*)(struct rtimer*, void *))synch_slot_operation, NULL);
	}

	return 1;
}

void schedule_next_slot(struct rtimer *t)
{
	rtimer_clock_t time_to_next_active_slot;
	rtimer_clock_t prev_slot_start;
	do
	{
		ASN_INC(current_asn, 1);
		time_to_next_active_slot = PRKMCS_TIMESLOT_LENGTH;
		prev_slot_start = current_slot_start;
		current_slot_start += time_to_next_active_slot;
			
	} while (!prkmcs_schedule_slot_operation(t, prev_slot_start, time_to_next_active_slot));
}

struct rtimer slot_operation_timer;
void prkmcs_slot_operation_start()
{			
	current_slot_start = 0;
	ASN_INIT(current_asn, 0, 0);
	network_time_set(0);
	schedule_next_slot(&slot_operation_timer);
}
//Building Signalmap
static void signalmap_slot_operation(struct rtimer *st, void *ptr)
{
	//printf("Slot %lu.\r\n", current_asn.ls4b);
	SetChannel(control_channel);
	wait_us(GetRand() % CCA_MAX_BACK_OFF_TIME);
	test_send();

	start_rx();
	schedule_next_slot(&slot_operation_timer);
}
static uint8_t is_ER_initialized = 0;
//Transceiving motes' slot operation
static void prkmcs_slot_operation(struct rtimer *st, void *ptr)
{		
	// initialize local link er table
	if (!is_ER_initialized)
	{
		setLocalLinkERTable();
		is_ER_initialized = 1;
	}
	
	if (prkmcs_is_synchronized)
	{
		printf("Slot %lu\r\n", current_asn.ls4b);
		if (current_asn.ls4b % TIME_SYNCH_FREQUENCY != 0)
		{
			runLama(current_asn);
			if (data_channel == INVALID_CHANNEL)
			{
				SetChannel(control_channel);
				printf("Control channel %u \r\n", control_channel);

				wait_us(GetRand() % CCA_MAX_BACK_OFF_TIME);
				uint8_t channel_idle = getCCA(RF231_CCA_2, 0);
				if (channel_idle)
				{
					//ctimer_set(&send_timer, 1000, prkmcs_send_ctrl, NULL);
					prkmcs_send_ctrl();
				}
				else
				{
					log_debug("Busy control channel");
				}
			}
			else
			{
				SetChannel(data_channel);
				if (!is_receiver)
				{
					ctimer_set(&send_timer, 1000, prkmcs_send_data, NULL);
				}
				else
				{
					start_rx();
				}
				printf("Link index %u channel %u \r\n", my_link_index, data_channel);
			}
		}
		else
		{
			SetChannel(control_channel);
			start_rx();
		}
	}
	else
	{
		SetChannel(control_channel);
		start_rx();	
	}

	start_rx();
	schedule_next_slot(&slot_operation_timer);
}

// Time synch mote's slot operation
static void synch_slot_operation(struct rtimer* st, void* ptr)
{
	printf("Slot %lu\r\n", current_asn.ls4b);
	SetChannel(control_channel);
	if (building_signalmap)
	{
		if (current_asn.ls4b == BUILD_SIGNALMAP_PERIOD)
		{
			building_signalmap = 0;
			current_slot_start = 0;
			ASN_INIT(current_asn, 0, 0);
			network_time_set(0);
					
			time_synch_send();
		}
	}
	else
	{
		if (!prkmcs_is_synchronized || current_asn.ls4b % TIME_SYNCH_FREQUENCY == 0)
		{
			ctimer_set(&send_timer, 500, time_synch_send, NULL);
		}
	}
	schedule_next_slot(&slot_operation_timer);
}
