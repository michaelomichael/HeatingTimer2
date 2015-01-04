/*
Follows
an example here: http://www.embedds.com/stm32-interrupts-and-programming-with-gcc/
*/
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "stm32f0xx_tim.h"
#include "stm32f0xx_rcc.h"
#include "stm32f0xx_gpio.h"
#include "stm32f0xx_exti.h"
#include "stm32f0xx_syscfg.h"
#include "orz.h"
#include "HardwareSerial.h"

#include "HeatingCommon.h"
// #include "HeatingUI.h"
// #include "HeatingWifly.h"
#include "HeatingESP8266.h"
#include "RTC.h"

//#define debug(x) ;





/*

Internet Central Heating Timer.

Connect up the CA-42 serial lead as usual:

    Lead Pin 1 to ground,
    Lead Pin 2 (Rx) to AVR Pin 3 (PD1 - Tx)
    Lead Pin 3 (Tx) to AVR Pin 2 (PD0 - Rx)

CONNECTIONS
-----------

1. Indicator LED and 220R resistor between PC5 and Gnd.

2. Water relay (or yellow LED and resistor) to PC2.

3. Heating relay (or red LED and resistor) to PC3.

4. 32.768 KHz crystal between PC14 and PC15.

5. 10pF capacitor between PC14 and Gnd, and another one between PC15 and Gnd.

6. ESP8266 red wire to the 3V pin, and black wire to Gnd.

7. ESP8266 green wire to PA3 and yellow wire to PA2.  ("Serial1")
    Note: Outputs from STM32 are all 3V so no 5V conversion necessary.

8. Nokia CA-42 lead ("Serial"):
        a. Pin with writing to Gnd
        b. Middle pin to PA9
        c. Final pin to PA10



*/

/*
#include <WProgram.h>
#include <NewSoftSerial.h>
#include <Time.h>
#include "HeatingCommon.h"
#include "HeatingESP8266.h"
#include "HeatingClock.h"
#include "HeatingUI.h"
*/





//
// Simple macro to copy the given string into a useable buffer called vsProgmemBuffer_m.
// example use here:
//
//      GET_PROGMEM_STRING(THREE);
//      realSerial.println(vsProgmemBuffer_m);
//
//#define GET_PROGMEM_STRING(sProgCharString) strcpy_P(vsProgmemBuffer_m, sProgCharString)

//
// How long to wait between calls onto the internal RTC to update 
// the time (useful if internet is disabled)
//
const uint32_t RTC_UPDATE_TIME_MILLIS = 20000UL;

//
// Pins used to trigger the relays
//
const int WATER_RELAY_PIN = PC2;//10; // was 7; 
const int HEATING_RELAY_PIN = PC3; // was 8;

//
// You can't see the relays, so we use this third pin
// to indicate whether either (or both) of the relays 
// is currently active.
//
const int ACTIVE_INDICATOR_PIN = PC5;

//
// Connected to a thermistor.  This needs to be on PC1 for ADC to work PD2.
//
const int TEMPERATURE_SENSOR_PIN = PC1; 

//
// Extra pins that can be internet-controlled
//
const int GPIO_PINS[1] = { PB0 };

//
// The Wifly is extremely flaky, and occasionally needs 
// to be properly powered off and back on again, so we'll
// control its +3V power supply via this pin and a transistor.
// I tried a few different pins as the supply for the transistor,
// but PA5 seemed to be the only one that would actually work!
//
const int WIFLY_POWER_PIN = PA5;  


// HTTP Parsing states
const int PARSE_STATE_IDLE = 0;
const int PARSE_STATE_FOUND_ASTERISK = 1;
const int PARSE_STATE_FOUND_OPEN = 2;
const int PARSE_STATE_FOUND_NEWLINE = 3;
const int PARSE_STATE_READING_MESSAGE_BODY = 4;





const unsigned long HTTP_TIMEOUT = 13000;  // WAS 3000 for the real wifly!
const char* HTTP_BODY_SEARCH_STRING = "\r\n\r\nOK";
const int MAX_HTTP_BODY_CHARS_TO_READ = 73;

//
// We take periodic temperature readings, and average them
// out to get a less 'jumpy' reading.
//
const unsigned long TEMPERATURE_SAMPLE_DELAY_MILLIS = 5000;//60000;
const byte MAX_TEMPERATURE_SAMPLES = 3;

//
// When the wifly module doesn't respond, we have the option of doing a soft reset
// or a hard reset.  The soft reset connects to the Wifly via UART to send a
// 'RESET' command.  The hard reset temporarily disables the power to the Wifly
// via the transistor on WIFLY_POWER_PIN.
//
const int MAX_FAILED_INTERNET_SYNCHRONISATIONS_BEFORE_SOFT_RESET = 1;
const int MAX_FAILED_INTERNET_SYNCHRONISATIONS_BEFORE_HARD_RESET = 3;

const char* BASE64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


//const int MAX_PROGMEM_BUFFER_SIZE = 20;

#define GET_PROGMEM_STRING(sProgCharString) vsProgmemBuffer_m = (char*) sProgCharString
#define PROGMEM
#define prog_char const char

char *vsProgmemBuffer_m;

const char MSG_SERIALDEBUG_INIT[] = "Serial init.."        ;
PROGMEM prog_char MSG_SERIALDEBUG_PARSING_HEATING_INFO[] = "Parsing";
PROGMEM prog_char MSG_SERIALDEBUG_GPIO_SETTING[] = "GPIO";
PROGMEM prog_char MSG_SERIALDEBUG_DAY_WINDOW_SETTING[] = "Day";
PROGMEM prog_char MSG_SERIALDEBUG_SENDING_1[] = "Sending [";
PROGMEM prog_char MSG_SERIALDEBUG_SENDING_2[] = "], b64 [";
PROGMEM prog_char MSG_SERIALDEBUG_SENDING_3[] = "]";
PROGMEM prog_char MSG_SERIALDEBUG_SENDING_RESULT[] = "Result ";
PROGMEM prog_char MSG_WIFLY_STARTUP_MESSAGE[] = "STARTUP";




