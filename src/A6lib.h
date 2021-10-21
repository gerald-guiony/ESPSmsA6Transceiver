//************************************************************************************************************************
// A6lib.h
// Creation date: June, 2017
// Author: Gerald Guiony
// Forked repository: https://github.com/skorokithakis/A6lib
//************************************************************************************************************************

// Links :
// http://www.electrodragon.com/w/GSM_GPRS_A6_Module
// https://github.com/skorokithakis/A6lib
// https://github.com/skorokithakis/A6lib/blob/master/examples/sms/sms.ino
// https://alselectro.wordpress.com/2016/09/14/gsm-a6-with-arduino-making-a-call-sending-sms/
// https://lastminuteengineers.com/a6-gsm-gprs-module-arduino-tutorial/


// Firmware upgrade :
// https://www.iot-experiments.com/ai-thinker-a6-module-firmware-update/?fbclid=IwAR2-ezxAlr4Ypz3-UyKDd73-PIj9W_GmRtn7kPu_z6xgcCGH7AJn0Sqcy88
// https://wiki.ai-thinker.com/gprs/firmware?fbclid=IwAR1VQHhOuERZcnsP8HwXSjNQkj-ldle2x5GADLGRVgah0kx64Ku4Ft5sodE

// AT Commands :
// https://www.technologuepro.com/gsm/commande_at.htm


#ifndef A6lib_h
#define A6lib_h

#include <Arduino.h>
#include <SoftwareSerial.h>

#include <functional>

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define A6_OK 0
#define A6_NOTOK 1
#define A6_TIMEOUT 2
#define A6_FAILURE 3

//#define A6_CMD_TIMEOUT 2000
#define A6_CMD_TIMEOUT 1000


enum call_direction {
    DIR_OUTGOING = 0,
    DIR_INCOMING = 1
};

enum call_state {
    CALL_ACTIVE = 0,
    CALL_HELD = 1,
    CALL_DIALING = 2,
    CALL_ALERTING = 3,
    CALL_INCOMING = 4,
    CALL_WAITING = 5,
    CALL_RELEASE = 7
};

enum call_mode {
    MODE_VOICE = 0,
    MODE_DATA = 1,
    MODE_FAX = 2,
    MODE_VOICE_THEN_DATA_VMODE = 3,
    MODE_VOICE_AND_DATA_VMODE = 4,
    MODE_VOICE_AND_FAX_VMODE = 5,
    MODE_VOICE_THEN_DATA_DMODE = 6,
    MODE_VOICE_AND_DATA_DMODE = 7,
    MODE_VOICE_AND_FAX_FMODE = 8,
    MODE_UNKNOWN = 9
};

struct SMSmessage {
    String number;
    String date;
    String message;
};

struct callInfo {
    int index;
    call_direction direction;
    call_state state;
    call_mode mode;
    int multiparty;
    String number;
    int type;
};


class A6lib {
public:
    A6lib(SoftwareSerial * A6connection);
    ~A6lib();

    byte begin  (long baudRate);

    void powerOn(int pin, std::function <void(SMSmessage &)> onUnsolicitedSMSReceived);
    bool powerOff(int pin);

    bool dial(String number);
    bool redial();
    bool answer();
    bool hangUp();
    callInfo checkCallStatus();
    int getSignalStrength();

    byte sendSMS(String number, String text);
    int getUnreadSMSLocs(int* buf, int maxItems);
    int getSMSLocs(int* buf, int maxItems);
    int getSMSLocsOfType(int* buf, int maxItems, String type);
    SMSmessage readSMS(int index);
    byte deleteSMS(int index);
    byte deleteSMS(int index, int flag);
    byte setSMScharset(String charset);

    bool setVol(byte level);
    bool enableSpeaker(byte enable);

    String getRealTimeClock();

private:
    byte waitRegistrationState (int state);
    byte waitUnsolicitedPduSMS (std::function <void(SMSmessage &)> onUnsolicitedSMSReceived);
    String read();
    byte A6command(const char *command, const char *resp1, const char *resp2, uint16_t timeout, uint8_t repetitions, String *response);
    byte A6waitFor(const char *resp1, const char *resp2, uint16_t timeout, String *response);
    long detectRate();
    char setRate(long baudRate);

private:
    SoftwareSerial *A6conn;

};
#endif
