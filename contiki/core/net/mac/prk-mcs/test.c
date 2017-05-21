#include "core/net/mac/prk-mcs/prkmcs.h"

static uint16_t test_seq_no;
void test_init()
{
	test_seq_no = 0;
}

/* | type | sender | sequence no | # of SM entry | SM entry | */

void test_send()
{
	SetPower(RF231_TX_PWR_MAX);
	uint8_t *buf_ptr = rf231_tx_buffer;
	uint8_t *num_sm = NULL;
	uint8_t data_type = TEST_PACKET;
	buf_ptr += FCF;
	memcpy(buf_ptr, &data_type, sizeof(uint8_t));
	buf_ptr += sizeof(uint8_t);
	memcpy(buf_ptr, &node_addr, sizeof(linkaddr_t));
	buf_ptr += sizeof(linkaddr_t);
	memcpy(buf_ptr, &test_seq_no, sizeof(uint16_t));
	buf_ptr += sizeof(uint16_t);
	num_sm = buf_ptr;
	buf_ptr += sizeof(uint8_t);

	*num_sm = 0;
	for (uint8_t i = 0; i < MAX_SM_NUM; ++i)
	{
		if (sm_sending_index < valid_sm_entry_size)
		{
			prepareSMSegment(buf_ptr);
			buf_ptr += SM_SEGMENT_LENGTH;
			++(*num_sm);			
		}
		else
		{
			sm_sending_index = 0;
			break;
		}
	}
	rf231_tx_buffer_size = buf_ptr - rf231_tx_buffer + CHECKSUM_LEN;

	rf231_send();
	printf("Test %u\r\n", test_seq_no);
	++test_seq_no;
}

void test_receive(uint8_t *ptr)
{
	linkaddr_t sender;
	uint16_t sequence_no;
	uint8_t num_entry;

	memcpy(&sender, ptr, sizeof(linkaddr_t));
	ptr += sizeof(linkaddr_t);
	memcpy(&sequence_no, ptr, sizeof(uint16_t));
	ptr += sizeof(uint16_t);
	memcpy(&num_entry, ptr, sizeof(uint8_t));
	ptr += sizeof(uint8_t);

	printf("Received Test %u from %u: R-ED %u\r\n", sequence_no, sender, rf231_rx_buffer[rf231_rx_buffer_head].tx_ed);
	if (sequence_no % 2 == 1)
	{
		//printf("Received Test %u from %u: R-ED %u\r\n", sequence_no, sender, rf231_rx_buffer[rf231_rx_buffer_head].tx_ed);
		// Compute inbound gain only when EDs are valid
		if ((rf231_rx_buffer[rf231_rx_buffer_head].tx_ed != INVALID_ED) && (rf231_rx_buffer[rf231_rx_buffer_head].noise_ed != INVALID_ED)
			&& (rf231_rx_buffer[rf231_rx_buffer_head].tx_ed >= 0))
		{
			
			updateInboundED(sender, rf231_rx_buffer[rf231_rx_buffer_head].tx_ed);
		}
		else
		{
			// do nothing
		}
	}
	else
	{
		// do nothing
	}

	for (uint8_t i = 0; i < num_entry; ++i)
	{
		sm_receive(ptr, sender);
		ptr += SM_SEGMENT_LENGTH;
	}
}
