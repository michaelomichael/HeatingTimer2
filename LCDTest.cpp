#if FALSE

#include "LCDTest.h"
#include "LiquidCrystal.h"



#define LCD_VSS PC6 /* Avoid PC8 and PC9 (builtin LEDs) and PA8-10 (useful for USART) */
#define LCD_VDD PC7 /* Not used: you need +5V from the corner pin */
#define LCD_VO PA11
#define LCD_RS PA12
#define LCD_RW PF6  /* Avoid PA13 or you'll brick it! */
#define LCD_ENABLE PF7
#define LCD_DATA0 PA14
#define LCD_DATA1 PA15
#define LCD_DATA2 PC10
#define LCD_DATA3 PC11
#define LCD_DATA4 PC12
#define LCD_DATA5 PD2
#define LCD_DATA6 PB3
#define LCD_DATA7 PB4
#define LCD_A PB5
#define LCD_K PB6


LiquidCrystal lcdTest_m(LCD_RS, LCD_ENABLE, LCD_DATA4, LCD_DATA5, LCD_DATA6, LCD_DATA7); //7, 6, 5, 4, 3, 2);

void initLCD()
{

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
    
    analogWrite(LCD_VO, 50);  // Any higher than this and it'll be hidden
    
    lcdTest_m.begin(16,2);
    lcdTest_m.clear();
    lcdTest_m.print("Hello");
    
    
}    
#endif