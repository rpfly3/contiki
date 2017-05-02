#ifndef __SLOT_SCHEDULE_H__
#define __SLOT_SCHEDULE_H__

#include "contiki.h"

enum link_type {LINK_TYPE_NORMAL, LINK_TYPE_ADVERTISING, LINK_TYPE_ADVERTISING_ONLY};

struct prkmcs_link {
	struct prkmcs_link *next;
	uint16_t addr;
	uint16_t slotframe_handle;
	uint16_t timeslot;
	uint16_t channel_offset;
	uint8_t link_options;
	enum link_type link_type;
	void *data;
};

struct prkmcs_slotframe {
	struct prkmcs_slotframe *next;
	uint16_t handle;
	LIST_STRUCT(links_list);
};

#endif /*__SLOT_SCHEDULE_H__*/
