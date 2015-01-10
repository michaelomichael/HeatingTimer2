
#include "HeatingESP8266.h"
#include "WifiDetails.h"

HeatingESP8266::HeatingESP8266()
{
}


void HeatingESP8266::begin(HardwareSerial* pESP8266Serial_p, HardwareSerial* pDebugger_p)
{
    pDebugger_i = pDebugger_p;
    esp8266_i.init(pESP8266Serial_p, pDebugger_p);
}



int HeatingESP8266::sendHttpRequest(const char *psMessage_p, unsigned long lTimeoutMillis_p)
{
    strcpy(vsHttpSendBuffer_i, WIFI_TARGET_PATH_PART_1);
    strcat(vsHttpSendBuffer_i, psMessage_p);
    strcat(vsHttpSendBuffer_i, WIFI_TARGET_PATH_PART_2);
    
    if (! esp8266_i.connectToAP(WIFI_SSID, WIFI_PASSPHRASE, lTimeoutMillis_p))
    {
        return ERROR_TIMED_OUT;
    }
    
    if (! esp8266_i.sendToServer(WIFI_TARGET_IP, WIFI_TARGET_PORT, vsHttpSendBuffer_i, WIFI_EXPECTED_START_STRING, lTimeoutMillis_p))
    {
        esp8266_i.closeServerConnection();
        return ERROR_TIMED_OUT;
    }
    
    if (! esp8266_i.readServerResponse(vsHttpBodyBuffer_i, WIFI_NUM_EXPECTED_CHARS, lTimeoutMillis_p))
    {
        esp8266_i.closeServerConnection();
        return ERROR_TIMED_OUT;
    }   

    esp8266_i.closeServerConnection();       
    
    esp8266_i.disconnectFromAP();
    
    return SUCCESS;
}


char* HeatingESP8266::getHttpBody()
{
    return vsHttpBodyBuffer_i;
}

void HeatingESP8266::softReset()
{
    
}

void HeatingESP8266::hardReset()
{
}

    




void HeatingESP8266::printDebug(int iValue_p)
{
    if (NULL != pDebugger_i)
    {            
        pDebugger_i->print(iValue_p, DEC);
    } 
}



void HeatingESP8266::printDebug(char* psMessage_p)
{
    if (NULL != pDebugger_i)
    {    
        pDebugger_i->print(psMessage_p);        
    } 
}


void HeatingESP8266::printDebugNewline()
{
    printDebug("\r\n");
}