RealTimeClock rtc_m;
HeatingESP8266 wifly_m;
#define debugSerial_m Serial
#define wiflySerial_m Serial1
int iState_m = PARSE_STATE_IDLE;
char vsSendBuffer_m[80];  // MO was 50
const int MAX_BASE64_SEND_BUFFER_SIZE = 100;  // MO was 70
char vsBase64SendBuffer_m[MAX_BASE64_SEND_BUFFER_SIZE];  





int temperatureSamples_m[MAX_TEMPERATURE_SAMPLES] = { 0, 0, 0};
unsigned long lLastTimeCheckTime_m = 0;



const unsigned long DEFAULT_INTERNET_SYNC_DELAY_MILLIS = 10000; // 2 mins




HeatingInfo heatingInfo_m;
bool isHeatingInfoChangedLocally_m = false;
bool isEnergised = false;
unsigned long lLastSynchroniseSendTime_m = 0;
unsigned long lLastSynchroniseSuccessTime_m = 0;
unsigned long lLastTemperatureSampleTime_m = 0;
unsigned long lMaxInternetSyncDelayMillis_m = DEFAULT_INTERNET_SYNC_DELAY_MILLIS;

int iNumFailedInternetSyncs_m = 0;



void parseHeatingInfo();
void readAllAvailableCharsOnWifly();
bool checkForWiflyOutput();
void synchroniseWithInternet(bool bForceSynchronise_p);



int getNumFailedInternetSyncs_m()
{
    return iNumFailedInternetSyncs_m;
}

 
int base64Encode(char* psOriginal_p, char* psOutput_p, int iMaxOutputChars_p)
{
    
    bool isEndReached = false;
    int iInputCharsProcessed = 0;
    int iOutputCharsProcessed = 0;
    int iNumBlankBytes = 0;
    
    //
    //  We're dealing in blocks of 3 chars - i.e. 24 bits.  This allows us to deal
    //  with 4 groups of 6 bits.
    //
    while (! isEndReached)
    {
        char c1 = psOriginal_p[iInputCharsProcessed++];
        char c2 = psOriginal_p[iInputCharsProcessed++];
        char c3 = psOriginal_p[iInputCharsProcessed++];
        
        // If any of these are NULLs then we will be finishing soon.        
        if (0 == c1)
        {
            // c1, c2, and c3 are of no consequence so just return now.
            isEndReached = true;            
            break;
        }
        else if (0 == c2)
        {
            // We can process c1, but leave c2 and c3 as NULL.
            isEndReached = true;
            iNumBlankBytes = 2;
            c3 = 0;
        }
        else if (0 == c3)        
        {
            // We can process c1 and c2, but leave c3 as NULL.
            isEndReached = true;
            iNumBlankBytes = 1;
        }
            
        byte b1 = ((c1 & 0xFC) >> 2);                      // First 6 bits of c1
        byte b2 = ((c1 & 0x03) << 4) + ((c2 & 0xF0) >> 4); // Last 2 bits of c1 and first 4 bits of c2
        byte b3 = ((c2 & 0x0F) << 2) + ((c3 & 0xC0) >> 6); // Last 4 bits of c2 and first 2 bits of c3
        byte b4 =  (c3 & 0x3F);                            // Last 6 bits of c3
        
        // Now convert these values into their base64 chars and add them to the output string
        if (iOutputCharsProcessed + 4 > iMaxOutputChars_p)
        {
            return -1;
        }
        
        psOutput_p[iOutputCharsProcessed++] = BASE64_CHARS[b1];
        psOutput_p[iOutputCharsProcessed++] = BASE64_CHARS[b2];
        psOutput_p[iOutputCharsProcessed++] = BASE64_CHARS[b3];
        psOutput_p[iOutputCharsProcessed++] = BASE64_CHARS[b4];
        
        while (iNumBlankBytes > 0)
        {
            psOutput_p[iOutputCharsProcessed - iNumBlankBytes] = '=';
            iNumBlankBytes--;
        }
    }
    
    // Null terminate
    psOutput_p[iOutputCharsProcessed] = 0;
    
    return iOutputCharsProcessed;   
}

/*

void testBase64Encoding(char* psTestName_p, char* psAscii_p, char* psExpectedBase64_p)
{
    char szActualBase64[100];
    int iOutputChars = base64Encode(psAscii_p, szActualBase64, 100);
    
    if (-1 == iOutputChars)
    {        
        Serial.print("Test failed - ");
        Serial.print(psTestName_p);
        Serial.print(" - overflow in the output buffer");
        Serial.println();
    }
    else if (0 != strcmp(psExpectedBase64_p, szActualBase64))
    {
        Serial.print("Test failed - ");
        Serial.print(psTestName_p);
        Serial.print(" - expected '");
        Serial.print(psExpectedBase64_p);
        Serial.print("' but got '");
        Serial.print(szActualBase64);
        Serial.print("'");
        Serial.println();
    }
}

*/

                    
                    
/*
unsigned long millisSince(unsigned long lLastMillis_p)
{
    unsigned long lCurrentMillis = millis();
    
    if (lCurrentMillis < lLastMillis_p)
    {
        // Assume rollover
        return lCurrentMillis + (0xFFFFFFFFUL - lLastMillis_p);
    }
    else
    {
        return lCurrentMillis - lLastMillis_p;   
    }   
}                    
*/



bool isWithinWindow(char cStart_p, char cEnd_p)
{
    char cCurrentTime = (heatingInfo_m.currentTimeHours * 10) + (heatingInfo_m.currentTimeMins / 10);
    
    return cCurrentTime >= cStart_p  &&  cCurrentTime < cEnd_p;
}


