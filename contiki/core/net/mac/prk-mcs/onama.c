/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/16/2016
 * @Description: LAMA scheduling
 */
#include "core/net/mac/prk-mcs/prkmcs.h"

uint8_t inactive_channels[CHANNEL_NUM];

/******************* LAMA *********************/

/* compute LAMA priority. For efficiency, we use uint8_t priority */
static uint8_t computePriority(uint8_t index, struct asn_t time_slot, uint8_t channel) 
{
	return index ^ (uint8_t)time_slot.ls4b ^ channel;
}

static void resetChannel()
{
	for (uint8_t i = 0; i < CHANNEL_NUM; ++i)
	{
		if (i == 4 || i == 9 || i == 14)
		{
			inactive_channels[i] = AVAILABLE;
		}
		else
		{
			inactive_channels[i] = UNAVAILABLE;
		}
	}
}

/* get the status of node in current_slot */
void runLama(struct asn_t current_slot) { 
	// Reset channel status
	resetChannel();
		
	uint8_t channel_count = 0;
	uint8_t prio, my_prio = 0, my_prio_in_channel;
	// Select one link first from primary conflict links of this node
	for (uint8_t j = 0; j < localLinksSize; ++j)
	{
		prio = localLinks[j] ^ (uint8_t)current_slot.ls4b;
		if (prio > my_prio)
		{
			my_prio = prio;
			my_link_index = localLinks[j];
		}
	}

	uint8_t local_link_er_index = findLocalLinkERTableIndex(my_link_index);
	if (local_link_er_index == INVALID_INDEX)
	{
		log_error("The link is not found in local link er table");
		my_link_index = INVALID_INDEX;
		data_channel = INVALID_CHANNEL;
		return;
	}
	
	for (uint8_t j = 0; j < link_er_size; ++j)
	{
		//if (linkERTable[j].conflict_flag & (1 << local_link_er_index))
		if ((linkERTable[j].conflict_flag & (1 << local_link_er_index)) && (linkERTable[j].primary & (1 << local_link_er_index)))
		{
			// Compare the selected link with its primary conflict links
			uint8_t prio = linkERTable[j].link_index ^ (uint8_t)current_slot.ls4b;
			if (prio > my_prio)
			{
				// when a primary conflict link is scheduled, this link is unschedulable
				my_link_index = INVALID_INDEX;
				data_channel = INVALID_CHANNEL;
				return;
			}
		}
	}

	//data_channel = RF231_CHANNEL_25;
	// In data channel, schedule one link basing on conflict graph
	for (uint8_t i = 0; i < CHANNEL_NUM; ++i)
	{
		if (inactive_channels[i] == UNAVAILABLE)
		{
			continue;
		}

		my_prio_in_channel = my_prio ^ (i + RF231_CHANNEL_MIN);
		for (uint8_t j = 0; j < link_er_size; ++j)
		{
			if ((linkERTable[j].conflict_flag & (1 << local_link_er_index)) && !(linkERTable[j].primary & (1 << local_link_er_index)))
			{
				// Compare the selected link with its primary conflict links
				uint8_t prio = computePriority(linkERTable[j].link_index, current_slot, i + RF231_CHANNEL_MIN);
				if (prio > my_prio_in_channel)
				{
					inactive_channels[i] = UNAVAILABLE;
					break;
				}
			}
		}	
		// Record the scheduling result
		if (inactive_channels[i] == AVAILABLE)
		{
			++channel_count;
		}
	}

	
	if (channel_count == 0)
	{
		my_link_index = INVALID_INDEX;
		data_channel = INVALID_CHANNEL;
		return;
	}

	my_prio_in_channel = 0;
	for (uint8_t i = 0; i < CHANNEL_NUM; i++)
	{
		if (inactive_channels[i] == UNAVAILABLE)
		{
			continue;
		}
		else
		{
			prio = my_prio ^ (i + RF231_CHANNEL_MIN);
			if (prio > my_prio_in_channel)
			{
				my_prio_in_channel = prio;
				data_channel = i + RF231_CHANNEL_MIN;
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

void onama_init()
{
	control_channel = RF231_CHANNEL_26;
}
