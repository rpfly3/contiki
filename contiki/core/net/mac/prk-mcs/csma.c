/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: CSMA scheduling in control plane
 */

#include "contiki.h"
#include "core/net/linkaddr.h"
#include "core/lib/list.h"
#include "csma.h"

/* for future extension, still use the following two data structures 
 * defined in standard CSMA.
 */

/* packet metadata */
struct qbuf_metadata {
    mac_callback_t sent;
    void *cptr;
    uint8_t max_transmissions;
};

/* every neighbor has its own packet queue */
struct neighbro_queue {
    struct neighbor_queue *next;
    linkaddr_t addr;
    struct ctimer transmit_timer;
    uint8_t transmissions;
    uint8_t collisions, deferrals;
    LIST_STRUCT(queued_packet_list);
};

/* the maximum number of pending packet per neighbor */
#ifdef CSMA_CONF_MAX_PACKET_PER_NEIGHBOR
#define CSMA_MAX_PACKET_PER_NEIGHBOR CSMA_CONF_MAX_PACKET_PER_NEIGHBOR
#else
#define CSMA_MAX_PACKET_PER_NEIGHBOR MAX_QUEUED_PACKETS
#endif /* CSMA_CONF_MAX_PACKET_PER_NEIGHBOR */

#define MAX_QUEUED_PACKETS QUEUEBUF_NUM

/* initialize CSMA module */
void csmaInit() {
    /* initialize the related memory */
    memb_init();
}

/* for future extension, we use the default time base function in standard CSMA */
static clock_time_t defaultTimeBase() {
    clock_time_t time;

    /* The retransmission time must be proportional to the channel
     * check interval of the underlying radio duty cycling layer. 
     */
    time = NETSTACK_RDC.channel_check_interval();

    /* If the radio duty cycle has no channel check interval (i.e., it
     * does not turn the radio off), we make the retransmission time 
     * proportional to the configured MAC channel check rate.
     */
    if(time == 0) {
        //Todo: define the NETSTACK_RDC_CHANNEL_CHECK_RATE
        time = CLOCK_SECOND / NETSTACK_RDC_CHANNEL_CHECK_RATE;
    }

    return time;
}

static void transmitPacketList(void *ptr) {
    struct neighbor_queue *n = ptr;
    if(n) {
        struct rdc_buf_list *q = list_head(n->queued_packet_list);
        if(q != NULL) {
            /* send packets in the neighbor's list */
            NETSTACK_RDC.send_list(packetSent, n, q);
        }
    }
}

static void freePacket(struct neighbor_queue *n, struct rdc_buf_list *p) {
    if(p != NULL) {
        /* remove packet from list and deallocate */
        list_remove(n->queued_packet_list, p);

        queuebuf_free(p->buf);
        memb_free(&metadata_memb, p->ptr);
        memb_free(&packet_memb, p);

        if(list_head(n->queued_packet_list) != NULL) {
            /* there exists a next packet. we reset current tx info */
            n->transmissions = 0;
            n->collisions = 0;
            n->deferrals = 0;
            /* set a timer for next transmissions */
            ctimer_set(&n->transmit_timer, defaultTimeBase(), transmit_packet_list, n);
        } else {
            /* this was the last packet in the queue, we free the neighbor */
            ctimer_stop(&n->transmit_timer);
            list_remove(neighbor_list, n);
            memb_free(&neighbor_memb, n);
        }
    }
}

/* check the CSMA scheduling result:success or defer, and adjust the parameters accordingly */
void packetSent(uint8_t status, int num_transmissions) {

    uint8_t backoff_exponent;
    uint8_t backoff_transmission;

    if(status == MAC_TX_OK) {
        n->transmissions += num_transmissions;
    } else if(status == MAC_TX_DEFERRED) {

        n->deferrals += num_transmissions;

        time = default_timebase();

        /* set the backoff */
        if(backoff_exponnet > CSMA_MAX_BACKOFF_EXPONENT) {
            backoff_exponent = CSMA_MAX_BACKOFF_EXPONENT;
        }

        backoff_transmissions = 1 << backoff_exponent;

        /* it cannot exceed the slot bound */
        //Todo: check the time against the slot bound

        /* pick a time for next transmission, within the interval:
         * [time, time + 2^backoff_exponent * time] */
        time = time + (random_rand() % (backoff_transmissions * time));

        if(n-> transmissions < max_transmissions) {
            /* set the back off timer */
            ctimer_set(&n->transmit_timer, time, sendPacket, n);
        }
    }
}

/* prepare packet and put it in the according buffer */
void sendPacket(mac_callback_t sent, void *ptr) {
    struct rdc_buf_list *q;
    struct neighbor_queue *n;
    static uint8_t initialized = 0;
    static uint16_t seqno;
    const linkaddr_t *addr = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);

    if(!initialized) {
        initialized = 1;
        /* initialize the sequence number to a random value as per 802.15.4 */
        seqno = random_rand();
    } 

    if(seqno == 0) {
        /* PACKETBUF_ATTR_MAC_SEQNO cannot be zero, due to the percuilarity
         * in framer-802154.c */
        seqno++;
    }

    packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, seqno++);

    /* look for the neighbor entry */
    n = neighbor_queue_from_addr(addr);
    if(n == NULL) {
        /* allocate a new neighbor entry */
        n = memb_alloc(&neighbor_memb);
        if(n != NULL) {
            /* init neighbor entry */
            linkaddr_copy(&n->addr, addr);
            n->transmissions = 0;
            n->collisions = 0;
            n->deferrals = 0;

            /* init packet list for this neighbor */
            LIST_STRUCT_INIT(n, queued_packet_list);
            /* add neighbor to the list */
            list_add(neighbor_list, n); 
        }
    }

    if(n != NULL) {
        /* add packet to the neighbor's queue */
        if(list_length(n->queued_packet_list) < CSMA_MAX_PACKET_PERNEIGHBOR) {
            q = memb_alloc(&packet_memb);
            if(q != NULL) {
                q->ptr = memb_alloc(&metadata_memb);
                if(q->ptr != NULL) {
                    q->buf = queuebuf_new_from_packetbuf();
                    if(q->buf != NULL) {
                        struct qbuf_metadata *metadata = (struct qbuf_metadata *)q->ptr;
                        /* neighbor and packet successfully allocated */
                        if(packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS) == 0) {
                            metata->max_transmissions = CSMA_MAXMAC_TRANSMISSIONS;
                        } else {
                            metata->max_transmissions = packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSONS);
                        }
                        metadata->sent = sent;
                        metadata->cptr = ptr;

                        /* set the packet type attribute */
                        //Todo: check how to set packet type attribute 
                        if(pacektbuf_attr(PACKETBUF_ATTR_PACKET_TYPE) == PACKETBUF_ATTR_PACKET_TYPE_ACK) {
                            list_push(n->queued_packet_list, q);
                        } else {
                            list_add(n->queued_packet_list, q);
                        }

                        /* if q is the first packet in the neighbor's queue, send ASAP */
                        if(list_head(n->queued_packet_list) == q) {
                            ctimer_set(n&->transmit_timer, 0, transmit_packet_list, n);
                        }
                        return;
                    }
                    memb_free(&metadat_memb, q->ptr);
                }
                memb_free(&packet_memb, q);
            }
            /* the packet allocation failed. remove and free neighbor entyr if empty. */
            if(list_length(n->queued_packet_list) == 0) {
                list_remove(neighbor_list, n);
                memb_free(&neighbor_memb, n);
            }
        }
    }
    /* call back the call-back function */
    mac_call_sent_callback(sent);
}

void csmaScheduling() {
}