void updateRelays()
{    
    // Flash LEDs to show we mean business
    debug("Updating relays");
    
    for (int i=0; i < 3; i++)
    {
        digitalWrite(ACTIVE_INDICATOR_PIN, HIGH);
        delay(50);
        digitalWrite(ACTIVE_INDICATOR_PIN, LOW);
        delay(50);
    }    
    
    for (byte yGPIO = 0; yGPIO < NUM_GPIO_PINS; yGPIO++)
    {
        digitalWrite(GPIO_PINS[yGPIO], heatingInfo_m.gpioStatuses[yGPIO] ? HIGH : LOW);
    }
    
    // Look at the current time, figure out if one or both
    // relays should be on/off, and set the output pins appropriately.   
    byte yDay = heatingInfo_m.currentDay;
    bool isNowInWindow = false;
    
    for (byte yWindow=0; yWindow < WINDOWS_PER_DAY; yWindow++)
    {
        if (isWithinWindow(heatingInfo_m.startWindows[yDay][yWindow], heatingInfo_m.endWindows[yDay][yWindow]))
        {
            isNowInWindow = true;
            break;
        }
    }
    
    /*
        Previous state      New state       Advance state  Reset Advance?   Action
        -------------       ---------       -------------  --------------   ------
        Not in window       Not in window   None                            Relays off
        Not in window       Not in window   +1 hour        If > 1hr         If within 1 hour of when you clicked the button then Relays on, else set advance to 'none'
        Not in window       Not in window   Advance                         Relays on
        Not in window       In window       None                            Relays on
        Not in window       In window       +1 hour        Yes              Relays on, and set advance to 'none'
        Not in window       In window       Advance        Yes              Relays on, and set advance to 'none'
        In window           Not in window   None                            Relays off
        In window           Not in window   +1 hour        Yes              Relays off, and set advance to 'none'
        In window           Not in window   Advance        Yes              Relays off, and set advance to 'none'
        In window           In window       None                            Relays on
        In window           In window       +1 hour        If > 1hr         If within 1 hour of when you clicked the button then Relays off, else set advance to 'none'
        In window           In window       Advance                         Relays off
        
    Once we've sorted out all of the resetting of 'advance' status, it boils down to:
        New state       Advance state   Relays
        ---------       -------------   ------
        Not in window   None            Off
        Not in window   +1/Advance      On
        In window       None            On
        In window       +1/Advance      Off
        
    */
    // Reset the advance status if we've crossed a window boundary
    bool isAdvanceStatusChanged = false;
    
    debugSerial_m.print("Was in window: ");
    debugSerial_m.print(heatingInfo_m.wasInWindow);
    debugSerial_m.print(", now in window: ");
    debugSerial_m.println(isNowInWindow);
    
    
    if (heatingInfo_m.wasInWindow != isNowInWindow)
    {
        if (ADVANCE_AS_SCHEDULED != heatingInfo_m.advanceStatus)
        {
            debug("We WERE in a window but now we're not so switching the 'advance' status");
            isAdvanceStatusChanged = true;
            isHeatingInfoChangedLocally_m = true; // Notify the web server so it doesn't revert back to what's in the DB.            
        }
        
        debug("Setting 'advance' status back to normal");
        heatingInfo_m.advanceStatus = ADVANCE_AS_SCHEDULED;        
    }                                      
    heatingInfo_m.wasInWindow = isNowInWindow;
    
    //
    //  Cancel the '+1 hour' setting if it's been active for longer than an hour
    //
    if (ADVANCE_PLUS_ONE_HOUR == heatingInfo_m.advanceStatus  &&  millisSince(heatingInfo_m.lastAdvanceStatusChangeMillis) > MILLIS_IN_AN_HOUR)
    {
        debug("Cancelling the +1 hour");
        debugSerial_m.print("Last advance status change was ");
        debugSerial_m.print(heatingInfo_m.lastAdvanceStatusChangeMillis);
        debugSerial_m.print(" and millis now is ");
        debugSerial_m.println(millis());
     
        if (ADVANCE_AS_SCHEDULED != heatingInfo_m.advanceStatus)
        {
            isAdvanceStatusChanged = true;
            isHeatingInfoChangedLocally_m = true;  // Notify the web server so it doesn't get set back to what's in the DB.
        }               
        
        heatingInfo_m.advanceStatus = ADVANCE_AS_SCHEDULED;
    }
    
    bool isWindowOverridden = (ADVANCE_AS_SCHEDULED != heatingInfo_m.advanceStatus);    
    bool bActivateRelays = (isNowInWindow != isWindowOverridden);  // XOR       
    
    debugSerial_m.print("Is window overridden: ");
    debugSerial_m.print(isWindowOverridden);
    debugSerial_m.print(", activate relays: ");
    debugSerial_m.println(bActivateRelays);
    
    //
    //  Now that we know whether the relay(s) should be on, let's
    //  make sure they're both in the correct state.
    //
    if (APPLIANCES_HOT_WATER_ONLY == heatingInfo_m.appliances)
    {
        digitalWrite(HEATING_RELAY_PIN, LOW);
        digitalWrite(WATER_RELAY_PIN, (bActivateRelays ? HIGH : LOW));
        digitalWrite(ACTIVE_INDICATOR_PIN, (bActivateRelays ? HIGH : LOW));
        isEnergised = bActivateRelays;
    }
    else if (APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING == heatingInfo_m.appliances)
    {
        // FORLATER: bool isRoomTooCold = (heatingInfo_m.ambientTemp < heatingInfo_m.maxTemp);
        bool isRoomTooCold = true;
        digitalWrite(HEATING_RELAY_PIN, (bActivateRelays && isRoomTooCold) ? HIGH : LOW);
        digitalWrite(WATER_RELAY_PIN, (bActivateRelays ? HIGH : LOW));   
        digitalWrite(ACTIVE_INDICATOR_PIN, (bActivateRelays ? HIGH : LOW));
        isEnergised = bActivateRelays;
    }
    else
    {
        digitalWrite(HEATING_RELAY_PIN, LOW);
        digitalWrite(WATER_RELAY_PIN, LOW);        
        digitalWrite(ACTIVE_INDICATOR_PIN, LOW);
        isEnergised = false;
    }
    
    if (isAdvanceStatusChanged)
    {
        // FORLATER: updateDisplay(&heatingInfo_m, false);
    }
}








