#ifndef __TOPOLOGY_H__
#define __TOPOLOGY_H__

#include "contiki.h"
#include "net/linkaddr.h"

typedef struct {
    linkaddr_t sender;
    linkaddr_t receiver;
} link_t;

enum
{
	INVALID_ADDR = 0xff,
	LOCAL_LINKS_SIZE = 40,
};

void topologyInit();
/* find the index of <sender, receiver> in active link table  */
uint8_t findLinkIndex(linkaddr_t sender, linkaddr_t receiver);
/* find the index of link <sender, receiver> in active link table */
uint8_t findLinkIndexForLocal(linkaddr_t sender, linkaddr_t receiver);
/* find and return the local index in local link table */
uint8_t findLocalIndex(uint8_t link_index);

#endif /* __TOPOLOGY_H__ */
