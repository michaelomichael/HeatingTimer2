#include "Thermometer.h"

static const int TEMPERATURE_LOOKUP_TABLE_SIZE = 71;

//
//  This lookup table lets us convert the ADC reading that we get back
//  from analogRead() into a temperature in degrees celsius.
//  The first value in the table is -30C, with each successive value
//  increasing by 1C, up to 40C.
//
//  See the "ThermistorCalculations.xls" spreadsheet for details on how 
//  the conversion was calculated.  In particular, see the section highlighted 
//  in green, where I manually checked the ADC's reading for various measured
//  voltages (the voltages having been calculated using the formula on the 
//  NTCLE100E3103 10k thermistor's datasheet - see the section in yellow) and 
//  where I also just got lazy and extrapolated these out roughly (the section
//  in red).
//
//  We're expecting that it's a 10k thermistor that's been connected as R2
//  in a potential divider circuit, with R1 being 22k.
//
//  Note that this is for an STM32F0 chip's ADC, when running on
//  3V.  For a 5V arduino, I've used the following code in the past, 
//  although not 100% sure what resistor was used for R1 in this 
//  case, probably a 10k.
//
//    double dTemperatureSample = (iTemperatureSample / 4094.0) * 1024.0;
//    double dTemp;
//    dTemp = logf(((10240000/dTemperatureSample) - 10000));
//    dTemp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * dTemp * dTemp ))* dTemp );
//    dTemp = dTemp - 273.15;            // Convert Kelvin to Celcius
//    int iTemperatureCentigrade = dTemp;
//
static const int TEMPERATURE_LOOKUP_TABLE[] = {    3289,  // -30C
    3251,3213,3175,3137,3099,3061,3023,2985,2947,2909, // -20C
    2871,2833,2795,2757,2719,2681,2643,2605,2567,2529, // -10C
    2491,2453,2415,2377,2339,2301,2263,2230,2180,2147, // 0C
    2109,2064,2039,1999,1961,1919,1875,1834,1793,1753, // 10C
    1713,1671,1630,1591,1555,1508,1475,1438,1403,1374, // 20C
    1335,1306,1258,1220,1181,1160,1121,1082,1043,1004, // 30C
     965, 926, 887, 848, 809, 770, 731, 692, 653, 614  // 40C
};




void Thermometer::init(int iPin_p)
{
    iPin_i = iPin_p;
    setPinMode(iPin_i, PIN_MODE_ANALOG_READ);
    iNumSamplesTaken_i = 0;
    iCurrentAverageInDegrees_i = 0;
}




//
//  Takes a sample, stores it, then calculates the average.
//  Returns true if the new average is different from the 
//  old one.
//
bool Thermometer::takeSample()
{       
    
    int iSampleVoltage = analogRead(iPin_i);
    int iSampleDegrees = convertVoltageToTemperature(iSampleVoltage);
    
    if (iNumSamplesTaken_i < MAX_SAMPLES_TO_AVERAGE)
    {
        viSamples_i[iNumSamplesTaken_i] = iSampleDegrees;
        iNumSamplesTaken_i++;
    }
    else
    {
        for (int i=0; i < MAX_SAMPLES_TO_AVERAGE-1; i++)
        {   
            viSamples_i[i] = viSamples_i[i+1];
        }
        
        viSamples_i[MAX_SAMPLES_TO_AVERAGE-1] = iSampleDegrees;
    }
    
    int iTotalDegrees = 0;
    
    for (int i=0; i < iNumSamplesTaken_i; i++)
    {   
        iTotalDegrees += viSamples_i[i];
    }
    
    int iNewAverage = iTotalDegrees / iNumSamplesTaken_i;
    
    bool isAverageChanged = (iNewAverage != iCurrentAverageInDegrees_i);
    
    iCurrentAverageInDegrees_i = iNewAverage;
    
    return isAverageChanged;
}




int Thermometer::getAverageInDegrees()
{
    return iCurrentAverageInDegrees_i;
}





//=============================================================================
//
//   PRIVATE SECTION
//
//=============================================================================

int Thermometer::convertVoltageToTemperature(int iADCReading_p)
{
    for (int iLastIndex=0; iLastIndex < TEMPERATURE_LOOKUP_TABLE_SIZE-1; iLastIndex++)
    {
        if (iADCReading_p > TEMPERATURE_LOOKUP_TABLE[iLastIndex+1])
        {
            //
            //  If iLastIndex=0 then we want to return -30.
            //  If it's 30 then we want to return 0.
            //
            return iLastIndex + MIN_RECORDABLE_TEMPERATURE + REAL_WORLD_DIFFERENTIAL_DEGREES;
        }
    }
        
    return MAX_RECORDABLE_TEMPERATURE;
    
}




#if UNIT_TEST

static bool assert(HardwareSerial *pDebugSerial_p, int iTestIndex_p, int iExpected_p, int iActual_p)
{
    if (iExpected_p != iActual_p)
    {
        pDebugSerial_p->print("Test ");
        pDebugSerial_p->print(iTestIndex_p, DEC);        
        pDebugSerial_p->print(" FAILED! Expected ");
        pDebugSerial_p->print(iExpected_p, DEC);
        pDebugSerial_p->print(" but got ");
        pDebugSerial_p->print(iActual_p, DEC);
        pDebugSerial_p->println(".");        
        return false;
    }
    else
    {
        pDebugSerial_p->print("Test ");
        pDebugSerial_p->print(iTestIndex_p, DEC);        
        pDebugSerial_p->println(" passed.");
        return true;
    }
}
        
        
        

bool Thermometer::runTests(HardwareSerial *pDebugSerial_p)
{
    bool bResult = true;
    bResult = assert(pDebugSerial_p, 1, -30, convertVoltageToTemperature(3290))  &&  bResult;
    bResult = assert(pDebugSerial_p, 2, -30, convertVoltageToTemperature(3289))  &&  bResult;
    bResult = assert(pDebugSerial_p, 3, -30, convertVoltageToTemperature(3252))  &&  bResult;
    bResult = assert(pDebugSerial_p, 4, -29, convertVoltageToTemperature(3251))  &&  bResult;
    bResult = assert(pDebugSerial_p, 5, 0, convertVoltageToTemperature(2147))  &&  bResult;
    bResult = assert(pDebugSerial_p, 6, 0, convertVoltageToTemperature(2110))  &&  bResult;
    bResult = assert(pDebugSerial_p, 7, 1, convertVoltageToTemperature(2109))  &&  bResult;
    bResult = assert(pDebugSerial_p, 8, 24, convertVoltageToTemperature(1182))  &&  bResult;
    bResult = assert(pDebugSerial_p, 9, 25, convertVoltageToTemperature(1181))  &&  bResult;
    bResult = assert(pDebugSerial_p, 10, 25, convertVoltageToTemperature(1180))  &&  bResult;
    bResult = assert(pDebugSerial_p, 11, 25, convertVoltageToTemperature(1161))  &&  bResult;
    bResult = assert(pDebugSerial_p, 12, 26, convertVoltageToTemperature(1160))  &&  bResult;
    bResult = assert(pDebugSerial_p, 13, 40, convertVoltageToTemperature(614))  &&  bResult;
    bResult = assert(pDebugSerial_p, 14, 40, convertVoltageToTemperature(613))  &&  bResult;
    bResult = assert(pDebugSerial_p, 15, 40, convertVoltageToTemperature(0))  &&  bResult;
    
    return bResult;
}

#endif
