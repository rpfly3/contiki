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
process_event_t control_signaling;

/*-------------------------------------------------------------------------------*/
static void rf231_reset();
static void rf231_rx_on();
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
		status = RF231_GetStatus();
	} while (status != RF231_TRX_STATUS__RX_ON);
}

// put it in TRX_OFF status from active states
void put_trx_off()
{
	uint8_t new_status, original_status = RF231_GetStatus();
	if (original_status == RF231_TRX_STATUS__TRX_OFF)
	{
		return;
	}
	else if (original_status != RF231_TRX_STATUS__RX_ON && 
		original_status != RF231_TRX_STATUS__BUSY_RX)
	{
		log_error("Wrong status %u", original_status);
		return;
	}

	RF231_SLP_TRClr();
	RF231_StateCtrl(RF231_TRX_STATE__FORCE_TRX_OFF);
	delay_us(tTR12); 
	
	do
	{
		new_status = RF231_GetStatus();
	} while (new_status != RF231_TRX_STATUS__TRX_OFF);
}

// Transit state from PLL_ON to RX_ON and be ready for reception
void start_rx()
{
	uint8_t new_status, original_status = RF231_GetStatus();
	if (original_status == RF231_TRX_STATUS__RX_ON || original_status == RF231_TRX_STATUS__BUSY_RX)
	{
		// already in rx status
	}
	else if (original_status != RF231_TRX_STATUS__PLL_ON && 
		original_status != RF231_TRX_STATUS__BUSY_TX &&
		original_status != RF231_TRX_STATUS__TX_ARET_ON &&
		original_status != RF231_TRX_STATUS__BUSY_TX_ARET)
	{
		log_error("Wrong status %u", original_status);
	}
	else
	{

		RF231_StateCtrl(RF231_TRX_STATE__RX_ON);
		delay_us(tTR8);

		do
		{
			new_status = RF231_GetStatus();
		} while (new_status != RF231_TRX_STATUS__RX_ON);
	}
	return;
}

// Transit state from RX_ON to PLL_ON and be ready for transmission
void start_tx() 
{
	
	uint8_t new_status, original_status = RF231_GetStatus();
	if (original_status == RF231_TRX_STATUS__PLL_ON || original_status == RF231_TRX_STATUS__BUSY_TX)
	{
		// already in tx status
	}
	else if (original_status != RF231_TRX_STATUS__RX_ON && original_status != RF231_TRX_STATUS__BUSY_RX)
	{
		log_error("Wrong status %u", original_status);
	}
	else
	{
		RF231_StateCtrl(RF231_TRX_STATE__FORCE_PLL_ON);
		delay_us(tTR9);

		do
		{
			new_status = RF231_GetStatus();
		} while (new_status != RF231_TRX_STATUS__PLL_ON);
	}
	return;
}

