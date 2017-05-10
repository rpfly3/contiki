#ifndef __SLOT_OPERATION_H__
#define __SLOT_OPERATION_H__

#include "contiki.h"
#include "core/net/mac/prk-mcs/asn.h"

enum {
    SLOT_LEN = ((uint32_t) 512) << 10,
    /* contention window size */
    MIN_CW = 2048,
    /* x % CW == x & CW_HEX_MODULAR */
    CW_HEX_MODULAR = 0x7FF,
    /* sample Tx probability every TX_PROB_SAMPLE_WINDOW slots */
    TX_PROB_SAMPLE_WINDOW = 100,
    
    /******* Subslot Related ******/
    DATA_SUBSLOT_LEN_MILLI = 24,
    DATA_SUBSLOT_LEN = ((uint32_t)DATA_SUBSLOT_LEN_MILLI << 10),
    COMM_SUBSLOT_FTSP_BEACON_CNT = 4,
    COMM_SUBSLOT_FTSP_BEACON_PERIOD_MILLI = 3,
    COMM_SUBSLOT_FTSP_BEACON_PERIOD = ((uint32_t)COMM_SUBSLOT_FTSP_BEACON_PERIOD_MILLI << 10),

    COMM_SUBSLOT_CTRL_BEACON_CNT = 10,
    COMM_SUBSLOT_CTRL_BEACON_PERIOD_MILLI = 24,
    COMM_SUBSLOT_CTRL_BEACON_PERIOD = ((uint32_t)COMM_SUBSLOT_CTRL_BEACON_PERIOD_MILLI << 10),

    CONTENTION_INTERVAL = 13,

    COMM_SUBSLOT_BEACON_CNT = COMM_SUBSLOT_FTSP_BEACON_CNT + COMM_SUBSLOT_CTRL_BEACON_CNT,
    COMM_SUBSLOT_LEN_MILLI = 324,
    COMM_SUBSLOT_LEN = ((uint32_t)COMM_SUBSLOT_LEN_MILLI << 10),

    CC2420_CONTROL_CHANNEL = 19,
	
	DEQUEUED_BUF_SIZE = 48,
	INPUT_BUF_SIZE = 48,
};

/*---------- External Variables ---------*/
void prkmcs_slot_operation_start();
void schedule_next_slot(struct rtimer *t);
void prkmcs_control_signaling(uint8_t node_index);
#endif /* __SLOT_OPERATION_H__ */
