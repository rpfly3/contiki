/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: estimate link quality and generate PDR; wire data plane through link estimation.
 * Note that the data packets are sent and received in this module !!!
 */

#include "core/net/mac/prk-mcs/prkmcs.h"

pdr_t pdrTable[PDR_TABLE_SIZE]; //keep PDR info of neighbor senders
uint8_t pdr_table_size;


/**************** PDR Table Management ****************/

/* initialize the neighbor table */
void linkestimateInit() {
	pdr_table_size = 0;
	for (uint8_t i = 0; i < localLinksSize; ++i)
	{
		// Trap the invalid PDR table access
		if (pdr_table_size >= PDR_TABLE_SIZE)
		{
			do
			{
				log_error("The PDR table is full.");
			} while (1);
		}
		else
		{
			if (activeLinks[localLinks[i]].receiver == node_addr)
			{
				// Initialize PDR table entry
				pdrTable[pdr_table_size].sender = activeLinks[localLinks[i]].sender;
				pdrTable[pdr_table_size].sequence_num = 0;
				pdrTable[pdr_table_size].next_update_sequence = PDR_COMPUTE_WINDOW_SIZE;
				pdrTable[pdr_table_size].received_pkt = 0;
				pdrTable[pdr_table_size].sent_pkt = 0;
				pdrTable[pdr_table_size].pdr = INVALID_PDR;
				pdrTable[pdr_table_size].pdr_sample = INVALID_PDR;
				pdrTable[pdr_table_size].nb_I = 0;
				++pdr_table_size;
			}
			else
			{
				// do nothing
			}
		}
	}
}

/* find the index to the entry for neighbor */
uint8_t findPDRTableIndex(linkaddr_t sender) 
{
	uint8_t pdr_index = INVALID_INDEX;
    for(uint8_t i = 0; i < pdr_table_size; i++) 
    {
	    if (pdrTable[i].sender == sender) 
	    {
		    pdr_index = i;
		    break;
        }
    }
    return pdr_index;
}

/************************ Link Quality Updating *************************/
void updateLinkQuality(linkaddr_t sender, uint16_t sequence_num, uint8_t rx_ed) 
{
    uint8_t pdr_index = findPDRTableIndex(sender);
    if(pdr_index != INVALID_INDEX) 
    {
	    if (sequence_num > pdrTable[pdr_index].sequence_num)
	    {
		    // update stastics
		    pdrTable[pdr_index].sent_pkt += (sequence_num - pdrTable[pdr_index].sequence_num);
		    pdrTable[pdr_index].received_pkt += 1;
		    pdrTable[pdr_index].sequence_num = sequence_num;
		    float rx_IdBm = ed2dBm(rx_ed);
		    uint8_t inbound_ed = getInboundED(sender);
		    float rx_dBm = ed2dBm(inbound_ed);
		    float nb_ImW = dbm2mW(rx_IdBm) - dbm2mW(rx_dBm);
		    if (nb_ImW <= 0)
		    {
			    nb_ImW = dbm2mW(-97);
		    }
		    else
		    {
			    // do nothing
		    }
		    pdrTable[pdr_index].nb_I += nb_ImW;

			// check if it's time to update PDR
		    if (sequence_num >= pdrTable[pdr_index].next_update_sequence)
		    {
		    	// update pdr info
			    pdrTable[pdr_index].pdr_sample = pdrTable[pdr_index].received_pkt * 100 / pdrTable[pdr_index].sent_pkt;
			    pdrTable[pdr_index].pdr = (pdrTable[pdr_index].pdr != INVALID_PDR) ? (ALPHA * pdrTable[pdr_index].pdr + (1 - ALPHA) * pdrTable[pdr_index].pdr_sample) : pdrTable[pdr_index].pdr_sample;
			    pdrTable[pdr_index].nb_I = (pdrTable[pdr_index].nb_I / pdrTable[pdr_index].received_pkt);

			    // find the local_link_er_index and update corresponding local er table entry
			    for (uint8_t i = 0; i < local_link_er_size; ++i)
			    {
				    if ((localLinkERTable[i].neighbor == sender) && (!localLinkERTable[i].is_sender))
				    {
					    printf("Link %u PDR %u\r\n", localLinkERTable[i].link_index, pdrTable[pdr_index].pdr);
					    updateER(i, pdr_index);
					    break;
				    }
				    else
				    {
					    continue;
				    }
			    }
			    pdrTable[pdr_index].sent_pkt = 0;
			    pdrTable[pdr_index].received_pkt = 0;
			    pdrTable[pdr_index].nb_I = 0;
			    pdrTable[pdr_index].next_update_sequence = sequence_num + PDR_COMPUTE_WINDOW_SIZE;
		    }
		    else
		    {
				// do nothing
		    }
	    }
	    else
	    {
		    log_error("Packet with smaller sequence number");
	    }
	} 
	else 
	{
		log_error("Cannot find the PDR table index.");
    }
	
	return;
}
