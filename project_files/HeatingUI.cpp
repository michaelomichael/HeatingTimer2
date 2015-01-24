#include "HeatingUI.h"
#include "Encoder.h"
#include "orz.h"
#include <math.h>


//
// Simple macro to copy the given string into a useable buffer called vsProgmemBuffer_m.
// example use here:
//
//      GET_PROGMEM_STRING(THREE);
//      realSerial.println(vsProgmemBuffer_m);
//


// #define debug(x) ;

unsigned long lLastUIStateChangeTime_m = 0;
byte yUIState_m = UI_STATE_NORMAL;
byte yPreviousUIState_m = UI_STATE_NORMAL;

int iEncoderValue_m = 0;
Encoder encoder_m(ENCODER_WHEEL_PIN_1, ENCODER_WHEEL_PIN_2);

LiquidCrystal lcd_m(LCD_RS, LCD_ENABLE, LCD_DATA4, LCD_DATA5, LCD_DATA6, LCD_DATA7); //7, 6, 5, 4, 3, 2);

int iLcdBrightness_m = 50;  // was 40



const char LONG_DAY_NAMES[DAYS_IN_A_WEEK][4] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
const char SHORT_DAY_NAMES[DAYS_IN_A_WEEK][3] = { "Su", "Mo", "Tu", "We", "Th", "Fr", "Sa" };


//const int MAX_PROGMEM_BUFFER_SIZE = 20;
char *vsProgmemBufferUI_m; //[MAX_PROGMEM_BUFFER_SIZE];


#define GET_PROGMEM_STRING(sProgCharString) vsProgmemBufferUI_m = ((char*)sProgCharString)
#define PROGMEM
#define prog_char const char


PROGMEM prog_char MSG_LCD_SOFT_REBOOTING_WIFLY[] = "Soft reboot ";
PROGMEM prog_char MSG_LCD_HARD_REBOOTING_WIFLY[] = "Hard reboot...  ";
PROGMEM prog_char MSG_LCD_SYNCHRONISING[] = "Synchronising..."; /*__attribute__ ((section(".text"))) */
PROGMEM prog_char MSG_LCD_SYNCHRONISED[] = "Yay!            ";
PROGMEM prog_char MSG_LCD_TIMED_OUT[] = "Timed out       ";
PROGMEM prog_char MSG_LCD_SET_MAX_TEMP_QUESTION[] = "Set max temp?";
PROGMEM prog_char MSG_LCD_CURRENT_TEMP[] = "Current:  ";
PROGMEM prog_char MSG_LCD_SET_MODE_QUESTION[] = "Set mode?";
PROGMEM prog_char MSG_LCD_CURRENT_MODE[] = "Current:  ";
PROGMEM prog_char MSG_LCD_MODE_OFF[] = "Off";
PROGMEM prog_char MSG_LCD_MODE_HW[] = "HW";
PROGMEM prog_char MSG_LCD_MODE_HW_CH[] = "HW&CH";
PROGMEM prog_char MSG_LCD_SET_WINDOW_QUESTION_1[] = "Set ";
PROGMEM prog_char MSG_LCD_SET_WINDOW_QUESTION_2[] = " times?";
PROGMEM prog_char MSG_LCD_SET_WINDOW_TO[] = " to ";
PROGMEM prog_char MSG_LCD_LAST_SYNC_WAS[] = "Last sync was";
PROGMEM prog_char MSG_LCD_LAST_SYNC_NEVER[] = "never!";
PROGMEM prog_char MSG_LCD_LAST_SYNC_AGES_AGO[] = "ages ago";
PROGMEM prog_char MSG_LCD_LAST_SYNC_MINUTES_AGO[] = " mins ago";
PROGMEM prog_char MSG_LCD_INTERNET_SYNC[] = "Internet sync";
PROGMEM prog_char MSG_LCD_INTERNET_SYNC_ENABLED_QUESTION[] = "enabled? ";
PROGMEM prog_char MSG_LCD_INTERNET_SYNC_ENABLED_YES[] = "YES";
PROGMEM prog_char MSG_LCD_INTERNET_SYNC_ENABLED_NO[] = "NO";
PROGMEM prog_char MSG_LCD_SET_SYNC_DELAY_QUESTION[] = "Set sync delay?";
PROGMEM prog_char MSG_LCD_SET_SYNC_DELAY_CURRENT[] = "Current: ";
PROGMEM prog_char MSG_LCD_SET_LCD_BRIGHTNESS_QUESTION[] = "Set LCD bright?";
PROGMEM prog_char MSG_LCD_SET_LCD_BRIGHTNESS_CURRENT[] = "Current: ";
PROGMEM prog_char MSG_LCD_SET_LCD_BRIGHTNESS[] = "Set LCD bright";
PROGMEM prog_char MSG_LCD_SET_LCD_BRIGHTNESS_NEW[] = "New:    [";
PROGMEM prog_char MSG_LCD_SET_CLOCK[] = "Set clock"     ;
PROGMEM prog_char MSG_LCD_SET_MAX_TEMP[] = "Set max temp";
PROGMEM prog_char MSG_LCD_SET_MAX_TEMP_NEW[] = "New:     [";
PROGMEM prog_char MSG_LCD_SET_MODE[] = "Set mode";
PROGMEM prog_char MSG_LCD_SET_MODE_NEW[] = "New:     ["        ;
PROGMEM prog_char MSG_LCD_SET_MODE_OFF[] = "Off  "                ;
PROGMEM prog_char MSG_LCD_SET_MODE_HW[] = "HW   ";
PROGMEM prog_char MSG_LCD_SET_MODE_BOTH[] = "HW&CH";
PROGMEM prog_char MSG_LCD_SET_SYNC_DELAY[] = "Set sync delay";
PROGMEM prog_char MSG_LCD_MODE_SHORT_OFF[] = "off";
PROGMEM prog_char MSG_LCD_MODE_SHORT_HW[] = "W "   ;
PROGMEM prog_char MSG_LCD_MODE_SHORT_BOTH[] = "WH";
PROGMEM prog_char MSG_LCD_SET_SYNC_DELAY_NEW[] = "New:    [";
PROGMEM prog_char MSG_LCD_SET_WINDOW_TIMES_1[] = "Set ";
PROGMEM prog_char MSG_LCD_SET_WINDOW_TIMES_2[] = " times";
PROGMEM prog_char MSG_LCD_UNKNOWN_UI_STATE[] = "Unknown UI state";
PROGMEM prog_char MSG_LCD_HELLO[] = "Hello";

