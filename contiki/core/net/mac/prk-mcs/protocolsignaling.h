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
	uint16_t er_version;
	uint8_t I_edge;
	/* each bit indicates the conflict relation with corresponding link in local er table */
	uint16_t primary;
	uint16_t secondary;
} link_er_t;


void protocolSignalingInit();
uint8_t findLinkERTableIndex(uint8_t index);
uint8_t findEmptyLinkERTableIndex();
bool prepareERSegment(uint8_t *ptr);
void updateLinkER(uint8_t link_index, uint16_t er_version, uint8_t I_edge);
void updateConflictGraphForERChange(uint8_t link_er_index);
void updateConflictGraphForLocalERChange(uint8_t local_link_er_index);
#endif /* __PROTOCOLSIGNALING_H__ */
