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
uint8_t valid_sm_entry_size = 0;
uint8_t sm_sending_index = 0;

/* neighbors' signal maps: used for get conflicting relations */
nb_sm_t nbSignalMap[NB_SM_SIZE];
uint8_t valid_nb_sm_entry_size = 0;


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

/* sort Signal Map in ascending order of inbound gain after updating */
static void sortSignalMap(uint8_t index) 
{
	uint8_t i;
	sm_entry_t* signal;

	sm_entry_t tmp = signalMap[index];
	int8_t updated_inbound_ed = tmp.inbound_ed;

	if (index > 0 && updated_inbound_ed > signalMap[index - 1].inbound_ed) 
	{
		for (i = index; i > 0; --i) 
		{
			signal = &signalMap[i - 1];
			if (updated_inbound_ed > signal->inbound_ed) 
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
	else if (index + 1 < valid_sm_entry_size && updated_inbound_ed < signalMap[index + 1].inbound_ed) 
	{
		for (i = index; i + 1 < valid_sm_entry_size; ++i) 
		{
			signal = &signalMap[i + 1];
			if (updated_inbound_ed < signal->inbound_ed) 
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


/* Note that outbound_ed is assumed to be INVALID_ED. */
static void updateOutboundED(linkaddr_t receiver, int8_t outbound_ed) 
{
	sm_entry_t* signal;

	uint8_t index = findSignalMapIndex(receiver);

	if (index != INVALID_INDEX) 
	{
		signal = &signalMap[index];
		signal->outbound_ed = outbound_ed;
	} 
	else 
	{
		if (valid_sm_entry_size < SIGNAL_MAP_SIZE)
		{
			index = valid_sm_entry_size;
			++valid_sm_entry_size;

			signal = &signalMap[index];
			signal->neighbor = receiver;
			signal->outbound_ed = outbound_ed;
			signal->inbound_ed = INVALID_ED;
		}
		else
		{
			log_error("Signal map is full.");
		}
	} 
	return;
}

/* add entries to local signal map to build the inital signal map */
void updateInboundED(linkaddr_t sender, int8_t inbound_ed)
{
	sm_entry_t* signal;
	uint8_t sm_index = findSignalMapIndex(sender);

	if (sm_index != INVALID_INDEX) 
	{
		signal = &signalMap[sm_index];
		if (signal->inbound_ed != INVALID_ED)
		{
			signal->inbound_ed = signal->inbound_ed * (1 - ALPHA) + inbound_ed * ALPHA;
		}
		else
		{
			signal->inbound_ed = inbound_ed;
		} 
	 	// Resort the signal map
		sortSignalMap(sm_index);
	} 
	else 
	{
		if (valid_sm_entry_size < SIGNAL_MAP_SIZE)
		{
			sm_index = valid_sm_entry_size;
			++valid_sm_entry_size;
			signal = &signalMap[sm_index];

			signal->neighbor = sender;

			signal->inbound_ed = inbound_ed;
			signal->outbound_ed = INVALID_ED;
	
			// Resort the signal map
			sortSignalMap(sm_index);
		}
		else
		{
			log_error("Signal map is full.");
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

void updateNbSignalMap(uint8_t nbSM_index, linkaddr_t neighbor, linkaddr_t nb_neighbor, int8_t inbound_ed, int8_t outbound_ed)
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
			nbSignalMap[nbSM_index].entry_num += 1;

			nbSignalMap[nbSM_index].nb_sm[entry_index].neighbor = nb_neighbor;
			nbSignalMap[nbSM_index].nb_sm[entry_index].inbound_ed = inbound_ed;
			nbSignalMap[nbSM_index].nb_sm[entry_index].outbound_ed = outbound_ed;
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
				nbSignalMap[nbSM_index].entry_num += 1;

				nbSignalMap[nbSM_index].nb_sm[entry_index].neighbor = nb_neighbor;
				nbSignalMap[nbSM_index].nb_sm[entry_index].inbound_ed = inbound_ed;
				nbSignalMap[nbSM_index].nb_sm[entry_index].outbound_ed = outbound_ed;
			}
			else
			{
				log_error("Neighbor signal map entry is full.");
			}

		}
		else
		{
			if (inbound_ed != INVALID_ED)
			{
				nbSignalMap[nbSM_index].nb_sm[entry_index].inbound_ed = inbound_ed;
			}
			else
			{
				// do nothing
			}

			if (outbound_ed != INVALID_ED)
			{
				nbSignalMap[nbSM_index].nb_sm[entry_index].outbound_ed = outbound_ed;
			}
			else
			{
				// do nothing
			}
		}
	}
}


/************** radio power translation **************/
//float is large enough for our computation: 1.175494351e-38 to 3.40282347e+38
/* RF231 datasheet power: 3.0, 2.8, 2.3, 1.8, 1.3, 0.7, 0.0, -1, -2, -3, -4, -5, -7, -9, -12, -17 */
float powerLevelDBm[] = { 3.0, 2.8, 2.3, 1.8, 1.3, 0.7, 0.0, -1, -2, -3, -4, -5, -7, -9, -12, -17 };
float powerLevel2dBm(uint8_t power_level) 
{
	float dBm = INVALID_DBM;
	/* note that here the index of powre level is reversed */
	if (power_level <= RF231_TX_PWR_MIN && power_level >= RF231_TX_PWR_MAX)
	{
		dBm = powerLevelDBm[power_level];
	}
	else if (power_level > RF231_TX_PWR_MIN)
	{
		dBm = powerLevelDBm[RF231_TX_PWR_MIN];
	}
	else
	{
		dBm = powerLevelDBm[RF231_TX_PWR_MAX];
	}

	return dBm;
}


/* RF231 RF input power with ED value in the valid range of 0 to 84: P_RF = -91 + ED [dBm] */
float ed2dBm(int8_t ed)
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

float dbm2mW(float dbm)
{
	dbm = dbm / 10;
	return pow(10, dbm);
}

float mW2dBm(float mW)
{
	return log10(mW) * 10;
}


/**************** Signal Map Interface *********************/

float computeInboundGain(uint8_t tx_power_level, int8_t tx_ed, int8_t noise_ed) 
{
	float tx_dbm, noise_dbm = -97;
	tx_dbm = ed2dBm(tx_ed);

	float inbound_gain = powerLevel2dBm(tx_power_level) - mW2dBm(dbm2mW(tx_dbm) - dbm2mW(noise_dbm));
	return inbound_gain;
}

/* get the inbound gain of given sender */
int8_t getInboundED(linkaddr_t sender)
{
	int8_t inbound_ed = INVALID_ED;
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i)
	{
		if (signalMap[i].neighbor == sender)
		{
			inbound_ed = signalMap[i].inbound_ed;
			break;
		}
	}	
	return inbound_ed;
}
/* get the outbound gain to a given receiver */
int8_t getOutboundED(linkaddr_t receiver)
{
	int8_t outbound_ed = INVALID_ED;
	for (uint8_t i = 0; i < valid_sm_entry_size; ++i)
	{
		if (signalMap[i].neighbor == receiver)
		{
			outbound_ed = signalMap[i].outbound_ed;
			break;
		}
	}	
	return outbound_ed;
}

int8_t getNbInboundED(linkaddr_t sender, linkaddr_t receiver)
{
	int8_t inbound_ed = INVALID_ED;
	
	uint8_t index = findNbSignalMapIndex(receiver);
	if (index != INVALID_INDEX)
	{
		for (uint8_t i = 0; i < nbSignalMap[index].entry_num; ++i)
		{
			if (nbSignalMap[index].nb_sm[i].neighbor == sender)
			{
				inbound_ed = nbSignalMap[index].nb_sm[i].inbound_ed;
				break;
			}
			else
			{
				// do nothing
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
					inbound_ed = nbSignalMap[index].nb_sm[i].outbound_ed;
					break;
				}
				else
				{
					// do nothing
				}
			}
		}
		else
		{
			// do nothing
		}
	}
	return inbound_ed;
}

int8_t getNbOutboundED(linkaddr_t sender, linkaddr_t receiver)
{
	int8_t outbound_ed = INVALID_ED;

	uint8_t index = findNbSignalMapIndex(sender);
	if (index != INVALID_INDEX)
	{
		for (uint8_t i = 0; i < nbSignalMap[index].entry_num; ++i)
		{
			if (nbSignalMap[index].nb_sm[i].neighbor == receiver)
			{
				outbound_ed = nbSignalMap[index].nb_sm[i].outbound_ed;
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
					outbound_ed = nbSignalMap[index].nb_sm[i].inbound_ed;
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
	return outbound_ed;
}

/* prepare sm info for sending */
void prepareSMSegment(uint8_t *ptr)
{
	memcpy(ptr, &(signalMap[sm_sending_index].neighbor), sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);
	memcpy(ptr, &(signalMap[sm_sending_index].inbound_ed), sizeof(int8_t));
	ptr += sizeof(int8_t);
	memcpy(ptr, &(signalMap[sm_sending_index].outbound_ed), sizeof(int8_t));
	ptr += sizeof(int8_t);	

	++sm_sending_index;
	return;
}

void sm_receive(uint8_t *ptr, linkaddr_t sender)
{
	linkaddr_t neighbor;
	memcpy(&neighbor, ptr, sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);
	int8_t inbound_ed, outbound_ed;
	memcpy(&inbound_ed, ptr, sizeof(int8_t));
	ptr += sizeof(int8_t);
	memcpy(&outbound_ed, ptr, sizeof(int8_t));
	ptr += sizeof(int8_t);

	if (neighbor != node_addr)
	{
		//Add to neighbor signal map
		uint8_t nbSM_index = findNbSignalMapIndex(sender);

		if (nbSM_index != INVALID_INDEX)
		{
			updateNbSignalMap(nbSM_index, sender, neighbor, inbound_ed, outbound_ed);
		}
		else
		{
			nbSM_index = findNbSignalMapIndex(neighbor);
			updateNbSignalMap(nbSM_index, neighbor, sender, outbound_ed, inbound_ed);	
		}
	}
	else
	{
		// If related to node_addr, add to local signal map
		if (inbound_ed != INVALID_ED)
		{
			updateOutboundED(sender, inbound_ed);
		}
		else
		{
			// do nothing
		}
	}
	return;
}