void updateRTCWithHeatingInfo(HeatingInfo *pHeatingInfo_p)
{
    debug("Updating RTC with heating info");
    rtc_m.setWeekday((Weekday) ((pHeatingInfo_p->currentDay + 7) % 8));  // Convert Sun=0,Mon=1,Sat=6 to Sun=7,Mon=1,Sat=6
    rtc_m.setTime(pHeatingInfo_p->currentTimeHours, pHeatingInfo_p->currentTimeMins);
}



void updateHeatingInfoWithRTC(HeatingInfo *pHeatingInfo_p)
{
    debug("Updating heating info with RTC data");
    pHeatingInfo_p->currentDay = rtc_m.getWeekday() % 7;  // Convert Sun=7,Mon=1,Sat=6 to Sun=0,Mon=1,Sat=6
    pHeatingInfo_p->currentTimeHours = rtc_m.getHour();
    pHeatingInfo_p->currentTimeMins = rtc_m.getMinute();
}



bool sampleTemperature()
{
    // Returns a value between 0 and 4094 (i.e. 0xFFF)
    uint16_t iTemperatureSample = analogRead(TEMPERATURE_SENSOR_PIN); // analogRead(5);
    
    // The formula below was for arduino, which returns ADC value between 0 and 1024
    double dTemperatureSample = (iTemperatureSample / 4094.0) * 1024.0;
    double dTemp;
    dTemp = logf(((10240000/dTemperatureSample) - 10000));
    dTemp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * dTemp * dTemp ))* dTemp );
    dTemp = dTemp - 273.15;            // Convert Kelvin to Celcius
    
    int iTemperatureCentigrade = dTemp;
    //iTemperatureCentigrade = -5;
    debugSerial_m.print("Temp sample:");
    debugSerial_m.print(iTemperatureSample, DEC);
    debugSerial_m.print(" = ");
    debugSerial_m.println(iTemperatureCentigrade, DEC);
    
    for (byte i=0; i < MAX_TEMPERATURE_SAMPLES-1; i++)
    {
        if (0 == lLastTemperatureSampleTime_m)
        {
            temperatureSamples_m[i] = iTemperatureCentigrade;  // First time, just fill with the current value
        }
        else
        {
            temperatureSamples_m[i] = temperatureSamples_m[i+1];  // Roll back towards index [0]
        }             
    }
    
    temperatureSamples_m[MAX_TEMPERATURE_SAMPLES-1] = iTemperatureCentigrade; 

    int iAverageTemperatureReading = 0;
        
    for (byte i=0; i < MAX_TEMPERATURE_SAMPLES; i++)
    {
        iAverageTemperatureReading += temperatureSamples_m[i];    
    }
    
    iAverageTemperatureReading /= MAX_TEMPERATURE_SAMPLES;
            
    bool isTemperatureChanged = (iAverageTemperatureReading != heatingInfo_m.ambientTemp);        
    heatingInfo_m.ambientTemp = iAverageTemperatureReading;
    heatingInfo_m.isAmbientTempSet = true;
    lLastTemperatureSampleTime_m = millis();
    
    debugSerial_m.print("Storing new ambient temp as ");
    debugSerial_m.println(heatingInfo_m.ambientTemp, DEC);
    
    return isTemperatureChanged;
}



void setup()   
{       
    debugSerial_m.begin(57600);//921600);
    GET_PROGMEM_STRING(MSG_SERIALDEBUG_INIT);  // debugSerial_m.println("Serial init..");        
    debugSerial_m.println();
    debugSerial_m.println(vsProgmemBuffer_m); 

    wiflySerial_m.begin(9600);
    
    setPinMode(PC8, PIN_MODE_DIGITAL_WRITE);  // Set these as some of our library code 
    setPinMode(PC9, PIN_MODE_DIGITAL_WRITE);  // occasionally likes to flash them

    setPinMode(TEMPERATURE_SENSOR_PIN, PIN_MODE_ANALOG_READ);
    // FORLATER: setPinMode(ADVANCE_BUTTON_PIN, PIN_MODE_DIGITAL_READ);
    // FORLATER: setPinMode(ENCODER_BUTTON_PIN, PIN_MODE_DIGITAL_READ);
    //pinMode(BACK_BUTTON_PIN, INPUT);    
    setPinMode(HEATING_RELAY_PIN, PIN_MODE_DIGITAL_WRITE);
    setPinMode(WATER_RELAY_PIN, PIN_MODE_DIGITAL_WRITE);
    setPinMode(ACTIVE_INDICATOR_PIN, PIN_MODE_DIGITAL_WRITE);
    
    for (byte yGPIO = 0; yGPIO < NUM_GPIO_PINS; yGPIO++)
    {
        setPinMode(GPIO_PINS[yGPIO], PIN_MODE_DIGITAL_WRITE);
        digitalWrite(GPIO_PINS[yGPIO], LOW);   
    }
    
    // Flash LEDs to show we mean business
    digitalWrite(ACTIVE_INDICATOR_PIN, HIGH);
    delay(500);
    digitalWrite(ACTIVE_INDICATOR_PIN, LOW);
    delay(500);
    digitalWrite(ACTIVE_INDICATOR_PIN, HIGH);
    delay(500);
    digitalWrite(ACTIVE_INDICATOR_PIN, LOW);
    delay(500);
    
    digitalWrite(HEATING_RELAY_PIN, LOW);
    digitalWrite(WATER_RELAY_PIN, LOW);
    
    heatingInfo_m.internetSyncEnabled = true;
    heatingInfo_m.maxTemp = 21;
    heatingInfo_m.currentTimeHours = 12;
    heatingInfo_m.appliances = APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING;
    heatingInfo_m.isAmbientTempSet = false;
    
    sampleTemperature();
    
    
    for (int iDay = 0; iDay < DAYS_IN_A_WEEK; iDay++)
    {
        heatingInfo_m.startWindows[iDay][0] = 60;
        heatingInfo_m.endWindows[iDay][0] = 63;
        heatingInfo_m.startWindows[iDay][1] = 160;
        heatingInfo_m.endWindows[iDay][1] = 170;        
    }
    
    // Different times for sat/sun   
    heatingInfo_m.startWindows[0][0] = 80;
    heatingInfo_m.endWindows[0][0] = 83;
    heatingInfo_m.startWindows[0][1] = 160;
    heatingInfo_m.endWindows[0][1] = 163;
    
    // These are just for easy testing...
    heatingInfo_m.startWindows[0][0] = 121; 
    heatingInfo_m.endWindows[0][0] = 122;
    heatingInfo_m.startWindows[0][1] = 124;
    heatingInfo_m.endWindows[0][1] = 125;
    
    heatingInfo_m.startWindows[1][0] = 80;
    heatingInfo_m.endWindows[1][0] = 83;
    heatingInfo_m.startWindows[1][1] = 160;
    heatingInfo_m.endWindows[1][1] = 163;        
    
    debug("Calling rtc init");
    
    // FORLATER: rtc_m.init();
    // FORLATER: updateRTCWithHeatingInfo(&heatingInfo_m);
    debug("Done with rtc init.");
      
    // FORLATER: initUI();    
        
    // FORLATER: updateDisplay(&heatingInfo_m, false);    
        
    wifly_m.begin(&wiflySerial_m, &debugSerial_m);    
    synchroniseWithInternet(true);

    debug("Setup complete.");
}




