//************************************************************************************************************************
// A6lib.cpp
// Creation date: June, 2017
// Author: Gerald Guiony
// Forked repository: https://github.com/skorokithakis/A6lib
//************************************************************************************************************************

#include <Arduino.h>

#include <EspBoard.h>
#include <Print/Logger.h>
/*
template<class T> inline Print & operator <<(Print & printer, T arg) { printer.print(arg); return printer; }
#ifdef DEBUG
#	define Log(s)	(Serial << s)
#	define Logln(s) (Serial << s << F("\n"))
#else
#	define Log(s)
#	define Logln(s)
#endif
*/

#include "A6lib.h"
#include "Pdu.h"


#if defined (ESP8266) || defined (ESP32)
#	define min(a,b)	(a)>(b)?(b):(a)
#	define max(a,b)	(a)>(b)?(a):(b)
#endif


namespace a6gsm {


/////////////////////////////////////////////
// Public methods.
//
A6lib::A6lib (SoftwareSerial * A6connection) {
	A6conn = A6connection;
	A6conn->setTimeout(100);
//	A6conn->begin(115200);
}

A6lib::~A6lib() {
}

// Initialize the software serial connection and change the baud rate from the
// default (autodetected) to the desired speed.
byte A6lib::begin (long baudRate) {

	A6conn->flush();

	if (A6_OK != setRate(baudRate)) return A6_NOTOK;

	// Factory reset.
//   if (A6_OK != A6command("AT&F", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// Echo off.
	if (A6_OK != A6command("ATE0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

//	A6command("AT+COPS=?", "OK", "yy", 20000, 2, NULL);
	// +COPS: (2,"OrangeF","OrangeF","20801"),(3,"SFRFR","SFRFR","20810"),(3,"BouyguesFR","BouyguesFR","20820")

//	A6command("AT+COPS?", "OK", "yy", 20000, 2, NULL);
	// +COPS: 1,2,"20801"

	// Firmware version.
//	if (A6_OK != A6command("ATI", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// List of supported baudrate.
//	if (A6_OK != A6command("AT+IPR=?", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// Enable network registration unsolicited result code +CREG: <stat>
	if (A6_OK != A6command("AT+CREG=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// Set SMS to text mode.
	if (A6_OK != A6command("AT+CMGF=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// Switch audio to headset.
//	enableSpeaker(0);

	// Set caller ID on.
	if (A6_OK != A6command("AT+CLIP=1", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// No keypad event reporting, no display event reporting, no indicator event reporting.
	// Permanent, even if A6 is rebooted
	if (A6_OK != A6command("AT+CMER=3,0,0,0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// Turn SMS indicators off.
//	if (A6_OK != A6command("AT+CNMI=1,0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;
	if (A6_OK != A6command("AT+CNMI=0,0", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL)) return A6_NOTOK;

	// Set SMS storage to the GSM modem. If this doesn't work for you, try changing the command to:
	// "AT+CPMS=SM,SM,SM"
	if (A6_OK != A6command("AT+CPMS=ME,ME,ME", "OK", "yy", A6_CMD_TIMEOUT, 3, NULL))
		// This may sometimes fail, in which case the modem needs to be
		// rebooted.
	{
//		if (A6_OK != A6command("AT+CPMS=SM,SM,SM", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL))
		{
			return A6_FAILURE;
		}
	}

	// Set SMS character set.
	setSMScharset("UCS2");

	return A6_OK;
}

// Wait registration state (unsolicited result code +CREG: <stat>)
byte A6lib::waitRegistrationState (int state) {
	char buffer [12];
	sprintf (buffer, "+CREG: %d", state);
	return A6waitFor(buffer, "yy", 20000, NULL);
}

// Sample
//
// +CIEV: "MESSAGE",1
//
//
//
// +CMT: ,25
//
// 07913396050046F6240B913326991487F100009190519151818006C3777DFCAE03
//
byte A6lib::waitUnsolicitedPduSMS (std::function <void (SMSmessage &)> onUnsolicitedSMSReceived) {
	String response = "";
	if (A6_OK == A6waitFor("+CMT: ,", "yy", 20000, &response)) {
		// Parse the response if it contains a valid +CMT.
		int respStart = response.indexOf("+CMT:");
		if (respStart >= 0) {

			char length [5]; // The length of the message body

			// Parse the message header.
			response = response.substring(respStart);
			sscanf (response.c_str(), "+CMT: ,%s\r\n", length);

			// The rest is the pdu message, extract it.
			String pduMessage = response.substring (strlen("+CMT: ,") + strlen(length) + 2, response.length() - 2);

			Logln ("Unsolicited PDU received (body length=" << length << ") : [" << pduMessage << "]");

			unsigned char bufHex [160] = {0};
			int bufHexLen = min (160, pduMessage.length()/2);

			char strHexa [3] = {0};
			int val = 0;

			for (int i=0; i<bufHexLen; i++) {

				strHexa [0] = pduMessage [i*2];
				strHexa [1] = pduMessage [i*2 + 1];

				sscanf (strHexa, "%x", &val);
				bufHex[i] = val;
			}

			char output_sms_text			[160]   = {0};
			char output_sender_phone_number [12]	= {0};

			if (pdu_decode (bufHex, bufHexLen,
							output_sender_phone_number, sizeof (output_sender_phone_number),
							output_sms_text, sizeof (output_sms_text)) > 0) {

				Logln ("Decoded message from (" << output_sender_phone_number << "): [" << output_sms_text << "]");

				SMSmessage sms {output_sender_phone_number, "", output_sms_text};

				// Notify unsolicited SMS Received
				onUnsolicitedSMSReceived (sms);
			}

			return A6_OK;
		}
	}

	return A6_NOTOK;
}

// Turn the modem power on.
void A6lib::powerOn (int pin, std::function <void(SMSmessage &)> onUnsolicitedSMSReceived) {
	pinMode(pin, OUTPUT);						   // PWR_KEY power button,> 1.9V more than 2s to boot
	digitalWrite(pin, HIGH);

	// Waiting for the module to be initialized...
	A6conn->begin (115200);							// Default baudrate value after reboot
	waitRegistrationState (5);						// With firmware V03.03.20161229 wait network registration state : 5 (registered, roaming)
//  A6waitFor("+CIEV: READY", "yy", 14000, NULL);   // With firmware V03.06.20171127R wait ready state

	digitalWrite(pin, LOW);

	while (waitUnsolicitedPduSMS (onUnsolicitedSMSReceived) == A6_OK);
}

// Turn the modem power completely off.
bool A6lib::powerOff (int pin) {
	pinMode(pin, OUTPUT);
	digitalWrite(pin, LOW);

	// Power off command => if pin = PWR_KEY and pin is low, the modem will not restart
	return (A6_OK == A6command("AT+CPOF", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL));
}

// Dial a number.
bool A6lib::dial(String number) {
	char buffer[50];

	Logln("Dialing number...");

	sprintf(buffer, "ATD%s;", number.c_str());
	return (A6_OK == A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL));
}


// Redial the last number.
bool A6lib::redial() {
	Logln("Redialing last number...");
	return (A6_OK == A6command("AT+DLST", "OK", "CONNECT", A6_CMD_TIMEOUT, 2, NULL));
}


// Answer a call.
bool A6lib::answer() {
	return (A6_OK == A6command("ATA", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL));
}


// Hang up the phone.
bool A6lib::hangUp() {
	return (A6_OK == A6command("ATH", "OK", "yy", A6_CMD_TIMEOUT, 2, NULL));
}


// Check whether there is an active call.
callInfo A6lib::checkCallStatus() {
	char number[50];
	String response = "";
	uint32_t respStart = 0;
	callInfo cinfo = (const struct callInfo) {
		0
	};

	// Issue the command and wait for the response.
	A6command("AT+CLCC", "OK", "+CLCC", A6_CMD_TIMEOUT, 2, &response);

	// Parse the response if it contains a valid +CLCC.
	respStart = response.indexOf("+CLCC");
	if (respStart >= 0) {
		sscanf(response.substring(respStart).c_str(), "+CLCC: %d,%d,%d,%d,%d,\"%s\",%d", &cinfo.index, &cinfo.direction, &cinfo.state, &cinfo.mode, &cinfo.multiparty, number, &cinfo.type);
		cinfo.number = String(number);
	}

	uint8_t comma_index = cinfo.number.indexOf('"');
	if (comma_index != -1) {
		Logln("Extra comma found.");
		cinfo.number = cinfo.number.substring(0, comma_index);
	}

	return cinfo;
}


// Get the strength of the GSM signal.
int A6lib::getSignalStrength() {
	String response = "";
	uint32_t respStart = 0;
	int strength, error  = 0;

	// Issue the command and wait for the response.
	A6command("AT+CSQ", "OK", "+CSQ", A6_CMD_TIMEOUT, 2, &response);

	respStart = response.indexOf("+CSQ");
	if (respStart < 0) {
		return 0;
	}

	sscanf(response.substring(respStart).c_str(), "+CSQ: %d,%d",
		   &strength, &error);

	// Bring value range 0..31 to 0..100%, don't mind rounding..
	strength = (strength * 100) / 31;
	return strength;
}


// Get the real time from the modem. Time will be returned as yy/MM/dd,hh:mm:ss+XX
String A6lib::getRealTimeClock() {
	String response = "";

	// Issue the command and wait for the response.
	A6command("AT+CCLK?", "OK", "yy", A6_CMD_TIMEOUT, 1, &response);
	int respStart = response.indexOf("+CCLK: \"") + 8;
	response.setCharAt(respStart - 1, '-');

	return response.substring(respStart, response.indexOf("\""));
}


// Send an SMS.
byte A6lib::sendSMS(String number, String text) {
	char ctrlZ[2] = { 0x1a, 0x00 };
	char buffer[100];

	if (text.length() > 159) {
		// We can't send messages longer than 160 characters.
		return A6_NOTOK;
	}

	Log("Sending SMS to ");
	Log(number);
	Logln("...");

	sprintf(buffer, "AT+CMGS=\"%s\"", number.c_str());
	A6command(buffer, ">", "yy", A6_CMD_TIMEOUT, 2, NULL);
	delay(100);
	A6conn->println(text.c_str());
	A6conn->println(ctrlZ);
	A6conn->println();

	delay (5000);

	return A6_OK;
}


// Retrieve the number and locations of unread SMS messages.
int A6lib::getUnreadSMSLocs(int* buf, int maxItems) {
	return getSMSLocsOfType(buf, maxItems, "REC UNREAD");
}

// Retrieve the number and locations of all SMS messages.
int A6lib::getSMSLocs(int* buf, int maxItems) {
	return getSMSLocsOfType(buf, maxItems, "ALL");
}

// Retrieve the number and locations of all SMS messages.
int A6lib::getSMSLocsOfType(int* buf, int maxItems, String type) {
	String seqStart = "+CMGL: ";
	String response = "";

	String command = "AT+CMGL=\"";
	command += type;
	command += "\"";

	// Issue the command and wait for the response.
	A6command(command.c_str(), "\xff\r\nOK\r\n", "\r\nOK\r\n", A6_CMD_TIMEOUT, 2, &response);

	int seqStartLen = seqStart.length();
	int responseLen = response.length();
	int index, occurrences = 0;

	// Start looking for the +CMGL string.
	for (int i = 0; i < (responseLen - seqStartLen); i++) {
		// If we found a response and it's less than occurrences, add it.
		if (response.substring(i, i + seqStartLen) == seqStart && occurrences < maxItems) {
			// Parse the position out of the reply.
			sscanf(response.substring(i, i + 12).c_str(), "+CMGL: %u,%*s", &index);

			buf[occurrences] = index;
			occurrences++;
		}
	}
	return occurrences;
}


// Return the SMS at index.
SMSmessage A6lib::readSMS(int index) {
	String response = "";
	char buffer[30];

	// Issue the command and wait for the response.
	sprintf(buffer, "AT+CMGR=%d", index);
	A6command(buffer, "\xff\r\nOK\r\n", "\r\nOK\r\n", A6_CMD_TIMEOUT, 2, &response);

	char number[50];
	char date[50];
	char type[10];
	int respStart = 0;
	SMSmessage sms = (const struct SMSmessage) {
		"", "", ""
	};

	// Parse the response if it contains a valid +CLCC.
	respStart = response.indexOf("+CMGR");
	if (respStart >= 0) {
		// Parse the message header.
		sscanf(response.substring(respStart).c_str(), "+CMGR: \"REC %s\",\"%s\",,\"%s\"\r\n", type, number, date);
		if ((strcmp (type, "READ") == 0) || (strcmp (type, "UNREAD") == 0)) {
			sms.number = String(number);
			sms.date = String(date);
			// The rest is the message, extract it.
			sms.message = response.substring(strlen(type) + strlen(number) + strlen(date) + 24, response.length() - 8);
		}
	}
	return sms;
}

// Delete the SMS at index.
byte A6lib::deleteSMS(int index) {
	char buffer[20];
	sprintf(buffer, "AT+CMGD=%d", index);
	return A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}

// Delete SMS with special flags; example 1,4 delete all SMS from the storage area
byte A6lib::deleteSMS(int index, int flag) {
	String command = "AT+CMGD=";
	command += String(index);
	command += ",";
	command += String(flag);
	return A6command(command.c_str(), "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}

// Set the SMS charset.
byte A6lib::setSMScharset(String charset) {
	char buffer[30];

	sprintf(buffer, "AT+CSCS=\"%s\"", charset.c_str());
	return A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL);
}


// Set the volume for the speaker. level should be a number between 5 and
// 8 inclusive.
bool A6lib::setVol(byte level) {
	char buffer[30];

	// level should be between 5 and 8.
	level = min(max(level, 5), 8);
	sprintf(buffer, "AT+CLVL=%d", level);
	return (A6_OK == A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL));
}


// Enable the speaker, rather than the headphones. Pass 0 to route audio through
// headphones, 1 through speaker.
bool A6lib::enableSpeaker(byte enable) {
	char buffer[30];

	// enable should be between 0 and 1.
	enable = min(max(enable, 0), 1);
	sprintf(buffer, "AT+SNFS=%d", enable);
	return (A6_OK == A6command(buffer, "OK", "yy", A6_CMD_TIMEOUT, 2, NULL));
}



/////////////////////////////////////////////
// Private methods.
//


// Autodetect the connection rate.
long A6lib::detectRate() {
	unsigned long rate = 0;
	unsigned long rates[] = {9600, 115200};
//	unsigned long rates[] = {115200, 9600};

	// Try to autodetect the rate.
	Logln("Autodetecting connection rate...");
	for (int i = 0; i < countof(rates); i++) {
		rate = rates[i];

		A6conn->begin(rate);
		Log("Trying rate ");
		Log(rate);
		Logln("...");

		delay(200 /*100*/);

 //	   if (A6command("\rAT", "OK", "+CME", 2000, 2, NULL) == A6_OK) {
		if (A6command("AT", "OK", "+CME", 2000, 20, NULL) == A6_OK) {
			return rate;
		}
	}

	Logln("Couldn't detect the rate.");

	return A6_NOTOK;
}


// Set the A6 baud rate.
char A6lib::setRate(long baudRate) {
	int rate = 0;

	rate = detectRate();
	if (rate == A6_NOTOK) {
		return A6_NOTOK;
	}

	// The rate is already the desired rate, return.
	if (rate == baudRate) return A6_OK;

	Logln("Setting baud rate on the module...");

	// Change the rate to the requested.
	char buffer[30];
	sprintf(buffer, "AT+IPR=%d", baudRate);
	A6command(buffer, "OK", "+IPR=", A6_CMD_TIMEOUT, 3, NULL);

	Logln("Switching to the new rate...");
	// Begin the connection again at the requested rate.
	A6conn->begin(baudRate);
	Logln("Rate set.");

	return A6_OK;
}


// Read some data from the A6 in a non-blocking manner.
String A6lib::read() {
	String reply = "";
	if (A6conn->available()) {
		reply = A6conn->readString();
	}

	// XXX: Replace NULs with \xff so we can match on them.
	for (int x = 0; x < reply.length(); x++) {
		if (reply.charAt(x) == 0) {
			reply.setCharAt(x, 255);
		}
	}
	return reply;
}


// Issue a command.
byte A6lib::A6command(const char *command, const char *resp1, const char *resp2, uint16_t timeout, uint8_t repetitions, String *response) {
	byte returnValue = A6_NOTOK;
	byte count = 0;


	while (count < repetitions && returnValue != A6_OK) {
		Log("Issuing command: ");
		Logln(command);

		// Get rid of any buffered output.
		A6conn->flush();

		A6conn->write(command);
		A6conn->write('\r');

		if (A6waitFor(resp1, resp2, timeout, response) == A6_OK) {
			returnValue = A6_OK;
		} else {
			returnValue = A6_NOTOK;
		}
		count++;
	}
	return returnValue;
}


// Wait for responses.
byte A6lib::A6waitFor(const char *resp1, const char *resp2, uint16_t timeout, String *response) {
	unsigned long entry = millis();
	String reply = "";
	byte retVal = 99;
	do {
		reply += read();
#if defined (ESP8266) || defined (ESP32)
		//yield();
		delay(100);
#endif
	} while (((reply.indexOf(resp1) + reply.indexOf(resp2)) == -2) && ((millis() - entry) < timeout));

	if (reply != "") {
		Log("Reply in ");
		Log((millis() - entry));
		Log(" ms: ");
		Logln(reply);
	}

	if (response != NULL) {
		*response = reply;
	}

	if ((millis() - entry) >= timeout) {
		retVal = A6_TIMEOUT;
		Logln("Timed out.");
	} else {
		if (reply.indexOf(resp1) + reply.indexOf(resp2) > -2) {
			Logln("Reply OK.");
			retVal = A6_OK;
		} else {
			Logln("Reply NOT OK.");
			retVal = A6_NOTOK;
		}
	}
	return retVal;
}

}
