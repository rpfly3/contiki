/* Author: Pengfei Ren
 * Date: September 19, 2015
 * Pin Mapping for RF231 on .NOW: 
 * SCK --> PA5; MISO --> PA6; MOSI --> PA7; CLKM --> OSC_IN (not connected to GPIO, routed to a timer); IRQ --> PE1; SLP_TR --> PG8; /SEL --> PG6; /RST --> PE2;
 * Note that: the CLKM is not implemented temporarily.
 */
#include "platform/now/rf231-arch.h" //Macro definitions for RF231
#include "platform/now/spi1.h"
#include "platform/now/rf231.h"
#include "core/lib/crc16.h"
#include "contiki.h"
#include <string.h>
#include <stdio.h>

// probably the semaphore would be used, so I use wrap the disable/enable with take and give
static inline void take()
{
    __disable_irq();
}

static inline void give()
{
    __enable_irq();
}

void rf231_arch_init(void) {
    /* Initialize spi */
    spi1_init();
    /* Config interrupt */
    exti_pe1_config();
}


/*========Functions used to read and write values to register========*/

void WriteRegister(uint8_t reg, uint8_t value)
{
    /*
	if (RF231_GetSEL() == 0)
	{
		log_error("Radio being accessed by others");
		return;
	}
    */
    take();

    /* Initiate write operation */
    RF231_SELClr();

    /* Send command byte */
    SPI1_Write(RF231_CMD_REGISTER_WRITE | reg);
    SPI1_Read();

    /* Write data byte */
    SPI1_Write(value);
    SPI1_Read();

    /* End write operation */
    RF231_SELSet();

    give();

    return;
}
uint8_t ReadRegister(uint8_t reg)
{
    /*
	if (RF231_GetSEL() == 0)
	{
		log_error("Radio being accessed by others");
		return 0;
	}
    */
    uint8_t RegisterValue;

    take();
    /* Initiate read operation */
    RF231_SELClr();

    /* Send command byte */
    SPI1_Write(RF231_CMD_REGISTER_READ | reg);
    SPI1_Read();

    /* Read value */
    SPI1_Write(0);
    RegisterValue = SPI1_Read();

    /* End read operation */
    RF231_SELSet(); 

    give();

    return RegisterValue;
}

/*========Functions used to read and write values to frame========*/

void WriteFrame(uint8_t *WriteBuffer, uint8_t length)
{
    /*
	if (RF231_GetSEL() == 0)
	{
		log_error("Radio being accessed by others");
		return;
	}
    */
    
    take();

    /* Initiate write operation */
    RF231_SELClr();

    /* Send command byte */
    SPI1_Write(RF231_CMD_FRAME_WRITE);
    SPI1_Read();
    
    /* Send PHR info */
    SPI1_Write(length);
    SPI1_Read();

    /* Send PSDU data */
    while(length--)
    {
        SPI1_Write(*WriteBuffer++);
        SPI1_Read();
    }
    
    /* End write operation */
    RF231_SELSet();

    give();

    return;
}


void ReadFrame(rx_frame_t *ReadBuffer)
{
    /*
	if (RF231_GetSEL() == 0)
	{
		log_error("Radio being accessed by others");
		return;
	}
    */
    
    take();

    /* Initiate read operation */
    RF231_SELClr();

    /* Send command byte */
    SPI1_Write(RF231_CMD_FRAME_READ);
    SPI1_Read();
    
    /* Read the PHR info */
    SPI1_Write(0);
    uint8_t FrameLength = SPI1_Read();

    /* Check for correct frame length */
    if((FrameLength < RF231_MIN_FRAME_LENGTH) || (FrameLength > RF231_MAX_FRAME_LENGTH)) {
        /* Length test fail and set the corresponding field*/
        ReadBuffer->length = 0;
    }
    else
    {
        ReadBuffer->length = FrameLength - CHECKSUM_LEN;
        uint8_t *rx_data = (ReadBuffer->data);

        uint8_t temp = ReadBuffer->length;

        /* Get PSDU data */
        while (temp--)
        {
            SPI1_Write(0);
            *rx_data++ = SPI1_Read();
        }

        /* discard automatically verified checksum */
        temp = CHECKSUM_LEN;
        while (temp--)
        {
            SPI1_Write(0);
            SPI1_Read();
        }

        /* Discard LQI info */
        SPI1_Write(0);
        SPI1_Read();
    }

    /* End read */
    RF231_SELSet();
    give();

    return;
} 

/*========Functions used to read and write to SRAM========*/

/* Write Frame buffer with random access */
void WriteSRAM(uint8_t addr, uint8_t *data, uint8_t len) {
    /* Check the address and lenth */
    if (addr + len <= 0x7F)
    {

        /*
	if (RF231_GetSEL() == 0)
	{
		log_error("Radio being accessed by others");
		return;
	}
    */
        take();
        RF231_SELClr();

        /* Send command byte and address */
        SPI1_Write(RF231_CMD_SRAM_WRITE);
        SPI1_Read();
        SPI1_Write(addr);
        SPI1_Read();

        /* Write data */
        while (len--)
        {
            SPI1_Write(*data++);
            SPI1_Read();
        }

        RF231_SELSet();
        give();
    }
    else
    {
        // invalid data -- do nothing
    }
    return;
}

/* Read Frame buffer with random access */
void ReadSRAM(uint8_t addr, uint8_t *data, uint8_t len) {
    /* Check the address and length */
    if (addr + len <= 0x7F)
    {
        /*
	if (RF231_GetSEL() == 0)
	{
		log_error("Radio being accessed by others");
		return;
	}
*/
        take();
        RF231_SELClr();

        /* Send command byte and address */
        SPI1_Write(RF231_CMD_SRAM_READ);
        SPI1_Read();
        SPI1_Write(addr);
        SPI1_Read();

        /* Read data */
        while (len--)
        {
            SPI1_Write(0);
            *data++ = SPI1_Read();
        }

        RF231_SELSet();
        give();
    }
    return;
}
