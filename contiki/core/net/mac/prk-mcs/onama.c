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
	2) output the oldest state, i.e., the entry indexed by the circular array head, to finalized OLAMA decision
	3) right shift by 1 
	4) update lates state right after oldest state update

*/
enum{
	UNDECIDED = 0, // 00
	UNDEFINED = 1, // 01
	INACTIVE = 2, // 10
	ACTIVE = 3, // 11
}
enum{
	SLOT_MASK = 0x3F,
	MAX_SLOT_FORWARD = SLOT_MASK + 1,
	CONFLICT_GRAPH_DIAMETER = 7,
	CONTROL_SIGNALING_INTERVAL = 16,
	ONAMA_CONVERGENCE_TIME = CONFLICT_GRAPH_DIAMETER * CONTROL_SIGNALING_INTERVAL,
	GROUP_SIZE = 16,
	ROUND_SIZE = ONAMA_CONVERGENCE_TIME / GROUP_SIZE + 1,
	ONAMA_SCHEDULE_SIZE = MAX_SLOT_FORWARD / 3,
}

bool next_state_initialized = false, conflict_graph_initialized = false;

bool LAMA()
{
	bool scheduled = false;
	return scheduled;
}

void initNextBitState(uint8_t x, uint8_t y)
{
	for(uint8_t i = 0; i < link_er_size; ++i)
	{
		linkERTable[i].active_bitmap[x] |= (1 << y);
	}
}

uint8_t onamaSchedule[ONAMA_SCHEDULE_SIZE];
void runONAMA(struct asn_t time_slot)
{
	uint32_t current_slot = time_slot.ls4b;
	uint32_t group_index = current_slot / GROUP_SIZE;
	uint8_t group_offset = current_slot % GROUP_SIZE;
	uint8_t round_offset = group_index % ROUND_SIZE;

	if(group_offset == 0 || !conflict_graph_initialized) 
	{
		for(uint8_t i = 0; i < LINK_ER_TABLE_SIZE; ++i)
		{
			bool is_conflict = linkERTable[i].secondary & (1 << local_link_er_index);
			if(conflict_graph_initialized)
			{
				if(is_conflict)
				{
					// not add conflict member yet
					linkERTable[i].conflict |= (1 << round_offset);
				}
				else
				{
					linkERTable[i].conflict &= ~(1 << round_offset);
				}
			}
			else
			{
				// each bit indicate the conflict relation in a round (M consecutive slots)
				linkERTable[i].conflict = is_conflict ? 255 : 0;
			}
		}
		conflict_graph_initialized = true;
	}
	else
	{
		// nothing
	}

	uint8_t onama_round_index = 0;
}

void onama_task()
{
	uint8_t index = (i + bitmap_head) % ONAMA_CONVERGENCE_TIME;

	uint8_t x = index / 8;
	uint8_t y = index % 8;
	uint8_t x_2 = index / 4;
	uint8_t y_2 = index % 4;

	uint8_t schedule_status = ((bitmap[x_2] >> (y_2 + 1)) & 1) << 1 + ((bitmap[x_2] >> y_2) & 1);

	if(schedule_status == UNDEFINED)
	{
		log_error("Invalid onama schedule status");
	}
	else if(schedule_status == UNDECIDED)
	{
		if(LAMA())
		{
			bitmap[x_2] |= (1 << y_2);
			bitmap[x_2] |= (1 << (y_2 + 1));
		}
		else
		{
			// do nothing	
		}
	}

	// output transient_bitmap's oldest 2bit as olama_bitmap's latest bit
	if(i == 0)
	{
		x_ = bitmap_head / 8;
		y_ = bitmap_head % 8;

		schedule_status = ((bitmap[x_2] >> (y_2 + 1)) & 1) << 1 + ((bitmap[x_2] >> y_2) & 1);

		// be conservative and treat undecided as inactive
		if(schedule_status == ACTIVE)
		{
			onamaSchedule[x_] |= (1 << y_);
		}
		else
		{
			onamaSchedule[x_] &= ~(1 << y_);
		}

		bitmap[x_2] &= ~(1 << y_2);
		bitmap[x_2] &= ~(1 << (y_2 + 1));

		initNextBitState(x, y);
		next_state_initialized = true;
	}
}