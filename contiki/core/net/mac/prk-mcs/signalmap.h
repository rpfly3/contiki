#ifndef __SIGNALMAP_H__
#define __SIGNALMAP_H__

#include "contiki.h"
#include "core/net/linkaddr.h"

#define FLT_EPSILON 0.01
enum {
	SIGNAL_MAP_SIZE = 127,
	NB_SIGNAL_MAP_SIZE = 45,
	NB_SM_SIZE = 127,
	INVALID_GAIN = 0,
};

/* signal map for the node itself, and note that the unit of gain is db */
typedef struct
{
    /* node address where the signal comes from */
    linkaddr_t neighbor;
    /* used to compute ER */
    float inbound_gain;
    /* used to determine if the node in the ER of the neighbors */
    float outbound_gain;
} sm_entry_t;

/* store neighbor's signal map */
typedef struct
{
	linkaddr_t neighbor;
	uint8_t entry_num;
	sm_entry_t nb_sm[NB_SIGNAL_MAP_SIZE];
} nb_sm_t;

bool isEqual(float x, float y);

/* initialize the signal map */
void signalMapInit();
/* find the index to the valid entry of a neighbor in signalMap table*/
uint8_t findSignalMapIndex(linkaddr_t neighbor);
/* find the total number of valid entries in the signalMap table */
uint8_t getSignalMapValidEntrySize();

/* add entries to local signal map to build the inital signal map */
void updateInboundGain(linkaddr_t sender, float inbound_gain);

/* get the transmission power with given power level */
float powerLevel2dBm(uint8_t power_level);

/* get the inbound of given sender */
float getInboundGain(linkaddr_t sender);
/* get the outbound of given sender */
float getOutboundGain(linkaddr_t sender);
/* get the inbound of given receiver from given sender in neighbor signal map */
float getNbInboundGain(linkaddr_t sender, linkaddr_t receiver);
/* get the outbound of given sender to given sender in neighbor signal map */
float getNbOutboundGain(linkaddr_t sender, linkaddr_t receiver);
float computeInboundGain(uint8_t tx_power_level, uint8_t tx_ed, uint8_t noise_ed);
void prepareSMSegment(uint8_t *ptr);
void sm_receive(uint8_t *ptr);

#endif /* __SIGNALMAP_H__ */
