/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/16/2016
 * @Description: LAMA scheduling
 */
#include "core/net/mac/prk-mcs/prkmcs.h"

uint8_t channel_status[CHANNEL_NUM];

/******************* LAMA *********************/

/* compute LAMA priority. For efficiency, we use uint8_t priority */
static uint16_t linkPriority(uint8_t index, struct asn_t time_slot) 
{
	uint8_t seed = index ^ (uint8_t)time_slot.ls4b;
	srand(seed);
	uint16_t priority = ((uint8_t)rand() << 8) + index;
	return priority;
}

/* get the status of node in current_slot */
void runLama(struct asn_t current_slot) { 
	uint16_t prio, my_prio = 0;
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