/*
#define MSG_LCD_SYNCHRONISING "Synchronising..."
#define MSG_LCD_SYNCHRONISED  "Yay!            "
#define MSG_LCD_TIMED_OUT "Timed out       "
#define MSG_LCD_SET_MAX_TEMP_QUESTION "Set max temp?"
#define MSG_LCD_CURRENT_TEMP "Current:  "
#define MSG_LCD_SET_MODE_QUESTION "Set mode?"
#define MSG_LCD_CURRENT_MODE "Current:  "
#define MSG_LCD_MODE_OFF "Off"
#define MSG_LCD_MODE_HW "HW"
#define MSG_LCD_MODE_HW_CH "HW&CH"
#define MSG_LCD_SET_WINDOW_QUESTION_1 "Set "
#define MSG_LCD_SET_WINDOW_QUESTION_2 " times?"
#define MSG_LCD_SET_WINDOW_TO " to "
#define MSG_LCD_LAST_SYNC_WAS "Last sync was"
#define MSG_LCD_LAST_SYNC_NEVER "never!"
#define MSG_LCD_LAST_SYNC_AGES_AGO "ages ago"
#define MSG_LCD_LAST_SYNC_MINUTES_AGO " mins ago"
#define MSG_LCD_INTERNET_SYNC "Internet sync"
#define MSG_LCD_INTERNET_SYNC_ENABLED_QUESTION "enabled? "
#define MSG_LCD_INTERNET_SYNC_ENABLED_YES "YES"
#define MSG_LCD_INTERNET_SYNC_ENABLED_NO "NO"
#define MSG_LCD_SET_SYNC_DELAY_QUESTION "Set sync delay?"
#define MSG_LCD_SET_SYNC_DELAY_CURRENT "Current: "
#define MSG_LCD_SET_CLOCK "Set clock"     
#define MSG_LCD_SET_MAX_TEMP "Set max temp"
#define MSG_LCD_SET_MAX_TEMP_NEW "New:     ["
#define MSG_LCD_SET_MODE "Set mode"
#define MSG_LCD_SET_MODE_NEW "New:     ["        
#define MSG_LCD_SET_MODE_OFF "Off  "                
#define MSG_LCD_SET_MODE_HW "HW   "
#define MSG_LCD_SET_MODE_BOTH "HW&CH"
#define MSG_LCD_SET_SYNC_DELAY "Set sync delay"
#define MSG_LCD_MODE_SHORT_OFF "off"
#define MSG_LCD_MODE_SHORT_HW " W "   
#define MSG_LCD_MODE_SHORT_BOTH "W&H"
#define MSG_LCD_SET_SYNC_DELAY_NEW "New:    ["
#define MSG_LCD_SET_WINDOW_TIMES_1 "Set "
#define MSG_LCD_SET_WINDOW_TIMES_2 " times"
#define MSG_LCD_UNKNOWN_UI_STATE "Unknown UI state"
#define MSG_LCD_HELLO "Hello"

*/




const char* getLongDayName(byte yDayIndex_p)
{
    return LONG_DAY_NAMES[yDayIndex_p];
}

const char* getShortDayName(byte yDayIndex_p)
{
    return SHORT_DAY_NAMES[yDayIndex_p];
}


int getLcdBrightness()
{
    return iLcdBrightness_m;
}


void setLcdBrightness(int iValue_p)
{
    iLcdBrightness_m = iValue_p;
    analogWrite(LCD_VO, iLcdBrightness_m);  // TODO - this is contrast not brightness!
}




void waitForDebounce(int iPin_p)
{
    unsigned long lStartTime = millis();
    
    int iOriginalState = digitalRead(iPin_p);
    
    while (iOriginalState == digitalRead(iPin_p))
    {
        delay(100);
        if (millisSince(lStartTime) > MAX_DEBOUNCE_MILLIS)
        {
            break;
        }
    }
    delay(100);
}




void setUIState(HeatingInfo *pHeatingInfo_p, byte yNewUIState_p, bool isPartialDisplayUpdate_p)
{
    lLastUIStateChangeTime_m = millis();
    yUIState_m = yNewUIState_p;
    
    debug("setUIState: about to call updateDisplay");
    updateDisplay(pHeatingInfo_p, isPartialDisplayUpdate_p);
    
    if (UI_STATE_NORMAL == yNewUIState_p)
    {
        debug("setUIState: Updating relays...");
        updateRelays();
    }
    debug("setUIState: end");
    
}


bool isDialogState()
{
    return UI_STATE_SYNCHRONISING == yUIState_m  ||  
           UI_STATE_SYNCHRONISATION_COMPLETE == yUIState_m  ||  
           UI_STATE_SYNCHRONISATION_FAILED == yUIState_m  ||  
           UI_STATE_SOFT_REBOOTING_WIFLY == yUIState_m  ||
           UI_STATE_HARD_REBOOTING_WIFLY == yUIState_m;
}


