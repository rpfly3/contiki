/* 
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: controller input: current link PDR; controller output: \delata I;
 */


/*
 * TODO: 1. change ER version from uint16_t to uint8_t
 * 2. add data channel agreement in control packet or at the beginning of data subslot
 * 3. add ER info share control (check xiaohui's code)
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

/* used to compute current interference level when this information is not available from sampling */
float PDR_SINR[] = {0, 0.37427, 0.51427, 0.61427, 0.69427, 0.76427, 0.82427, 0.87427, 0.91427, 0.96427, 1.0043, 1.0443, 1.0843, 1.1143, 1.1543, 1.1843, 1.2143, 1.2443,
1.2743, 1.3043, 1.3343, 1.3643, 1.3943, 1.4243, 1.4443, 1.4743, 1.5043, 1.5243, 1.5543, 1.5843, 1.6043, 1.6343, 1.6543, 1.6843, 1.7043, 1.7343, 1.7643, 1.7843, 1.8143,
1.8343, 1.8643, 1.8843, 1.9143, 1.9443, 1.9643, 1.9943, 2.0143, 2.0443, 2.0743, 2.0943, 2.1243, 2.1543, 2.1743, 2.2043, 2.2343, 2.2643, 2.2943, 2.3243, 2.3543, 2.3843,
2.4143, 2.4443, 2.4743, 2.5043, 2.5343, 2.5743, 2.6043, 2.6443, 2.6743, 2.7143, 2.7543, 2.7843, 2.8243, 2.8643, 2.9143, 2.9543, 2.9943, 3.0443, 3.0943, 3.1443, 3.1943,
3.2443, 3.3043, 3.3643, 3.4243, 3.4943, 3.5643, 3.6343, 3.7243, 3.8043, 3.9043, 4.0043, 4.1243, 4.2543, 4.4043, 4.5843, 4.8043, 5.0743, 7.5334, 16.093};

local_link_er_t localLinkERTable[LOCAL_LINK_ER_TABLE_SIZE];
uint8_t local_link_er_size = 0;
uint8_t local_er_sending_index = 0;

PROCESS(install_latest_er_process, "Install latest er process");
process_event_t er_info_sent;
/***************** Local Link ER Table Management ****************/

void initController() 
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
		localLinkERTable[i].updated_er_version = 0;
		localLinkERTable[i].updated_sm_index = INVALID_INDEX;
		localLinkERTable[i].updated_I_edge = INVALID_ED;
		localLinkERTable[i].installed = 1;
    }

	return;
}

// initialize maximum local ER with existing signal map info make this entry valid
void initLocalLinkER(uint8_t local_link_er_index)
{
	if (valid_sm_entry_size != 0)
	{
		uint8_t index = valid_sm_entry_size - 1;	
/*
		localLinkERTable[local_link_er_index].I_edge = signalMap[index].inbound_ed;
		localLinkERTable[local_link_er_index].er_version = 1;
		localLinkERTable[local_link_er_index].sm_index = index;
*/

		localLinkERTable[local_link_er_index].updated_I_edge = signalMap[index].inbound_ed;
		localLinkERTable[local_link_er_index].updated_er_version = 1;
		localLinkERTable[local_link_er_index].updated_sm_index = index;
		localLinkERTable[local_link_er_index].installed = 0;
		//updateConflictGraphForLocalERChange(local_link_er_index);
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
uint8_t findLocalLinkERTableIndex(uint8_t link_index) {

	uint8_t local_link_er_index = INVALID_INDEX;
    for(uint8_t i = 0; i < local_link_er_size; ++i) 
    {
	    if (localLinkERTable[i].link_index == link_index) 
	    {
		    local_link_er_index = i;
		    break;
        }
    }
    return local_link_er_index;
}

/*************************** Controller ******************************/
/* compute the delta I for link with @sender; note that this task is executed only by receiver */
static float pdr2DeltaIdB(uint8_t pdr_index, uint8_t local_link_er_index) 
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

	if(deltaI_dB > 7)
	{
		deltaI_dB = 7;
	}
	else
	{
		// do nothing
	}
	return deltaI_dB;
}

static float deltaIdB2mW(uint8_t pdr_index, float deltaI_dB)
{
	float current_I_mW, current_I_dBm;

	if(pdrTable[pdr_index].nb_I > MINIMUM_MW)
	{
		current_I_mW= pdrTable[pdr_index].nb_I;
		current_I_dBm = mW2dBm(current_I_mW);
	}
	else
	{
		uint8_t link_pdr = pdrTable[pdr_index].pdr;
		current_I_dBm = tx_power - PDR_SINR[link_pdr];
		current_I_mW = dbm2mW(current_I_dBm);
	}

	float next_I_dBm = current_I_dBm + deltaI_dB;
	float next_I_mW = dbm2mW(next_I_dBm);
	
	float deltaI_mW = next_I_mW - current_I_mW;
	return deltaI_mW;
}

/* receiver update ER by computing \delta I */
void updateER(uint8_t pdr_index) 
{
	uint8_t local_link_er_index = pdrTable[pdr_index].local_link_er_index;
	// check if ER is valid
	if (localLinkERTable[local_link_er_index].er_version == 0)
	{
		initLocalLinkER(local_link_er_index);
	}
	else
	{
		float deltaI_dB = pdr2DeltaIdB(pdr_index, local_link_er_index);
		float deltaI_mW = deltaIdB2mW(pdr_index, deltaI_dB);

		uint8_t i = localLinkERTable[local_link_er_index].sm_index;
		if (deltaI_mW < 0 && i < valid_sm_entry_size - 1)
		{
			float totalI_mW = 0, inbound_gain, erI_mW;
			do
			{
				i += 1;
				// unknown ed indicates the largest er
				if(signalMap[i].inbound_ed != INVALID_ED)
				{
					inbound_gain = computeInboundGain(RF231_TX_PWR_MAX, signalMap[i].inbound_ed, 0);
					erI_mW = dbm2mW(tx_power - inbound_gain);
					totalI_mW += erI_mW;
				}
				else
				{
					break;
				}
			} while (fabs(totalI_mW) < fabs(deltaI_mW) && i < valid_sm_entry_size - 1);
		}
		else if (deltaI_mW > 0 && i > 0)
		{
			float totalI_mW = 0, inbound_gain, erI_mW;
			do
			{
				i -= 1;
				// unknown ed won't contribute to interference
				if(signalMap[i].inbound_ed != INVALID_ED)
				{
					inbound_gain = computeInboundGain(RF231_TX_PWR_MAX, signalMap[i].inbound_ed, 0);
					erI_mW = dbm2mW(tx_power - inbound_gain);
					totalI_mW += erI_mW;
				}
				else
				{
					continue;
				}
			} while (fabs(totalI_mW) < fabs(deltaI_mW) && i > 0);
		}
/*	
		localLinkERTable[local_link_er_index].sm_index = i;
		localLinkERTable[local_link_er_index].I_edge = signalMap[i].inbound_ed;
		localLinkERTable[local_link_er_index].er_version += 1;
*/
		localLinkERTable[local_link_er_index].updated_sm_index = i;
		localLinkERTable[local_link_er_index].updated_I_edge = signalMap[i].inbound_ed;
		localLinkERTable[local_link_er_index].updated_er_version += 1;
		localLinkERTable[i].installed = 0;
		// er version overflow warning
		//if(localLinkERTable[local_link_er_index].er_version == 255)
		if(localLinkERTable[local_link_er_index].updated_er_version == 255)
		{
			log_error("ER version will overflow soon");
		}
		else
		{
			// do nothing
		}

		// delay update or quick update (er_sending_index = 0) ???
		//updateConflictGraphForLocalERChange(local_link_er_index);
		//deltaI_mW *= 1000000;
		//printf("SM index %u, SM size %u, scaled deltaI_mW %f\r\n", i, valid_sm_entry_size, deltaI_mW);
	}

	return;
}

/* sender update local er by receiving er from receiver */
void updateLocalER(uint8_t local_link_er_index, uint8_t er_version, int8_t I_edge)
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
	return;
}

