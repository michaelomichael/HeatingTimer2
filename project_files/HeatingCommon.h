#ifndef HeatingCommon_h
#define HeatingCommon_h

#include "stm32f0xx.h"
#include "orz.h"

#define UNIT_TEST TRUE

const int CR = 13;
const int LF = 10;






const int NUM_GPIO_PINS = 1;


const int DAYS_IN_A_WEEK = 7;
const int WINDOWS_PER_DAY = 3;
const int MINUTES_IN_A_DAY = 1440;
const unsigned long MILLIS_IN_AN_HOUR = 3600000UL;  


const int MAX_DAY_NAME_LENGTH = 3;
const int MAX_TIME_LENGTH = 5;
const int MAX_MOTD_LENGTH = 16;
const int MAX_HTTP_BODY_BYTES = 69;




const int APPLIANCES_NONE = 0;
const int APPLIANCES_HOT_WATER_ONLY = 1;
const int APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING = 2;
const int APPLIANCES_MAX = 2;

const int ADVANCE_AS_SCHEDULED = 0;
const int ADVANCE_PLUS_ONE_HOUR = 1;
const int ADVANCED_TO_NEXT_EVENT = 2;



struct HeatingInfo
{
    byte currentDay;  // 0 == Sun, 6 == Sat
    bool isRelayExcited;
    int ambientTemp;
    bool isAmbientTempSet;
    byte maxTemp; 
    byte currentTimeHours;
    byte currentTimeMins; 
    char messageOfTheDay[MAX_MOTD_LENGTH+1];
    byte appliances;
    byte advanceStatus;
    unsigned long lastAdvanceStatusChangeMillis;
    bool internetSyncEnabled;
    char startWindows[DAYS_IN_A_WEEK][WINDOWS_PER_DAY];  // Stores times as ints e.g. 00:00 = 0 and 23:50 = 235
    char endWindows[DAYS_IN_A_WEEK][WINDOWS_PER_DAY];  // Stores times as ints e.g. 00:00 = 0 and 23:50 = 235
    bool wasInWindow;
    bool gpioStatuses[NUM_GPIO_PINS];
};




void updateRelays();
unsigned long millisSince(unsigned long lLastMillis_p);

unsigned long getLastSynchroniseSuccessTime();
unsigned long getMaxInternetSyncDelayMillis();
int getNumFailedInternetSyncs_m();
int getLcdBrightness();

void setMaxInternetSyncDelayMillis(unsigned long lValue_p);
void setLcdBrightness(int iValue_p);

void notifyHeatingInfoChangedLocally();


void debug(const char* psMessage_p);
#endif
