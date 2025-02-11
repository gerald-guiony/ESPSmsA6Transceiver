// https://github.com/RoyalAliceAcademyOfSciences/PhoneOverIp

#ifndef SMS_PDU_H_
#define SMS_PDU_H_

//#include <time.h>

namespace a6gsm {

/*
 * Decode an SMS message. Output the decoded message into the sms text buffer.
 * Returns the length of the SMS dencoded message or a negative number in
 * case encoding failed (for example provided output buffer has not enough
 * space).
 */
int pdu_decode(const unsigned char* pdu, int pdu_len,
		   /*time_t* sms_time,*/
		   char* phone_number, int phone_number_size,
		   char* text, int text_size);

}


#endif   // SMS_SMS_H_
