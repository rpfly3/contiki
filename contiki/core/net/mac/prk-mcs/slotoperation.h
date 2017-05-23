#ifndef __SLOT_OPERATION_H__
#define __SLOT_OPERATION_H__

#include "contiki.h"
#include "core/net/mac/prk-mcs/asn.h"

void prkmcs_slot_operation_start();
void schedule_next_slot(struct rtimer *t);
#endif /* __SLOT_OPERATION_H__ */