void updateDisplay(HeatingInfo *pHeatingInfo_p, bool isPartial_p)
{
    lcd_m.clear();
    lcd_m.setCursor(0, 0);
    
   // lcd_m.print("Display ");
   // lcd_m.print(yUIState_m, DEC);
    
    
    if (UI_STATE_NORMAL == yUIState_m  ||  isDialogState())
    {
        lcd_m.print(getShortDayName(pHeatingInfo_p->currentDay));   
        lcd_m.print(" ");
        
        if (pHeatingInfo_p->currentTimeHours < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeHours, DEC);
        lcd_m.print(":");
        if (pHeatingInfo_p->currentTimeMins < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeMins, DEC);
        lcd_m.print(" ");
        
        lcd_m.print(pHeatingInfo_p->ambientTemp, DEC);
        lcd_m.print(DEGREES_SYMBOL);
        
        if (pHeatingInfo_p->ambientTemp > -10  &&  pHeatingInfo_p->ambientTemp < 100)
        {
            lcd_m.print(" ");
        }
        
        if (APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_MODE_SHORT_BOTH);  // lcd_m.print("W&H");
            lcd_m.print(MSG_LCD_MODE_SHORT_BOTH);
        }
        else if (APPLIANCES_HOT_WATER_ONLY == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_MODE_SHORT_HW);  // lcd_m.print(" W ");   
            lcd_m.print(MSG_LCD_MODE_SHORT_HW);
        }
        else
        {
            //GET_PROGMEM_STRING(MSG_LCD_MODE_SHORT_OFF);  // lcd_m.print("off");
            lcd_m.print(MSG_LCD_MODE_SHORT_OFF);
        }
        
        switch (pHeatingInfo_p->advanceStatus)
        {
            case ADVANCE_AS_SCHEDULED:
                lcd_m.print(" ");
                break;
            
            case ADVANCE_PLUS_ONE_HOUR:
                lcd_m.print("+");
                break;
                
            case ADVANCED_TO_NEXT_EVENT:
                lcd_m.print(RIGHT_ARROW_SYMBOL);
                break;
            
            default:
                lcd_m.print("?");
                break;
        }
          
        debug("updateDisplay1");
        lcd_m.setCursor(0, 1);
        debug("updateDisplay2");
        if (UI_STATE_SYNCHRONISING == yUIState_m)
        {
            //GET_PROGMEM_STRING(MSG_LCD_SYNCHRONISING);  // lcd_m.print("Synchronising...");
            lcd_m.print(MSG_LCD_SYNCHRONISING);            
        }
        else if (UI_STATE_SYNCHRONISATION_COMPLETE == yUIState_m)
        {
            //GET_PROGMEM_STRING(MSG_LCD_SYNCHRONISED);  // lcd_m.print("Synchronised!   ");
            lcd_m.print(MSG_LCD_SYNCHRONISED);            
        }
        else if (UI_STATE_SYNCHRONISATION_FAILED == yUIState_m)
        {
            //GET_PROGMEM_STRING(MSG_LCD_TIMED_OUT);  // lcd_m.print("Timed out       ");   
            lcd_m.print(MSG_LCD_TIMED_OUT);            
        }    
        else if (UI_STATE_SOFT_REBOOTING_WIFLY == yUIState_m)
        {
            lcd_m.print(MSG_LCD_SOFT_REBOOTING_WIFLY);
            lcd_m.print(getNumFailedInternetSyncs_m(), DEC);
        }    
        else if (UI_STATE_HARD_REBOOTING_WIFLY == yUIState_m)
        {
            lcd_m.print(MSG_LCD_HARD_REBOOTING_WIFLY);
            lcd_m.print(getNumFailedInternetSyncs_m(), DEC);
        }
        else
        {            
            lcd_m.print(pHeatingInfo_p->messageOfTheDay);      
        }
        debug("updateDisplay3");
    }
    else if (UI_STATE_MENU_1_MAX_TEMPERATURE == yUIState_m)
    {
        //GET_PROGMEM_STRING(MSG_LCD_SET_MAX_TEMP_QUESTION);  // "Set max temp?"
        lcd_m.print(MSG_LCD_SET_MAX_TEMP_QUESTION);
        lcd_m.setCursor(0, 1);
        //GET_PROGMEM_STRING(MSG_LCD_CURRENT_TEMP);  // "Current:  "
        lcd_m.print(MSG_LCD_CURRENT_TEMP);
        lcd_m.print(pHeatingInfo_p->maxTemp, DEC);
        lcd_m.print(DEGREES_SYMBOL);        
    }
    else if (UI_STATE_MENU_2_APPLIANCES == yUIState_m)
    {
        //GET_PROGMEM_STRING(MSG_LCD_SET_MODE_QUESTION);  // "Set mode?"
        lcd_m.print(MSG_LCD_SET_MODE_QUESTION);
        lcd_m.setCursor(0, 1);
        //GET_PROGMEM_STRING(MSG_LCD_CURRENT_MODE);  // "Current:  "
        lcd_m.print(MSG_LCD_CURRENT_MODE);
        if (APPLIANCES_NONE == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_MODE_OFF);  // "Off" 
            lcd_m.print(MSG_LCD_MODE_OFF);
        }
        else if (APPLIANCES_HOT_WATER_ONLY == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_MODE_HW);  // "HW"
            lcd_m.print(MSG_LCD_MODE_HW);
        }
        else if (APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_MODE_HW_CH);  // "HW&CH"
            lcd_m.print(MSG_LCD_MODE_HW_CH);
        }        
    }
    else if (yUIState_m >= UI_STATE_MENU_3_WINDOW_TIMES_START  &&  yUIState_m <= UI_STATE_MENU_3_WINDOW_TIMES_END)
    {
        byte yDay = (byte) ((yUIState_m - UI_STATE_MENU_3_WINDOW_TIMES_START) / 3);
        byte yWindow = (byte) ((yUIState_m - UI_STATE_MENU_3_WINDOW_TIMES_START) % 3);
        byte yStartTime = pHeatingInfo_p->startWindows[yDay][yWindow];
        byte yEndTime = pHeatingInfo_p->endWindows[yDay][yWindow];
        
        //GET_PROGMEM_STRING(MSG_LCD_SET_WINDOW_QUESTION_1);  // "Set "
        lcd_m.print(MSG_LCD_SET_WINDOW_QUESTION_1);
        lcd_m.print(getLongDayName(yDay));
        lcd_m.print(" ");
        lcd_m.print(yWindow+1, DEC);
        
        //GET_PROGMEM_STRING(MSG_LCD_SET_WINDOW_QUESTION_2);  // " times?"
        lcd_m.print(MSG_LCD_SET_WINDOW_QUESTION_2);
        lcd_m.setCursor(0, 1);
        lcd_m.print(" ");
        if (yStartTime < 100)
        {
            lcd_m.print("0");
        }
        lcd_m.print(int(yStartTime/10), DEC);
        lcd_m.print(":");
        lcd_m.print(yStartTime % 10, DEC);
        lcd_m.print("0");
        
        //GET_PROGMEM_STRING(MSG_LCD_SET_WINDOW_TO);  // " to "
        lcd_m.print(MSG_LCD_SET_WINDOW_TO);
        if (yEndTime < 100)
        {
            lcd_m.print("0");
        }
        lcd_m.print(int(yEndTime/10), DEC);
        lcd_m.print(":");        
        lcd_m.print(yEndTime % 10, DEC);
        lcd_m.print("0");
    }
    else if (UI_STATE_MENU_4_CLOCK == yUIState_m)
    {
           // TODO - continue the progmem settings here
        lcd_m.print("Set clock?");
        lcd_m.setCursor(0, 1);
        lcd_m.print("  ");
        lcd_m.print(getLongDayName(pHeatingInfo_p->currentDay));        
        lcd_m.print("  ");
        if (pHeatingInfo_p->currentTimeHours < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeHours, DEC);
        lcd_m.print(":");
        if (pHeatingInfo_p->currentTimeMins < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeMins, DEC);
    }
    
    
    else if (UI_STATE_MENU_5_DISPLAY_LAST_SYNC_TIME == yUIState_m)
    {
        unsigned long lMillisSinceLastSync = millisSince(getLastSynchroniseSuccessTime());
        unsigned long lMinsSinceLastSync = lMillisSinceLastSync / 60000L;   
                
        //GET_PROGMEM_STRING(MSG_LCD_LAST_SYNC_WAS);  // lcd_m.print("Last sync was");        
        lcd_m.print(MSG_LCD_LAST_SYNC_WAS);
        lcd_m.setCursor(0, 1);
        
        if (0 == getLastSynchroniseSuccessTime())
        {            
            //GET_PROGMEM_STRING(MSG_LCD_LAST_SYNC_NEVER);  // lcd_m.print("never!");
            lcd_m.print(MSG_LCD_LAST_SYNC_NEVER);
            
        }
        else if (lMinsSinceLastSync > 999999)
        {
            //GET_PROGMEM_STRING(MSG_LCD_LAST_SYNC_AGES_AGO);  // lcd_m.print("ages ago");
            lcd_m.print(MSG_LCD_LAST_SYNC_AGES_AGO);
        }
        else
        {
            lcd_m.print(lMinsSinceLastSync, DEC);
            //GET_PROGMEM_STRING(MSG_LCD_LAST_SYNC_MINUTES_AGO);  // lcd_m.print(" mins ago");
            lcd_m.print(MSG_LCD_LAST_SYNC_MINUTES_AGO);
        }        
    }
    else if (UI_STATE_MENU_6_TOGGLE_SYNC == yUIState_m)
    {
        //GET_PROGMEM_STRING(MSG_LCD_INTERNET_SYNC);  // lcd_m.print("Internet sync");
        lcd_m.print(MSG_LCD_INTERNET_SYNC);
        lcd_m.setCursor(0, 1);
        //GET_PROGMEM_STRING(MSG_LCD_INTERNET_SYNC_ENABLED_QUESTION);  // lcd_m.print("enabled? ");        
        lcd_m.print(MSG_LCD_INTERNET_SYNC_ENABLED_QUESTION);
        if (pHeatingInfo_p->internetSyncEnabled)
        {
            //GET_PROGMEM_STRING(MSG_LCD_INTERNET_SYNC_ENABLED_YES);  // "YES"
            lcd_m.print(MSG_LCD_INTERNET_SYNC_ENABLED_YES);
        }
        else
        {
            //GET_PROGMEM_STRING(MSG_LCD_INTERNET_SYNC_ENABLED_NO);  // "NO"
            lcd_m.print(MSG_LCD_INTERNET_SYNC_ENABLED_NO);
        }
        //lcd_m.print(vsProgmemBufferUI_m);
    }
    else if (UI_STATE_MENU_7_SYNC_DELAY == yUIState_m)
    {        
        //GET_PROGMEM_STRING(MSG_LCD_SET_SYNC_DELAY_QUESTION);  // lcd_m.print("Set sync delay?");
        lcd_m.print(MSG_LCD_SET_SYNC_DELAY_QUESTION);        
        lcd_m.setCursor(0, 1);
        //GET_PROGMEM_STRING(MSG_LCD_SET_SYNC_DELAY_CURRENT);  // lcd_m.print("Current: ");
        lcd_m.print(MSG_LCD_SET_SYNC_DELAY_CURRENT);        
        lcd_m.print(int(getMaxInternetSyncDelayMillis()/1000), DEC);        
        lcd_m.print("s");
    }
    else if (UI_STATE_MENU_8_LCD_BRIGHTNESS == yUIState_m)
    {
        lcd_m.print(MSG_LCD_SET_LCD_BRIGHTNESS_QUESTION);
        lcd_m.setCursor(0, 1);
        lcd_m.print(MSG_LCD_SET_LCD_BRIGHTNESS_CURRENT);
        lcd_m.print(int(getLcdBrightness()), DEC);
        lcd_m.print("%");
    }    
    else if (UI_STATE_EDIT_CLOCK_1_DAY_OF_WEEK == yUIState_m)
    {
        //GET_PROGMEM_STRING(MSG_LCD_SET_CLOCK);  // lcd_m.print("Set clock");     
        lcd_m.print(MSG_LCD_SET_CLOCK);        
        lcd_m.setCursor(0, 1);
        lcd_m.print(" [");
        lcd_m.print(getLongDayName(pHeatingInfo_p->currentDay));        
        lcd_m.print("] ");
        if (pHeatingInfo_p->currentTimeHours < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeHours, DEC);
        lcd_m.print(":");
        if (pHeatingInfo_p->currentTimeMins < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeMins, DEC);
    }    
    else if (UI_STATE_EDIT_CLOCK_2_HOUR == yUIState_m)
    {
        //GET_PROGMEM_STRING(MSG_LCD_SET_CLOCK);  // lcd_m.print("Set clock");     
        lcd_m.print(MSG_LCD_SET_CLOCK);        
        lcd_m.setCursor(0, 1);
        lcd_m.print("  ");
        lcd_m.print(getLongDayName(pHeatingInfo_p->currentDay));        
        lcd_m.print(" [");
        if (pHeatingInfo_p->currentTimeHours < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeHours, DEC);
        lcd_m.print("]");
        if (pHeatingInfo_p->currentTimeMins < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeMins, DEC);
    }    
    else if (UI_STATE_EDIT_CLOCK_3_MINUTES == yUIState_m)
    {
        //GET_PROGMEM_STRING(MSG_LCD_SET_CLOCK);  // lcd_m.print("Set clock");     
        lcd_m.print(MSG_LCD_SET_CLOCK);        
        lcd_m.setCursor(0, 1);
        lcd_m.print("  ");
        lcd_m.print(getLongDayName(pHeatingInfo_p->currentDay));        
        lcd_m.print("  ");
        if (pHeatingInfo_p->currentTimeHours < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeHours, DEC);
        lcd_m.print("[");
        if (pHeatingInfo_p->currentTimeMins < 10)
        {
            lcd_m.print("0");
        }
        lcd_m.print(pHeatingInfo_p->currentTimeMins, DEC);   
        lcd_m.print("]");
    }    
    else if (UI_STATE_EDIT_MAX_TEMPERATURE == yUIState_m)
    {     
        //GET_PROGMEM_STRING(MSG_LCD_SET_MAX_TEMP);  // lcd_m.print("Set max temp");
        lcd_m.print(MSG_LCD_SET_MAX_TEMP);        
        lcd_m.setCursor(0, 1);
        //GET_PROGMEM_STRING(MSG_LCD_SET_MAX_TEMP_NEW);  // lcd_m.print("New:     [");
        lcd_m.print(MSG_LCD_SET_MAX_TEMP_NEW);        
        lcd_m.print(pHeatingInfo_p->maxTemp, DEC);
        lcd_m.print(DEGREES_SYMBOL);   
        lcd_m.print("]");
    }
    else if (UI_STATE_EDIT_APPLIANCES == yUIState_m)
    {
        //GET_PROGMEM_STRING(MSG_LCD_SET_MODE);  // lcd_m.print("Set mode");
        lcd_m.print(MSG_LCD_SET_MODE);        
        lcd_m.setCursor(0, 1);
        //GET_PROGMEM_STRING(MSG_LCD_SET_MODE_NEW);  // lcd_m.print("New:     [");
        lcd_m.print(MSG_LCD_SET_MODE_NEW);        
        if (APPLIANCES_NONE == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_SET_MODE_OFF);  // lcd_m.print("Off  ");
            lcd_m.print(MSG_LCD_SET_MODE_OFF);                
        }
        else if (APPLIANCES_HOT_WATER_ONLY == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_SET_MODE_HW);  // lcd_m.print("HW   ");
            lcd_m.print(MSG_LCD_SET_MODE_HW);                
        }
        else if (APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING == pHeatingInfo_p->appliances)
        {
            //GET_PROGMEM_STRING(MSG_LCD_SET_MODE_BOTH);  // lcd_m.print("HW&CH");
            lcd_m.print(MSG_LCD_SET_MODE_BOTH);                
        }
        lcd_m.print("]");
    }
    else if (UI_STATE_EDIT_SYNC_DELAY == yUIState_m)
    {        
        //GET_PROGMEM_STRING(MSG_LCD_SET_SYNC_DELAY);  // lcd_m.print("Set sync delay");
        lcd_m.print(MSG_LCD_SET_SYNC_DELAY);                 
        lcd_m.setCursor(0, 1);        
        //GET_PROGMEM_STRING(MSG_LCD_SET_SYNC_DELAY_NEW);  // lcd_m.print("New:    [");
        lcd_m.print(MSG_LCD_SET_SYNC_DELAY_NEW);                 
        lcd_m.print(int(getMaxInternetSyncDelayMillis()/1000), DEC);        
        lcd_m.print("s]");   
    }
    else if (UI_STATE_EDIT_LCD_BRIGHTNESS == yUIState_m)
    {
        lcd_m.print(MSG_LCD_SET_LCD_BRIGHTNESS);
        lcd_m.setCursor(0, 1);
        lcd_m.print(MSG_LCD_SET_LCD_BRIGHTNESS_NEW);
        lcd_m.print(int(getLcdBrightness()), DEC);
        lcd_m.print("%]");
    }
    else if (yUIState_m >= UI_STATE_EDIT_WINDOW_TIMES_START  &&  yUIState_m <= UI_STATE_EDIT_WINDOW_TIMES_END)
    {
        int iIndex = yUIState_m - UI_STATE_EDIT_WINDOW_TIMES_START;
        //
        //  0   Day 0   Window 0    Start
        //  1   Day 0   Window 0    End
        //  2   Day 0   Window 1    Start
        //  3   Day 0   Window 1    End  
        //  4   Day 0   Window 2    Start
        //  5   Day 0   Window 2    End  
        //  6   Day 1   Window 0    Start
        //  7   Day 1   Window 0    Start
        //
        byte yDay = (byte) (iIndex / 6);
        byte yWindow = (byte) ((iIndex % 6) / 2);        
        bool isStartTime = (0 == (iIndex % 2));
        byte yStartTime = pHeatingInfo_p->startWindows[yDay][yWindow];
        byte yEndTime = pHeatingInfo_p->endWindows[yDay][yWindow];
        
        //GET_PROGMEM_STRING(MSG_LCD_SET_WINDOW_TIMES_1);  // lcd_m.print("Set ");
        lcd_m.print(MSG_LCD_SET_WINDOW_TIMES_1);                 
        lcd_m.print(getLongDayName(yDay));
        lcd_m.print(" ");
        lcd_m.print(yWindow+1, DEC);        
        //GET_PROGMEM_STRING(MSG_LCD_SET_WINDOW_TIMES_2);  // lcd_m.print(" times");
        lcd_m.print(MSG_LCD_SET_WINDOW_TIMES_2);                 
        lcd_m.setCursor(0, 1);
        
        if (isStartTime)
        {
            lcd_m.print("[");
            if (yStartTime < 100)
            {
                lcd_m.print("0");
            }
            lcd_m.print(int(yStartTime / 10), DEC);
            lcd_m.print(":");
            lcd_m.print(yStartTime % 10, DEC);
            lcd_m.print("0]to ");           
            if (yEndTime < 100)
            {
                lcd_m.print("0");
            }
            lcd_m.print(int(yEndTime / 10), DEC);
            lcd_m.print(":");
            lcd_m.print(yEndTime % 10, DEC);
            lcd_m.print("0");
        }
        else
        {
            lcd_m.print(" ");
            if (yStartTime < 100)
            {
                lcd_m.print("0");
            }
            lcd_m.print(int(yStartTime / 10), DEC);
            lcd_m.print(":");
            lcd_m.print(yStartTime % 10, DEC);
            lcd_m.print("0 to[");           
            if (yEndTime < 100)
            {
                lcd_m.print("0");
            }            
            lcd_m.print(int(yEndTime / 10), DEC);
            lcd_m.print(":");
            lcd_m.print(yEndTime % 10, DEC);
            lcd_m.print("0]");
        }
    }
    else
    {
        // ERROR?
        //GET_PROGMEM_STRING(MSG_LCD_UNKNOWN_UI_STATE);  // lcd_m.print("Unknown UI state");
        lcd_m.print(MSG_LCD_UNKNOWN_UI_STATE);                 
        lcd_m.setCursor(0,1);
        lcd_m.print(yUIState_m, DEC);
        delay(3000);
        setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);
    }
    
    
    debug("End of updateDisplay()");
}


