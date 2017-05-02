/* Author: Pengfei Ren
 * Date: September 19, 2015
 * Pin Mapping for RF231 on .NOW: 
 * SCK --> PA5; MISO --> PA6; MOSI --> PA7; CLKM --> OSC_IN (not connected to GPIO, routed to a timer); IRQ --> PE1; SLP_TR --> PG8; /SEL --> PG6; /RST --> PE2;
 * Note that: the CLKM is not implemented temporarily.
 */

#include <stm32f10x_exti.h>
#include "platform/now/rf231.h"
#include "platform/now/delay.h"
#include "core/net/mac/prk-mcs/prkmcs.h"

uint8_t rf231_tx_buffer[RF231_MAX_FRAME_LENGTH];
uint8_t rf231_tx_buffer_size;
rx_frame_t rf231_rx_buffer[RF231_CONF_RX_BUFFERS];
uint8_t rf231_rx_buffer_head, rf231_rx_buffer_tail;

PROCESS(irq_clear_process, "IRQ clear process");
PROCESS(rx_process, "Radio receive process");
process_event_t rx_packet_ready;

/*-------------------------------------------------------------------------------*/
static void rf231_reset();
static void rf231_rx_on();
static void TXAretConfig();
static void SetCCAMode(uint8_t cca);
static void SetEDThreshold(uint8_t interrupt);
void SetInterrupt(uint8_t interrupt);

void rf231_init(void) {

	//Initialize pins
	rf231_arch_init();

	delay_us(tTR1);

	//Reset radio state
	rf231_reset();

	// Configuration for CCA
	TXAretConfig();
	SetCCAMode(RF231_CCA_2);
	SetEDThreshold(DEFAULT_CCA_THRES);

	//Keep radio on by default
	rf231_rx_on();

	SetInterrupt(DEFAULT_INTERRUPTS);
}

// Do fully radio reset and put it in TRX_OFF status
static void rf231_reset()
{
	RF231_RSTClr();
	RF231_SELSet();
	RF231_SLP_TRClr();
	
	delay_us(t10);
	
	RF231_RSTSet();

	uint8_t status;
	RF231_StateCtrl(RF231_TRX_STATE__FORCE_TRX_OFF);
	delay_us(tTR17); 
	
	do
	{
		log_info("Waiting for enter TRX_OFF status");
		status = RF231_GetStatus();
	} while (status != RF231_TRX_STATUS__TRX_OFF);
}

// Put radio in RX_ON status
static void rf231_rx_on(void) 
{
	uint8_t status;
	RF231_StateCtrl(RF231_TRX_STATE__RX_ON);
	delay_us(tTR6);

	do
	{
		log_info("Waiting for enter RX_ON status");
		status = RF231_GetStatus();
	} while (status != RF231_TRX_STATUS__RX_ON);
}

// Transit state from PLL_ON to RX_ON and be ready for reception
void start_rx()
{
	uint8_t new_status, original_status = RF231_GetStatus();
	if (original_status == RF231_TRX_STATUS__RX_ON || original_status == RF231_TRX_STATUS__BUSY_RX)
	{
		return;
	}
	else if (original_status != RF231_TRX_STATUS__PLL_ON && 
		original_status != RF231_TRX_STATUS__BUSY_TX &&
		original_status != RF231_TRX_STATUS__TX_ARET_ON &&
		original_status != RF231_TRX_STATUS__BUSY_TX_ARET)
	{
		log_error("Wrong status %u", original_status);
		return;
	}
	RF231_StateCtrl(RF231_TRX_STATE__RX_ON);
	delay_us(tTR8);

	do
	{
		new_status = RF231_GetStatus();
	} while (new_status != RF231_TRX_STATUS__RX_ON);
}

