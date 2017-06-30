/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/16/2016
 * @Description: LAMA scheduling
 */
#include "core/net/mac/prk-mcs/prkmcs.h"

uint8_t channel_status[CHANNEL_NUM];

/******************* LAMA *********************/

/* compute LAMA priority. For efficiency, we use uint8_t priority */
static uint32_t linkPriority(uint8_t index, struct asn_t time_slot) 
{
	uint32_t seed = index ^ time_slot.ls4b;
	srand(seed);
	uint32_t priority = (rand() << 8) + index;
	return priority;
}

static uint32_t linkChannelPriority(uint8_t index, struct asn_t time_slot, uint8_t channel)
{
	uint32_t seed = index ^ time_slot.ls4b ^ channel;
	srand(seed);
	uint32_t priority = (rand() << 8) + index;
	return priority;
}

static uint32_t channelPriority(struct asn_t time_slot, uint8_t channel)
{
	uint32_t seed = time_slot.ls4b ^ channel;
	srand(seed);
	uint32_t priority = (rand() << 8) + channel;
	return priority;
}

static void resetChannelStatus()
{
	for(uint8_t i = 0; i < CHANNEL_NUM; ++i)
	{
		channel_status[i] = AVAILABLE;
	}

	return;
}
/* multichannel LAMA
// LAMA has very low concurrency, so there is no interference control
void runLama(struct asn_t current_slot)
{
	uint32_t prio, my_prio = 0;
	uint8_t local_link_er_index;

	resetChannelStatus();

	// Select one link first from primary conflict links of this node
	for (uint8_t i = 0; i < localLinksSize; ++i)
	{
		prio = linkPriority(localLinks[i], current_slot);
		if (prio > my_prio)
		{
			my_prio = prio;
			my_link_index = localLinks[i];
			local_link_er_index = i;
		}
		else
		{
			// do nothing
		}
	}

	bool highest_prio = true;
	for(uint8_t i = 0; i < link_er_size; ++i)
	{
		if(linkERTable[i].primary & (1 << local_link_er_index))
		{
			prio = linkPriority(linkERTable[i].link_index, current_slot);
			if (prio > my_prio)
			{
				highest_prio = false;
				break;
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			// do nothing
		}
	}

	if(highest_prio)
	{
		uint8_t channel_count = CHANNEL_NUM;

		for(uint8_t i = 0; i < CHANNEL_NUM; ++i)
		{
			if(channel_status[i] == AVAILABLE)
			{
				my_prio = linkChannelPriority(my_link_index, current_slot, i + RF231_CHANNEL_MIN);
				for(uint8_t j = 0; j < link_er_size; ++j)
				{
					if(linkERTable[j].secondary & (1 << local_link_er_index))
					{
						prio = linkChannelPriority(linkERTable[j].link_index, current_slot, i + RF231_CHANNEL_MIN);
						if(prio > my_prio)
						{
							channel_status[i] = UNAVAILABLE;
							channel_count -= 1;
							break;
						}
						else
						{
							// do nothing
						}
					}
					else
					{
						// do nothing
					}
				}
			}
			else
			{
				channel_count -= 1;
			}
		}

		if(channel_count != 0)
		{
			my_prio = 0;
			for(uint8_t i = 0; i < CHANNEL_NUM; ++i)
			{
				if(channel_status[i] == AVAILABLE)
				{
					prio = channelPriority(current_slot, i + RF231_CHANNEL_MIN);
					if(prio > my_prio)
					{
						my_prio = prio;
						data_channel = i + RF231_CHANNEL_MIN;
					}
					else
					{
						// do nothing
					}
				}
				else
				{
					// do nothing
				}
			}

			if(activeLinks[my_link_index].sender == node_addr)
			{
				is_receiver = 0;
				pair_addr = activeLinks[my_link_index].receiver;
			}
			else
			{
				is_receiver = 1;
				pair_addr = activeLinks[my_link_index].sender;
			}
		}
		else
		{
			my_link_index = INVALID_INDEX;
			data_channel = INVALID_CHANNEL;
		}
	}
	else
	{
		my_link_index = INVALID_INDEX;
		data_channel = INVALID_CHANNEL;
	}
	return;
}
*/
/*
// single channel lama
void runLama(struct asn_t current_slot) { 
	uint32_t prio, my_prio = 0;
	uint8_t local_link_er_index;

	// Select one link first from primary conflict links of this node
	for (uint8_t j = 0; j < localLinksSize; ++j)
	{
		prio = linkPriority(localLinks[j], current_slot);
		if (prio > my_prio)
		{
			my_prio = prio;
			my_link_index = localLinks[j];
			local_link_er_index = j;
		}
		else
		{
			continue;
		}
	}


	data_channel = RF231_CHANNEL_26;	
	for (uint8_t j = 0; j < link_er_size; ++j)
	{
		if (linkERTable[j].primary & (1 << local_link_er_index) || linkERTable[j].secondary & (1 << local_link_er_index))
		{
			// Compare the selected link with its primary conflict links
			prio = linkPriority(linkERTable[j].link_index, current_slot);
			if (prio > my_prio)
			{
				my_link_index = INVALID_INDEX;
				data_channel = INVALID_CHANNEL;
				break;
			}
			else
			{
				continue;
			}
		}
		else
		{
			continue;
		}
	}

	if(my_link_index != INVALID_INDEX)
	{

		// Store the corresponding scheduling info
		if (activeLinks[my_link_index].sender == node_addr)
		{
			is_receiver = 0;
			pair_addr = activeLinks[my_link_index].receiver;
		}
		else
		{
			is_receiver = 1;
			pair_addr = activeLinks[my_link_index].sender;
		}
	}
	else
	{
		// do nothing	
	}
	return;
}

*/