int abs(int i)
{
    if (i < 0)
    {
        return -i;
    }
    else
    {
        return i;
    }
}


int constrain(int iValue_p, int iMin_p, int iMax_p)
{
    if (iValue_p < iMin_p)
    {
        return iMin_p;
    }
    
    
    else if (iValue_p > iMax_p)
    {
        return iMax_p;
    }
    else
    {
        return iValue_p;
    }
}




void initUI()
{
    debug("start of initUI()");
    setPinMode(ADVANCE_BUTTON_PIN, PIN_MODE_DIGITAL_READ);    
    setPinMode(ENCODER_BUTTON_PIN, PIN_MODE_DIGITAL_READ);
    
    //pinMode(BACK_BUTTON_PIN, INPUT);    

    
    setPinMode(LCD_VSS, PIN_MODE_DIGITAL_WRITE);
    setPinMode(LCD_VDD, PIN_MODE_DIGITAL_WRITE);
    setPinMode(LCD_VO, PIN_MODE_ANALOG_WRITE);
    setPinMode(LCD_RW, PIN_MODE_DIGITAL_WRITE);
    setPinMode(LCD_DATA0, PIN_MODE_DIGITAL_WRITE);
    setPinMode(LCD_DATA1, PIN_MODE_DIGITAL_WRITE);
    setPinMode(LCD_DATA2, PIN_MODE_DIGITAL_WRITE);
    setPinMode(LCD_DATA3, PIN_MODE_DIGITAL_WRITE);
    //setPinMode(LCD_A, PIN_MODE_ANALOG_WRITE);  // Conflicts with AnalogRead on PC1 unforts
    setPinMode(LCD_A, PIN_MODE_DIGITAL_WRITE);
    setPinMode(LCD_K, PIN_MODE_DIGITAL_WRITE);
    
    digitalWrite(LCD_VSS, LOW);
    digitalWrite(LCD_VDD, HIGH);
    digitalWrite(LCD_RW, LOW);
    digitalWrite(LCD_DATA0, LOW);
    digitalWrite(LCD_DATA1, LOW);
    digitalWrite(LCD_DATA2, LOW);
    digitalWrite(LCD_DATA3, LOW);
    //analogWrite(LCD_A, 10);
    digitalWrite(LCD_A, HIGH);
    digitalWrite(LCD_K, LOW);
    
    analogWrite(LCD_VO, iLcdBrightness_m); 

    
    lcd_m.begin(16, 2);
    lcd_m.clear();        
    //GET_PROGMEM_STRING(MSG_LCD_HELLO);  // lcd_m.print("Hello");
    //lcd_m.print("Hi!");   
        lcd_m.print(MSG_LCD_HELLO);
    debug("Ok, in middle of initUI()");
    
    encoder_m.write(0);
    //setPinMode(LCD_BACKLIGHT_PIN, PIN_MODE_DIGITAL_WRITE);
    //digitalWrite(LCD_BACKLIGHT_PIN, HIGH);
    //setPinMode(LCD_BACKLIGHT_PIN, PIN_MODE_ANALOG_WRITE);
    //analogWrite(LCD_BACKLIGHT_PIN, iLcdBrightness_m);
    
    
    
    delay(1000);
}