void installLatestLocalER()
{
	for(uint8_t i = 0; i < local_link_er_size; ++i)
	{
		if(localLinkERTable[i].installed == 0)
		{
			localLinkERTable[i].er_version = localLinkERTable[i].updated_er_version;
			localLinkERTable[i].sm_index = localLinkERTable[i].updated_sm_index;
			localLinkERTable[i].I_edge = localLinkERTable[i].updated_I_edge;
			localLinkERTable[i].installed = 1;
			
			updateConflictGraphForLocalERChange(i);
		}
		else
		{
			// do nothing
		}
	}
}

/* prepare local ER info for sending */
bool prepareLocalERSegment(uint8_t *ptr)
{
	bool prepared = false;
	if(localLinkERTable[local_er_sending_index].updated_er_version > 0)
	{
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].link_index), sizeof(uint8_t));
		ptr += sizeof(uint8_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].updated_er_version), sizeof(uint8_t));
		ptr += sizeof(uint8_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].updated_I_edge), sizeof(int8_t));
		ptr += sizeof(int8_t);
		prepared = true;

		process_post(&install_latest_er_process, er_info_sent, NULL);
	}
	/*
	else if (localLinkERTable[local_er_sending_index].er_version > 0)
	{
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].link_index), sizeof(uint8_t));
		ptr += sizeof(uint8_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].er_version), sizeof(uint8_t));
		ptr += sizeof(uint8_t);
		memcpy(ptr, &(localLinkERTable[local_er_sending_index].I_edge), sizeof(int8_t));
		ptr += sizeof(int8_t);
		
		prepared = true;
	}
	*/
	else
	{
		// do nothing
	}
	++local_er_sending_index;
	return prepared;
}

// add version control according to version control
void er_receive(uint8_t *ptr)
{
	uint8_t link_index;
	memcpy(&link_index, ptr, sizeof(uint8_t));
	ptr += sizeof(uint8_t);
	uint8_t er_version;
	memcpy(&er_version, ptr, sizeof(uint8_t));
	ptr += sizeof(uint8_t);
	int8_t I_edge;
	memcpy(&I_edge, ptr, sizeof(int8_t));
	ptr += sizeof(int8_t);

	if (link_index < activeLinksSize)
	{
		//log_debug("link index %d -- er_version %d -- I_edge %f", link_index, er_version, I_edge);
		uint8_t local_link_er_index = findLocalLinkERTableIndex(link_index);
		if (local_link_er_index != INVALID_INDEX)
		{
			if (activeLinks[link_index].sender == node_addr)
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

PROCESS_THREAD(install_latest_er_process, ev, data)
{
	PROCESS_BEGIN();
	while (1) 
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == er_info_sent);
		installLatestLocalER();
	}
	PROCESS_END();
}