/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/16/2016
 * @Description: LAMA scheduling
 */
#include "core/net/mac/prk-mcs/prkmcs.h"

uint8_t channel_status[CHANNEL_NUM];

/******************* LAMA *********************/

/* compute LAMA priority. For efficiency, we use uint8_t priority */
static uint16_t linkPriority(uint8_t index, struct asn_t time_slot, uint8_t channel) 
{
	uint16_t priority;
	uint8_t seed;
	if (channel != INVALID_CHANNEL)
	{
		seed = index ^ (uint8_t)time_slot.ls4b ^ channel;
	}
	else
	{
		seed = index ^ (uint8_t)time_slot.ls4b;
	}

	srand(seed);
	priority = ((uint8_t)rand() << 8) + index;
	return priority;
}

static uint16_t channelPriority(uint8_t index, struct asn_t time_slot, uint8_t channel)
{
	uint16_t priority;
	uint8_t seed = index ^ (uint8_t)time_slot.ls4b ^ channel;
	srand(seed);
	priority = ((uint8_t)rand() << 8) + channel;
	return priority;
}

static void resetChannelStatus()
{
	for (uint8_t i = 0; i < CHANNEL_NUM; ++i)
	{
		channel_status[i] = AVAILABLE;
	}
}

/* get the status of node in current_slot */
void runLama(struct asn_t current_slot) { 
	// Reset channel status
	resetChannelStatus();
		
	uint16_t prio, my_prio = 0;
	uint8_t local_link_er_index;
	// Select one link first from primary conflict links of this node
	for (uint8_t j = 0; j < localLinksSize; ++j)
	{
		prio = linkPriority(localLinks[j], current_slot, INVALID_CHANNEL);
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

	bool highest_priority = true;

	for (uint8_t j = 0; j < link_er_size; ++j)
	{
		if (linkERTable[j].primary & (1 << local_link_er_index))
		{
			// Compare the selected link with its primary conflict links
			prio = linkPriority(linkERTable[j].link_index, current_slot, INVALID_CHANNEL);
			if (prio > my_prio)
			{

				highest_priority = false;
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

	if (highest_priority)
	{
		uint8_t channel_count = CHANNEL_NUM;

		for (uint8_t i = 0; i < CHANNEL_NUM; ++i)
		{
			my_prio = linkPriority(my_link_index, current_slot, i + RF231_CHANNEL_MIN);
			for (uint8_t j = 0; j < link_er_size; ++j)
			{
				// Compare the selected link with its secondary conflict links
				if (linkERTable[j].secondary & (1 << local_link_er_index))
				{
					prio = linkPriority(linkERTable[j].link_index, current_slot, i + RF231_CHANNEL_MIN);
					if (prio > my_prio)
					{
						channel_status[i] = UNAVAILABLE;
						--channel_count;
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
	
		if (channel_count != 0)
		{
			my_prio = 0;
			for (uint8_t i = 0; i < CHANNEL_NUM; i++)
			{
				if (channel_status[i] == UNAVAILABLE)
				{
					continue;
				}
				else
				{
					prio = channelPriority(my_link_index, current_slot, i + RF231_CHANNEL_MIN);
					if (prio > my_prio)
					{
						my_prio = prio;
						data_channel = i + RF231_CHANNEL_MIN;
					}
					else
					{
						continue;
					}
				}
			}
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
			my_link_index = INVALID_INDEX;
			data_channel = INVALID_CHANNEL;
		}
	}
	else
	{
  		// when a primary conflict link is scheduled, this link is unschedulable
		my_link_index = INVALID_INDEX;
		data_channel = INVALID_CHANNEL;
	}

	return;
}

void onama_init()
{
	control_channel = RF231_CHANNEL_26;
}
