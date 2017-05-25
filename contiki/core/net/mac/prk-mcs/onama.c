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

/*
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