void loop()                     
{
     
    // FORLATER: if ((! isUIBusy())   &&  millisSince(lLastTimeCheckTime_m) > RTC_UPDATE_TIME_MILLIS)
    if (millisSince(lLastTimeCheckTime_m) > RTC_UPDATE_TIME_MILLIS)
    {
        debugSerial_m.println("loop: Incrementing time");
        lLastTimeCheckTime_m = millis();
        byte yOldDay = heatingInfo_m.currentDay;
        byte yOldTimeHours = heatingInfo_m.currentTimeHours;
        byte yOldTimeMins = heatingInfo_m.currentTimeMins;
        
        // FORLATER: updateHeatingInfoWithRTC(&heatingInfo_m);
        
        bool isTimeChanged = (yOldDay != heatingInfo_m.currentDay  ||  
                              yOldTimeHours != heatingInfo_m.currentTimeHours  ||
                              yOldTimeMins != heatingInfo_m.currentTimeMins);
        
        if (isTimeChanged)
        {
            updateRelays();
            // FORLATER: updateDisplay(&heatingInfo_m, false);
        }
    }   
    
    // FORLATER: checkForAdvanceAndEncoderInputs(&heatingInfo_m);
    
    
    /*
    if (digitalRead(BACK_BUTTON_PIN))
    {
        setUIState(&heatingInfo_m, yPreviousUIState_m);
        yPreviousUIState_m = UI_STATE_NORMAL;
        updateDisplay(&heatingInfo_m, false);
        
        waitForDebounce(BACK_BUTTON_PIN);
    }
    */


    // Check if the temperature has changed and, if so, how long it's been changed for
    // Take average of last 3 temp ratings over 1 minute each.  
    //Temp is 1 below/higher than max then wait a minute and try again.  if it's still 1 below/higher (same sign) then assume that's right.    
        
    if (millisSince(lLastTemperatureSampleTime_m) > TEMPERATURE_SAMPLE_DELAY_MILLIS  ||  !heatingInfo_m.isAmbientTempSet)
    {        
        bool isTemperatureChanged = sampleTemperature();
        if (isTemperatureChanged)
        {            
            // FORLATER: updateDisplay(&heatingInfo_m, false);
            updateRelays();
        }
    }
    
    
    // FORLATER: checkForUIIdle(&heatingInfo_m);
    
    
    // FORLATER: if (! isUIBusy())
    {
        if (heatingInfo_m.internetSyncEnabled  &&  millisSince(lLastSynchroniseSendTime_m) > lMaxInternetSyncDelayMillis_m)
        {            
            debugSerial_m.print(lMaxInternetSyncDelayMillis_m, DEC);
            debugSerial_m.println("Resync!");
            synchroniseWithInternet(false);                
        }
    }    

}