/*==================== Radio State Check =================== */
static bool is_sleeping()
{
	bool sleeping = !RF231_GetSLP_TR();
	return sleeping;
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

uint8_t GetPower()
{
	uint8_t power = ReadRegister(RF231_PHY_TX_PWR);
	power &= 0x0F;
	return power;
}

void SetPower(uint8_t power)
{
	if (power > RF231_TX_PWR_MIN) 
	{
		log_error("Invalid power argument");
	}
	else
	{
		uint8_t Reg = ReadRegister(RF231_PHY_TX_PWR);
		uint8_t original_power = Reg & 0x0F;
		if (original_power != power)
		{
			Reg &= 0xF0;
			Reg |= power;
			WriteRegister(RF231_PHY_TX_PWR, Reg);
			if (GetPower() != power)
			{
				log_error("Power not set");
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			// target power set
		}
	}
	return;
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
	}
	else
	{
		Reg = ReadRegister(RF231_PHY_CC_CCA);
		original_channel = Reg & 0x1F;

		if (original_channel != channel)
		{
			Reg &= 0xE0;
			Reg |= channel;
			WriteRegister(RF231_PHY_CC_CCA, Reg);

			uint8_t status = RF231_GetStatus();
			if (status == RF231_TRX_STATUS__RX_ON || status == RF231_TRX_STATUS__PLL_ON || status == RF231_TRX_STATUS__TX_ARET_ON || status == RF231_TRX_STATUS__RX_AACK_ON)
			{
				delay_us(tTR20);
			}
			else
			{
				// do nothing
			}

			if (GetChannel() != channel)
			{
				log_error("Channel not set");
			}
			else
			{
				// channel is set
			}
		}
		else
		{
			// in target channel
		}
	}
	return;
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
	}
	else
	{
		Reg = ReadRegister(RF231_PHY_RX_SYN);
		original_rxthres = Reg & 0x0F;

		if (original_rxthres != rxthres)
		{
			Reg &= 0xF0;
			Reg |= rxthres;
			WriteRegister(RF231_PHY_RX_SYN, Reg);
			if (GetRxSensitivity() != rxthres)
			{
				log_error("Rx sensitivity not set");
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			// with target rx sensitivity
		}
	}
	return;
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

void InitED()
{
	WriteRegister(RF231_PHY_ED_LEVEL, 0);
}

uint8_t GetED()
{
	uint8_t ed = INVALID_ED;
	ed = ReadRegister(RF231_PHY_ED_LEVEL);
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
	}
	else
	{
		uint8_t Reg = ReadRegister(RF231_PHY_CCA_THRES);
		uint8_t original_thres = Reg & 0x0F;
		if (original_thres != threshold)
		{
			Reg &= 0xC0;
			Reg |= threshold;
			WriteRegister(RF231_PHY_CCA_THRES, Reg);
			if (GetEDThreshold() != threshold)
			{
				log_error("ED threshold not set");
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			// with target ED threshold
		}
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
	}
	else
	{
		uint8_t Reg, original_mode;
		Reg = ReadRegister(RF231_PHY_CC_CCA);
		original_mode = Reg & 0x60;
		if (original_mode != CCA)
		{
			Reg &= 0x1F;
			Reg |= CCA;
			WriteRegister(RF231_PHY_CC_CCA, Reg);
			if (GetCCAMode() != CCA)
			{
				log_error("CCA mode not set");
			}
			else
			{
				// do nothing
			}
		}
		else
		{
			// already set	
		}
	}
	return;
}

void InitCCARequest()
{
	uint8_t Reg = ReadRegister(RF231_PHY_CC_CCA);
	WriteRegister(RF231_PHY_CC_CCA, 0x80 | Reg);
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
	else
	{
		// do nothing
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

/* Perform a CCA to find out if there is a packet in the air or not*/
uint8_t getCCA(uint8_t CCA, uint8_t threshold) 
{
	uint8_t cca = 0;

	// Wait for radio to finish previous operation
	while (is_busy())
		;	
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
	}
	else
	{
		WriteFrame(rf231_tx_buffer, rf231_tx_buffer_size);
		RF231_SLP_TRSet();
		RF231_SLP_TRClr();
	}
}

rtimer_clock_t packet_reception_time;
void time_synch_process()
{
	uint8_t *buf_ptr = rf231_rx_buffer[rf231_rx_buffer_tail].data;
	uint8_t length = rf231_rx_buffer[rf231_rx_buffer_tail].length;
	uint8_t data_type;

	buf_ptr += FCF;
	memcpy(&data_type, buf_ptr, sizeof(uint8_t));
	buf_ptr += sizeof(uint8_t);

	// Handle TIME_SYNCH_BEACON immediately
	if (data_type == TIME_SYNCH_BEACON && length == TIME_SYNCH_BEACON_LENGTH + 1)
	{
		rtimer_clock_t network_time;
		memcpy(&network_time, buf_ptr, sizeof(rtimer_clock_t));
		buf_ptr += sizeof(rtimer_clock_t);
		uint16_t time_synch_seq_no;
		memcpy(&time_synch_seq_no, buf_ptr, sizeof(uint16_t));
		buf_ptr += sizeof(uint16_t);

		// synchronize time and reschedule
		network_time_set(network_time);
		current_slot_start = network_time - (network_time % PRKMCS_TIMESLOT_LENGTH);
		ASN_INIT(current_asn, 0, network_time / PRKMCS_TIMESLOT_LENGTH);

		printf("TS Packet %u at %u\r\n", time_synch_seq_no, packet_reception_time);
		// Consecutively processing five packets to make sure motes are synchronized
		if (time_synch_seq_no >= 5)
		{
			prkmcs_is_synchronized = 1;
		}

		schedule_next_slot(&slot_operation_timer);
	}
	else
	{
		// Check if the receiver buffer is full
		if (rf231_rx_buffer_head == (rf231_rx_buffer_tail + 1) % RF231_CONF_RX_BUFFERS)
		{
			if(data_type == DATA_PACKET)
			{
				log_error("A DATA packet is dropped");
			}
			else if(data_type == CONTROL_PACKET)
			{
				log_error("A CONTROL packet is dropped");
			}
			else if(data_type == TEST_PACKET)
			{
				log_error("A TEST packet is dropped");
			}
		}
		else
		{
			rf231_rx_buffer_tail = (rf231_rx_buffer_tail + 1) % RF231_CONF_RX_BUFFERS;
		}

		process_poll(&rx_process);
	}
}

// only change the radio status in one place
// only read radio register in ISR
uint8_t interrupt_source;
uint8_t radio_status;
void EXTI1_IRQHandler(void)
{			
	if (EXTI_GetITStatus(EXTI_Line1) != RESET) 
	{
		EXTI_ClearITPendingBit(EXTI_Line1);
		interrupt_source = ReadRegister(RF231_IRQ_STATUS);
		if (interrupt_source & RF231_TRX_END_MASK)
		{
			packet_reception_time = RTIMER_NOW();
			radio_status = RF231_GetStatus();
			if (radio_status == RF231_TRX_STATUS__RX_ON ||
				radio_status == RF231_TRX_STATUS__BUSY_RX)
			{
				if (is_crc_valid())
				{
					ReadFrame(&rf231_rx_buffer[rf231_rx_buffer_tail]);
					if(rf231_rx_buffer[rf231_rx_buffer_tail].length)
					{
						rf231_rx_buffer[rf231_rx_buffer_tail].tx_ed = GetED();
						rf231_rx_buffer[rf231_rx_buffer_tail].noise_ed = 0;
						time_synch_process();
					}
					else
					{
						// invalid packet -- do nothing
					}
				}
				else
				{
					// invalid packet -- do nothing
				}
			}
			else if(radio_status == RF231_TRX_STATUS__PLL_ON ||
					radio_status == RF231_TRX_STATUS__BUSY_TX)
			{
				//start_rx();
				process_poll(&irq_clear_process);
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
		start_rx();
	}
	PROCESS_END();
}

PROCESS_THREAD(rx_process, ev, data)
{
	PROCESS_BEGIN();
	while (1)
	{

		PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
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
