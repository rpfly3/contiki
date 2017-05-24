#ifndef __TEST_H__
#define __TEST_H__

#include "contiki.h"
#include "core/net/linkaddr.h"

#define MAX_SM_NUM 39
void test_send();
void test_receive(uint8_t *ptr, int8_t rx_ed);
#endif /* __TEST_H__ */
