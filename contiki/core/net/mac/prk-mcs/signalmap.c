/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: Generate signal map and prepare signalmap segment for control packet
 * Note that senders and receivers use different ways to maintain signal map list !!!
 */

#include "core/net/mac/prk-mcs/prkmcs.h"

/* NOTE: the signalMap is always sorted, the entries after an invalid entry are all invalid!!! */
/* signal map: used for adjust ER */
sm_entry_t signalMap[SIGNAL_MAP_SIZE];
uint8_t valid_sm_entry_size;
static uint8_t sm_sending_index;

/* neighbors' signal maps: used for get conflicting relations */
nb_sm_t nbSignalMap[NB_SM_SIZE];
static uint8_t valid_nb_sm_entry_size;
static uint8_t nb_sm_sending_index;
static uint8_t nb_sm_entry_sending_index;

PROCESS(signalmap_process, "Signalmap process");
process_event_t	signalmap_ready;

/*************** Signal Map tabel Management ***************/
/* find the table index to the valid entry for a neighbor */
uint8_t findSignalMapIndex(linkaddr_t neighbor) 
{
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i) 
	{
        if(signalMap[i].neighbor == neighbor) 
        {
            return i;
        }
    }
	return INVALID_INDEX;
}

/* find the total number of valid entries in the signal map table */
uint8_t getSignalMapValidEntrySize() 
{
	return valid_sm_entry_size;
}

/* sort Signal Map in ascending order of inbound gain after updating */
static void sortSignalMap(uint8_t index) 
{
	uint8_t i;
	sm_entry_t* signal;

	sm_entry_t tmp = signalMap[index];
	uint16_t updated_inbound_gain = tmp.inbound_gain;

	if (index > 0 && updated_inbound_gain < signalMap[index - 1].inbound_gain) 
	{
		for (i = index; i > 0; i--) 
		{
			signal = &signalMap[i - 1];
			if (signal->inbound_gain > updated_inbound_gain) 
			{
				signalMap[i] = signalMap[i - 1];
			}
			else 
			{
				break;
			}
		}
		signalMap[i] = tmp;
	}
	else if (index < valid_sm_entry_size - 1 && updated_inbound_gain > signalMap[index + 1].inbound_gain) 
	{
		for (i = index; i + 1 < valid_sm_entry_size; i++) 
		{
			signal = &signalMap[i + 1];
			if (signal->inbound_gain < updated_inbound_gain) 
			{
				signalMap[i] = signalMap[i + 1];
			}
			else 
			{
				break;
			}
		}
		signalMap[i] = tmp;
	}
	//process_post(&signalmap_process, signalmap_ready, NULL);
}


/* update a signal map entry with given parameters: keep the closest SIGNAL_MAP_SIZE neighbors, 
 * in terms of inbound gain. If outbound_gain ix INVALID_GAIN, it means that we don't need to update outbound gain.
*/
void updateOutboundGain(linkaddr_t sender, float outbound_gain) 
{
	if (outbound_gain == INVALID_GAIN) 
	{
		log_error("Outbound gain is invalid.");
		return;
	}

    sm_entry_t* signal;

    uint8_t index = findSignalMapIndex(sender);

    if(index != INVALID_INDEX) 
    {
        signal = &signalMap[index];
		signal->outbound_gain = outbound_gain;
    } 
	else 
	{
		if (valid_sm_entry_size >= SIGNAL_MAP_SIZE)
		{
			log_error("Signal map is full.");
			return;
		}
		else
		{
			signal = &signalMap[valid_sm_entry_size];
			++valid_sm_entry_size;

			signal->neighbor = sender;
			signal->outbound_gain = outbound_gain;
		}
	} 
}

/* add entries to local signal map to build the inital signal map */
void updateInboundGain(linkaddr_t sender, float inbound_gain)
{
	if (inbound_gain == INVALID_GAIN) 
	{
		log_error("Inbound gain is invalid.");
		return;
	}

	sm_entry_t* signal;
	uint8_t index = findSignalMapIndex(sender);

	if (index != INVALID_INDEX) 
	{
		signal = &signalMap[index];
		signal->inbound_gain = signal->inbound_gain * ALPHA + inbound_gain * (1 - ALPHA);
	} 
	else 
	{
		if (valid_sm_entry_size >= SIGNAL_MAP_SIZE)
		{
			log_error("Signal map is full.");
			return;
		}
		else
		{
			signal = &signalMap[valid_sm_entry_size];
			++valid_sm_entry_size;

			signal->neighbor = sender;
			signal->inbound_gain = inbound_gain;
		}
	} 

	// Resort the signal map
	sortSignalMap(index);	
}

/************************* neighbor signal map management **************************/

uint8_t findNbSignalMapIndex(linkaddr_t neighbor)
{
	for (uint8_t i = 0; i < valid_nb_sm_entry_size; ++i)
	{
		if (nbSignalMap[i].neighbor == neighbor)
		{
			return i;
		}
	}
	return INVALID_INDEX;
}

