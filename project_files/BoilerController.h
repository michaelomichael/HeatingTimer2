#ifndef BoilerController_H
#define BoilerController_H

#include <inttypes.h>
#include "stm32f0xx.h"
#include "orz.h"
#include <string.h>
#include <HardwareSerial.h>
#include "HeatingCommon.h"


class BoilerController
{
public:
    void init(Print *pDebug_p);
    bool updateRelays(HeatingInfo *pInfo_p);
    bool isHeatingRelayClosed();
    bool isWaterRelayClosed();
    bool isEnergised();
#if UNIT_TEST
    bool runTests();
#endif
    
private:
    Print *pDebug_i; 
    bool isHeatingRelayClosed_i;
    bool isWaterRelayClosed_i;
    bool isEnergised_i;
    static bool isWithinWindow(HeatingInfo *pInfo_p, char cStart_p, char cEnd_p);
};

 
#endif
