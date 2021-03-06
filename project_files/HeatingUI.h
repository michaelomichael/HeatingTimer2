#ifndef HeatingUI_h
#define HeatingUI_h


#include <LiquidCrystal.h>
#include <Encoder.h>

#include "HeatingCommon.h"


// Input/Output pins
const int ENCODER_WHEEL_PIN_1 = PA0; // TODO
const int ENCODER_WHEEL_PIN_2 = PA1; // TODO
const int ENCODER_BUTTON_PIN = PB14; //PA7; // TODO
//const int BACK_BUTTON_PIN = 99;
const int ADVANCE_BUTTON_PIN = PB1; //PB11; // Note: I tried PF0 and PF1 but they don't work for reading!!


const int LCD_VSS = PC6; /* Avoid PC8 and PC9 (builtin LEDs) and PA8-10 (useful for USART) */
const int LCD_VDD = PC7; /* Not used: you need +5V from the corner pin */
const int LCD_VO = PA11;
const int LCD_RS = PA12;
const int LCD_RW = PF6;  /* Avoid PA13 or you'll brick it! */
const int LCD_ENABLE = PF7;
const int LCD_DATA0 = PA14;
const int LCD_DATA1 = PA15;
const int LCD_DATA2 = PC10;
const int LCD_DATA3 = PC11;
const int LCD_DATA4 = PC12;
const int LCD_DATA5 = PD2;
const int LCD_DATA6 = PB3;
const int LCD_DATA7 = PB4;
const int LCD_A = PB5;  /* PWM */
const int LCD_K = PB6;



const int MAX_DEBOUNCE_MILLIS = 100;

const int UI_STATE_SOFT_REBOOTING_WIFLY = 95;
const int UI_STATE_HARD_REBOOTING_WIFLY = 96;
const int UI_STATE_SYNCHRONISING = 97;
const int UI_STATE_SYNCHRONISATION_COMPLETE = 98;
const int UI_STATE_SYNCHRONISATION_FAILED = 99;
const int UI_STATE_MENU_MIN = 0;
const int UI_STATE_NORMAL = 0;
const int UI_STATE_MENU_1_MAX_TEMPERATURE = 1;
const int UI_STATE_MENU_2_APPLIANCES = 2;
const int UI_STATE_MENU_3_WINDOW_TIMES_START = 3;
const int UI_STATE_MENU_3_WINDOW_TIMES_END = 23;
const int UI_STATE_MENU_4_CLOCK = 24;
const int UI_STATE_MENU_5_DISPLAY_LAST_SYNC_TIME = 25;
const int UI_STATE_MENU_6_TOGGLE_SYNC = 26;
const int UI_STATE_MENU_7_SYNC_DELAY = 27;
const int UI_STATE_MENU_8_LCD_BRIGHTNESS = 28;
const int UI_STATE_MENU_MAX = 28;
const int UI_STATE_EDIT_CLOCK_1_DAY_OF_WEEK = 40;
const int UI_STATE_EDIT_CLOCK_2_HOUR = 41;
const int UI_STATE_EDIT_CLOCK_3_MINUTES = 42;
const int UI_STATE_EDIT_MAX_TEMPERATURE = 43;
const int UI_STATE_EDIT_APPLIANCES = 44;
const int UI_STATE_EDIT_SYNC_DELAY = 45;
const int UI_STATE_EDIT_LCD_BRIGHTNESS = 46;
const int UI_STATE_EDIT_WINDOW_TIMES_START = 50;
const int UI_STATE_EDIT_WINDOW_TIMES_END = 92;
const int UI_STATE_MAX = 92;

const unsigned long MAX_UI_STATE_IDLE_TIME_MILLIS = 120000;





const char RIGHT_ARROW_SYMBOL = 126;
const char DEGREES_SYMBOL = 223;





void initUI();
void updateDisplay(HeatingInfo *pHeatingInfo_p, bool isPartial_p);
void setUIState(HeatingInfo *pHeatingInfo_p, byte yNewUIState_p, bool isPartialDisplayUpdate_p);
void waitForDebounce(int iPin_p);
void checkForAdvanceAndEncoderInputs(HeatingInfo *pHeatingInfo_p);
bool isUIBusy();
void checkForUIIdle(HeatingInfo *pHeatingInfo_p);
//const char* getShortDayName(byte yDayIndex_p);
//const char* getLongDayName(byte yDayIndex_p);


#endif