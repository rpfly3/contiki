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

	if (ref_time + offset <= now)
	{
		log_info("Miss slot %lu", current_asn.ls4b);
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
	SetChannel(control_channel);
	wait_us(rand() % CCA_MAX_BACK_OFF_TIME);
	test_send();

	start_rx();
	schedule_next_slot(&slot_operation_timer);
}

void prkmcs_control_signaling(uint8_t node_index)
{
	if (node_addr == activeNodes[node_index])
	{
		ctimer_set(&send_timer, 200, prkmcs_send_ctrl, NULL);
	}
	else
	{
		start_rx();
	}

}

//Transceiving motes' slot operation
static void prkmcs_slot_operation(struct rtimer *st, void *ptr)
{		
	if (prkmcs_is_synchronized)
	{
		printf("Slot %lu\r\n", current_asn.ls4b);
		uint8_t duty_cicle = current_asn.ls4b % TIME_SYNCH_FREQUENCY;
		if (duty_cicle == 0)
		{
			SetChannel(control_channel);
			start_rx();
		}
		else if (duty_cicle == 1)
		{
			SetChannel(control_channel);
			uint8_t node_index = (current_asn.ls4b / TIME_SYNCH_FREQUENCY) % activeNodesSize;
			prkmcs_control_signaling(node_index);
		}
		else if (duty_cicle == 2)
		{
			runLama(current_asn);
			if (data_channel != INVALID_CHANNEL)
			{
				SetChannel(data_channel);
				if (!is_receiver)
				{
					ctimer_set(&send_timer, 200, prkmcs_send_data, NULL);
				}
				else
				{
					start_rx();
				}
				log_debug("Link index %u channel %u", my_link_index, data_channel);
			}
			else
			{
				log_debug("Not scheduled by LAMA");
			}
		}
	}
	else
	{
		SetChannel(control_channel);
		start_rx();	
	}
	schedule_next_slot(&slot_operation_timer);
}

// base station mote's slot operation
static void synch_slot_operation(struct rtimer* st, void* ptr)
{
	printf("Slot %lu\r\n", current_asn.ls4b);
 	// base station only stay in control channel
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
		else
		{
			// do nothing
		}
	}
	else
	{
		if (!prkmcs_is_synchronized)
		{
			ctimer_set(&send_timer, 200, time_synch_send, NULL);
		}
		else
		{
			uint8_t duty_cicle = current_asn.ls4b % TIME_SYNCH_FREQUENCY;	
			if (duty_cicle == 0)
			{
				ctimer_set(&send_timer, 200, time_synch_send, NULL);
			}
			else
			{
				// do nothing
			}
		}
	}
	schedule_next_slot(&slot_operation_timer);
}
