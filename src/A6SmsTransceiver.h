//************************************************************************************************************************
// A6SmsTransceiver.h
// Version 1.0 June, 2017
// Author Gerald Guiony
//************************************************************************************************************************


#pragma once


#include <vector>

#include <Stream.h>
#include <StreamString.h>

#include <Common.h>

#include "A6lib.h"


#define NB_SMS_MAX						10


//------------------------------------------------------------------------------
//
class A6SmsTransceiver
{

private:
	uint8_t _pwrKey						= 0;

	A6lib * _A6l						= nullptr;			// GSM
	SoftwareSerial * _a6Conn			= nullptr;

	int _SMSLocs[NB_SMS_MAX] 			= {0};

	std::vector <SMSmessage>			_unsolicitedSmsReceived;

public:

	Delegate <SMSmessage &> 			notifySmsReceived;

public:

	A6SmsTransceiver					(uint8_t rxPin, uint8_t txPin, uint8_t pwrKey);
	~A6SmsTransceiver					();

	bool start							();
	void stop							();

	int getNbSms						();
	int getSignalStrength				();

	byte deleteSms						(int index);
	byte deleteReadSms					();
	byte deleteAllSms					();

	bool handleSms						(int index);
	void sendSMS						(SMSmessage & sms);

	static StreamString printSms		(SMSmessage & sms);
};


