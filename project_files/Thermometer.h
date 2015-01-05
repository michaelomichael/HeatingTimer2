#ifndef Thermometer_H
#define Thermometer_H

#include <inttypes.h>
#include "stm32f0xx.h"
#include "orz.h"
#include <string.h>
#include <HardwareSerial.h>
#include "HeatingCommon.h"

/*

Assumes that a 10k Thermistor has been connected to a pin (e.g. PC1) as part of
a Voltage Divider circuit as follows:

        +3V ----------------+
                            |
                          +-+-+
                          |   |
                          |   |  22k Resistor
                          |   |
                          +-+-+
                            |
                            +----------------> to pin (e.g. PC1)
                            |
                          +-+-+
                          |   |
                          |   |  10k Thermistor
                          |   |
                          +-+-+
                            |
        Gnd ----------------+-----------------                                                  

*/
class Thermometer
{
public:
    void init(int iPin_p);
    bool takeSample();
    int getAverageInDegrees();
    static const int MIN_RECORDABLE_TEMPERATURE = -30;
    static const int MAX_RECORDABLE_TEMPERATURE = 40;
#if UNIT_TEST
    bool runTests(HardwareSerial *pDebugSerial_p);
#endif
    
private:
    static const int MAX_SAMPLES_TO_AVERAGE = 5;
    int viSamples_i[MAX_SAMPLES_TO_AVERAGE];
    int iNumSamplesTaken_i;
    int iPin_i;
    int iCurrentAverageInDegrees_i;
    int convertVoltageToTemperature(int iADCReading_p);
};

 
#endif
