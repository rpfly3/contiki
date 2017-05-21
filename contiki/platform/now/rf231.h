#ifndef RF231_H__
#define RF231_H__

#include "stm32f10x.h"
#include "rf231-arch.h"

/*========= RF231 Timing Constant ========*/
// all us
enum
{
	t10 = 625, //--> Reset pulse width
	tTR1 = 380, //--> State transition from P_ON until CLKM is available
	tTR2 = 240, //--> State transition from SLEEP to TRX_OFF
	tTR6 = 110, //--> State transition from TRX_OFF to RX_ON
	tTR8 = 1, //--> State transition from PLL_ON to RX_ON
	tTR9 = 1, //--> State transition from RX_ON to PLL_ON
	tTR12 = 1, // --> Transition from all active states to TRX_OFF
	tTR17 = 60, //--> DVREG settling time
	tTR20 = 11, //--> PLL settling time on channel switch
	tReadED = 108, //--> Duration between TRX_IRQ_RX_START and end of ED measurement when receiving a frame
	tED = 140, //--> Duration of a manually started ED measurement
	tHED = 236, //--> Minimum time an ED measurement is valid
};

/*========SPI access command macros========*/

#define RF231_CMD_REGISTER_READ (0x80) //--> Register Read
#define RF231_CMD_REGISTER_WRITE (0xc0) //--> Register Write
#define RF231_CMD_REGISTER_MASK (0x3f) //--> Register MASK
#define RF231_CMD_FRAME_READ (0x20) //--> Frame Read
#define RF231_CMD_FRAME_WRITE (0x60) //--> Frame Write
#define RF231_CMD_SRAM_READ (0x00) //--> SRAM Read
#define RF231_CMD_SRAM_WRITE (0x40) //--> SRAM Write


/*========RF231 register addresses========*/

#define RF231_TRX_STATUS (0x01) //--> Radio Status
#define RF231_TRX_STATE (0x02) //--> Radio control command 
#define RF231_TRX_CTRL_0 (0x03) //--> Radio TRX control
#define RF231_TRX_CTRL_1 (0x04) //--> Radio TRX control
#define RF231_PHY_TX_PWR (0x05) //--> Radio transmission power control
#define RF231_PHY_RSSI (0x06) //--> RSSI value
#define RF231_PHY_ED_LEVEL (0x07) //--> Energy Detection
#define RF231_PHY_CC_CCA (0x08) //--> Radio transmission channel control
#define RF231_PHY_CCA_THRES (0x09) //--> ED threshold level for CCA
#define RF231_TRX_CTRL_2 (0x0C) //--> Radio TRX control
#define RF231_PHY_RX_SYN (0x15) //--> Control sensitivity threshold of the receiver

#define RF231_PLL_CF (0x1A) //--> Center frequency calibration loop control
#define RF231_PLL_DCU (0x1B) //--> Delay cell calibration loop control
#define RF231_XAH_CTRL_0 (0x2C) //--> Control register for extended operating mode
#define RF231_CSMA_SEED_0 (0x2D) //--> CSMA seed
#define RF231_CSMA_SEED_1 (0x2E) //-->CSMA seed
#define RF231_CSMA_BE (0x2F) //--> CSMA backoff


/*========RF231 state control commands for TRX_STATE========*/

/*--------Basic Operating Mode--------*/
#define RF231_TRX_STATE__NOP (0x00) //default value
#define RF231_TRX_STATE__TX_START (0x02)
#define RF231_TRX_STATE__FORCE_TRX_OFF (0x03)
#define RF231_TRX_STATE__FORCE_PLL_ON (0x04)
#define RF231_TRX_STATE__RX_ON (0x06)
#define RF231_TRX_STATE__TRX_OFF (0x08)
#define RF231_TRX_STATE__PLL_ON (0x09)
/*--------Extended Operating Mode--------*/
#define RF231_TRX_STATE__RX_AACK_ON (0x16)
#define RF231_TRX_STATE__TX_ARET_ON (0x19)

/*========RF231 TRX status======== */

#define RF231_TRX_STATUS_MASK__TRX_STATUS (0x1F)
#define RF231_TRX_STATUS__P_ON (0x00) //default value
#define RF231_TRX_STATUS__BUSY_RX (0x01)
#define RF231_TRX_STATUS__BUSY_TX (0x02)
#define RF231_TRX_STATUS__RX_ON (0x06)
#define RF231_TRX_STATUS__TRX_OFF (0x08)
#define RF231_TRX_STATUS__PLL_ON (0x09)
#define RF231_TRX_STATUS__SLEEP (0x0F)
#define RF231_TRX_STATUS__BUSY_RX_AACK (0x11)
#define RF231_TRX_STATUS__BUSY_TX_ARET (0x12)
#define RF231_TRX_STATUS__RX_AACK_ON (0x16)
#define RF231_TRX_STATUS__TX_ARET_ON (0x19)
#define RF231_TRX_STATUS__RX_ON_NOCLK (0x1C)
#define RF231_TRX_STATUS__RX_AACK_ON_NOCLK (0x1D)
#define RF231_TRX_STATUS__BUSY_RX_AACK_NOCLK (0x1E)
#define RF231_TRX_STATUS__STATE_TRANSITION_IN_PROGRESS (0x1F)


