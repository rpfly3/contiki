/*
 * @Author: Pengfei Ren (rpfly0818@gmail.com)
 * @Updated: 02/11/2016
 * @Description: Packets (including data packets and control packets) transmission scheduling
 */

#include "core/net/mac/prk-mcs/prkmcs.h"

PROCESS_NAME(irq_clear_process);
PROCESS_NAME(rx_process);
PROCESS_NAME(control_signaling_process);
PROCESS_NAME(heart_beat_process);

static uint16_t prkmcs_data_sequence_no[LOCAL_LINKS_SIZE];
static uint16_t prkmcs_control_packet_seq_no = 1, prkmcs_data_packet_seq_no;

float tx_power;
/* initialize PRK multi-channel scheduling module */
void prkmcsInit(void) 
{
	topologyInit();
	protocolSignalingInit();
	initController();
	linkestimateInit();
	onama_init();
	signalMapInit();
	time_synch_init();
	test_init();

	process_start(&irq_clear_process, NULL);
	process_start(&rx_process, NULL);
	process_start(&control_signaling_process, NULL);
	process_start(&heart_beat_process, NULL);

	// Initialize data packet sequence number
	for (uint8_t i = 0; i < localLinksSize; ++i)
	{
		prkmcs_data_sequence_no[i] = 1;
	}

	prkmcs_slot_operation_start();
	log_info("PRKMCS is enabled");
}

void prkmcs_send_ctrl() 
{
	SetPower(RF231_TX_PWR_MAX);

	uint8_t *buf_ptr = rf231_tx_buffer;
	uint8_t data_type = CONTROL_PACKET;

	buf_ptr += FCF;
	memcpy(buf_ptr, &data_type, sizeof(uint8_t));
	buf_ptr += sizeof(uint8_t);
	memcpy(buf_ptr, &node_addr, sizeof(linkaddr_t));
	buf_ptr += sizeof(linkaddr_t);
	memcpy(buf_ptr, &prkmcs_control_packet_seq_no, sizeof(uint16_t));
	buf_ptr += sizeof(uint16_t);

	for (uint8_t i = 0; i < CONTROL_PACKET_ER_SEG_NUM; ++i)
	{
		prepareERSegment(buf_ptr);
		buf_ptr += ER_SEGMENT_LENGTH;
	}

	for (uint8_t i = 0; i < CONTROL_PACKET_SM_SEG_NUM; ++i)
	{
		prepareSMSegment(buf_ptr);
		buf_ptr += SM_SEGMENT_LENGTH;
	}

	rf231_tx_buffer_size = FCF + CONTROL_PACKET_LENGTH + CHECKSUM_LEN;
	//rf231_csma_send();
	rf231_send();
	printf("Control %u\r\n", prkmcs_control_packet_seq_no);
	++prkmcs_control_packet_seq_no;
}

void prkmcs_send_data() 
{
	SetPower(RF231_TX_PWR_MAX);

	uint8_t local_index = findLocalIndex(my_link_index);
	prkmcs_data_packet_seq_no = prkmcs_data_sequence_no[local_index];

	uint8_t *buf_ptr = rf231_tx_buffer;
	uint8_t data_type = DATA_PACKET;

	buf_ptr += FCF;
	memcpy(buf_ptr, &data_type, sizeof(uint8_t));
	buf_ptr += sizeof(uint8_t);
	memcpy(buf_ptr, &pair_addr, sizeof(linkaddr_t));
	buf_ptr += sizeof(linkaddr_t);
	memcpy(buf_ptr, &node_addr, sizeof(linkaddr_t));
	buf_ptr += sizeof(linkaddr_t);
	memcpy(buf_ptr, &prkmcs_data_packet_seq_no, sizeof(uint16_t));
	buf_ptr += sizeof(uint16_t);

	for (uint8_t i = 0; i < CONTROL_PACKET_ER_SEG_NUM; ++i)
	{
		prepareERSegment(buf_ptr);
		buf_ptr += ER_SEGMENT_LENGTH;
	}

	for (uint8_t i = 0; i < CONTROL_PACKET_SM_SEG_NUM; ++i)
	{
		prepareSMSegment(buf_ptr);
		buf_ptr += SM_SEGMENT_LENGTH;
	}
	rf231_tx_buffer_size = FCF + DATA_PACKET_LENGTH + CHECKSUM_LEN;
	//rf231_csma_send();
	rf231_send();
	printf("Data %u: for %u\r\n", prkmcs_data_packet_seq_no, pair_addr);
	++prkmcs_data_sequence_no[local_index];
}

