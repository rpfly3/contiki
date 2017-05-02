/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: generate conflict graph for scheduling
 * Note that only senders need to maintain conflict graph. This module doesn't
 * deal with control messages.
 */

#include "core/net/mac/prk-mcs/prkmcs.h"

link_er_t linkERTable[LINK_ER_TABLE_SIZE];
uint8_t link_er_size;
uint8_t er_sending_index;

/**************************** Link ER Table Management ************************/

/* initialize link ER table at the very beginning */
void initLinkERTable() 
{
	link_er_size = 0;
	uint8_t added = 0;
	// add non-local primary conflict links to the link ER table
	for (uint8_t j = 0; j < activeLinksSize; ++j)
	{
		added = 0;
		if (findLocalIndex(j) == INVALID_INDEX)
		{
			for (uint8_t i = 0; i < localLinksSize; ++i)
			{
				if (activeLinks[j].sender == activeLinks[localLinks[i]].sender || 
					activeLinks[j].sender == activeLinks[localLinks[i]].receiver || 
					activeLinks[j].receiver == activeLinks[localLinks[i]].sender ||
					activeLinks[j].receiver == activeLinks[localLinks[i]].receiver)
				{				
					// already added to linker er table but conflict with another link
					if (added)
					{
						linkERTable[link_er_size].conflict_flag |= (1 << i);	
						linkERTable[link_er_size].primary |= (1 << i);	
					}
					else
					{
						linkERTable[link_er_size].sender = activeLinks[j].sender;
						linkERTable[link_er_size].receiver = activeLinks[j].receiver;
						linkERTable[link_er_size].link_index = j;
						linkERTable[link_er_size].conflict_flag = 0;
						linkERTable[link_er_size].conflict_flag |= (1 << i);
						linkERTable[link_er_size].primary = 0;
						linkERTable[link_er_size].primary |= (1 << i);
						linkERTable[link_er_size].er_version = 0;
						linkERTable[link_er_size].I_edge = INVALID_DBM;

						added = 1;
						++link_er_size;	
					}
				}
			}
		}
	}
}

/* find the index to the entry of a link */
uint8_t findLinkERTableIndex(uint8_t index) 
{
	for (uint8_t i = 0; i < link_er_size; i++) 
	{
		if (linkERTable[i].link_index == index) 
		{
			return i;
		}
	}
	return INVALID_INDEX;
}

/* update the link er table according to received ER information */
void updateLinkER(uint8_t link_index, uint16_t er_version, float I_edge)
{
	uint8_t link_er_index = findLinkERTableIndex(link_index); 
	if (link_er_index == INVALID_INDEX)
	{
		if (link_er_size >= LINK_ER_TABLE_SIZE)
		{
			log_error("link ER table is full.");
			return;
		}
  
		link_er_index = link_er_size;

		linkERTable[link_er_index].sender = activeLinks[link_index].sender;
		linkERTable[link_er_index].receiver = activeLinks[link_index].receiver;
		linkERTable[link_er_index].link_index = link_index;
		linkERTable[link_er_index].er_version = er_version;
		linkERTable[link_er_index].I_edge = I_edge;

		++link_er_size;

		updateConflictGraphForERChange(link_index);
	}
	else
	{
		if (er_version > linkERTable[link_er_index].er_version)
		{
			linkERTable[link_er_index].er_version = er_version;
			linkERTable[link_er_index].I_edge = I_edge;

			updateConflictGraphForERChange(link_index);
		}
	}
}

/*********************** Contention Table  Management **********************/

uint8_t inER(float tx_power, float gain, float I_edge) 
{
    return (tx_power - gain >= I_edge);
}

uint8_t isConflicting(uint8_t link_er_index, uint8_t local_link_er_index) 
{
	float inbound_gain, outbound_gain;

	if (!localLinkERTable[local_link_er_index].is_sender)
	{
		inbound_gain = getInboundGain(linkERTable[link_er_index].sender);
		if (inbound_gain == INVALID_GAIN)
		{
			return 0;
		}
		if (inER(tx_power, inbound_gain, localLinkERTable[local_link_er_index].I_edge))
		{
			return 1;
		}
	}
	else
	{
		outbound_gain = getOutboundGain(linkERTable[link_er_index].receiver); 
		if (outbound_gain == INVALID_GAIN)
		{
			return 0;
		}
		if (inER(tx_power, outbound_gain, linkERTable[link_er_index].I_edge))
		{
			return 1;	
		}
	}

	return 0;
}

/* update contention due to local link er change of link @index */
void updateConflictGraphForLocalERChange(uint8_t local_link_er_index)
{
	for (uint8_t i = 0; i < link_er_size; i++)
	{
		// ER change doesn't affect primary conflict
		if (linkERTable[i].primary & (1 << local_link_er_index))
		{
			continue;
		}
		// Only update secondary conflict relations
		if (isConflicting(i, local_link_er_index))
		{
			linkERTable[i].conflict_flag |= (1 << local_link_er_index);
		}
		else
		{
			linkERTable[i].conflict_flag &= ~(1 << local_link_er_index);
		}
	}
}

/* update contention due to link er change of node @index */
void updateConflictGraphForERChange(uint8_t link_er_index)
{
	for (uint8_t i = 0; i < local_link_er_size; i++)
	{
 		// ER change doesn't affect primary conflict
		if (linkERTable[link_er_index].primary & (1 << i))
		{
			continue;
		}
		// Only update secondary conflict relations
		if (isConflicting(link_er_index, i))
		{
			linkERTable[link_er_index].conflict_flag |= (1 << i);
		}
		else
		{
			linkERTable[link_er_index].conflict_flag &= ~(1 << i);
		}
	}
}

/******************* interfaces to other modules ********************/

static uint8_t is_sending_local_er = 1;

void prepareERSegment(uint8_t *ptr)
{
	// check if there is anything to send
	if (local_link_er_size == 0)
	{
		return;
	}

	// alternatively send local er table and er table
	if (is_sending_local_er == 1 || link_er_size == 0)
	{
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].link_index), sizeof(uint8_t));
		ptr += sizeof(uint8_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].er_version), sizeof(uint16_t));
		ptr += sizeof(uint16_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].I_edge), sizeof(float));
		ptr += sizeof(float);

		++local_er_sending_index;
		// local er table is not empty, so tail check is good enough
		if (local_er_sending_index >= local_link_er_size)
		{
			is_sending_local_er = 0;
			local_er_sending_index = 0;
			er_sending_index = 0;
		}
		else
		{
			is_sending_local_er = 1;
		}
	}
	else
	{
		memcpy(ptr, &(linkERTable[er_sending_index].link_index), sizeof(uint8_t));
		ptr += sizeof(uint8_t);
		memcpy(ptr, &(linkERTable[er_sending_index].er_version), sizeof(uint16_t));
		ptr += sizeof(uint16_t);
		memcpy(ptr, &(linkERTable[er_sending_index].I_edge), sizeof(float));
		ptr += sizeof(float);

		++er_sending_index;
		// tail check the valid index
		if (er_sending_index >= link_er_size)
		{
			is_sending_local_er = 1;
			er_sending_index = 0;
			local_er_sending_index = 0;
		}
	}
}

/* init protocol signaling module */
void protocolSignalingInit()
{
	er_sending_index = 0;
	local_er_sending_index = 0;
	is_sending_local_er = 1;

	initLinkERTable();
}