uint8_t findNbSignalMapEntryIndex(uint8_t nb_sm_index, linkaddr_t neighbor)
{
	for (uint8_t i = 0; i < nbSignalMap[nb_sm_index].entry_num; ++i)
	{
		if (nbSignalMap[nb_sm_index].nb_sm[i].neighbor == neighbor)
		{
			return i;
		}
	}
	return INVALID_INDEX;
}

void updateNbSignalMap(linkaddr_t neighbor, sm_entry_t new_sm)
{
	uint8_t index = findNbSignalMapIndex(neighbor);
	if (index == INVALID_INDEX)
	{
		if (valid_nb_sm_entry_size >= NB_SM_SIZE)
		{
			log_error("Neighbor signal map is full.");
			return;
		}

		index = valid_nb_sm_entry_size;
		++valid_nb_sm_entry_size;

		nbSignalMap[index].neighbor = neighbor;
		nbSignalMap[index].entry_num = 0;
	}

	uint8_t entry_index = findNbSignalMapEntryIndex(index, new_sm.neighbor);
	if (entry_index == INVALID_INDEX)
	{
		if (nbSignalMap[index].entry_num >= NB_SIGNAL_MAP_SIZE)
		{
			log_error("Neighbor signal map entry is full.");
			return;
		}

		entry_index = nbSignalMap[index].entry_num;
		++(nbSignalMap[index].entry_num);

		nbSignalMap[index].nb_sm[entry_index].neighbor = new_sm.neighbor;

		if (new_sm.inbound_gain != INVALID_GAIN)
		{
			nbSignalMap[index].nb_sm[entry_index].inbound_gain = new_sm.inbound_gain;
		}

		if (new_sm.outbound_gain != INVALID_GAIN)
		{
			nbSignalMap[index].nb_sm[entry_index].outbound_gain = new_sm.outbound_gain;
		}
	}
	//process_post(&signalmap_process, signalmap_ready, NULL);
}


/************** radio power translation **************/

/* RF231 datasheet power: 3.0, 2.8, 2.3, 1.8, 1.3, 0.7, 0.0, -1, -2, -3, -4, -5, -7, -9, -12, -17 */
/* scale 10 times to integer dBm */
float powerLevelDBm[] = { 3.0, 2.8, 2.3, 1.8, 1.3, 0.7, 0.0, -1, -2, -3, -4, -5, -7, -9, -12, -17 };
float powerLevel2dBm(uint8_t power_level) {
	/* note that here the index of powre level is reversed */
	if (power_level <= RF231_TX_PWR_MIN && power_level >= RF231_TX_PWR_MAX)
	{
		return powerLevelDBm[power_level];
	}
	else if (power_level > RF231_TX_PWR_MIN)
	{
		return powerLevelDBm[RF231_TX_PWR_MIN];
	}
	else
	{
		return powerLevelDBm[RF231_TX_PWR_MAX];
	}
}

/* RF231 RF input power with RSSI value in the  valid range of 1 to 28: P_RF = RSSI_BASE_VAL + 3 * (RSSI - 1) [dBm] */
int rssi2dBm(uint8_t rssi)
{
	if (rssi == 0)
	{
		return -91;
	}
	else if (rssi >= 1 && rssi <= 28)
	{
		return -91 + (rssi - 1) * 3;
	}
	else
	{
		return INVALID_DBM;
	}
}

/* RF231 RF input power with ED value in the valid range of 0 to 84: P_RF = -91 + ED [dBm] */
int ed2dBm(uint8_t ed)
{
	if (ed <= 84)
	{
		return -91 + ed;
	}
	else
	{
		return INVALID_DBM;
	}
}

float ed2mW(uint8_t ed)
{
	float dbm = ed2dBm(ed);
	dbm = dbm / 10;
	return pow(10, dbm);
}

float mW2dBm(double mW)
{
	return log10(mW) * 10;
}


/**************** Signal Map Interface *********************/

float computeInboundGain(uint8_t tx_power_level, uint8_t tx_ed, uint8_t noise_ed) 
{
	float inbound_gain = powerLevel2dBm(tx_power_level) - mW2dBm(ed2mW(tx_ed) - ed2mW(noise_ed));
	return inbound_gain;
}

/* get the inbound gain of given sender */
float getInboundGain(linkaddr_t sender)
{
	float inbound_gain = INVALID_GAIN;
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i)
	{
		if (signalMap[i].neighbor == sender)
		{
			inbound_gain = signalMap[i].inbound_gain;
			break;
		}
	}	
	return inbound_gain;
}
/* get the outbound gain of given sender */
float getOutboundGain(linkaddr_t sender)
{
	float outbound_gain = INVALID_GAIN;
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i)
	{
		if (signalMap[i].neighbor == sender)
		{
			outbound_gain = signalMap[i].inbound_gain;
			break;
		}
	}	
	return outbound_gain;
}

