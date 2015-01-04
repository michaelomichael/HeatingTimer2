#ifndef HeatingESP8266_h
#define HeatingESP8266_h


#include "orz.h"
#include "ESP8266.h"


class HeatingESP8266
{
public:
    static const int SUCCESS = 0;
    static const int ERROR_HTTP_BODY_SEARCH_STRING_TOO_LONG = 1;
    static const int ERROR_HTTP_BODY_NUM_CHARS_TO_FETCH_TOO_LONG = 2;
    static const int ERROR_TIMED_OUT = 3;

    static const int POWER_CYCLE_TIME_MILLIS = 2000;
    
    HeatingESP8266();
    
    void begin(HardwareSerial* pESP8266Serial_p, HardwareSerial* pDebugger_p = NULL);
    
    // Returns the HTTP body buffer (or NULL if it timed out)
    int sendHttpRequest(const char *psMessage_p, unsigned long lTimeoutMillis_p);
    char* getHttpBody();
    
    void softReset();
    void hardReset();

private:

    ESP8266 esp8266_i;
    HardwareSerial* pDebugger_i;

    static const int MAX_HTTP_SEND_BUFFER_SIZE = 300;
    char vsHttpSendBuffer_i[MAX_HTTP_SEND_BUFFER_SIZE];

    static const int MAX_HTTP_BODY_BUFFER_SIZE = 100;  // Include an extra char for null-termination
    char vsHttpBodyBuffer_i[MAX_HTTP_BODY_BUFFER_SIZE];    
    int iHttpBodyBufferPos_i;
    
    void printDebug(int iValue_p);
    void printDebug(char* psMessage_p);    
    void printDebugNewline();    
};

#endif