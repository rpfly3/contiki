#ifndef __TEST_H__
#define __TEST_H__

#include "contiki.h"
#include "core/net/linkaddr.h"

#define TEST_PACKET_LENGTH (sizeof(uint8_t) + sizeof(linkaddr_t) + sizeof(uint16_t))
void test_init();
void test_send();
void test_receive(uint8_t *ptr);
#endif /* __TEST_H__ */