// Transit state from RX_ON to PLL_ON and be ready for transmission
void start_tx() 
{
	
	uint8_t new_status, original_status = RF231_GetStatus();
	if (original_status == RF231_TRX_STATUS__PLL_ON || original_status == RF231_TRX_STATUS__BUSY_TX)
	{
		return;
	}
	else if (original_status != RF231_TRX_STATUS__RX_ON && original_status != RF231_TRX_STATUS__BUSY_RX)
	{
		log_error("Wrong status %u", original_status);
		return;
	}
	RF231_StateCtrl(RF231_TRX_STATE__FORCE_PLL_ON);
	delay_us(tTR9);

	do
	{
		new_status = RF231_GetStatus();
	} while (new_status != RF231_TRX_STATUS__PLL_ON);
}

// Transit state from RX_ON to TX_ARET_ON and be ready for CSMA transmission
void start_tx_aret()
{
	uint8_t new_status, original_status = RF231_GetStatus();

	if (original_status == RF231_TRX_STATUS__TX_ARET_ON || original_status == RF231_TRX_STATUS__BUSY_TX_ARET)
	{
		return;
	}
	else if (original_status != RF231_TRX_STATUS__RX_ON && original_status != RF231_TRX_STATUS__BUSY_RX)
	{
		log_error("Wrong status %u", original_status);
		return;
	}

	// From RX_ON to PLL_ON
	RF231_StateCtrl(RF231_TRX_STATE__FORCE_PLL_ON);
	delay_us(tTR9);
	
	do
	{
		new_status = RF231_GetStatus();
	} while (new_status != RF231_TRX_STATUS__PLL_ON);

	// From PLL_ON to TX_ARET_ON
	RF231_StateCtrl(RF231_TRX_STATE__TX_ARET_ON);
	delay_us(tTR8);

	do
	{
		new_status = RF231_GetStatus();
	} while (new_status != RF231_TRX_STATUS__TX_ARET_ON);
}

