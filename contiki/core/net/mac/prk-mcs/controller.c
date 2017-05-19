/* 
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: controller input: current link PDR; controller output: \delata I;
 */

#include "core/net/mac/prk-mcs/prkmcs.h"

/* used to compute 1/a(t)*/
float PDR_SLOPE_TABLE[] = {840.5, 19.3, 11.4, 8.5, 7.0, 6.1, 5.4, 4.9, 4.5, 4.2, 4.0, 3.8, 3.6, 3.5, 3.4, 3.3, 3.1, 3.1, 3.0, 2.9, 2.9, 
	2.8, 2.8, 2.8, 2.7, 2.7, 2.7, 2.6, 2.6, 2.6, 2.6, 2.6, 2.6, 2.6, 2.6, 2.5, 2.5, 2.5, 2.5, 2.6, 2.6, 2.6, 2.6, 2.6, 2.6, 2.6, 2.6, 2.6, 2.7, 
	2.7, 2.7, 2.7, 2.8, 2.8, 2.8, 2.9, 2.9, 2.9, 3.0, 3.0, 3.1, 3.1, 3.2, 3.3, 3.3, 3.4, 3.5, 3.5, 3.6, 3.7, 3.8, 3.9, 4.0, 4.1, 4.3, 4.4, 4.6, 
	4.7, 4.9, 5.1, 5.3, 5.6, 5.8, 6.1, 6.5, 6.9, 7.3, 7.8, 8.4, 9.2, 10.0, 11.1, 12.5, 14.1, 16.4, 19.6, 24.3, 32.3, 619.7, 1233.2, 3429.6};

/* used to compute 1/a_{0} using PDR*/
float PDR_INV_TABLE[] = {0, 0.36, 0.51, 0.61, 0.69, 0.75, 0.81, 0.86, 0.91, 0.95, 0.99, 1.03, 107, 1.11, 1.14, 1.17, 1.21, 1.24, 1.27, 
	1.30, 1.33, 1.36, 1.38, 1.41, 1.44, 1.47, 1.49, 1.52, 1.55, 1.57, 1.60, 1.62, 1.65, 1.67, 1.70, 1.73, 1.75, 1.78, 1.80, 1.83, 
	1.85, 1.88, 1.90, 1.93, 1.96, 1.98, 2.01, 2.03, 2.06, 2.09, 2.11, 2.14, 2.17, 2.20, 2.23, 2.25, 2.28, 2.31, 2.34, 2.37, 2.40, 
	2.43, 2.46, 2.50, 2.53, 2.56, 2.60, 2.63, 2.67, 2.70, 2.74, 2.78, 2.82, 2.86, 2.90, 2.94, 2.99, 3.03, 3.08, 3.13, 3.18, 3.24, 
	3.29, 3.35, 3.42, 3.48, 3.55, 3.63, 3.71, 3.80, 3.89, 4.00, 4.12, 4.25, 4.40, 4.58, 4.79, 5.07, 7.53, 16.09, 28.71};

local_link_er_t localLinkERTable[LOCAL_LINK_ER_TABLE_SIZE];
uint8_t local_link_er_size;
uint8_t local_er_sending_index;

/***************** Local Link ER Table Management ****************/

/* initialize local link ER table */
void initLocalLinkERTable() 
{
	if (localLinksSize >= LOCAL_LINK_ER_TABLE_SIZE)
	{
		do
		{
			log_error("The local link ER table is too small.");
		} while (1);
	}

	local_link_er_size = localLinksSize;

	for (uint8_t i = 0; i < localLinksSize; ++i) 
	{
		if (node_addr == activeLinks[localLinks[i]].sender)
		{
			localLinkERTable[i].neighbor = activeLinks[localLinks[i]].receiver;
			localLinkERTable[i].is_sender = 1;
		}
		else
		{
			localLinkERTable[i].neighbor = activeLinks[localLinks[i]].sender;
			localLinkERTable[i].is_sender = 0;
		}
		localLinkERTable[i].link_index = localLinks[i];
		localLinkERTable[i].pdr_req = PDR_REQUIREMENT;
		localLinkERTable[i].er_version = 0;
		localLinkERTable[i].sm_index = INVALID_INDEX;
		localLinkERTable[i].I_edge = INVALID_ED;
    }

	return;
}

// initialize maximum local ER with existing signal map info make this entry valid
void initLocalLinkER(uint8_t local_link_er_index)
{
	if (valid_sm_entry_size != 0)
	{
		uint8_t index = valid_sm_entry_size - 1;	

		localLinkERTable[local_link_er_index].I_edge = signalMap[index].inbound_ed;
		localLinkERTable[local_link_er_index].er_version = 1;
		localLinkERTable[local_link_er_index].sm_index = index;

		updateConflictGraphForLocalERChange(local_link_er_index);
	}
	else
	{
		do
		{
			log_error("No signal map info");
		} while (1);
	}

	return;
}

/* find the local_link_er_index in local link er table according to active link index */
uint8_t findLocalLinkERTableIndex(uint8_t index) {

	uint8_t local_link_er_index = INVALID_INDEX;
    for(uint8_t i = 0; i < local_link_er_size; i++) 
    {
	    if (localLinkERTable[i].link_index == index) 
	    {
		    local_link_er_index = i;
		    break;
        }
    }
    return local_link_er_index;
}

/*************************** Controller ******************************/
/* controller module initialization */
void initController() 
{
	local_link_er_size = 0;
	local_er_sending_index = 0;
	initLocalLinkERTable();
}

