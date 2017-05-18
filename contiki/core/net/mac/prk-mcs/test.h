#ifndef __TEST_H__
#define __TEST_H__

#include "contiki.h"
#include "core/net/linkaddr.h"

#define MAX_SM_NUM 40
void test_init();
void test_send();
void test_receive(uint8_t *ptr);
#endif /* __TEST_H__ */