/*
	OLAMA State update algorithm
	1) for each state in OLAMA state vector
		if state == UNDECIDED
			if all higher priority conflicg links are INACTIVE
				state = ACTIVE
		else
			do nothing
	2) output the oldest state, i.e., the entry indexed by the circular array head, to finalize OLAMA decision
	3) right shift by 1 
	4) update lates state right after oldest state update

*/
enum{
	INACTIVE = 0, // 0000 0000
	ACTIVE_CHANNEL_1 = 1, // 0000 0001
	ACTIVE_CHANNEL_2 = 2, // 0000 0010
	ACTIVE_CHANNEL_3 = 3, // 0000 0011
	ACTIVE_CHANNEL_4 = 4, // 0000 0100
	ACTIVE_CHANNEL_5 = 5, // 0000 0101
	ACTIVE_CHANNEL_6 = 6, // 0000 0110
	ACTIVE_CHANNEL_7 = 7, // 0000 0111
	ACTIVE_CHANNEL_8 = 8, // 0000 1000
	ACTIVE_CHANNEL_9 = 9, // 0000 1001
	ACTIVE_CHANNEL_10 = 10, // 0000 1010
	ACTIVE_CHANNEL_11 = 11, // 0000 1011
	ACTIVE_CHANNEL_12 = 12, // 0000 1100
	ACTIVE_CHANNEL_13 = 13, // 0000 1101
	ACTIVE_CHANNEL_14 = 14, // 0000 1110
	ACTIVE_CHANNEL_15 = 15, // 0000 1111
	UNDECIDED = 16, // 0001 0000
}
enum{
	ONAMA_CONVERGENCE_TIME = 128,
	GROUP_SIZE = 16,
	ROUND_SIZE = 8,
	ONAMA_SCHEDULE_SIZE = ONAMA_CONVERGENCE_TIME >> 1,
}

// ONAMA final decisions on whether to be active in the next ONAMA_CONVERGENCE_TIME slots
uint8_t onamaSchedule[ONAMA_SCHEDULE_SIZE];
uint8_t transientSchedule[ONAMA_CONVERGENCE_TIME];