//
//  Takes the line of text that was read from the HTTP response body and parses it into
//  individual fields.
//  The string is in the format:
//      
//      12112345123456789012345611
//      OK$hh:mmMessageOfTheeDay*&ABDCEFGHIJKLM... etc.
//
//      OKMessageOfTheDay %*&ABCDEF...
//
//  where:
//      '$' is the index of the weekday as an int e.g. "0" = "Sunday", "6" = "Saturday"
//      'hh:mm' is the current server time in 24 hour clock 
//      'MessageOfTheeDay' is an arbitrary 16-character string
//      '%' is the max temp as a one-char binary ASCII representation (with 100 added to it, e.g. 'd'=100="0C", 'x'=120="20C", 'Z'=90="-10C"
//      '*' is the appliances as an int (0=none, 1=hot water only, 2=hot water and central heating)
//      '&' is the advance status as an int (0=normal, 1=plus one hour, 2=advance)
//      '#' is the GPIO 1,2, and 3 status as a binary encoded int (0 = all off, 1 = GPIO1 on, 2 = GPIO2 on, 3 = GPIO 1&2 on, 7 = all 3 on).
//      'A', 'B', etc are binary representations of windows - grouped by day, with 3 windows per day and 2 events per day, 42 chars in total.
//      The binary representations are e.g. 101 for "10:00", 1 for "00:00", 236 for "23:50" (note we add 1 so that we don't send null chars).
//
//  There will be 26 bytes for everything before the windows, and 7*3*2 bytes (=42) for the windows, so that's 68 in total.
//
//
void parseHeatingInfo(char *psHttpBody_p)
{
    GET_PROGMEM_STRING(MSG_SERIALDEBUG_PARSING_HEATING_INFO);  // debugSerial_m.print("Parsing");
    debugSerial_m.print(vsProgmemBuffer_m);                 
    
    char* psOriginal = psHttpBody_p;
    /*
    if (0 != strncmp(psOriginal, "OK", 2))
    {
        strncpy(heatingInfo_m.messageOfTheDay, psOriginal, 16);
        return;   
    }    
    
    psOriginal += 2;
    */
    HeatingInfo newHeatingInfo;
    
    memset(&newHeatingInfo, 0, sizeof(newHeatingInfo));
    
    // ambientTemp, wasInWindow
    
    newHeatingInfo.currentDay = int((*psOriginal) - '0');    
    //strncpy(newHeatingInfo.currentDay, psOriginal, MAX_DAY_NAME_LENGTH);    
    psOriginal += 1;
    
    //newHeatingInfo.currentDay = getDayIndex(newHeatingInfo.currentDayName);  // TODO - this has been replaced with a single char
    newHeatingInfo.currentTimeHours = ((psOriginal[0] - '0') * 10) + (psOriginal[1] - '0');
    newHeatingInfo.currentTimeMins = ((psOriginal[3] - '0') * 10) + (psOriginal[4] - '0');    
    
    // TODO - call the setTime method insteadsetTime(&newHeatingInfo, ((psOriginal[0] - '0') * 10) + (psOriginal[1] - '0'), ((psOriginal[3] - '0') * 10) + (psOriginal[4] - '0'));
    
    //strncpy(newHeatingInfo.currentDisplayTime, psOriginal, MAX_TIME_LENGTH);    
    psOriginal += MAX_TIME_LENGTH;
    
    // Convert the display time into a single char (like we use for the 'windows')
    //char szSingleCharTime[4];
    //szSingleCharTime[0] = newHeatingInfo.currentDisplayTime[0]; // first char of the hour
    //szSingleCharTime[1] = newHeatingInfo.currentDisplayTime[1]; // second char of the hour
    //szSingleCharTime[2] = newHeatingInfo.currentDisplayTime[3]; // first char of the minute
    //szSingleCharTime[3] = 0;
    //debugSerial_m.print("XXX");
    //debugSerial_m.print(szSingleCharTime);
    //debugSerial_m.print("XXX");
    
    //newHeatingInfo.currentTime = (char) int(szSingleCharTime);    
    
    debugSerial_m.print(newHeatingInfo.currentTimeHours, DEC);
    debugSerial_m.print(":");
    debugSerial_m.print(newHeatingInfo.currentTimeMins, DEC);
    debugSerial_m.println();
    
    strncpy(newHeatingInfo.messageOfTheDay, psOriginal, MAX_MOTD_LENGTH);    
    psOriginal += MAX_MOTD_LENGTH;
    
    // Next char is the max temp, binary, with 100 added to it
    newHeatingInfo.maxTemp = int(*psOriginal) - 100;
    psOriginal++;
    
    // Next char is the appliances
    switch (psOriginal[0])
    {
        case '0':
            newHeatingInfo.appliances = APPLIANCES_NONE;
            break;       
            
        case '1':
            newHeatingInfo.appliances = APPLIANCES_HOT_WATER_ONLY;
            break;       
            
        default:
            newHeatingInfo.appliances = APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING;
            break;
    }
        
    psOriginal++;
    
    // Next char is the advance status
    switch (psOriginal[0])
    {
        case '1':
            newHeatingInfo.advanceStatus = ADVANCE_PLUS_ONE_HOUR;
            break;
        
        case '2':
            newHeatingInfo.advanceStatus = ADVANCED_TO_NEXT_EVENT;
            break;
            
        default:
            newHeatingInfo.advanceStatus = ADVANCE_AS_SCHEDULED;
            break;
    }
        
    psOriginal++;
    
    int iGPIOStatuses = (*psOriginal) - '0';
    
    newHeatingInfo.gpioStatuses[0] = (iGPIOStatuses & 0x1) > 0;
    newHeatingInfo.gpioStatuses[1] = (iGPIOStatuses & 0x2) > 0;
    //newHeatingInfo.gpioStatuses[2] = (iGPIOStatuses & 0x4) > 0;    
    
    psOriginal++;
    
    // Now we have schedule windows for each day - 3 windows per day, 7 days
    for (int iDay = 0; iDay < DAYS_IN_A_WEEK; iDay++)
    {
        for (int iWindow = 0; iWindow < WINDOWS_PER_DAY; iWindow++)
        {
            // Binary data - one char for each start and each end window time
            // Note that we subtract 1 from the values because we were attempting
            // to avoid NULL (0) chars from making it look like it was the end of 
            // the string, so we had added 1 at the PHP stage.
            newHeatingInfo.startWindows[iDay][iWindow] = (*psOriginal)-1;
            psOriginal++;
            newHeatingInfo.endWindows[iDay][iWindow] = (*psOriginal)-1;
            psOriginal++;
            /*
            // Read groups of 3 characters
            char szWindow[4];
            strncpy(szWindow, psOriginal, 3);
            szWindow[3] = 0; // null terminate
            newHeatingInfo.startWindows[iDay][iWindow] = char(int(szWindow));
            psOriginal+=3;
            
            // Read groups of 3 characters
            strncpy(szWindow, psOriginal, 3);
            szWindow[3] = 0; // null terminate
            newHeatingInfo.endWindows[iDay][iWindow] = char(int(szWindow));
            psOriginal+=3;
            */
        }
    }
    
    debugSerial_m.print("D:");
    debugSerial_m.println(newHeatingInfo.currentDay, DEC);
    debugSerial_m.print("T:");
    debugSerial_m.print(newHeatingInfo.currentTimeHours, DEC);
    debugSerial_m.print(":");
    debugSerial_m.println(newHeatingInfo.currentTimeMins, DEC);
    debugSerial_m.print("MOTD: ");
    debugSerial_m.println(newHeatingInfo.messageOfTheDay);    
    debugSerial_m.print("Apps:");
    debugSerial_m.println(newHeatingInfo.appliances, DEC);
    debugSerial_m.print("Adv: ");
    debugSerial_m.println(newHeatingInfo.advanceStatus, DEC);
    
    for (int iGPIO = 0; iGPIO < NUM_GPIO_PINS; iGPIO++)
    {             
        GET_PROGMEM_STRING(MSG_SERIALDEBUG_GPIO_SETTING);  // debugSerial_m.print("GPIO");
        debugSerial_m.print(vsProgmemBuffer_m);    
        debugSerial_m.print(iGPIO, DEC);
        debugSerial_m.print(": ");
        debugSerial_m.println(newHeatingInfo.gpioStatuses[iGPIO] ? "Y" : "N");
    }
    
    for (int iDay = 0; iDay < DAYS_IN_A_WEEK; iDay++)
    {        
        GET_PROGMEM_STRING(MSG_SERIALDEBUG_DAY_WINDOW_SETTING);  // debugSerial_m.print("Day");
        debugSerial_m.print(vsProgmemBuffer_m);    
        debugSerial_m.print(iDay, DEC);
        debugSerial_m.print(":");
        for (int iWindow = 0; iWindow < WINDOWS_PER_DAY; iWindow++)
        {
            debugSerial_m.print(" ");
            debugSerial_m.print(int(newHeatingInfo.startWindows[iDay][iWindow]), DEC);                        
            debugSerial_m.print("-");                        
            debugSerial_m.print(int(newHeatingInfo.endWindows[iDay][iWindow]), DEC);                        
        }
        debugSerial_m.println();
    }       
    
    // Copy fields from one to the other
    if (heatingInfo_m.advanceStatus != newHeatingInfo.advanceStatus)
    {
        newHeatingInfo.lastAdvanceStatusChangeMillis = millis();
    }
            
    // This was originally in twice.  No idea why.
    //  if (heatingInfo_m.advanceStatus != newHeatingInfo.advanceStatus)
    //  {
    //      heatingInfo_m.lastAdvanceStatusChangeMillis = millis();
    //  }
                        
    heatingInfo_m.currentDay = newHeatingInfo.currentDay;
    heatingInfo_m.maxTemp = newHeatingInfo.maxTemp;
    heatingInfo_m.currentTimeHours = newHeatingInfo.currentTimeHours;
    heatingInfo_m.currentTimeMins = newHeatingInfo.currentTimeMins;
    strncpy(heatingInfo_m.messageOfTheDay, newHeatingInfo.messageOfTheDay, MAX_MOTD_LENGTH);
    heatingInfo_m.appliances = newHeatingInfo.appliances;
    heatingInfo_m.advanceStatus = newHeatingInfo.advanceStatus;
    
    // FORLATER: updateRTCWithHeatingInfo(&heatingInfo_m);
    
    for (byte yGPIO = 0; yGPIO < NUM_GPIO_PINS; yGPIO++)
    {
        heatingInfo_m.gpioStatuses[yGPIO] = newHeatingInfo.gpioStatuses[yGPIO];
    }
    
    for (byte yDay = 0; yDay < DAYS_IN_A_WEEK; yDay++)
    {
        for (byte yWindow = 0; yWindow < WINDOWS_PER_DAY; yWindow++)
        {
            heatingInfo_m.startWindows[yDay][yWindow] = newHeatingInfo.startWindows[yDay][yWindow];
            heatingInfo_m.endWindows[yDay][yWindow] = newHeatingInfo.endWindows[yDay][yWindow];
        }
    }    
    
    lLastTimeCheckTime_m = millis();
}




 


