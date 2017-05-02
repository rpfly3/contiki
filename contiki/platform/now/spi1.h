#ifndef __SPI1_H__
#define __SPI1_H__

#include <stdio.h>
#include "stm32f10x.h"

/*========Functions declartions for the ones defined in spi1.c========*/

void spi1_init();
void exti_pe1_config();

void rf231_irq_disable();
void rf231_irq_enable();

uint8_t SPI1_Read();
void SPI1_Write(uint8_t data);

void RF231_SELSet();
void RF231_SELClr();
uint8_t RF231_GetSEL();

void RF231_RSTSet();
void RF231_RSTClr();
uint8_t RF231_GetRST();

void RF231_SLP_TRSet();
void RF231_SLP_TRClr();
uint8_t RF231_GetSLP_TR();


#endif //__SPI1_H__
