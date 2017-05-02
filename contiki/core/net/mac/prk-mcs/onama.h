#ifndef __ONAMA_H__
#define __ONAMA_H__

#include "core/net/mac/prk-mcs/asn.h"
#include "core/net/linkaddr.h"

enum
{
	UNAVAILABLE = 0,
	AVAILABLE = 1,
};
void onama_init();
void runLama(struct asn_t current_slot);
#endif /* __ONAMA_H__*/