void checkForAdvanceAndEncoderInputs(HeatingInfo *pHeatingInfo_p)
{
    //debug("checkForAdvanceAndEncoderInputs: start");    
    if (! digitalRead(ADVANCE_BUTTON_PIN))  // It's normally high - pushing the button grounds it.
    {
        debug("checkForAdvanceAndEncoderInputs: Advance button pressed");
        notifyHeatingInfoChangedLocally();
                
        pHeatingInfo_p->advanceStatus = (pHeatingInfo_p->advanceStatus + 1) % 3;
        pHeatingInfo_p->lastAdvanceStatusChangeMillis = millis();
        updateDisplay(pHeatingInfo_p, false);
        updateRelays();
        waitForDebounce(ADVANCE_BUTTON_PIN);
        debug("checkForAdvanceAndEncoderInputs: Done processing the advance button");
    }

    
    if (! digitalRead(ENCODER_BUTTON_PIN))  //  It's normally high - pushing the button grounds it.
    {
        debug("checkForAdvanceAndEncoderInputs: Encoder pin pressed");
        
        if (UI_STATE_NORMAL == yUIState_m  ||  isDialogState())
        {
            // Do nothing   
        }
        else if (UI_STATE_MENU_1_MAX_TEMPERATURE == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_MAX_TEMPERATURE, false);
        }
        else if (UI_STATE_MENU_2_APPLIANCES == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_APPLIANCES, false);
        }
        else if (yUIState_m >= UI_STATE_MENU_3_WINDOW_TIMES_START  &&  yUIState_m <= UI_STATE_MENU_3_WINDOW_TIMES_END)
        {
            byte yDay = (byte) ((yUIState_m - UI_STATE_MENU_3_WINDOW_TIMES_START) / 3);
            byte yWindow = (byte) ((yUIState_m - UI_STATE_MENU_3_WINDOW_TIMES_START) % 3);
     
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_WINDOW_TIMES_START + (yDay * 6) + (yWindow * 2), false);
        }
        else if (UI_STATE_MENU_4_CLOCK == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_CLOCK_1_DAY_OF_WEEK, false);
        }
        else if (UI_STATE_MENU_5_DISPLAY_LAST_SYNC_TIME == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_CLOCK_1_DAY_OF_WEEK, false);
        }
        else if (UI_STATE_MENU_6_TOGGLE_SYNC == yUIState_m)
        {
            pHeatingInfo_p->internetSyncEnabled = ! pHeatingInfo_p->internetSyncEnabled;
            notifyHeatingInfoChangedLocally();
            updateDisplay(pHeatingInfo_p, true);
        }
        else if (UI_STATE_MENU_7_SYNC_DELAY == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_SYNC_DELAY, false);   
        }
        else if (UI_STATE_MENU_8_LCD_BRIGHTNESS == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_LCD_BRIGHTNESS, false);
        }
        else if (UI_STATE_EDIT_MAX_TEMPERATURE == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);
        }
        else if (UI_STATE_EDIT_APPLIANCES == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);
        }
        else if (UI_STATE_EDIT_SYNC_DELAY == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);   
        }
        else if (UI_STATE_EDIT_LCD_BRIGHTNESS == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);
        }
        else if (UI_STATE_EDIT_CLOCK_1_DAY_OF_WEEK == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_CLOCK_2_HOUR, false);
        }
        else if (UI_STATE_EDIT_CLOCK_2_HOUR == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_EDIT_CLOCK_3_MINUTES, false);
        }
        else if (UI_STATE_EDIT_CLOCK_3_MINUTES == yUIState_m)
        {
            setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);
        }
        else if (yUIState_m >= UI_STATE_EDIT_WINDOW_TIMES_START  &&  yUIState_m <= UI_STATE_EDIT_WINDOW_TIMES_END)
        {
            bool isStartTime = (0 == (yUIState_m % 2));
            
            if (isStartTime)
            {
                setUIState(pHeatingInfo_p, yUIState_m+1, false);
            }
            else
            {
                setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);
            }
        }   
        else
        {
            // Unknown state?
        }
        
        debug("checkForAdvanceAndEncoderInputs: Waiting for debounce on encoder button pin");
        waitForDebounce(ENCODER_BUTTON_PIN);
        debug("checkForAdvanceAndEncoderInputs: Done waiting for debounce on encoder button pin");
        debug("checkForAdvanceAndEncoderInputs: Done processing the encoder button");
    }
    
    
    
    
    int iOldEncoderValue = iEncoderValue_m;
    int iNewEncoderValue = encoder_m.read();
    
    #define ENCODER_STEPS_PER_DETENT 3
    
    if (abs(iNewEncoderValue - iOldEncoderValue) >= ENCODER_STEPS_PER_DETENT)
    {
        
        debug("checkForAdvanceAndEncoderInputs: encoder value has updated!");
        iEncoderValue_m = iNewEncoderValue;
        debug("Setting iEncoderValue_m");
        int iEncoderDifference = (iNewEncoderValue - iOldEncoderValue) / ENCODER_STEPS_PER_DETENT;
        
        
        // Do something   
        if (yUIState_m >= UI_STATE_MENU_MIN  &&  yUIState_m <= UI_STATE_MENU_MAX)
        {            
            debug("Menu");
            // Just rotate around the menus 
           setUIState(pHeatingInfo_p, constrain(yUIState_m + iEncoderDifference, UI_STATE_MENU_MIN, UI_STATE_MENU_MAX), false);
        }
        
        else if (UI_STATE_EDIT_MAX_TEMPERATURE == yUIState_m)
        {
            debug("Max temp");
            // Twiddle temp
            pHeatingInfo_p->maxTemp = constrain(pHeatingInfo_p->maxTemp + iEncoderDifference, 0, 30);            
            notifyHeatingInfoChangedLocally();
            updateDisplay(pHeatingInfo_p, true);
            // No need to update relays as it'll happen when you go back to NORMAL UI state
        }
        else if (UI_STATE_EDIT_APPLIANCES == yUIState_m)
        {
            // Twiddle appliances
            pHeatingInfo_p->appliances = constrain(pHeatingInfo_p->appliances + iEncoderDifference, 0, APPLIANCES_MAX);
            notifyHeatingInfoChangedLocally();
            updateDisplay(pHeatingInfo_p, true);            
            // No need to update relays as it'll happen when you go back to NORMAL UI state
        }
        else if (UI_STATE_EDIT_SYNC_DELAY == yUIState_m)
        {
            // Twiddle delay            
            setMaxInternetSyncDelayMillis(constrain(getMaxInternetSyncDelayMillis() + (iEncoderDifference*10000UL), 10000UL, 3600000UL));
            updateDisplay(pHeatingInfo_p, true);               
        }
        else if (UI_STATE_EDIT_LCD_BRIGHTNESS == yUIState_m)
        {
            // Twiddle brightness
            setLcdBrightness(constrain(getLcdBrightness() + (iEncoderDifference*5), 0, 100));
            updateDisplay(pHeatingInfo_p, true);
        }
        else if (UI_STATE_EDIT_CLOCK_1_DAY_OF_WEEK == yUIState_m)
        {
            // Twiddle day
            pHeatingInfo_p->currentDay = constrain(pHeatingInfo_p->currentDay + iEncoderDifference, 0, DAYS_IN_A_WEEK-1);
            notifyHeatingInfoChangedLocally();
            updateDisplay(pHeatingInfo_p, true);
        }
        else if (UI_STATE_EDIT_CLOCK_2_HOUR == yUIState_m)
        {
            // Twiddle hour
            pHeatingInfo_p->currentTimeHours = constrain(pHeatingInfo_p->currentTimeHours + iEncoderDifference, 0, 23);
            notifyHeatingInfoChangedLocally();
            updateDisplay(pHeatingInfo_p, true);
        }
        else if (UI_STATE_EDIT_CLOCK_3_MINUTES == yUIState_m)
        {
            // Twiddle mins
            pHeatingInfo_p->currentTimeMins = constrain(pHeatingInfo_p->currentTimeMins + iEncoderDifference, 0, 59);
            notifyHeatingInfoChangedLocally();
            updateDisplay(pHeatingInfo_p, true);
        }
        else if (yUIState_m >= UI_STATE_EDIT_WINDOW_TIMES_START  &&  yUIState_m <= UI_STATE_EDIT_WINDOW_TIMES_END)
        {   
            
            // Figure out which one it is, update something, and refresh the LCD
            // This first bit is the same as in the LCD update method
            int iIndex = yUIState_m - UI_STATE_EDIT_WINDOW_TIMES_START;
            byte yDay = (byte) (iIndex / 6);
            byte yWindow = (byte) ((iIndex % 6) / 2);        
            bool isStartTime = (0 == (iIndex % 2));
            
            int iOriginalValue;
            int iNewValue;
            
            if (isStartTime)
            {
                iOriginalValue = (int) pHeatingInfo_p->startWindows[yDay][yWindow];                
            }
            else
            {
                iOriginalValue = (int) pHeatingInfo_p->endWindows[yDay][yWindow];
            }
            
            // convert iOriginalValue into minutes
            int iHours = int(iOriginalValue / 10);
            int iMinutes = (iOriginalValue % 10) * 10;
            int iTotalMinutes = (iHours * 60) + iMinutes;
            // num mins from 00:00 to 24:00 is 24 * 60 + 1 = 1441
            iTotalMinutes = (iTotalMinutes + (iEncoderDifference * 10)) % (MINUTES_IN_A_DAY+1);
            
            // convert total minutes back into a char (e.g. 235 = 23:50)
            iHours = int(iTotalMinutes / 60);
            iMinutes = int(iTotalMinutes % 60);
            iNewValue = (iHours * 10) + int(iMinutes / 10);           
            iNewValue = constrain(iNewValue, 0, 240); 
            
            if (isStartTime)
            {
                pHeatingInfo_p->startWindows[yDay][yWindow] = (char) iNewValue;
                
                if (pHeatingInfo_p->endWindows[yDay][yWindow] < (char) iNewValue)
                {
                    pHeatingInfo_p->endWindows[yDay][yWindow] = (char) iNewValue;
                }
            }
            else
            {
                pHeatingInfo_p->endWindows[yDay][yWindow] = (char) iNewValue;
            }
            
            notifyHeatingInfoChangedLocally();
            updateDisplay(pHeatingInfo_p, true);
            
        }
        
    }
    //debug("checkForAdvanceAndEncoderInputs: end");    
}



bool isUIBusy()
{
    return (UI_STATE_NORMAL != yUIState_m  &&  UI_STATE_SYNCHRONISING != yUIState_m  &&  UI_STATE_SYNCHRONISATION_COMPLETE != yUIState_m);
}


void checkForUIIdle(HeatingInfo *pHeatingInfo_p)
{
    //debug("Checking for idle");
    if (! isUIBusy())
    {
        if (millisSince(lLastUIStateChangeTime_m) > MAX_UI_STATE_IDLE_TIME_MILLIS)
        {
            setUIState(pHeatingInfo_p, UI_STATE_NORMAL, false);
        }
    }
}
