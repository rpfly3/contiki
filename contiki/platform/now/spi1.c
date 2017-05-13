/* Author: Pengfei Ren
 * Date: September 19, 2015
 * Pin Mapping for RF231 on .NOW: 
 * SCK --> PA5; MISO --> PA6; MOSI --> PA7; CLKM --> OSC_IN (not connected to GPIO, routed to a timer); IRQ --> PE1; SLP_TR --> PG8; /SEL --> PG6; /RST --> PE2;
 * Note that: the CLKM is not implemented temporarily.
 */
#include "contiki.h"
#include "platform/now/spi1.h"
#include <stm32f10x_gpio.h>
#include <stm32f10x_rcc.h>
#include <stm32f10x_spi.h>
#include <misc.h>
#include <stm32f10x_exti.h>
#include <stdio.h>

/*========Init SPI1 and configure used pins to operate RF231========*/

void spi1_init()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    SPI_InitTypeDef SPI_InitStructure;

    // Enable the clocks for GPIOA, GPIOE and GPIOG, and AFIO clock is generally enabled at the same time.
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOE | RCC_APB2Periph_GPIOG | RCC_APB2Periph_AFIO, ENABLE);
    // Enable the clock for SPI1
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    // Configure pin5, pin6, pin7 for SCK, MISO, MOSI respectively
	
    // Configure SCK and MOSI pins as  Alternate Function Push Pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
	
    // Configure MISO pin as Input Floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Configure PE2 for /RST, using Mode_Out_PP mode
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    // Configure PE1 for IRQ using Mode_IN_FLOATING mode
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    // Configure PG6 and PG8 for /SEL and SLP_TR respectively
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOG, &GPIO_InitStructure);

    
    // Fill the SPI_InitStructure with parameters 
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &SPI_InitStructure);
    
    // Enable SPI1
    SPI_Cmd(SPI1, ENABLE);
    //printf("SPI1 is initialized ...\r\n");
}

void exti_pe1_config(void) {
    /* Set variables used */
    GPIO_InitTypeDef GPIO_InitStructure;
    EXTI_InitTypeDef EXTI_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* Enable clock for GPIOE */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE | RCC_APB2Periph_AFIO, ENABLE);

    /* Set pin as input */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOE, &GPIO_InitStructure);
	
    /* Tell system that PE1 will use EXTI_Line1 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource1);


    /* PE1 is connected to EXTI_Line1 */
    EXTI_InitStructure.EXTI_Line = EXTI_Line1;
    /* Interrupt mode */
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    /* Trigger on rising edge */ 
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;
    /* Enable interrupt */
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

	/* PE1 uses EXTI1_IRQChannel vector */
	NVIC_InitStructure.NVIC_IRQChannel = EXTI1_IRQn;
	/* Set highest priority */
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
}

inline void rf231_irq_disable()
{
	EXTI->IMR &= ~(1 << EXTI_Line1);
 /*
	EXTI_InitTypeDef EXTI_InitStructure;

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource1);
	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;     
	EXTI_InitStructure.EXTI_LineCmd = DISABLE;       
	EXTI_Init(&EXTI_InitStructure);
	EXTI_ClearITPendingBit(EXTI_Line1);
 */
}
inline void rf231_irq_enable()
{

	EXTI->IMR |= (1 << EXTI_Line1);
/*
	EXTI_InitTypeDef EXTI_InitStructure;

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOE, GPIO_PinSource1);
	EXTI_InitStructure.EXTI_Line = EXTI_Line1;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising;     
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;       
	EXTI_Init(&EXTI_InitStructure);
	EXTI_ClearITPendingBit(EXTI_Line1); 
 */
}

/*========Functions used to read and write data from RF231 via SPI========*/

//Read one byte data via MISO line
uint8_t SPI1_Read()
{
    //wait until the receive buffer not empty flag is SET
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET)
        ;
    return SPI_I2S_ReceiveData(SPI1);
}
//Write one byte data via MOSI line
void SPI1_Write(uint8_t data)
{
    //wait until the transmit buffer empty flag is SET
    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET)
        ;
    SPI_I2S_SendData(SPI1, data);
}

/*========Functions used to set and get valtage of pins, thus change the state of RF231========*/

/* The following functions are well tested. */

/* Set the /SEL, thus not select RF231 */
void RF231_SELSet()
{
    GPIO_WriteBit(GPIOG, GPIO_Pin_6, Bit_SET);
}
/* Clear the /SEL, thus select RF231 */
void RF231_SELClr()
{
    GPIO_WriteBit(GPIOG, GPIO_Pin_6, Bit_RESET);
}
//Get the /SEL value
uint8_t RF231_GetSEL()
{
    return GPIO_ReadOutputDataBit(GPIOG, GPIO_Pin_6);
}

//Set the /RST
void RF231_RSTSet()
{
    GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_SET);
}
//Clear the /RST
void RF231_RSTClr()
{
    GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_RESET);
}
//Get the /RST value
uint8_t RF231_GetRST()
{
    return GPIO_ReadOutputDataBit(GPIOE, GPIO_Pin_2);
}

//Set the SLP_TR
void RF231_SLP_TRSet()
{
    GPIO_WriteBit(GPIOG, GPIO_Pin_8, Bit_SET);
}
//Clear the SLP_TR
void RF231_SLP_TRClr()
{
    GPIO_WriteBit(GPIOG, GPIO_Pin_8, Bit_RESET);
}
//Get the SLP_TR value
uint8_t RF231_GetSLP_TR()
{
    return GPIO_ReadOutputDataBit(GPIOG, GPIO_Pin_8);
}

