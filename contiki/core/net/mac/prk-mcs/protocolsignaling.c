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

/* initialize link ER table with primary conflict links */
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
					// already added to link er table but conflict with another link
					if (!added)
					{
						linkERTable[link_er_size].sender = activeLinks[j].sender;
						linkERTable[link_er_size].receiver = activeLinks[j].receiver;
						linkERTable[link_er_size].link_index = j;
						linkERTable[link_er_size].primary = 0;
						linkERTable[link_er_size].secondary = 0;
						linkERTable[link_er_size].primary |= (1 << i);
						linkERTable[link_er_size].er_version = 0;
						linkERTable[link_er_size].I_edge = INVALID_DBM;

						added = 1;
					}
					else
					{
						linkERTable[link_er_size].primary |= (1 << i);	
					}
				}
			}
			if (added)
			{
				++link_er_size;	
			}
			else
			{
				// do nothing
			}
		}
	}
	return;
}

/* find the index to the entry of a link */
uint8_t findLinkERTableIndex(uint8_t index) 
{
	uint8_t link_er_index = INVALID_INDEX;

	for (uint8_t i = 0; i < link_er_size; i++) 
	{
		if (linkERTable[i].link_index == index) 
		{
			link_er_index = i;
			break;
		}
	}
	return link_er_index;
}

/* update the link er table according to received ER information 
 * Note that these info are assumed to be valid. So caller should do validness checking.
*/
void updateLinkER(uint8_t link_index, uint16_t er_version, float I_edge)
{
	uint8_t link_er_index = findLinkERTableIndex(link_index); 
	if (link_er_index == INVALID_INDEX)
	{
		if (link_er_size < LINK_ER_TABLE_SIZE)
		{
			link_er_index = link_er_size;
			++link_er_size;

			linkERTable[link_er_index].sender = activeLinks[link_index].sender;
			linkERTable[link_er_index].receiver = activeLinks[link_index].receiver;
			linkERTable[link_er_index].link_index = link_index;
			linkERTable[link_er_index].primary = 0;
			linkERTable[link_er_index].secondary = 0;
			linkERTable[link_er_index].er_version = er_version;
			linkERTable[link_er_index].I_edge = I_edge;

			// update conflict relation
			updateConflictGraphForERChange(link_er_index);
		}
		else
		{
			log_error("link ER table is full.");
		}
	}
	else
	{
		if (er_version > linkERTable[link_er_index].er_version)
		{
			linkERTable[link_er_index].er_version = er_version;
			linkERTable[link_er_index].I_edge = I_edge;

			// update conflict relation
			updateConflictGraphForERChange(link_er_index);
		}
		else
		{
			// outdated info -- do nothing
		}
	}

	return;
}

/*********************** Contention Table  Management **********************/

bool inER(float tx_power, float gain, float I_edge) 
{
	bool in_er = ((tx_power - gain) >= I_edge);
	return in_er;
}

bool isConflicting(uint8_t link_er_index, uint8_t local_link_er_index) 
{
	float inbound_gain, outbound_gain;
	bool conflicted = false;

	if (!localLinkERTable[local_link_er_index].is_sender)
	{
		// check if the link sender conflict with the receiver itself
		inbound_gain = getInboundGain(linkERTable[link_er_index].sender);
		if (!isEqual(inbound_gain, INVALID_GAIN))
		{
			if (inER(tx_power, inbound_gain, localLinkERTable[local_link_er_index].I_edge))
			{
				conflicted = true;
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

		
		// check if the sender conflicts with the link receiver
		outbound_gain = getNbOutboundGain(localLinkERTable[local_link_er_index].neighbor, linkERTable[link_er_index].receiver);
		if (!isEqual(outbound_gain, INVALID_GAIN))
		{
			if (inER(tx_power, outbound_gain, linkERTable[link_er_index].I_edge))
			{
				conflicted = true;
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
	else
	{
		// check if the sender itself conflict with the link receiver
		outbound_gain = getOutboundGain(linkERTable[link_er_index].receiver); 
		if (!isEqual(outbound_gain, INVALID_GAIN))
		{
			if (inER(tx_power, outbound_gain, linkERTable[link_er_index].I_edge))
			{
				conflicted = true;
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


		// check if the link sender conflict with the receiver
		inbound_gain = getNbInboundGain(linkERTable[link_er_index].sender, localLinkERTable[local_link_er_index].neighbor);
		if (!isEqual(inbound_gain, INVALID_GAIN))
		{
			if (inER(tx_power, inbound_gain, localLinkERTable[local_link_er_index].I_edge))
			{
				conflicted = true;
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

	return conflicted;
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
		else
		{
			// Only update secondary conflict relations
			if (isConflicting(i, local_link_er_index))
			{
				linkERTable[i].secondary |= (1 << local_link_er_index);
			}
			else
			{
				linkERTable[i].secondary &= ~(1 << local_link_er_index);
			}
		}
	}

	return;
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
		else
		{
			// Only update secondary conflict relations
			if (isConflicting(link_er_index, i))
			{
				linkERTable[link_er_index].secondary |= (1 << i);
			}
			else
			{
				linkERTable[link_er_index].secondary &= ~(1 << i);
			}
		}
	}

	return;
}

/******************* interfaces to other modules ********************/


/* prepare ER info for sending and invalid info can be detected by checking link_index == INVALID_INDEX */
bool prepareERSegment(uint8_t *ptr)
{
	bool prepared = false;
	if (!isEqual(linkERTable[er_sending_index].I_edge, INVALID_DBM))
	{
		if (linkERTable[er_sending_index].secondary || linkERTable[er_sending_index].primary)
		{
			memcpy(ptr, &(linkERTable[er_sending_index].link_index), sizeof(uint8_t));
			ptr += sizeof(uint8_t);
			memcpy(ptr, &(linkERTable[er_sending_index].er_version), sizeof(uint16_t));
			ptr += sizeof(uint16_t);
			memcpy(ptr, &(linkERTable[er_sending_index].I_edge), sizeof(float));
			ptr += sizeof(float);

			prepared = true;		
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
	++er_sending_index;
	return prepared;
}

/* init protocol signaling module */
void protocolSignalingInit()
{
	er_sending_index = 0;
	local_er_sending_index = 0;

	initLinkERTable();
}