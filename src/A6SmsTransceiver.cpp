//************************************************************************************************************************
// A6SmsTransceiver.cpp
// Version 1.0 June, 2017
// Author Gerald Guiony
//************************************************************************************************************************

#include "Gsm0338.h"
#include "A6SmsTransceiver.h"


//========================================================================================================================
//
//========================================================================================================================
A6SmsTransceiver :: A6SmsTransceiver (uint8_t rxPin, uint8_t txPin, uint8_t pwrKey) {

	Logln (F("Initializing A6 GSM"));

	_pwrKey = pwrKey;

	// Instantiate the class with Tx, Rx (remember to swap them when connecting to the A6, i.e. connect the A6's Rx pin to UART_TX).
#ifdef DEBUG
	//------------------------------------------------------------------------------
	class SoftwareSerialLogger : public SoftwareSerial
	{
	public:
		SoftwareSerialLogger (uint8_t receivePin, uint8_t transmitPin, bool inverse_logic = false) :
			SoftwareSerial (receivePin, transmitPin, inverse_logic) {}
		virtual size_t	write	(uint8_t byte)	override { LOGGER.write (byte); return SoftwareSerial::write (byte); }
	};
	_a6Conn = new SoftwareSerialLogger (rxPin, txPin, false);
#else
	_a6Conn = new SoftwareSerial (rxPin, txPin, false);
#endif

	_A6l = new A6lib (_a6Conn);

	// Do it as soon as possible to power off gsm module after a reboot
	stop ();
}

//========================================================================================================================
//
//========================================================================================================================
A6SmsTransceiver :: ~A6SmsTransceiver () {
	if (_A6l) {
		delete _a6Conn;
		delete _A6l;
		_A6l = nullptr;
	}
}

//========================================================================================================================
//
//========================================================================================================================
bool A6SmsTransceiver :: start () {

	_A6l->powerOn (
		_pwrKey,
		[this] (SMSmessage & sms) {
			this->_unsolicitedSmsReceived.push_back (sms);
		}
	);

	// Less error with 115200 because SoftwareSerial is half duplex :
	// During the send routine, interrupts are disabled, which of course means that the interrupt driven receive routine is shut down.
	// => not true ...
	if (_A6l->begin (9600 /*115200*/) != A6_OK) {
		Logln (F("Can't connect to A6 GSM "));
		stop ();
		EspBoard::blinks (10);
		return false;
	}

	// Be careful it seems to be mandatory to wait a delay before to read or send sms!
	delay(2000);

	_isStarted = true;
	notifyGsmStateChanged (true, 0);

	Logln (F("A6 GSM is up"));

	// Handle unsolicited sms received during powerOn
	for (auto sms : _unsolicitedSmsReceived) {
		notifySmsReceived (sms);
	}
	_unsolicitedSmsReceived.clear ();

	return true;
}

//========================================================================================================================
// Do it as soon as possible to power off gsm module after a reboot
//========================================================================================================================
bool A6SmsTransceiver :: stop () {

	if (_A6l->powerOff (_pwrKey)) {

		_isStarted = false;
		notifyGsmStateChanged (false, 0);

		Logln (F("A6 GSM is down"));

		return true;
	}
	return false;
}

//========================================================================================================================
//
//========================================================================================================================
bool A6SmsTransceiver :: isStarted () {
	return _isStarted;
}

//========================================================================================================================
//
//========================================================================================================================
int A6SmsTransceiver :: getNbSms () {

	// Check if Sms is available
	int nb = _A6l->getSMSLocs(_SMSLocs, NB_SMS_MAX);

	Logln (F("Nb sms:") << nb);
	return nb;
}

//========================================================================================================================
//
//========================================================================================================================
int A6SmsTransceiver :: getSignalStrength () {
	return _A6l->getSignalStrength();
}

//========================================================================================================================
//
//========================================================================================================================
void A6SmsTransceiver :: updateSignalStrength () {
	if (isStarted ()) {
		notifyGsmStateChanged (true, getSignalStrength());
	}
}

//========================================================================================================================
//
//========================================================================================================================
byte A6SmsTransceiver :: deleteSms (int index) {
	Logln (F("Delete sms at index: ") << index);
	return _A6l->deleteSMS (_SMSLocs[index]);
}

//========================================================================================================================
//
//========================================================================================================================
byte A6SmsTransceiver :: deleteReadSms () {
	Logln (F("Delete read sms"));
	return _A6l->deleteSMS (1, 2);
}

//========================================================================================================================
//
//========================================================================================================================
byte A6SmsTransceiver :: deleteAllSms () {
	Logln (F("Delete all sms"));
	return _A6l->deleteSMS (1, 4);
}

//========================================================================================================================
//
//========================================================================================================================
bool isValidPhoneNumber (String str) {
	boolean isValid = false;
	for (uint8_t i = 0; i < str.length(); i++) {
		isValid = isdigit(str.charAt(i)) || str.charAt(i) == '+';
		if (!isValid)
			return false;
	}
	return isValid;
}

//========================================================================================================================
//
//========================================================================================================================
bool isValidDate (String str) {
	boolean isValid = false;
	for (uint8_t i = 0; i < str.length(); i++) {
		char ch = str.charAt(i);
		isValid = isdigit(ch) || ch == '+' || ch == ':' || ch == '/' || ch == ',';
		if (!isValid)
			return false;
	}
	return isValid;
}

//========================================================================================================================
//
//========================================================================================================================
bool A6SmsTransceiver :: handleSms (int index) {

	Logln (F("Handle sms at index: ") << index);

	SMSmessage sms = _A6l->readSMS(_SMSLocs[index]);

	if (sms.message.length() > 0) {

		Logln (printSms (sms));

		if (isValidPhoneNumber (sms.number) && isValidDate (sms.date)) {

			deleteSms (index);

			sms.message = Gsm0338::gsm0338ToAscii (sms.message);
			notifySmsReceived (sms);

			return true;
		}
	}

	// Invalid sms ?
	return false;
}

//========================================================================================================================
//
//========================================================================================================================
bool A6SmsTransceiver :: sendSMS (const SMSmessage & sms) {

	//String gsm0338Message = Gsm0338::asciiToGsm0338 (message);
	// The max length of text message is 918, if you send more than 160 characters the message will be broken down in to chunks
	// of 153 characters before being sent
	String gsm0338Message = /*A6l->getRealTimeClock() + F(" ") +*/ sms.message;		// yy/MM/dd,hh:mm:ss+XX message
	gsm0338Message = gsm0338Message.substring(0, 159);							// Here we can't send messages longer than 160 characters.

	Logln (F("Send sms message: ") <<  gsm0338Message << F(" to: ") << sms.number);

	if (A6_OK == _A6l->sendSMS(sms.number, gsm0338Message)) {
		notifySmsSent (sms);
		return true;
	}
	return false;
}

//========================================================================================================================
//
//========================================================================================================================
StreamString A6SmsTransceiver :: printSms (const SMSmessage & sms) {

	StreamString sstr;

	sstr << F("sms from ") << sms.number << F(" received the ") << sms.date << F(":") << LN;
	sstr << F("\"") << sms.message << F("\"") << LN << LN;

	return sstr;
}