/* process pending input packets */
void prkmcs_receive() 
{
	uint8_t *buf_ptr = rf231_rx_buffer[rf231_rx_buffer_head].data;
	uint8_t data_type;

	buf_ptr += FCF; // remove fcf byte
	memcpy(&data_type, buf_ptr, sizeof(uint8_t));
	buf_ptr += sizeof(uint8_t);

	if (data_type == TEST_PACKET && (rf231_rx_buffer[rf231_rx_buffer_head].length == FCF + TEST_PACKET_LENGTH))
	{
		if (building_signalmap)
		{
			test_receive(buf_ptr);
		}
		else
		{
			// do nothing
		}
	}
	else if (data_type == CONTROL_PACKET && (rf231_rx_buffer[rf231_rx_buffer_head].length == FCF + CONTROL_PACKET_LENGTH))
	{
		linkaddr_t sender;
		uint16_t packet_seq_no;

		memcpy(&sender, buf_ptr, sizeof(linkaddr_t));
		buf_ptr += sizeof(linkaddr_t);

		memcpy(&packet_seq_no, buf_ptr, sizeof(uint16_t));
		buf_ptr += sizeof(uint16_t);
		printf("Received Control %u: from %u\r\n", packet_seq_no, sender);

		for (uint8_t i = 0; i < DATA_PACKET_ER_SEG_NUM; ++i)
		{
			er_receive(buf_ptr);
			buf_ptr += ER_SEGMENT_LENGTH;
		}
		
		for (uint8_t i = 0; i < DATA_PACKET_SM_SEG_NUM; ++i)
		{
			sm_receive(buf_ptr);
			buf_ptr += SM_SEGMENT_LENGTH;
		}

		// Compute inbound gain only when EDs are valid
		if (rf231_rx_buffer[rf231_rx_buffer_head].tx_ed != INVALID_ED && rf231_rx_buffer[rf231_rx_buffer_head].noise_ed != INVALID_ED 
			&& rf231_rx_buffer[rf231_rx_buffer_head].tx_ed > rf231_rx_buffer[rf231_rx_buffer_head].noise_ed)
		{
			float inbound_gain = computeInboundGain(RF231_TX_PWR_MAX, rf231_rx_buffer[rf231_rx_buffer_head].tx_ed, rf231_rx_buffer[rf231_rx_buffer_head].noise_ed);
			updateInboundGain(sender, inbound_gain);
		}
		else
		{
			// do nothing
		}
	}
	else if (data_type == DATA_PACKET && (rf231_rx_buffer[rf231_rx_buffer_head].length == FCF + DATA_PACKET_LENGTH))
	{
		linkaddr_t receiver;
		memcpy(&receiver, buf_ptr, sizeof(linkaddr_t));
		buf_ptr += sizeof(linkaddr_t);

		linkaddr_t sender;
		memcpy(&sender, buf_ptr, sizeof(linkaddr_t));
		buf_ptr += sizeof(linkaddr_t);

		uint16_t packet_seq_no;
		memcpy(&packet_seq_no, buf_ptr, sizeof(uint16_t));
		buf_ptr += sizeof(uint16_t);

		if (receiver == node_addr)
		{
			printf("Received Data %u: from %u\r\n", packet_seq_no, sender);
			updateLinkQuality(sender, packet_seq_no);

			for (uint8_t i = 0; i < DATA_PACKET_ER_SEG_NUM; ++i)
			{
				er_receive(buf_ptr);
				buf_ptr += ER_SEGMENT_LENGTH;
			}
		
			for (uint8_t i = 0; i < DATA_PACKET_SM_SEG_NUM; ++i)
			{
				sm_receive(buf_ptr);
				buf_ptr += SM_SEGMENT_LENGTH;
			}
		}
		else
		{
			// do nothing
		}
	}

	return;
}
