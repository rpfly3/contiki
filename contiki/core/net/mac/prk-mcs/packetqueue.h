#ifndef __PACKET_QUEUE_H__
#define __PACKET_QUEUE_H__

#include "contiki.h"
#include "core/net/linkaddr.h"
#define QUEUEBUF_NUM 100
#define NEIGHBOR_QUEUE_SIZE 100

/* PRK-MCS packet info */
struct prkmcs_packet {
	struct queuebuf *qb; // pointer to the queuebuf to be sent
	mac_callback_t sent; // callback of the packet
	void *ptr; // callback parameters
	uint8_t transmissions; // num of transmissions performed for the packet
	uint8_t ret; // status -- MAC return code
	uint8_t header_len; // length of header
};

/* PRK-MCS neighbor info */
struct prkmcs_neighbor {
    struct prkmcs_neighbor *next; // neighbors are stored as a list
    linkaddr_t addr; // address of the neighbor
    uint8_t is_broadcast; // check if it's a virtual neighbor
    struct prkmcs_packet *tx_array[NEIGHBOR_QUEUE_SIZE]; // ringbuf containing pointers to packets
    struct ringbufindex tx_ringbuf; // index of current packet
};

#endif
