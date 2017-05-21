#ifndef __PRKMCS_H__
#define __PRKMCS_H__

/* receivers need to maintain signal maps for a list of senders
 * senders need to maintain a neighbor receiver list and conflict graph list
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "contiki.h"
#include "config.h"
#include "core/net/linkaddr.h"
#include "core/net/mac/mac.h"
#include "core/net/mac/prk-mcs/asn.h"
#include "core/net/mac/prk-mcs/controller.h"
#include "core/net/mac/prk-mcs/linkestimate.h"
#include "core/net/mac/prk-mcs/onama.h"
#include "core/net/mac/prk-mcs/protocolsignaling.h"
#include "core/net/mac/prk-mcs/signalmap.h"
#include "core/net/mac/prk-mcs/slotoperation.h"
#include "core/net/mac/prk-mcs/test.h"
#include "core/net/mac/prk-mcs/timesynch.h"
#include "core/net/mac/prk-mcs/topology.h"
#include "cpu/arm/stm32f103/rtimer-arch.h"

#define DATA_POWER RF231_TX_PWR_MIN

/* packet type definition */
enum
{
	DATA_PACKET       = 3,
	CONTROL_PACKET    = 5,
	TIME_SYNCH_BEACON = 6,
	TEST_PACKET       = 9,
};

/* the unit is us because the rtimer graduality is us */
enum
{	
	BUILD_SIGNALMAP_PERIOD = 5000,
	PRKMCS_TIMESLOT_LENGTH = 20000,
};

enum
{
	TIME_SYNCH_FREQUENCY      = 5,
	PRKMCS_MAX_PACKET_LENGTH  = 125,
	PRKMCS_MIN_PACKET_LENGTH  = 3,

	INVALID_INDEX             = 0xff, 
	INVALID_CHANNEL           = 0,
	INVALID_POWER_LEVEL       = 0xFF,
	INVALID_ED				  = 0xFF,

	ER_SEGMENT_LENGTH         = 4,
	SM_SEGMENT_LENGTH         = 3,

	MAX_ER_SEG_NUM = 30,

	CCA_MAX_BACK_OFF_TIME     = 1000,
	CCA_ED_THRESHOLD          = 10,
};

/*
Packet format
-----------------------------------------------------------------
| Type | Receiver | Sender | Seq No | # of ER Seg | ER Segments |
-1 byte---1 byte----1 byte---2 byte------1 byte -----------------
ER segment format
------------------------------------
| Link Index | ER Version | I Edge |
----1 byte-------2 byte-----1 byte--
SM segment format
---------------------------------------------------
| Neighbor | Inbound Gain | Outbound Gain |
---1 byte-----1 bytes---------1 bytes-----
*/

/* active link info defined in topology module*/
extern link_t activeLinks[];
extern uint8_t activeLinksSize;
extern linkaddr_t activeNodes[];
extern uint8_t activeNodesSize;
extern uint8_t localLinks[LOCAL_LINKS_SIZE];
extern uint8_t localLinksSize;

/* slot info */
extern uint8_t prkmcs_is_synchronized;
extern volatile rtimer_clock_t current_slot_start;
extern struct asn_t current_asn;
extern struct rtimer slot_operation_timer;

/* scheduling info */
extern linkaddr_t node_addr, pair_addr;
extern uint8_t is_receiver, my_link_index, prkmcs_is_synchronized;

/* channel selection info defined in onama module */
extern uint8_t data_channel, control_channel;

/* a ringbuf storing incoming packets */
extern uint8_t rf231_rx_buffer_head, rf231_rx_buffer_tail;
extern rx_frame_t rf231_rx_buffer[RF231_CONF_RX_BUFFERS];

/* channel info defined in prkmcs main module */
extern uint8_t channel_status[CHANNEL_NUM]; // record the channel state for scheduling

/* link er table */
extern link_er_t linkERTable[LINK_ER_TABLE_SIZE];
extern uint8_t link_er_size;
extern uint8_t er_sending_index;
extern uint8_t conflict_set_size[LOCAL_LINK_ER_TABLE_SIZE];

/* local link er info defined in controller module */
extern local_link_er_t localLinkERTable[LOCAL_LINK_ER_TABLE_SIZE];
extern uint8_t local_link_er_size;
extern uint8_t local_er_sending_index;

/* signal map info defined in signal map module */
extern sm_entry_t signalMap[SIGNAL_MAP_SIZE];
extern uint8_t valid_sm_entry_size;
extern uint8_t sm_sending_index;
extern nb_sm_t nbSignalMap[NB_SM_SIZE];
extern uint8_t valid_nb_sm_entry_size;

/* pdr info defined in linkestimate module*/
extern pdr_t pdrTable[PDR_TABLE_SIZE];
extern uint8_t pdr_table_size;

/* transmission power info defined in prkmcs main module */
extern float tx_power;

void prkmcsInit();
void prkmcs_send_ctrl();
void prkmcs_send_data();
void prkmcs_receive();
#endif /* __PRKMCS_H__ */
