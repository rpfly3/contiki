#ifndef __SIGNALMAP_H__
#define __SIGNALMAP_H__

#include "contiki.h"
#include "core/net/linkaddr.h"

enum {
	SIGNAL_MAP_SIZE = 75,
	NB_SIGNAL_MAP_SIZE = 75,
	NB_SM_SIZE = 75,
	INVALID_GAIN = 255,
};

/* signal map for the node itself, and note that the unit of gain is db */
typedef struct
{
    /* node address where the signal comes from */
    linkaddr_t neighbor;
    /* used to compute ER */
	uint8_t inbound_ed;
    /* used to determine if the node in the ER of the neighbors */
    uint8_t outbound_ed;
} sm_entry_t;

/* store neighbor's signal map */
typedef struct
{
	linkaddr_t neighbor;
	uint8_t entry_num;
	sm_entry_t nb_sm[NB_SIGNAL_MAP_SIZE];
} nb_sm_t;

/* initialize the signal map */
void signalMapInit();
/* find the index to the valid entry of a neighbor in signalMap table*/
uint8_t findSignalMapIndex(linkaddr_t neighbor);
/* find the total number of valid entries in the signalMap table */
uint8_t getSignalMapValidEntrySize();

/* add entries to local signal map to build the inital signal map */
void updateInboundED(linkaddr_t sender, uint8_t inbound_ed);

/* get the transmission power with given power level */
float powerLevel2dBm(uint8_t power_level);
float ed2dBm(uint8_t ed);
float dbm2mW(float dbm);
float mW2dBm(float mW);
/* get the inbound of given sender */
uint8_t getInboundED(linkaddr_t sender);
/* get the outbound of given sender */
uint8_t getOutboundED(linkaddr_t sender);
/* get the inbound of given receiver from given sender in neighbor signal map */
uint8_t getNbInboundED(linkaddr_t sender, linkaddr_t receiver);
/* get the outbound of given sender to given sender in neighbor signal map */
uint8_t getNbOutboundED(linkaddr_t sender, linkaddr_t receiver);
float computeInboundGain(uint8_t tx_power_level, uint8_t tx_ed, uint8_t noise_ed);
void prepareSMSegment(uint8_t *ptr);
void sm_receive(uint8_t *ptr, linkaddr_t sender);

#endif /* __SIGNALMAP_H__ */