static uint8_t is_sending_local_sm = 1;
/* add one sm segment info to one packet in the output buffer each time */
void prepareSMSegment(uint8_t *ptr)
{
	// check if there is anything to send 
	if (valid_sm_entry_size == 0)
	{
		return;
	}

	// alternatively send local signal map and neighbor signal map
	if (is_sending_local_sm == 1 || valid_nb_sm_entry_size == 0)
	{
		memcpy(ptr, &(signalMap[sm_sending_index].neighbor), sizeof(linkaddr_t));
		ptr += sizeof(linkaddr_t);
		memcpy(ptr, &(node_addr), sizeof(linkaddr_t));
		ptr += sizeof(linkaddr_t);
		memcpy(ptr, &(signalMap[sm_sending_index].inbound_gain), sizeof(float));
		ptr += sizeof(float);
		memcpy(ptr, &(signalMap[sm_sending_index].outbound_gain), sizeof(float));
		ptr += sizeof(float);	

		++sm_sending_index;
		// local signal map is not empty, so tail check is good enough
		if (sm_sending_index >= valid_sm_entry_size)
		{
			sm_sending_index = 0;
			is_sending_local_sm = 0;
			nb_sm_sending_index = 0;
			nb_sm_entry_sending_index = 0;
		}
		else
		{
			is_sending_local_sm = 1;
		}
	}
	else
	{
		// neighbor signal map is not empty, so tail check is good enough
		memcpy(ptr, &(nbSignalMap[nb_sm_sending_index].nb_sm[nb_sm_entry_sending_index].neighbor), sizeof(linkaddr_t));
		ptr += sizeof(linkaddr_t);
		memcpy(ptr, &(nbSignalMap[nb_sm_sending_index].neighbor), sizeof(linkaddr_t));
		ptr += sizeof(linkaddr_t);
		memcpy(ptr, &(nbSignalMap[nb_sm_sending_index].nb_sm[nb_sm_entry_sending_index].inbound_gain), sizeof(float));
		ptr += sizeof(float);
		memcpy(ptr, &(nbSignalMap[nb_sm_sending_index].nb_sm[nb_sm_entry_sending_index].outbound_gain), sizeof(float));
		ptr += sizeof(float);

		++nb_sm_entry_sending_index;
		// traverse the neighbor entry and find a not sent one
		while (nb_sm_entry_sending_index >= nbSignalMap[nb_sm_sending_index].entry_num)
		{
			// check the first entry of a new neighbor
			nb_sm_entry_sending_index = 0;
			++nb_sm_sending_index;

			// tail check the valid index
			if (nb_sm_sending_index >= valid_nb_sm_entry_size)
			{
				nb_sm_sending_index = 0;
				is_sending_local_sm = 1;
				sm_sending_index = 0;
				return;
			}
		}
	}
}

void sm_receive(uint8_t *ptr)
{
	sm_entry_t new_nb_sm;

	linkaddr_t neighbor, local;
	memcpy(&neighbor, ptr, sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);
	memcpy(&local, ptr, sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);	
	
	float inbound_gain, outbound_gain;
	memcpy(&inbound_gain, ptr, sizeof(float));
	ptr += sizeof(float);
	memcpy(&outbound_gain, ptr, sizeof(float));
	ptr += sizeof(float);

	//log_debug("sm: %u to %u", neighbor, local);
	// If related to node_addr, add to local signal map
	if (local == node_addr)
	{
		return;
	}
	else if (neighbor == node_addr)
	{
		updateOutboundGain(local, inbound_gain);
	}
	else
	{
		//Add to neighbor signal map
		uint8_t local_index = findNbSignalMapIndex(local);

		if (local_index != INVALID_INDEX)
		{
			new_nb_sm.neighbor = neighbor;
			new_nb_sm.inbound_gain = inbound_gain;
			new_nb_sm.outbound_gain = outbound_gain;
			updateNbSignalMap(local, new_nb_sm);
		}
		else
		{
			new_nb_sm.neighbor = local;
			new_nb_sm.inbound_gain = outbound_gain;
			new_nb_sm.outbound_gain = inbound_gain;
			updateNbSignalMap(neighbor, new_nb_sm);	
		}
	}
}

void signalMapInit()
{
	valid_sm_entry_size = 0;
	sm_sending_index = 0;
	valid_nb_sm_entry_size = 0;
	nb_sm_sending_index = 0;
	nb_sm_entry_sending_index = 0;
	is_sending_local_sm = 1;

	//process_start(&signalmap_process, NULL);
}

/* Process for signal map debug */
PROCESS_THREAD(signalmap_process, ev, data)
{
	PROCESS_BEGIN();
	while (1) 
	{
		PROCESS_WAIT_EVENT_UNTIL(ev == signalmap_ready);
	}
	PROCESS_END();
}