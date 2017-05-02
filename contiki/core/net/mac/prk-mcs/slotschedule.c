#include "contiki.h"
#include "lib/memb.h"
#include "net/nbr-table.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/frame802154.h"
#include "sys/process.h"
#include "sys/rtimer.h"
#include <string.h>

MEMB(link_memb, struct prkmcs_link, MAX_SCHEDULE_LINKS);
MEMB(slotframe_memb, struct prkmcs_slotframe, MAX_SCHEDULE_SLOTFRAMES);
LIST(slotframe_list);

/*------------------------------- slotframe operation ----------------------------*/

/* find a slot frame by a handle */
struct prkmcs_slotframe * getSlotframe(uint16_t handle) {
    struct prkmcs_slotframe *sf = list_head(slotframe_list);
    while(sf != NULL) {
        if(sf->handle == handle) {
            return sf;
        }
        sf = list_item_next(sf);
    }
    return NULL;
}

/* add a slotframe */
struct prkmcs_slotframe * addSlotframe(uint16_t handle, uint16_t size) {
    if(size == 0) {
        return NULL;
    }

    /* check if the slotframe exists */
    if(getSlotframe(handle)) {
        return NULL;
    }

    struct prkmcs_slotframe *sf = memb_alloc(&slotframe_memb);
    if(sf != NULL) {
        sf->handle = handle;
        /* initialize the link list of the slotframe */
        LIST_STRUCT_INIT(sf, links_list);
        /* add the slotframe the the slotframe list */
        list_add(slotframe_list, sf);
    }

    return sf;
}

/* remove a slotframe */
int removeSlotframe(struct prkmcs_slotframe *slotframe) {
    if(slotframe != NULL) {
        struct prkmcs_link *l;
        while((l = list_head(slotframe->links_list))) {
            removeLink(slotframe, l);
        }

        memb_free(&slotframe_memb, slotframe);
        list_remove(slotframe_list, slotframe);
        return 1;
    }
    return 0;
}

/* remove all slotframes */
int removeAllSlotframes(void) {
    struct prkmcs_slotframe *sf;
    while((sf = list_head(slotframe_list))) {
        if(removeSlotframe(sf) == 0) {
            return 0;
        }
    }
    return 1;
}

/*--------------------------------- link operation -----------------------------*/

/* find a link by a handle */
struct prkmcs_link * getLink(uint16_t handle) {
    struct prkmcs_slotframe *sf = list_head(slotframe_list);
    /* loop over all slotframe */
    while(sf != NULL) {
        struct prkmcs_link *l = list_head(sf->links_list);
        /* loop over all links of a slotframe */
        while(l != NULL) {
            if(l->handle == handle) {
                return l;
            }
            l = list_item_next(l);
        }
        sf = list_item_next(sf);
    }

    return NULL;
}

/* remove a link from a slotframe */
int removeLink(struct prkmcs_slotframe *slotframe, struct prkmcs_link *l) {
    if(slotframe != NULL && l != NULL && l->slotframe_handle == slotframe->handle) {
        uint8_t link_options;
        linkaddr_t addr;
        
        link_options = l->link_options;
        linkaddr_copy(&addr, &l->addr);

        if(l == current_link) {
            current_ling = NULL;
        }

        list_remove(slotframe->links_list, l);
        memb_free(&link_memb, l);

        return 1;
    }
    return 0;
}

/* find a link according to the timeslot */
struct prkms_link * getLinkByTimeslot(struct prkmcs_slotframe *slotframe, uint16_t timeslot) {
    if(slotframe != NULL) {
        struct prkmcs_link *l = list_head(slotframe->links_list);
        /* loop over the link list of the slotframe */
        while(l != NULL) {
            if(l->slotframe == timeslot) {
                return l;
            }
            l = list_item_next(l);
        }
    }
    return NULL;
}

/* remove a link from a slotframe according to the timeslot */
int removeLinkByTimeslot(struct prkmcs_slotframe *slotframe, uint16_t timeslot) {
    return slotframe != NULL && removeLink(slotframe, getLinkByTimeslot(slotframe, timeslot));
}

/* add a link to a slotframe */
struct prkmcs_link * addLink(struct prkmcs_slotframe *slotframe, uint8_t link_options, enum link_type link_type, 
                             const linkaddr_t *address, uint16_t timeslot, uint16_t channel_offset) {
    struct prkmcs_link *l = NULL;
    if(slotframe != NULL) {
        /* Todo: modify accordingly */
        removeLinkByTimeslot(slotframe, timeslot);

        l = memb_alloc(&link_memb);
        if(l != NULL) {
            static int current_link_handle = 0;
            struct prkmcs_neighbor *n;

            /* initialize the link */
            l->handle = current_link_handle++;
            l->link_options = link_options;
            l->link_type = link_type;
            l->slotframe_handle = slotframe->handle;
            l->timeslot = timeslot;
            l->channel_offset = channel_offset;
            l->data = NULL;
            if(address == NULL) {
                address = &linkaddr_null;
            }
            linkaddr_copy(&l->addr, address);
            /* add the link to the slotframe */
            list_add(slotframe->links_list, l);
        }
    }
    return l;
}

/* initialize the slot schedule module */
int slotScheduleInit() {
    memb_init(&link_memb);
    memb_init(&slotframe_memb);
    list_init(slotframe_list);

    return 1;
}

