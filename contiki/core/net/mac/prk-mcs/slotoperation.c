#include "core/net/mac/prk-mcs/prkmcs.h"

static struct ctimer send_timer, ack_timer;

struct asn_t current_asn; //absolute slot number
volatile rtimer_clock_t current_slot_start;
uint8_t is_receiver, my_link_index, prkmcs_is_synchronized;
uint8_t control_channel = RF231_CHANNEL_26, data_channel;

static void signalmap_slot_operation(struct rtimer *st, void *ptr);
static void prkmcs_slot_operation(struct rtimer *st, void *ptr);
static void synch_slot_operation(struct rtimer *st, void *ptr);

/* schedule a wakeup at a specified offset from a reference time */
static bool prkmcs_schedule_slot_operation(struct rtimer *slot_timer, rtimer_clock_t ref_time, rtimer_clock_t offset)
{
	rtimer_clock_t now = RTIMER_NOW();
	bool scheduled = true;

	if (ref_time + offset <= now)
	{
		log_info("Miss slot %lu", current_asn.ls4b);
		scheduled = false;
	}
	else
	{
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
	}

	return scheduled;
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
// for now this one wont' work for multichannel 
static void prkmcs_control_signaling(uint8_t node_index)
{
	wait_us(rand() % CCA_MAX_BACK_OFF_TIME);
	bool idle_channel = getCCA(RF231_CCA_0, 0);
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
static void prkmcs_data_action()
{
	if(data_channel != INVALID_CHANNEL)
	{
		prkmcs_send_data();	
	}
	else
	{
		// do nothing
	}
}

static void prkmcs_data_scheduling()
{
	runLama(current_asn);
	if (data_channel != INVALID_CHANNEL)
	{
		if (is_receiver)
		{
			ctimer_set(&ack_timer, 500, prkmcs_schedule_ack, NULL);
		}
		else
		{
			start_rx();
			ctimer_set(&send_timer, 1000, prkmcs_data_action, NULL);
		}
		// the scheduling info is not the real one, which depends on the negotiation of sender and receiver
		log_debug("Link index %u channel %u node %u", my_link_index, data_channel, node_addr);
	}
	else
	{
		log_debug("Not scheduled by LAMA");
	}
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
		printf("Neighbor %u:  Inbound %d Outbound %d    ", signalMap[i].neighbor, signalMap[i].inbound_ed, signalMap[i].outbound_ed);

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
uint8_t node_index, duty_cicle;
//Transceiving motes' slot operation
static void prkmcs_slot_operation(struct rtimer *st, void *ptr)
{		
	if (prkmcs_is_synchronized)
	{
		printf("Slot %lu\r\n", current_asn.ls4b);
		duty_cicle = current_asn.ls4b % TIME_SYNCH_FREQUENCY;

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
		/*
		else if(current_asn.ls4b == BUILD_SIGNALMAP_PERIOD + 1)
		{
			showSM();
		}
		*/
		else
		{
			if (duty_cicle == 0)
			{
				start_rx();
				/*
				printf("Conflict link set size: \r\n");
				for(uint8_t i = 0; i < local_link_er_size; ++i)
				{
					printf("Local Link %u, size %u \r\n", localLinkERTable[i].link_index, conflict_set_size[i]);
				}
				*/
			}
			else if (duty_cicle == 1)
			{
				node_index = ((current_asn.ls4b / TIME_SYNCH_FREQUENCY) * 2 + 0) % activeNodesSize;
				prkmcs_control_signaling(node_index);
			}
			else if (duty_cicle == 2)
			{
				prkmcs_data_scheduling();
			}
			else if (duty_cicle == 3)
			{
				node_index = ((current_asn.ls4b / TIME_SYNCH_FREQUENCY) * 2 + 1) % activeNodesSize;
				prkmcs_control_signaling(node_index);
			}
			else if (duty_cicle == 4)
			{
				prkmcs_data_scheduling();
			}
		}
	}
	else
	{
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
		duty_cicle = current_asn.ls4b % TIME_SYNCH_FREQUENCY;	
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
