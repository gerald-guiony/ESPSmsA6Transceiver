#include <Arduino.h>

#include "Pdu.h"


namespace a6gsm {

enum {
	BITMASK_7BITS = 0x7F,
	BITMASK_8BITS = 0xFF,
	BITMASK_HIGH_4BITS = 0xF0,
	BITMASK_LOW_4BITS = 0x0F,

	TYPE_OF_ADDRESS_INTERNATIONAL_PHONE = 0x91,
	TYPE_OF_ADDRESS_NATIONAL_SUBSCRIBER = 0xC8,

	SMS_DELIVER_ONE_MESSAGE = 0x04,
	SMS_SUBMIT			  	= 0x11,

	SMS_MAX_7BIT_TEXT_LENGTH  = 160,
};


// Swap decimal digits of a number (e.g. 12 -> 21).
static unsigned char SwapDecimalNibble (const unsigned char x)
{
	return (x / 16) + ((x % 16) * 10);
}


// Decode PDU message by splitting 8 bit encoded buffer into 7 bit ASCII
// characters.
static int DecodePDUMessage (const unsigned char* buffer, int buffer_length, char* output_sms_text, int sms_text_length)
{
	int output_text_length = 0;
	if (buffer_length > 0)
		output_sms_text[output_text_length++] = BITMASK_7BITS & buffer[0];

	int carry_on_bits = 1;
	int i = 1;
	for (; i < buffer_length; ++i) {

		output_sms_text[output_text_length++] = BITMASK_7BITS &	((buffer[i] << carry_on_bits) | (buffer[i - 1] >> (8 - carry_on_bits)));

		if (output_text_length == sms_text_length) break;

		carry_on_bits++;

		if (carry_on_bits == 8) {
			carry_on_bits = 1;
			output_sms_text[output_text_length++] = buffer[i] & BITMASK_7BITS;
			if (output_text_length == sms_text_length) break;
		}

	}
	if (output_text_length < sms_text_length)  // Add last remainder.
		output_sms_text[output_text_length++] =	buffer[i - 1] >> (8 - carry_on_bits);

	return output_text_length;
}

// Decode a digit based phone number for SMS based format.
static int DecodePhoneNumber (const unsigned char* buffer, int phone_number_length, char* output_phone_number)
{
	int i = 0;
	for (; i < phone_number_length; ++i) {
		if (i % 2 == 0)
			output_phone_number[i] = (buffer[i / 2] & BITMASK_LOW_4BITS) + '0';
		else
			output_phone_number[i] = ((buffer[i / 2] & BITMASK_HIGH_4BITS) >> 4) + '0';
	}
	output_phone_number[phone_number_length] = '\0';  // Terminate C string.
	return phone_number_length;
}

int pdu_decode (const unsigned char* buffer, int buffer_length,
				// time_t* output_sms_time,
				char* output_sender_phone_number, int sender_phone_number_size,
				char* output_sms_text, int sms_text_size)
{

	if (buffer_length <= 0)
		return -1;

	const int sms_deliver_start = 1 + buffer[0];
	if (sms_deliver_start + 1 > buffer_length) return -1;
	if ((buffer[sms_deliver_start] & SMS_DELIVER_ONE_MESSAGE) != SMS_DELIVER_ONE_MESSAGE) return -1;

	const int sender_number_length = buffer[sms_deliver_start + 1];
	if (sender_number_length + 1 > sender_phone_number_size) return -1;  // Buffer too small to hold decoded phone number.

	// const int sender_type_of_address = buffer[sms_deliver_start + 2];
	DecodePhoneNumber(buffer + sms_deliver_start + 3, sender_number_length,  output_sender_phone_number);

	const int sms_pid_start = sms_deliver_start + 3 + (buffer[sms_deliver_start + 1] + 1) / 2;
/*
	// Decode timestamp.
	struct tm sms_broken_time;
	sms_broken_time.tm_year = 100 + SwapDecimalNibble(buffer[sms_pid_start + 2]);
	sms_broken_time.tm_mon  = SwapDecimalNibble(buffer[sms_pid_start + 3]) - 1;
	sms_broken_time.tm_mday = SwapDecimalNibble(buffer[sms_pid_start + 4]);
	sms_broken_time.tm_hour = SwapDecimalNibble(buffer[sms_pid_start + 5]);
	sms_broken_time.tm_min  = SwapDecimalNibble(buffer[sms_pid_start + 6]);
	sms_broken_time.tm_sec  = SwapDecimalNibble(buffer[sms_pid_start + 7]);
	const char gmt_offset   = SwapDecimalNibble(buffer[sms_pid_start + 8]);
	// GMT offset is expressed in 15 minutes increments.
	(*output_sms_time) = mktime(&sms_broken_time) - gmt_offset * 15 * 60;
*/
	const int sms_start = sms_pid_start + 2 + 7;
	if (sms_start + 1 > buffer_length) return -1;  // Invalid input buffer.

	const int output_sms_text_length = buffer[sms_start];
	if (sms_text_size < output_sms_text_length) return -1;  // Cannot hold decoded buffer.

	const int decoded_sms_text_size = DecodePDUMessage(buffer + sms_start + 1, buffer_length - (sms_start + 1),
							   output_sms_text, output_sms_text_length);

	if (decoded_sms_text_size != output_sms_text_length) return -1;  // Decoder length is not as expected.

	// Add a C string end.
	if (output_sms_text_length < sms_text_size)
		output_sms_text[output_sms_text_length] = 0;
	else
		output_sms_text[sms_text_size-1] = 0;

	return output_sms_text_length;
}

}

