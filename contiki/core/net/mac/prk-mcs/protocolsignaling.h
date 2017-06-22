#ifndef __PROTOCOLSIGNALING_H__
#define __PROTOCOLSIGNALING_H__

#include "contiki.h"

typedef struct {
    /* node address */
	linkaddr_t sender;
	linkaddr_t receiver;
	/* index within all active links, used to compute priority */
	uint8_t link_index;
	/* ER version of the receiver */
	uint8_t er_version;
	int8_t I_edge;
	/* each bit indicates the conflict relation with corresponding link in local er table */
	uint16_t primary;
	uint16_t secondary;

	/* freeze the conflict set during OLAMA: consecutive (ONAMA_CONVERGENCE_TIME/ 8) slots share the same conflict set */
	uint8_t conflict;
	/* each 4 bits store the current decision of the link's activation in a furture slot */
	uint8_t active_bitmap[ONAMA_CONVERGENCE_TIME >> 1];
} link_er_t;

void protocolSignalingInit();
uint8_t findLinkERTableIndex(uint8_t link_index);
bool prepareERSegment(uint8_t *ptr);
void updateLinkER(uint8_t link_index, uint8_t er_version, int8_t I_edge);
void updateConflictGraphForERChange(uint8_t link_er_index);
void updateConflictGraphForLocalERChange(uint8_t local_link_er_index);
#endif /* __PROTOCOLSIGNALING_H__ */
