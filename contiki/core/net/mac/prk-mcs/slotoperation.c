#include "core/net/mac/prk-mcs/prkmcs.h"

static struct ctimer send_timer;

struct asn_t current_asn; //absolute slot number
volatile rtimer_clock_t current_slot_start;
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
		//schedule slot operation for transceiving motes
		rtimer_set(slot_timer, current_slot_start, 0, (void(*)(struct rtimer*, void *))prkmcs_slot_operation, NULL);
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

static void signalmap_signaling(uint8_t node_index)
{
	if (node_addr == activeNodes[node_index])
	{
		ctimer_set(&send_timer, 500, test_send, NULL);
	}
	else
	{
		start_rx();
	}
}

void prkmcs_control_signaling(uint8_t node_index)
{
	wait_us(rand() % CCA_MAX_BACK_OFF_TIME);
	uint8_t idle_channel = getCCA(RF231_CCA_0, 0);
	if (idle_channel)
	{
		ctimer_set(&send_timer, 500, prkmcs_send_ctrl, NULL);
	}
	else
	{
		start_rx();
	}
/*
	if (node_addr == activeNodes[node_index])
	{
		ctimer_set(&send_timer, 500, prkmcs_send_ctrl, NULL);
	}
	else
	{
		start_rx();
	}
*/
}

void showSM()
{
	printf("SM size %u: \r\n", valid_sm_entry_size);
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i)
	{
		if (i % 10 == 0)
		{
			printf("\r\n");
		}
		printf("Neighbor %u:  Inbound %u Outbound %u    ", signalMap[i].neighbor, signalMap[i].inbound_ed, signalMap[i].outbound_ed);

	}
	printf("\r\n");
/*
	printf("Neighbor SM size %u: \r\n", valid_nb_sm_entry_size);
	for (uint8_t i = 0; i < valid_nb_sm_entry_size; ++i)
	{
		printf("Neighbor %u: \r\n", nbSignalMap[i].neighbor);
		for (uint8_t j = 0; j < nbSignalMap[i].entry_num; ++j)
		{
			printf("Node %u: in %f out %f ", nbSignalMap[i].nb_sm[j].neighbor, nbSignalMap[i].nb_sm[j].inbound_gain, nbSignalMap[i].nb_sm[j].outbound_gain);
			if (j % 5 == 0)
			{
				printf("\r\n");
			}
		}
		printf("\r\n");
	}
 */
}
uint8_t node_index;
//Transceiving motes' slot operation
static void prkmcs_slot_operation(struct rtimer *st, void *ptr)
{		
	if (prkmcs_is_synchronized)
	{
		printf("Slot %lu\r\n", current_asn.ls4b);
		uint8_t duty_cicle = current_asn.ls4b % TIME_SYNCH_FREQUENCY;

		if (current_asn.ls4b <= BUILD_SIGNALMAP_PERIOD)
		{
			if (duty_cicle == 0)
			{
				start_rx();
			}
			else if (duty_cicle == 1)
			{
				node_index = ((current_asn.ls4b / TIME_SYNCH_FREQUENCY) * 4 + 0) % activeNodesSize;				
				signalmap_signaling(node_index);
			}
			else if (duty_cicle == 2)
			{
				node_index = ((current_asn.ls4b / TIME_SYNCH_FREQUENCY) * 4 + 1) % activeNodesSize;				
				signalmap_signaling(node_index);	
			}
			else if (duty_cicle == 3)
			{
				node_index = ((current_asn.ls4b / TIME_SYNCH_FREQUENCY) * 4 + 2) % activeNodesSize;				
				signalmap_signaling(node_index);
			}
			else if (duty_cicle == 4)
			{
				node_index = ((current_asn.ls4b / TIME_SYNCH_FREQUENCY) * 4 + 3) % activeNodesSize;				
				signalmap_signaling(node_index);	
			}
		
		}
		else
		{

			if (duty_cicle == 0)
			{
				start_rx();
			}
			else if (duty_cicle == 1 || duty_cicle == 3)
			{
				uint8_t node_index = (current_asn.ls4b / TIME_SYNCH_FREQUENCY) % activeNodesSize;
				prkmcs_control_signaling(node_index);
			}
			else 
			{
				runLama(current_asn);
				if (data_channel != INVALID_CHANNEL)
				{
					log_debug("Link index %u channel %u", my_link_index, data_channel);
					if (!is_receiver)
					{
						ctimer_set(&send_timer, 500, prkmcs_send_data, NULL);
					}
					else
					{
						start_rx();
					}
				}
				else
				{
					log_debug("Not scheduled by LAMA");
				}
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
	if (!prkmcs_is_synchronized)
	{
		ctimer_set(&send_timer, 500, time_synch_send, NULL);
	}
	else
	{
		uint8_t duty_cicle = current_asn.ls4b % TIME_SYNCH_FREQUENCY;	
		if (duty_cicle == 0)
		{
			ctimer_set(&send_timer, 500, time_synch_send, NULL);
		}
		else
		{
			// do nothing
		}
	}
	schedule_next_slot(&slot_operation_timer);
}
