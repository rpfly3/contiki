#ifndef __CONTROLLER_H__
#define __CONTROLLER_H__

#include "contiki.h"
#include "core/net/linkaddr.h"

enum
{
	PDR_REQUIREMENT = 90,
	E_0 = 4,
	LOCAL_LINK_ER_TABLE_SIZE = 40,
	LINK_ER_TABLE_SIZE = 150,
	INVALID_DBM = 0xff,
	INVALID_ER_VERSION = 0xffff,
};

/* local link ER table entry */
typedef struct {
    /* the other node's address of the link */
	linkaddr_t neighbor;
	/* is node_addr the sender ? */
	uint8_t is_sender;
    /* index within all active links, used for compute priority */
    uint8_t link_index;
	/* PDR requirement */
	uint8_t pdr_req;
    /* ER version of the receiver */
    uint16_t er_version;
	/* signal map index defining the ER boundary */
	uint8_t sm_index;
    /* use the I_edge (in ED) to indicate the ER boundary */
    uint8_t I_edge;
} local_link_er_t;


void initController();
// Called to initialize local ER after signal map is built
void initLocalLinkER(uint8_t local_link_er_index);
uint8_t findLocalLinkERTableIndex(uint8_t index);
bool prepareLocalERSegment(uint8_t *ptr);
void er_receive(uint8_t *ptr, linkaddr_t sender);
void updateER(uint8_t local_link_er_index, uint8_t pdr_index);
#endif /* __CONTROLLER_H__ */
