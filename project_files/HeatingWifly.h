#ifndef HeatingWifly_h
#define HeatingWifly_h

#if FALSE

Not used at present




























#include "orz.h"
#include "HardwareSerial.h"


class HeatingWifly
{
public:
    static const int SUCCESS = 0;
    static const int ERROR_HTTP_BODY_SEARCH_STRING_TOO_LONG = 1;
    static const int ERROR_HTTP_BODY_NUM_CHARS_TO_FETCH_TOO_LONG = 2;
    static const int ERROR_TIMED_OUT = 3;

    static const int POWER_CYCLE_TIME_MILLIS = 2000;
    
    HeatingWifly();
    
    void begin(int iBaudRate_p, int iPowerPin_p, unsigned long lWarmupDelay_p, HardwareSerial* pDebugger_p = NULL);
    
    // Returns the HTTP body buffer (or NULL if it timed out)
    int sendHttpRequest(const char *psMessage_p, const char *psHttpBodySearchString_p, int iHttpBodyCharsToFetch_p, unsigned long lTimeoutMillis_p);
    char* getHttpBody();
    
    void softReset();
    void hardReset();

private:

    HardwareSerial* pDebugger_i;

    int iPowerPin_i;
    unsigned long lWarmupDelay_i;

    static const int MAX_CIRCULAR_SEARCH_BUFFER_SIZE = 10;  // must be > iHttpBodySearchStringLength!
    static const int MAX_HTTP_BODY_BUFFER_SIZE = 100;  // Include an extra char for null-termination
    
    char vsCircularSearchBuffer_i[MAX_CIRCULAR_SEARCH_BUFFER_SIZE];
    char vsHttpBodyBuffer_i[MAX_HTTP_BODY_BUFFER_SIZE];
    
    int iCircularSearchBufferPos_i;    
    int iHttpBodyBufferPos_i;
    
    bool isHttpBodyFound_i;    
    const char* psHttpBodySearchString_i;
    int iHttpBodySearchStringLength_i; 
    int iHttpBodyCharsToFetch_i;
    
    unsigned long millisSince(unsigned long lLastMillis_p);
    int normaliseCircularBufferPos(int iBufferPos_p);
    bool processWiflyChar(char c);
    bool readEverythingUntilTimeout(unsigned long lTimeoutMillis_p);
    void readAllCharsOnWifly();
    void sendCommand(const char* psCommand_p);
    
    void printDebug(int iValue_p);
    void printDebug(char* psMessage_p);    
    void printDebugNewline();
    
    
};

  
  
#endif
#endif