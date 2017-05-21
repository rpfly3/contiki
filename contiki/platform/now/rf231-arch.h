#ifndef RF231_ARCH_H__
#define RF231_ARCH_H__

#include "stm32f10x.h"

#define RF231_MIN_FRAME_LENGTH (0x03) //--> A frame should be at least 3 bytes
#define RF231_MAX_FRAME_LENGTH (0x7F) //--> A frame should be no more than 127 bytes



typedef enum {
    false = 0,
    true = !false
} bool;



/*========SPI access command macros========*/

#define RF231_CMD_REGISTER_READ (0x80) //--> Register Read
#define RF231_CMD_REGISTER_WRITE (0xc0) //--> Register Write
#define RF231_CMD_REGISTER_MASK (0x3f) //--> Register MASK
#define RF231_CMD_FRAME_READ (0x20) //--> Frame Read
#define RF231_CMD_FRAME_WRITE (0x60) //--> Frame Write
#define RF231_CMD_SRAM_READ (0x00) //--> SRAM Read
#define RF231_CMD_SRAM_WRITE (0x40) //--> SRAM Write


/*========RF231 Interrupt Masks========*/
#define RF231_BAT_LOW_MASK	(0x80)
#define RF231_TRX_UR_MASK	(0x40)
#define RF231_AMI_MASK          (0x20)
#define RF231_CCA_ED_DONE_MASK  (0x10)
#define RF231_TRX_END_MASK	(0x08)
#define RF231_RX_START_MASK	(0x04)
#define RF231_PLL_UNLOCK_MASK	(0x02)
#define RF231_PLL_LOCK_MASK	(0x01)

/*========RF231 Interrupt Registers========*/
#define RF231_IRQ_STATUS	(0x0F)
#define RF231_IRQ_MASK		(0x0E)

typedef struct {
	uint8_t length;				//--> frame length
	uint8_t data[RF231_MAX_FRAME_LENGTH];	//--> PSDU data
	uint8_t tx_ed; //received energy in transmission
	uint8_t noise_ed; //received energy without transmission
} rx_frame_t;

/*========Functions declartions for the ones defined in radio.c========*/
void rf231_arch_init();

void WriteRegister(uint8_t reg, uint8_t value);
uint8_t ReadRegister(uint8_t reg);

void WriteFrame(uint8_t *WriteBuffer, uint8_t length);
void ReadFrame(rx_frame_t *ReadBuffer);

void WriteSRAM(uint8_t addr, uint8_t *data, uint8_t len);
void ReadSRAM(uint8_t addr, uint8_t *data, uint8_t len);



#endif
