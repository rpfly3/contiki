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
uint8_t sm_sending_index;

/* neighbors' signal maps: used for get conflicting relations */
nb_sm_t nbSignalMap[NB_SM_SIZE];
uint8_t valid_nb_sm_entry_size;

bool isEqual(float x, float y)
{
	bool equality = false;
	if (fabs(x - y) < FLT_EPSILON)
	{
		equality = true;	
	}
	return equality;
}

/*************** Signal Map tabel Management ***************/
/* find the table index to the valid entry for a neighbor */
uint8_t findSignalMapIndex(linkaddr_t neighbor) 
{
	uint8_t index = INVALID_INDEX;
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i) 
	{
        if(signalMap[i].neighbor == neighbor) 
        {
            index = i;
	        break;
        }
    }
	return index;
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
	float updated_inbound_gain = tmp.inbound_gain;

	if (index > 0 && updated_inbound_gain < signalMap[index - 1].inbound_gain) 
	{
		for (i = index; i > 0; i--) 
		{
			signal = &signalMap[i - 1];
			if (updated_inbound_gain < signal->inbound_gain) 
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
	else if (index + 1 < valid_sm_entry_size && updated_inbound_gain > signalMap[index + 1].inbound_gain) 
	{
		for (i = index; i + 1 < valid_sm_entry_size; i++) 
		{
			signal = &signalMap[i + 1];
			if (updated_inbound_gain > signal->inbound_gain) 
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
}


/* update a signal map entry with given parameters: keep the closest SIGNAL_MAP_SIZE neighbors, 
 * Note that outbound_gain is assumed to be INVALID_GAIN.
*/
static void updateOutboundGain(linkaddr_t receiver, float outbound_gain) 
{
	sm_entry_t* signal;

	uint8_t index = findSignalMapIndex(receiver);

	if (index != INVALID_INDEX) 
	{
		signal = &signalMap[index];
		signal->outbound_gain = outbound_gain;
	} 
	else 
	{
		if (valid_sm_entry_size < SIGNAL_MAP_SIZE)
		{
			index = valid_sm_entry_size;
			++valid_sm_entry_size;

			signal = &signalMap[index];
			signal->neighbor = receiver;
			signal->outbound_gain = outbound_gain;
			signal->inbound_gain = INVALID_GAIN;
		}
		else
		{
			log_error("Signal map is full.");
		}
	} 
	return;
}

/* add entries to local signal map to build the inital signal map 
 * Note that inbound_gain is assumed to be INVALID_GAIN.
*/
void updateInboundGain(linkaddr_t sender, float inbound_gain)
{
	if (isEqual(inbound_gain, INVALID_GAIN))
	{
		log_debug("Inbound gain is invalid.");
	}
	else
	{
		sm_entry_t* signal;
		uint8_t index = findSignalMapIndex(sender);

		if (index != INVALID_INDEX) 
		{
			signal = &signalMap[index];
			if (!isEqual(signal->inbound_gain, INVALID_GAIN))
			{
				signal->inbound_gain = signal->inbound_gain * ALPHA + inbound_gain * (1 - ALPHA);
			}
			else
			{
				signal->inbound_gain = inbound_gain;
			}
   
			// Resort the signal map
			sortSignalMap(index);
		} 
		else 
		{
			if (valid_sm_entry_size < SIGNAL_MAP_SIZE)
			{
				index = valid_sm_entry_size;
				++valid_sm_entry_size;
				signal = &signalMap[index];

				signal->neighbor = sender;
				signal->inbound_gain = inbound_gain;
				signal->outbound_gain = INVALID_GAIN;
	
				// Resort the signal map
				sortSignalMap(index);
			}
			else
			{
				log_error("Signal map is full.");
			}
		} 
	}
	return;
}

/************************* neighbor signal map management **************************/

uint8_t findNbSignalMapIndex(linkaddr_t neighbor)
{
	uint8_t index = INVALID_INDEX;
	for (uint8_t i = 0; i < valid_nb_sm_entry_size; ++i)
	{
		if (nbSignalMap[i].neighbor == neighbor)
		{
			index = i;
			break;
		}
	}
	return index;
}

uint8_t findNbSignalMapEntryIndex(uint8_t nb_sm_index, linkaddr_t neighbor)
{
	uint8_t index = INVALID_INDEX;
	for (uint8_t i = 0; i < nbSignalMap[nb_sm_index].entry_num; ++i)
	{
		if (nbSignalMap[nb_sm_index].nb_sm[i].neighbor == neighbor)
		{
			index = i;
			break;
		}
	}
	return index;
}

void updateNbSignalMap(uint8_t nbSM_index, linkaddr_t neighbor, linkaddr_t nb_neighbor, float inbound_gain, float outbound_gain)
{
	uint8_t entry_index;
	if (nbSM_index == INVALID_INDEX)
	{
		if (valid_nb_sm_entry_size < NB_SM_SIZE)
		{
			nbSM_index = valid_nb_sm_entry_size;
			++valid_nb_sm_entry_size;

			nbSignalMap[nbSM_index].neighbor = neighbor;
			nbSignalMap[nbSM_index].entry_num = 0;

			entry_index = nbSignalMap[nbSM_index].entry_num;
			++(nbSignalMap[nbSM_index].entry_num);

			nbSignalMap[nbSM_index].nb_sm[entry_index].neighbor = nb_neighbor;
			nbSignalMap[nbSM_index].nb_sm[entry_index].inbound_gain = inbound_gain;
			nbSignalMap[nbSM_index].nb_sm[entry_index].outbound_gain = outbound_gain;
		}
		else
		{
			log_error("Neighbor signal map is full.");
		}

	}
	else
	{
		entry_index = findNbSignalMapEntryIndex(nbSM_index, nb_neighbor);
		if (entry_index == INVALID_INDEX)
		{
			if (nbSignalMap[nbSM_index].entry_num < NB_SIGNAL_MAP_SIZE)
			{
				entry_index = nbSignalMap[nbSM_index].entry_num;
				++(nbSignalMap[nbSM_index].entry_num);

				nbSignalMap[nbSM_index].nb_sm[entry_index].neighbor = nb_neighbor;
				nbSignalMap[nbSM_index].nb_sm[entry_index].inbound_gain = inbound_gain;
				nbSignalMap[nbSM_index].nb_sm[entry_index].outbound_gain = outbound_gain;
			}
			else
			{
				log_error("Neighbor signal map entry is full.");
			}

		}
		else
		{
			if (!isEqual(inbound_gain, INVALID_GAIN))
			{
				nbSignalMap[nbSM_index].nb_sm[entry_index].inbound_gain = inbound_gain;
			}
			else
			{
				// do nothing
			}

			if (!isEqual(outbound_gain, INVALID_GAIN))
			{
				nbSignalMap[nbSM_index].nb_sm[entry_index].outbound_gain = outbound_gain;
			}
			else
			{
				// do nothing
			}
		}
	}
}


/************** radio power translation **************/

/* RF231 datasheet power: 3.0, 2.8, 2.3, 1.8, 1.3, 0.7, 0.0, -1, -2, -3, -4, -5, -7, -9, -12, -17 */
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
float rssi2dBm(uint8_t rssi)
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
float ed2dBm(uint8_t ed)
{
	float dbm = INVALID_DBM;
	if (ed <= 84)
	{
		dbm = -91 + ed;
	}
	else
	{
		log_error("Invalid DBM");
	}
	return dbm;
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
/* get the outbound gain to a given receiver */
float getOutboundGain(linkaddr_t receiver)
{
	float outbound_gain = INVALID_GAIN;
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i)
	{
		if (signalMap[i].neighbor == receiver)
		{
			outbound_gain = signalMap[i].outbound_gain;
			break;
		}
	}	
	return outbound_gain;
}

float getNbInboundGain(linkaddr_t sender, linkaddr_t receiver)
{
	float inbound_gain = INVALID_GAIN;
	
	uint8_t index = findNbSignalMapIndex(receiver);
	if (index != INVALID_INDEX)
	{
		for (uint8_t i = 0; i < nbSignalMap[index].entry_num; ++i)
		{
			if (nbSignalMap[index].nb_sm[i].neighbor == sender)
			{
				inbound_gain = nbSignalMap[index].nb_sm[i].inbound_gain;
				break;
			}
			else
			{
				continue;
			}
		}
	}
	else
	{
		index = findNbSignalMapIndex(sender);
		if (index != INVALID_INDEX)
		{
			for (uint8_t i = 0; i < nbSignalMap[index].entry_num; ++i)
			{
				if (nbSignalMap[index].nb_sm[i].neighbor == receiver)
				{
					inbound_gain = nbSignalMap[index].nb_sm[i].outbound_gain;
					break;
				}
				else
				{
					continue;
				}
			}
		}
		else
		{
			// do nothing
		}
	}
	return inbound_gain;
}

float getNbOutboundGain(linkaddr_t sender, linkaddr_t receiver)
{
	float outbound_gain = INVALID_GAIN;

	uint8_t index = findNbSignalMapIndex(sender);
	if (index != INVALID_INDEX)
	{
		for (uint8_t i = 0; i < nbSignalMap[index].entry_num; ++i)
		{
			if (nbSignalMap[index].nb_sm[i].neighbor == receiver)
			{
				outbound_gain = nbSignalMap[index].nb_sm[i].outbound_gain;
				break;
			}
			else
			{
				continue;
			}
		}
	}
	else
	{
		index = findNbSignalMapIndex(receiver);
		if (index != INVALID_INDEX)
		{
			for (uint8_t i = 0; i < nbSignalMap[index].entry_num; ++i)
			{
				if (nbSignalMap[index].nb_sm[i].neighbor == sender)
				{
					outbound_gain = nbSignalMap[index].nb_sm[i].inbound_gain;
					break;
				}
				else
				{
					continue;
				}
			}
		}
		else
		{
			// do nothing	
		}
	}
	return outbound_gain;
}

/* prepare sm info for sending */
void prepareSMSegment(uint8_t *ptr)
{
	memcpy(ptr, &(signalMap[sm_sending_index].neighbor), sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);
	memcpy(ptr, &(signalMap[sm_sending_index].inbound_gain), sizeof(float));
	ptr += sizeof(float);
	memcpy(ptr, &(signalMap[sm_sending_index].outbound_gain), sizeof(float));
	ptr += sizeof(float);	

	++sm_sending_index;
	return;
}

void sm_receive(uint8_t *ptr, linkaddr_t sender)
{
	linkaddr_t neighbor;
	memcpy(&neighbor, ptr, sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);
	float inbound_gain, outbound_gain;
	memcpy(&inbound_gain, ptr, sizeof(float));
	ptr += sizeof(float);
	memcpy(&outbound_gain, ptr, sizeof(float));
	ptr += sizeof(float);

	//log_debug("sm: %u to %u", neighbor, local);
	if (neighbor != node_addr)
	{
		//Add to neighbor signal map
		uint8_t nbSM_index = findNbSignalMapIndex(sender);

		if (nbSM_index != INVALID_INDEX)
		{
			updateNbSignalMap(nbSM_index, sender, neighbor, inbound_gain, outbound_gain);
		}
		else
		{
			nbSM_index = findNbSignalMapIndex(neighbor);
			updateNbSignalMap(nbSM_index, neighbor, sender, outbound_gain, inbound_gain);	
		}
	}
	else
	{
		// If related to node_addr, add to local signal map
		if (!isEqual(inbound_gain, INVALID_GAIN))
		{
			updateOutboundGain(sender, inbound_gain);
		}
		else
		{
			// do nothing
		}
	}
	return;
}

void signalMapInit()
{
	valid_sm_entry_size = 0;
	sm_sending_index = 0;
	valid_nb_sm_entry_size = 0;
}
