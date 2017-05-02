#include "contiki.h"
#include "core/lib/list.h"
#include "packetqueue.h"

#include <string.h>

struct prkmcs_packet** 

/* Find a transmission neighbor */
struct prkmcs_neighbor* getNeighbor(const linkaddr_t *addr) {

    struct prkmcs_neighbor *n = list_head(neighbor_list);
    while(n != NULL) {
        if(linkaddr_cmp(&n->addr, addr)) {
            return n;
        }
        n = list_item_next(n);
    }
    return NULL;
}

/* Add a neighbor */
struct prkmcs_neighbor* addNeighbor(const linkaddr_t *addr) {

    struct prkmcs_neighbor *n = getNeighbor(addr);

    /* check if the neighbor exist */
    if(n == NULL) {
        n = memb_alloc(&neighbor_memb);
        if(n != NULL) {
            memset(n, 0, sizeof(struct prkmcs_neighbor));
            ringbufindex_init(&n->tx_ringbuf, NEIGHBOR_QUEUE_SIZE);
            linkaddr_copy(&n->addr, addr);
            n->is_broadcast = linkaddr_cmp(addr, &broadcast_address);
            list_add(neighbor_list, n);
        }
    }
    return n;
}

/* Check if a neighbor queue is empty */
int isEmpty(const struct prkmcs_neighbor *n) {
    return n != NULL && ringbufindex_empty(&n->tx_ringbuf);
}

/* Add packet to a neighbor queue */
struct prkmcs_packet * addPacket(const linkaddr_t *addr, mac_callback_t sent, void *ptr) {
    struct prkmcs_neighbor *n = NULL;
    int16_t put_index = -1;
    struct prkmcs_packet *p = NULL;

    /* check if the neighbor exists and add */
    n = addNeighbor(addr);

    if(n != NULL) {
        /* check if there is space in the neighbor's ringbuf */
        put_index = ringbufindex_peek_put(&n->tx_ringbuf);
        if(put_index != -1) {
            /* allocate memory */
            p = memb_alloc(&packet_memb);
            if(p != NULL) {
                /* allocate queuebuf for storing the packet */
                p->qb = queuebuf_new_from_packetbuf();
                if(p->qb != NULL) {
                    p->sent = sent;
                    p->ptr = ptr;
                    p->ret = MAX_TX_DEFERRED;
                    p->transmissions = 0;
                    /* put the packet into the neighbor's ringbuf */
                    n->tx_array[put_index] = p;
                    /* update the ringbuf index */
                    ringbufindex_put(&n->tx_ringbuf);
                    return p;
                } else {
                    memb_free(&packet_memb, p);
                }
            }
        }
    }
    return NULL;
}

/* Count the number of packets in the queue */
int packetCount(const linkaddr_t *addr) {
    /* check if the neighbor exists and add */
    struct prkmcs_neighbor *n = addNeighbor(addr);

    if(n != NULL) {
        return ringbufindex_elements(&n->tx_ringbuf);
    }
    return -1;
}

/* Get and remove the first packet from a neighbor queue */
struct prkmcs_packet * removePacket(struct prkmcs_neighbor *n) {
    if(n != NULL) {
        /* get and remove the first packet */
        int16_t get_index = ringbufindex_get(&n->tx_ringbuf);
        if(get_index != -1) {
            return n->tx_array[get_index];
        } else {
            return NULL;
        }
    }
    return NULL;
}

/* Free a packet */
void freePacket(struct prkmcs_packet *p) {
    if(p != NULL) {
        /* free the queuebuf storage */
        queuebuf_free(p->qb);
        /* free the packet memory */
        memb_free(&packet_memb, p);
    }
}

/* Flush a neighbor queue */
static void flushNeighborQueue(struct prkmcs_neighbor *n) {

    while(!isEmpty(n)) {
        struct prkmcs_packet *p = removePacket(n);
        if(p != NULL) {
            /* report removing the packet to the callbeack */
            p->ret = MAC_TX_ERR;
            mac_call_sent_callback(p->sent, p->ptr, p->ret, p->transmissions);

            /* free the packet */
            freePacket(p);
        }
    }
}

/* Remove neighbor */
static void removeNeighbor(struct prkmcs_neighbor *n) {

    /* Remove the neighbor from the neighbor list */
    list_remove(neighbor_list, n);
    /* Flush the queueu of this neighbor */
    flushNeighborQueue(n);
    /* Free the neighbor memory */
    memb_free(&neighbor_memb, n);
}

/* Flush all neighbor queues */
void flushAllQueue() {
    struct prkmcs_neighbor *n = list_head(neighbor_list);
    /* flush the neighbor queue one by one */
    while(n != NULL) {
        struct prkmcs_neighbor *next_n = list_item_next(n);
        flushNeighborQueue(n);
        n = next_n;
    }
}

/* Deallocate empty neighbor queues */
void freeEmptyQueue() {
    struct prkmcs_neighbor *n = list_head(neighbor_list);
    while(n != NULL) {
        struct prkmcs_neighbor *next_n = list_item_next(n);
        if(!n->is_broadcast && isEmpty(n)) {
            /* remove the empty neighbor */
            removeNeighbor(n);
        }
        n = next_n;
    }
}

/* Get the first packet from a neighbor queue */
struct prkmcs_packet * getHeadForNeighbor(const struct prkmcs_neighbor *n, struct prkmcs_link *link) {
    if(n != NULL) {
        int16_t get_index = ringbufindex_peek_get(&n->ringbuf);
        if(get_index != -1) {
            return n->tx_array[get_index];
        }
    }
    return NULL;
}

/* Get the first packet from a neighbor queue with neighbor address */
struct prkmcs_packet * getHeadForAddr(const linkaddr_t *addr, struct prkmcs_link *link) {
    return getHeadForNeighbor(getNeighbor(addr), link);
}

/* Initialize the neighbor queue management module */
void neighborQueueInit() {
    list_init(neighbor_list);
    memb_init(&neighbor_memeb);
    memb_init(&packet_memb);

    /* Add virtual broadcast neighbors */
    n_broadcast = addNeighbor(&prkmcs_broadcast_address);
}
