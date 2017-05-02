#ifndef CONTIKI_CONF_H__
#define CONTIKI_CONF_H__

#include "platform-conf.h"

/* Contiki Core InterFace */
#define CCIF 
/* Contiki Loadable InterFace */
#define CLIF 

/* The systick frequence */
#define MCK 64000000
/* A second, measured in system clock time: for .NOW it's 8 MHz */
#define CLOCK_CONF_SECOND 100000

#endif /* CONTIKI_CONF_H__ */