/* compute the delta I for link with @sender; note that this task is executed only by receiver */
float pdr2DeltaIdB(uint8_t pdr_index, uint8_t local_link_er_index) 
{
	float slope_inv;
	uint8_t link_pdr = pdrTable[pdr_index].pdr;
	uint8_t link_pdr_sample = pdrTable[pdr_index].pdr_sample;
	uint8_t link_pdr_req = localLinkERTable[local_link_er_index].pdr_req;
	
	if (abs(link_pdr - link_pdr_req) > E_0)
	{
		slope_inv = (PDR_INV_TABLE[link_pdr_req] - PDR_INV_TABLE[link_pdr]) / (link_pdr_req - link_pdr);
	}
	else
	{
		slope_inv = PDR_SLOPE_TABLE[link_pdr];
	}
	
	float deltaI_dB = (ALPHA * link_pdr + (1 - ALPHA) * link_pdr_sample - link_pdr_req) * slope_inv / 100;
	return deltaI_dB;
}

/* receiver update ER by computing \delta I */
void updateER(uint8_t local_link_er_index, uint8_t pdr_index) 
{
	// check if ER is valid
	if (localLinkERTable[local_link_er_index].sm_index == INVALID_INDEX)
	{
		initLocalLinkER(local_link_er_index);
	}
	else
	{
		uint8_t i = localLinkERTable[local_link_er_index].sm_index;
		float deltaI_dB = pdr2DeltaIdB(pdr_index, local_link_er_index);
		float current_I_mW = pdrTable[pdr_index].nb_I;
		float current_I_dBm = mW2dBm(current_I_mW);
		float next_I_dBm = current_I_dBm + deltaI_dB;
		float next_I_mW = dbm2mW(next_I_dBm);
		float deltaI_mW = next_I_mW - current_I_mW;
		float totalI_mW = 0;

		if (deltaI_mW < 0)
		{
			while (i < valid_sm_entry_size - 1)
			{
				float inbound_gain = computeInboundGain(RF231_TX_PWR_MAX, signalMap[i].inbound_ed, 0);
				float erI_mW = dbm2mW(tx_power - inbound_gain);
				totalI_mW += erI_mW;
				if (fabs(totalI_mW) >= fabs(deltaI_mW))
				{
					break;
				}
				++i;
			}
		}
		else if (deltaI_mW > 0)
		{
			while (i > 0)
			{
				float inbound_gain = computeInboundGain(RF231_TX_PWR_MAX, signalMap[i].inbound_ed, 0);
				float erI_mW = dbm2mW(tx_power - inbound_gain);
				totalI_mW += erI_mW;
				if (fabs(totalI_mW) > fabs(deltaI_mW))
				{
					break;
				}
				--i;
			}
		}
	
		localLinkERTable[local_link_er_index].sm_index = i;
		localLinkERTable[local_link_er_index].I_edge = signalMap[i].inbound_ed;
		++(localLinkERTable[local_link_er_index].er_version);

		updateConflictGraphForLocalERChange(local_link_er_index);
		printf("SM index %u, SM size %u, I_edge %u, deltaI_mW %f\r\n", i, valid_sm_entry_size, localLinkERTable[local_link_er_index].I_edge, deltaI_mW);
	}

	return;
}

/* sender update local er by receiving er from receiver */
void updateLocalER(uint8_t local_link_er_index, uint16_t er_version, uint8_t I_edge)
{
	if (er_version > localLinkERTable[local_link_er_index].er_version)
	{
		localLinkERTable[local_link_er_index].er_version = er_version;
		localLinkERTable[local_link_er_index].I_edge = I_edge;

		updateConflictGraphForLocalERChange(local_link_er_index);
	}
	else
	{
		// do nothing
	}
}

/* prepare local ER info for sending */
bool prepareLocalERSegment(uint8_t *ptr)
{
	bool prepared = false;
	if (localLinkERTable[local_er_sending_index].er_version > 0)
	{
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].link_index), sizeof(uint8_t));
		ptr += sizeof(uint8_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].er_version), sizeof(uint16_t));
		ptr += sizeof(uint16_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].I_edge), sizeof(uint8_t));
		ptr += sizeof(uint8_t);

		prepared = true;
	}
	else
	{
		// do nothing
	}
	++local_er_sending_index;
	return prepared;
}


void er_receive(uint8_t *ptr, linkaddr_t sender)
{
	uint8_t link_index;
	memcpy(&link_index, ptr, sizeof(uint8_t));
	ptr += sizeof(uint8_t);
	uint16_t er_version;
	memcpy(&er_version, ptr, sizeof(uint16_t));
	ptr += sizeof(uint16_t);
	uint8_t I_edge;
	memcpy(&I_edge, ptr, sizeof(uint8_t));
	ptr += sizeof(uint8_t);

	if (link_index < activeLinksSize)
	{
		//log_debug("link index %d -- er_version %d -- I_edge %f", link_index, er_version, I_edge);
		uint8_t local_link_er_index = findLocalLinkERTableIndex(link_index);
		if (local_link_er_index != INVALID_INDEX)
		{
			if (activeLinks[link_index].sender == node_addr && activeLinks[link_index].receiver == sender)
			{
				updateLocalER(local_link_er_index, er_version, I_edge);
			}	
			else
			{
				// do nothing
			}
		}
		else
		{
			updateLinkER(link_index, er_version, I_edge);
		}
	}
	else
	{
		log_error("Received invalid link index");
	}
	return;
}