/*==================== Radio State Check =================== */
static bool is_sleeping()
{
	if (RF231_GetSLP_TR() != 0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

static bool is_busy()
{
	uint8_t status = RF231_GetStatus();
	return (status == RF231_TRX_STATUS__BUSY_RX_AACK ||
	      status == RF231_TRX_STATUS__BUSY_TX_ARET ||
	      status == RF231_TRX_STATUS__BUSY_TX ||
	      status == RF231_TRX_STATUS__BUSY_RX ||
	      status == RF231_TRX_STATUS__BUSY_RX_AACK_NOCLK);
}

/*================= Radio State Configuration ============= */

void TXAretConfig()
{
	/* MAX_FRAME_RETRIES -- 0, MAX_CSMA_RETRIES -- 0, SLOTTED_OPERATION -- 0 */
	WriteRegister(RF231_XAH_CTRL_0, 0x0A);
	WriteRegister(RF231_CSMA_SEED_0, node_addr);
	WriteRegister(RF231_CSMA_SEED_1, 0x40);
	/* MAX_BE -- 0, MIN_BE -- 0 */
	WriteRegister(RF231_CSMA_BE, 0x80);
}

uint8_t GetPower()
{
	uint8_t power = ReadRegister(RF231_PHY_TX_PWR);
	power &= 0x0F;
	return power;
}

void SetPower(uint8_t power)
{
	uint8_t Reg, original_power;
	if (power > RF231_TX_PWR_MIN) 
	{
		log_error("Invalid power argument");
	    return;
    }
    Reg = ReadRegister(RF231_PHY_TX_PWR);
	original_power = Reg & 0x0F;
	if (original_power == power)
	{
		return;
	}

    Reg &= 0xF0;
    Reg |= power;
    WriteRegister(RF231_PHY_TX_PWR, Reg); 

	if (GetPower() != power)
	{
		log_error("Power not set");
	}
}

uint8_t GetChannel()
{
	uint8_t channel = ReadRegister(RF231_PHY_CC_CCA);
	channel &= 0x1F;
	return channel;
}

void SetChannel(uint8_t channel)
{
	uint8_t Reg, original_channel;

	if (channel < RF231_CHANNEL_MIN || channel > RF231_CHANNEL_MAX)
	{
		log_error("Invalid channel argument");
		return;
	}
    Reg = ReadRegister(RF231_PHY_CC_CCA); 
	original_channel = Reg & 0x1F;

	if (original_channel == channel)
	{
		return;
	}

    Reg &= 0xE0;
    Reg |= channel;
    WriteRegister(RF231_PHY_CC_CCA, Reg);

	uint8_t status = RF231_GetStatus();
	if (status == RF231_TRX_STATUS__RX_ON || status == RF231_TRX_STATUS__PLL_ON || status == RF231_TRX_STATUS__TX_ARET_ON || status == RF231_TRX_STATUS__RX_AACK_ON)
	{
		delay_us(tTR20);
	}

	if (GetChannel() != channel)
	{
		log_error("Channel not set");
	}
}

uint8_t GetRxSensitivity()
{
	uint8_t rxthres = ReadRegister(RF231_PHY_RX_SYN);
	rxthres &= 0x0F;
	return rxthres;
}
void SetRxSensitivity(uint8_t rxthres)
{
	uint8_t Reg, original_rxthres;
	if (rxthres > 0x0F)
	{
		log_error("Invalid RX sensitivity argument");
		return;
	}
	Reg = ReadRegister(RF231_PHY_RX_SYN); 
	original_rxthres = Reg & 0x0F;

	if (original_rxthres == rxthres)
	{
		return;
	}	
	Reg &= 0xF0;
	Reg |= rxthres;
	WriteRegister(RF231_PHY_RX_SYN, Reg);
	if (GetRxSensitivity() != rxthres)
	{
		log_error("Rx sensitivity not set");
	}
}

bool GetAutoTXCRC()
{
	uint8_t crc = ReadRegister(RF231_TRX_CTRL_1);
	crc &= 20;
	return crc;
}
void SetAutoTXCRC(bool auto_crc_on)
{
	uint8_t Reg, original_crc;
	Reg = ReadRegister(RF231_TRX_CTRL_1);
	original_crc &= 20;
	if ((!original_crc) == (!auto_crc_on))
	{
		return;
	}

	if (auto_crc_on)
	{
		Reg |= (1 << 5);
	}
	else
	{
		Reg &= ~(1 << 5);
	}
	WriteRegister(RF231_TRX_CTRL_1, Reg);
	if (GetAutoTXCRC() != auto_crc_on)
	{
		log_error("Auto TX CRC not set");
	}
}

uint8_t GetRSSI()
{
	uint8_t RSSI = ReadRegister(RF231_PHY_RSSI);
	RSSI &= 0x1F;
	return RSSI;
}
static uint8_t ED_Init = 0;
void InitED()
{
	WriteRegister(RF231_PHY_ED_LEVEL, 0);
	ED_Init = 1;
}

uint8_t GetED()
{
	uint8_t ed = 0xFF;
	ed = ReadRegister(RF231_PHY_ED_LEVEL);
	if (ed == 0xFF)
	{
		log_error("ED measurement failure");
	}
	return ed;
}

uint8_t GetEDThreshold()
{
	uint8_t ED = ReadRegister(RF231_PHY_CCA_THRES);
	ED &= 0x0F;
	return ED;
}

void SetEDThreshold(uint8_t threshold)
{
	if (threshold > 0x0F)
	{
		log_error("Invalid ED threshold argument");
		return;
	}
	uint8_t Reg, original_thres;
	Reg = ReadRegister(RF231_PHY_CCA_THRES);
	original_thres = Reg & 0x0F;
	if (original_thres == threshold)
	{
		return;
	}
	Reg &= 0xC0;
	Reg |= threshold;
	WriteRegister(RF231_PHY_CCA_THRES, Reg);
	if (GetEDThreshold() != threshold)
	{
		log_error("ED threshold not set");
	}
}

uint8_t GetCCAMode()
{
	uint8_t CCA = ReadRegister(RF231_PHY_CC_CCA);
	CCA &= 0x60;
	return CCA;
}

void SetCCAMode(uint8_t CCA)
{
	if (CCA != RF231_CCA_0 && CCA != RF231_CCA_1 && CCA != RF231_CCA_2 && CCA != RF231_CCA_3)
	{
		log_error("Invalid CCA argument");
		return;
	}
	uint8_t Reg, original_mode;
	Reg= ReadRegister(RF231_PHY_CC_CCA);
	original_mode = Reg & 0x60;
	if (original_mode == CCA)
	{
		return;
	}
	Reg &= 0x1F;
	Reg |= CCA;
	WriteRegister(RF231_PHY_CC_CCA, Reg);
	if (GetCCAMode() != CCA)
	{
		log_error("CCA mode not set");
	}
}

void InitCCARequest()
{
	uint8_t Reg = ReadRegister(RF231_PHY_CC_CCA);
	WriteRegister(RF231_PHY_CC_CCA, 0x80 | Reg);
}

/**
   @brief Returns a random number, composed of bits from the radio's
   random number generator.  Note that the radio must be in a receive
   mode for this to work, otherwise the library rand() function is
   used.
 */
uint8_t GetRand()
{
	uint8_t rand_val, val = 0;
	uint8_t status = RF231_GetStatus();
	if (status == RF231_TRX_STATUS__RX_ON || status == RF231_TRX_STATUS__RX_AACK_ON)
	{
		for (uint8_t i = 0; i < 4; ++i)
		{
			rand_val = ReadRegister(RF231_PHY_RSSI);
			rand_val &= 0x60;
			val = (val << 2) | rand_val;	
		}
		return val;
	}
	else
	{
		return rand();
	}
}

uint8_t GetInterrupt()
{
	return ReadRegister(RF231_IRQ_MASK);
}

void SetInterrupt(uint8_t interrupt) 
{
	ReadRegister(RF231_IRQ_STATUS);
	WriteRegister(RF231_IRQ_MASK, interrupt);
	if (ReadRegister(RF231_IRQ_MASK) != interrupt)
	{
		log_error("Interrupt not set");
	}
}

bool is_crc_valid()
{
	return (ReadRegister(RF231_PHY_RSSI) & 0x80);
}

/*! \brief  This function will reset the state machine (to TRX_OFF) from any of
 *          its states, except for the SLEEP state.
 *
 *  \ingroup radio
 */
void ResetStateMachine(void)
{
	RF231_SLP_TRClr();
	RF231_StateCtrl(RF231_TRX_STATE__FORCE_TRX_OFF);
	delay_us(tTR12);
	uint8_t status;
	do
	{
		status = RF231_GetStatus();
	} while (status != RF231_TRX_STATUS__TRX_OFF);
}

/*! \brief  This function will change the current state of the radio
 *          transceiver's internal state machine.
 *
 *  \param     new_state        Here is a list of possible states:
 *             - RX_ON        Requested transition to RX_ON state.
 *             - TRX_OFF      Requested transition to TRX_OFF state.
 *             - PLL_ON       Requested transition to PLL_ON state.
 *             - RX_AACK_ON   Requested transition to RX_AACK_ON state.
 *             - TX_ARET_ON   Requested transition to TX_ARET_ON state.
 */
void SetTrxStatus(uint8_t new_status)
{
	uint8_t status, original_status;

	//Check function paramter and current state of the radio transceiver.
	if (!((new_status == RF231_TRX_STATUS__TRX_OFF)    ||
	      (new_status == RF231_TRX_STATUS__RX_ON)      ||
	      (new_status == RF231_TRX_STATUS__PLL_ON)     ||
	      (new_status == RF231_TRX_STATUS__RX_AACK_ON) ||
	      (new_status == RF231_TRX_STATUS__TX_ARET_ON)))
	{
		log_error("Invalid radio status argument");
		return;
	}

	// Wait for radio to finish previous operation
	while (is_busy())
		;


	original_status = RF231_GetStatus();

	if (new_status == original_status)
	{
		return;
	}

	//At this point it is clear that the requested new_state is:
	//TRX_OFF, RX_ON, PLL_ON, RX_AACK_ON or TX_ARET_ON.

	//The radio transceiver can be in one of the following states:
	//TRX_OFF, RX_ON, PLL_ON, RX_AACK_ON, TX_ARET_ON.
	if (new_status == RF231_TRX_STATUS__TRX_OFF)
	{
		ResetStateMachine(); //Go to TRX_OFF from any state.
	}
	else
	{
	   //It is not allowed to go from RX_AACK_ON or TX_ARET_ON and directly to
	   //TX_ARET_ON or RX_AACK_ON respectively. Need to go via PLL_ON.
		if ((new_status == RF231_TRX_STATUS__TX_ARET_ON) && (original_status != RF231_TRX_STATUS__PLL_ON))
		{
			RF231_StateCtrl(RF231_TRX_STATE__PLL_ON);
			do
			{
				status = RF231_GetStatus();
			} while (status != RF231_TRX_STATUS__PLL_ON);
		}
		else if ((new_status == RF231_TRX_STATUS__RX_AACK_ON) && (original_status != RF231_TRX_STATUS__PLL_ON))
		{
			RF231_StateCtrl(RF231_TRX_STATE__PLL_ON);
			do
			{
				status = RF231_GetStatus();
			} while (status != RF231_TRX_STATUS__PLL_ON);
		}

		//Any other state transition can be done directly.
		RF231_StateCtrl(new_status);
	}

	//Verify state transition.
	do
	{
		status = RF231_GetStatus();
	} while (status != new_status);
}

// This function update the rx_buffer state after receiving
void flush_rx_buffer() 
{
	rf231_rx_buffer_head = (rf231_rx_buffer_head + 1) % RF231_CONF_RX_BUFFERS;
}

void rf231_receive()
{
}

/* Perform a CCA to find out if there is a packet in the air or not*/
uint8_t getCCA(uint8_t CCA, uint8_t threshold) 
{
	uint8_t cca = 0;

	// Keep radio in RX_ON state avoid BUSY_RX
	uint8_t val = ReadRegister(RF231_PHY_RX_SYN);
	WriteRegister(RF231_PHY_RX_SYN, 0x80 | val);

    /* CCA should be done in RX_ON status */
	start_rx();

	SetEDThreshold(threshold);
	SetCCAMode(CCA);
	InitCCARequest();

	delay_us(140);

	while ((cca & 0x80) == 0) {
		cca = ReadRegister(RF231_TRX_STATUS);
	}

	// Put radio back to original setting
	val = ReadRegister(RF231_PHY_RX_SYN);
	WriteRegister(RF231_PHY_RX_SYN, 0x7F & val);

	return (cca & 0x40);
}

void rf231_send()
{
	start_tx();
	if (RF231_GetStatus() != RF231_TRX_STATUS__PLL_ON)
	{
		log_error("Not in correct status");
		return;
	}
	WriteFrame(rf231_tx_buffer, rf231_tx_buffer_size);
	RF231_SLP_TRSet();
	RF231_SLP_TRClr();
}

void rf231_csma_send()
{
	start_tx_aret();

	if (RF231_GetStatus() != RF231_TRX_STATUS__TX_ARET_ON)
	{
		log_error("Not in correct status");
		return;
	}
	WriteFrame(rf231_tx_buffer, rf231_tx_buffer_size);
	RF231_SLP_TRSet();
	RF231_SLP_TRClr();
}


static volatile uint8_t interrupt_status, radio_status;
static rtimer_clock_t reception_time, network_time;
static uint16_t time_synch_seq_no;
process_event_t packet_in_buffer;
void EXTI1_IRQHandler(void)
{			
	if (EXTI_GetITStatus(EXTI_Line1) != RESET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line1);
		if (node_addr == BASE_STATION_ID)
		{
			return;
		}

		// simultaneous access radio would lead to packet loss and wrong radio status
		if (RF231_GetSEL() == 0)
		{
			process_poll(&irq_clear_process);
			return;
		}

		interrupt_status = ReadRegister(RF231_IRQ_STATUS);
  
		// Check if the receiver buffer is full 
		if (rf231_rx_buffer_head == (rf231_rx_buffer_tail + 1) % RF231_CONF_RX_BUFFERS)
		{
			log_error("Radio reception buffer is full");
			return;
		}

		if (interrupt_status & RF231_RX_START_MASK)
		{
			delay_us(tReadED);
			rf231_rx_buffer[rf231_rx_buffer_tail].tx_ed = GetED();
		}
		if (interrupt_status & RF231_TRX_END_MASK)
		{
			radio_status = RF231_GetStatus();	
			if (radio_status == RF231_TRX_STATUS__RX_ON)
			{
				if (is_crc_valid())
				{
					ReadFrame(&rf231_rx_buffer[rf231_rx_buffer_tail]);	

					// If the packet length is not correct, then discard it
					// Note: currently RF231_MAX_FRAME_LENGTH is set to 127, but data length should be less than 125, because we use automatically FCS
					if (rf231_rx_buffer[rf231_rx_buffer_tail].length > RF231_MAX_FRAME_LENGTH - 2 || rf231_rx_buffer[rf231_rx_buffer_tail].length < RF231_MIN_FRAME_LENGTH) 
					{
						rf231_rx_buffer[rf231_rx_buffer_tail].length = 0;
						return;
					} 

					uint8_t *buf_ptr = rf231_rx_buffer[rf231_rx_buffer_tail].data;
					uint8_t data_type;

					buf_ptr += FCF;
					memcpy(&data_type, buf_ptr, sizeof(uint8_t));
					buf_ptr += sizeof(uint8_t);

					// Handle TIME_SYNCH_BEACON immediately
					if (data_type == TIME_SYNCH_BEACON && rf231_rx_buffer[rf231_rx_buffer_tail].length == TIME_SYNCH_BEACON_LENGTH + 1)
					{
						memcpy(&network_time, buf_ptr, sizeof(rtimer_clock_t));
						buf_ptr += sizeof(rtimer_clock_t);
						memcpy(&time_synch_seq_no, buf_ptr, sizeof(uint16_t));
						buf_ptr += sizeof(uint16_t);

						network_time_set(network_time);
						current_slot_start = network_time - (network_time % PRKMCS_TIMESLOT_LENGTH);
						ASN_INIT(current_asn, 0, network_time / PRKMCS_TIMESLOT_LENGTH);

						building_signalmap = 0;

						// Consecutively processing five packets to make sure motes are synchronized
						if (time_synch_seq_no >= 5)
						{
							prkmcs_is_synchronized = 1;
						}

						rf231_rx_buffer[rf231_rx_buffer_tail].length = 0;
						// Note that slot scheduling should be done after parameter settings
						schedule_next_slot(&slot_operation_timer);
						//log_debug("Synch %u", time_synch_seq_no);
					}
					// Get ED after reception for other packets
					else if (data_type == TEST_PACKET || data_type == CONTROL_PACKET || data_type == DATA_PACKET)
					{
						InitED();
					}
				}
			}
			else if (radio_status == RF231_TRX_STATUS__PLL_ON || radio_status == RF231_TRX_STATUS__TX_ARET_ON)
			{
				start_rx();
			}
		}
		if (interrupt_status & RF231_CCA_ED_DONE_MASK)
		{
			// Distinguish ED done or CCA done
			if (ED_Init == 0)
			{
				return;
			}
			else
			{
				rf231_rx_buffer[rf231_rx_buffer_tail].noise_ed = GetED();
				rf231_rx_buffer_tail = (rf231_rx_buffer_tail + 1) % RF231_CONF_RX_BUFFERS;
				ED_Init = 0;
				process_post(&rx_process, packet_in_buffer, NULL);
			}
		}
	}
}

PROCESS_THREAD(irq_clear_process, ev, data)
{
	PROCESS_BEGIN();
	while (1)
	{
		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

		ReadRegister(RF231_IRQ_STATUS);
	}
	PROCESS_END();
}

PROCESS_THREAD(rx_process, ev, data)
{
	PROCESS_BEGIN();
	while (1)
	{
		PROCESS_YIELD_UNTIL(ev == packet_in_buffer);

		if (rf231_rx_buffer_head == rf231_rx_buffer_tail)
		{
			log_error("Radio reception buffer is empty");
		}
		else
		{

#ifdef PRKMCS_ENABLE
			prkmcs_receive();
#endif // PRKMCS_ENABLE		

			flush_rx_buffer();
		}
	}
	PROCESS_END();
}