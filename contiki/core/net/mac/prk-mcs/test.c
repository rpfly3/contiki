#include "core/net/mac/prk-mcs/prkmcs.h"

static uint16_t test_seq_no;
void test_init()
{
	test_seq_no = 0;
}

/* | type | sender | sequence no | */
void test_send()
{
	SetPower(RF231_TX_PWR_MAX);

	uint8_t *buf_ptr = rf231_tx_buffer;
	uint8_t data_type = TEST_PACKET;
	buf_ptr += FCF;
	memcpy(buf_ptr, &data_type, sizeof(uint8_t));
	buf_ptr += sizeof(uint8_t);
	memcpy(buf_ptr, &node_addr, sizeof(linkaddr_t));
	buf_ptr += sizeof(linkaddr_t);
	memcpy(buf_ptr, &test_seq_no, sizeof(uint16_t));
	buf_ptr += sizeof(uint16_t);

	rf231_tx_buffer_size = FCF + TEST_PACKET_LENGTH + CHECKSUM_LEN;

	//rf231_csma_send();
	rf231_send();
	++test_seq_no;
}

void test_receive(uint8_t *ptr)
{
	linkaddr_t sender;
	uint16_t sequence_no;

	memcpy(&sender, ptr, sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);
	memcpy(&sequence_no, ptr, sizeof(uint16_t));
	ptr += sizeof(uint16_t);

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
