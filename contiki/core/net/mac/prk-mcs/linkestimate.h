#ifndef __LINK_ESTIMATOR_H__
#define __LINK_ESTIMATOR_H__

#include "contiki.h"
#include "net/linkaddr.h"

#define ALPHA 0.8
enum {
	PDR_TABLE_SIZE = 40,
    INVALID_PDR = 0xff,
    /* number of packets to wait before computing a new DLQ (Data-driver Link Quality */
    PDR_COMPUTE_WINDOW_SIZE = 20,
};


/* link info type */
typedef struct {
    /* link layer address of the neighbor */
    linkaddr_t sender;
    /* last data sequence number received from this neighbor */
    uint16_t sequence_num;
	uint16_t next_update_sequence;
	/* packet stastics in a pdr compute window */
	uint8_t received_pkt;
	uint8_t sent_pkt;
    /* inbound qualities in the range [1 ... 100]: 1 bad, 100 good */
    uint8_t pdr;
    /* Y_{S, R}(t) for inbound DATA for the latest ER, range [0 ... 100] */
    uint8_t pdr_sample;
	/* average neighbor interference (mW)*/
	float nb_I;
} pdr_t;

uint8_t findPDRTableIndex(linkaddr_t sender);
void linkestimateInit();
void updateLinkQuality(linkaddr_t sender, uint16_t sequence_num, uint8_t rx_ed);
#endif /* __LINK_ESTIMATOR_H__ */