/*========RF231 transimite power values========*/
#define RF231_TX_PWR_3_DBM (0x0) //--> 3dbm
#define RF231_TX_PWR_2_8_DBM (0x1) //--> 2.8dbm
#define RF231_TX_PWR_2_3_DBM (0x2) //--> 2.3dbm
#define RF231_TX_PWR_1_8_DBM (0x3) //--> 1.8dbm
#define RF231_TX_PWR_1_3_DBM (0x4) //--> 1.3dbm
#define RF231_TX_PWR_0_7_DBM (0x5) //--> 0.7dbm
#define RF231_TX_PWR_0_0_DBM (0x6) //--> 0.0dbm
#define RF231_TX_PWR_M1_DBM (0x7) //--> -1dbm
#define RF231_TX_PWR_M2_DBM (0x8) //--> -2dbm
#define RF231_TX_PWR_M3_DBM (0x9) //--> -3dbm
#define RF231_TX_PWR_M4_DBM (0xA) //--> -4dbm
#define RF231_TX_PWR_M5_DBM (0xB) //--> -5dbm
#define RF231_TX_PWR_M7_DBM (0xC) //--> -7dbm
#define RF231_TX_PWR_M9_DBM (0xD) //--> -9dbm
#define RF231_TX_PWR_M12_DBM (0xE) //--> -12dbm
#define RF231_TX_PWR_M17_DBM (0xF) //--> -17dbm

#define RF231_TX_PWR_MAX RF231_TX_PWR_3_DBM
#define RF231_TX_PWR_MIN RF231_TX_PWR_M17_DBM


/*========RF231 transmite channels========*/
#define RF231_CHANNEL_11 (0x0B)
#define RF231_CHANNEL_12 (0x0C)
#define RF231_CHANNEL_13 (0x0D)
#define RF231_CHANNEL_14 (0x0E)
#define RF231_CHANNEL_15 (0x0F)
#define RF231_CHANNEL_16 (0x10)
#define RF231_CHANNEL_17 (0x11)
#define RF231_CHANNEL_18 (0x12)
#define RF231_CHANNEL_19 (0x13)
#define RF231_CHANNEL_20 (0x14)
#define RF231_CHANNEL_21 (0x15)
#define RF231_CHANNEL_22 (0x16)
#define RF231_CHANNEL_23 (0x17)
#define RF231_CHANNEL_24 (0x18)
#define RF231_CHANNEL_25 (0x19)
#define RF231_CHANNEL_26 (0x1A)

#define RF231_CHANNEL_MAX RF231_CHANNEL_26
#define RF231_CHANNEL_MIN RF231_CHANNEL_11
#define CHANNEL_NUM 16


/*============ CCA ===========*/
#define RF231_CCA_0 (0x00)
#define RF231_CCA_1 (0x20)
#define RF231_CCA_2 (0x40)
#define RF231_CCA_3 (0x60)

#define CSMA_SUCCESS (0x00)
#define CSMA_SUCCESS_DATA_PENDING (0x20)
#define CSMA_SUCCESS_WAIT_FOR_ACK (0x40)
#define CSMA_CHANNEL_ACCESS_FAILURE (0x60)
#define CSMA_NO_ACK (0xA0)
#define CSMA_INVALID (0xE0)

#define CHECKSUM_LEN 2

#ifndef RF231_CONF_RX_BUFFERS
#define RF231_CONF_RX_BUFFERS 100
#endif

#define DEFAULT_INTERRUPTS (RF231_TRX_END_MASK)
#define DEFAULT_CCA_THRES 0
#define FCF 1

/*========Functions declartions for the ones defined in radio.c========*/

extern uint8_t rf231_tx_buffer[RF231_MAX_FRAME_LENGTH];
extern uint8_t rf231_tx_buffer_size;
extern rx_frame_t rf231_rx_buffer[RF231_CONF_RX_BUFFERS];
extern uint8_t rf231_rx_buffer_head, rf231_rx_buffer_tail;

/* Get the radio status */
static inline uint8_t RF231_GetStatus()
{
	return ReadRegister(RF231_TRX_STATUS) & RF231_TRX_STATUS_MASK__TRX_STATUS;
}

/* Write the state control command to TRX_STATE */
static inline void RF231_StateCtrl(uint8_t cmd)
{
	WriteRegister(RF231_TRX_STATE, cmd);
}

void rf231_init();
void start_tx();
void start_tx_aret();
void start_rx();
void SetTrxStatus(uint8_t new_status);
void rf231_send();
void rf231_csma_send();

void flush_rx_buffer();
void SetPower(uint8_t power);
void SetChannel(uint8_t channel);
uint8_t getCCA(uint8_t CCA, uint8_t threshold);
uint8_t GetRand();
void rf231_set_interrupt(uint8_t interrupt);

void InitED();
uint8_t GetED();

#endif