bool conflict_graph_initialized = false;

uint8_t LAMA()
{
	uint8_t schedule_status = UNDECIDED;
	for(int i = 0; i < link_er_size; ++i)
	{
		
	}
	return schedule_status;
}

void initNextBitState(uint8_t transient_schedule_index, uint8_t onama_schedule_index)
{
	for(uint8_t i = 0; i < link_er_size; ++i)
	{
		// xiohui sets default to be active, but I set default to be inactive;
		if(transient_schedule_index % 2 == 0) 
		{
			linkERTable[i].active_bitmap[onama_schedule_index] &= 0x0F;
		}
		else
		{
			linkERTable[i].active_bitmap[onama_schedule_index] &= 0xF0;
		}
	}
}

void runONAMA(struct asn_t time_slot)
{
	uint32_t current_slot = time_slot.ls4b;
	uint32_t group_index = current_slot / GROUP_SIZE;
	uint8_t group_offset = current_slot % GROUP_SIZE, round_offset = group_index % ROUND_SIZE;


	// take a snapshot in the first slot of each group, and do this every GROUP_SIZE slots
	if(group_offset == 0 || !conflict_graph_initialized) 
	{
		for(uint8_t i = 0; i < LINK_ER_TABLE_SIZE; ++i)
		{
			// save the conflict relationships
			bool is_conflict = linkERTable[i].secondary & (1 << local_link_er_index);
			if(conflict_graph_initialized)
			{
				if(is_conflict)
				{
					linkERTable[i].conflict |= (1 << round_offset);
				}
				else
				{
					linkERTable[i].conflict &= ~(1 << round_offset);
				}
			}
			else
			{
				// each bit indicate the conflict relation in a round
				linkERTable[i].conflict = is_conflict ? 255 : 0;
			}
		}
		conflict_graph_initialized = true;
	}
	else
	{
		// no need to update conflict graph in other slots
	}

	// update ONAMA schedule status
	uint8_t transient_schedule_head = current_slot % ONAMA_CONVERGENCE_TIME;
	uint8_t oname_schedule_head = (current_slot >> 1) % ONAMA_SCHEDULE_SIZE;
	// update the schedule for each future slot until ONAMA_CONVERGENCE_TIME slot
	for(uint8_t i = 0; i < ONAMA_CONVERGENCE_TIME; ++i) 
	{
		onama_task(i, transient_schedule_head, onama_schedule_head);
	}
}

void onama_task(uint8_t i, uint8_t transient_schedule_head, uint8_t onama_schedule_head)
{
	// ONAMA state index for each time slot
	uint8_t transient_schedule_index = (i + transient_schedule_head) % ONAMA_CONVERGENCE_TIME;
	uint8_t onama_schedule_index = (transient_schedule_index >> 1) % ONAMA_SCHEDULE_SIZE;
	uint8_t schedule_status = transientSchedule[transient_schedule_index];

	// only for UNDECIDED
	if(schedule_status == UNDECIDED)
	{
		schedule_status = LAMA();
	} 
	else if(schedule_status > UNDECIDED)
	{
		log_error("Invalid onama schedule status");
	}
	else
	{
		// do nothing for decided statuses
	}

	// output transient_bitmap's oldest 2bit as olama_bitmap's latest bit
	if(i == 0)
	{

		schedule_status = transientSchedule[transient_schedule_index];

		// be conservative and treat undecided as inactive
		if(schedule_status == UNDECIDED)
		{
			schedule_status = INACTIVE;
		}

		if(schedule_status < UNDECIDED)
		{
			if(transient_schedule_index % 2 == 0) 
			{
				schedule_status = schedule_status << 4;
			}
			onamaSchedule[onama_schedule_index] |= schedule_status;
		}
		else
		{
			log_error("Invalid onama schedule status");
		}

		// reset transient schedule to undecided
		transientSchedule[transient_schedule_index] = UNDECIDED;

		initNextBitState(transient_schedule_index, onama_schedule_index);
	}
}