// 
//  Triggers an HTTP status update message to the website by simply sending 
//  data via the UART.  
//  The HTTP response will be read by the checkForWiflyOutput method later.
//  Normally it will send its current state to the server (so that the server
//  can be aware that the user has pressed some buttons on the physical box)
//  but, if the system is just starting up, it should pass the value 'true'
//  for the bForceSynchronise_p parameter so that it doesn't try to overwrite
//  what's on the server.
//
void synchroniseWithInternet(bool bForceSynchronise_p)
{   
    debug("synchroniseWithInternet: start");
    // FORLATER: setUIState(&heatingInfo_m, UI_STATE_SYNCHRONISING, false);    
    
    
    debug("synchroniseWithInternet:1");
    int iResult;
    
    iResult = HeatingESP8266::ERROR_TIMED_OUT;
    
    bool isStillTryingToSynchronise = true;
    
    while (isStillTryingToSynchronise)
    {    
        if (bForceSynchronise_p)
        {
            debug("synchroniseWithInternet:force (start)");
            GET_PROGMEM_STRING(MSG_WIFLY_STARTUP_MESSAGE);  // "STARTUP"
            
            
            //wifly_m.readAllCharsOnWifly();
            //iResult = wifly_m.sendHttpRequest(vsProgmemBuffer_m, HTTP_BODY_SEARCH_STRING, MAX_HTTP_BODY_CHARS_TO_READ, HTTP_TIMEOUT);
            iResult = wifly_m.sendHttpRequest(vsProgmemBuffer_m, HTTP_TIMEOUT);
            debug("");
            debug("synchroniseWithInternet:force (end)");
        }
        else
        {
            debug("synchroniseWithInternet:normal (start)");
            // Buffer to store the non-Base64 encoded text = 4general + (7days * 3windows * 2times) chars = 46 chars
            int iSendBufferPos = 0;
            
            vsSendBuffer_m[iSendBufferPos++] = isHeatingInfoChangedLocally_m ? 'Y' : 'N';
            // Add 1 to these values so we don't inadvertently null-terminate the string!
            vsSendBuffer_m[iSendBufferPos++] = heatingInfo_m.appliances + 1;
            vsSendBuffer_m[iSendBufferPos++] = heatingInfo_m.advanceStatus + 1;
            vsSendBuffer_m[iSendBufferPos++] = heatingInfo_m.maxTemp + 1;
            
            for (byte yDay = 0; yDay < DAYS_IN_A_WEEK; yDay++)
            {
                for (byte yWindow = 0; yWindow < WINDOWS_PER_DAY; yWindow++)
                {
                    // Add 1 to each numeric value (max would've been 240 == 24:00 anyway, so no
                    // risk of overflows) so that we avoid sending '0' NULL characters.
                    vsSendBuffer_m[iSendBufferPos++] = (heatingInfo_m.startWindows[yDay][yWindow]) + 1;
                    vsSendBuffer_m[iSendBufferPos++] = (heatingInfo_m.endWindows[yDay][yWindow]) + 1;                                
                }
            }   
            
            vsSendBuffer_m[iSendBufferPos++] = (char) heatingInfo_m.ambientTemp + 30;  // Add an offset to avoid accidentally null terminating the string if it's 0C
            vsSendBuffer_m[iSendBufferPos++] = (isEnergised ? 'Y' : 'N');
            vsSendBuffer_m[iSendBufferPos] = 0; // Null terminate
            
            // The data, once it's base64 encoded will be longer, around 70 bytes
            
            base64Encode(vsSendBuffer_m, vsBase64SendBuffer_m, MAX_BASE64_SEND_BUFFER_SIZE);
            
            strcat(vsBase64SendBuffer_m, "*");
            
            GET_PROGMEM_STRING(MSG_SERIALDEBUG_SENDING_1);  // "Sending ["        
            debugSerial_m.print(vsProgmemBuffer_m);
            debugSerial_m.print(vsSendBuffer_m);        
            GET_PROGMEM_STRING(MSG_SERIALDEBUG_SENDING_2);  // "], b64 ["
            debugSerial_m.print(vsProgmemBuffer_m);
            debugSerial_m.print(vsBase64SendBuffer_m);                
            GET_PROGMEM_STRING(MSG_SERIALDEBUG_SENDING_3);  // "]"
            debugSerial_m.println(vsProgmemBuffer_m);
            debug("Calling sendrequest");
            //iResult = wifly_m.sendHttpRequest(vsBase64SendBuffer_m, HTTP_BODY_SEARCH_STRING, MAX_HTTP_BODY_CHARS_TO_READ, HTTP_TIMEOUT);        
            iResult = wifly_m.sendHttpRequest(vsBase64SendBuffer_m, HTTP_TIMEOUT);        
            debug("synchroniseWithInternet:normal (end)");
        }
        
        GET_PROGMEM_STRING(MSG_SERIALDEBUG_SENDING_RESULT);  // "Result "
        debugSerial_m.print(vsProgmemBuffer_m);
        debugSerial_m.println(iResult, DEC);
        
        
        if (HeatingESP8266::SUCCESS == iResult)
        {
            parseHeatingInfo(wifly_m.getHttpBody());
            isHeatingInfoChangedLocally_m = false;
            lLastSynchroniseSuccessTime_m = millis();
            iNumFailedInternetSyncs_m = 0;
            isStillTryingToSynchronise = false;  // So we can break out of the loop
            // FORLATER: setUIState(&heatingInfo_m, UI_STATE_SYNCHRONISATION_COMPLETE, false);
        }
        else
        {
            iNumFailedInternetSyncs_m++;
                   
            if (iNumFailedInternetSyncs_m >= MAX_FAILED_INTERNET_SYNCHRONISATIONS_BEFORE_HARD_RESET)
            {
                // FORLATER: setUIState(&heatingInfo_m, UI_STATE_HARD_REBOOTING_WIFLY, false);
                //iNumFailedInternetSyncs_m = 0;
                //isAtLeastOneSuccessfulInternetSync_m = false;  // hopefully it'll get set to 'true' soon!
                wifly_m.hardReset();
                isStillTryingToSynchronise = false;  // If it hasn't worked by now then give up for a while
            }
            else if (iNumFailedInternetSyncs_m >= MAX_FAILED_INTERNET_SYNCHRONISATIONS_BEFORE_SOFT_RESET)
            {
                // FORLATER: setUIState(&heatingInfo_m, UI_STATE_SOFT_REBOOTING_WIFLY, false);
                //iNumFailedInternetSyncs_m = 0;
                //isAtLeastOneSuccessfulInternetSync_m = false;  // hopefully it'll get set to 'true' soon!
                wifly_m.softReset();
            }
            else
            {
                // FORLATER: setUIState(&heatingInfo_m, UI_STATE_SYNCHRONISATION_FAILED, false);
            }
        }
        
        delay(1000);  // why??
        
        lLastSynchroniseSendTime_m = millis(); 
            
        // FORLATER: setUIState(&heatingInfo_m, UI_STATE_NORMAL, false);
        updateRelays();
        
        debug("synchroniseWithInternet: end");
    }
}


void notifyHeatingInfoChangedLocally()
{
    isHeatingInfoChangedLocally_m = true;
    updateRTCWithHeatingInfo(&heatingInfo_m);
}



unsigned long getLastSynchroniseSuccessTime()
{
    return lLastSynchroniseSuccessTime_m;    
}

unsigned long getMaxInternetSyncDelayMillis()
{
    return lMaxInternetSyncDelayMillis_m;
}



void setMaxInternetSyncDelayMillis(unsigned long lValue_p)
{
    lMaxInternetSyncDelayMillis_m = lValue_p;
}

int main(void)
{
    setup();
    while(true)
    {
        loop();
    }
    
}

#define DEBUG_ENABLED 1

void debug(char* psMessage_p)
{
    if(DEBUG_ENABLED)
    {
        debugSerial_m.print(micros());
        debugSerial_m.print(": ");
        debugSerial_m.println(psMessage_p);
        debugSerial_m.flush();
    }
}
