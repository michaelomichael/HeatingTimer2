#if FALSE

Not used at present































#include "HeatingWifly.h" 
#include "HeatingCommon.h" // For debug()

#include <string.h>
#include <stdio.h>


#define wiflySerial_m Serial1

#define debug(x) Serial.println(x)

HeatingWifly::HeatingWifly()
{
    
}




void HeatingWifly::begin(int iBaudRate_p, int iPowerPin_p, unsigned long lWarmupDelay_p, HardwareSerial* pDebugger_p)
{
    wiflySerial_m.begin(iBaudRate_p);
    pDebugger_i = pDebugger_p;   
    iPowerPin_i = iPowerPin_p;
    lWarmupDelay_i = lWarmupDelay_p;
    
    // Add power
    setPinMode(iPowerPin_i, PIN_MODE_DIGITAL_WRITE);    
    digitalWrite(iPowerPin_i, HIGH); 
    delay(lWarmupDelay_i);
    
    printDebug("Wifly ready");
    printDebugNewline();
}
 
 
 
 
char* HeatingWifly::getHttpBody()
{
    return vsHttpBodyBuffer_i;   
}


void HeatingWifly::sendCommand(const char* psCommand_p)
{    
    delay(250);
    readAllCharsOnWifly();
    wiflySerial_m.println(psCommand_p);
    readAllCharsOnWifly();
    delay(250);
}


void HeatingWifly::hardReset()
{
    digitalWrite(iPowerPin_i, LOW);
    delay(POWER_CYCLE_TIME_MILLIS);
    digitalWrite(iPowerPin_i, HIGH);
    delay(lWarmupDelay_i);
}

void HeatingWifly::softReset()
{
    
    delay(250);
    wiflySerial_m.print("$$$");
    delay(250);
    /*
    sendCommand("factory RESET");
    sendCommand("reboot");
    delay(4000);
    
    wiflySerial_m.print("$$$");
    delay(250);
    */
/*
    sendCommand("set wlan ssid Michael$and$Kelly"); //
    sendCommand("set wlan phrase chickenwire"); 
    sendCommand("set sys auto 0");
    sendCommand("set option format 1");
    sendCommand("set dns name avr.musicbysines.com");
    sendCommand("set ip host 0"); //               (forces it to use DNS)
    sendCommand("set ip remote 80");
    sendCommand("set ip proto 18");
    sendCommand("set com remote GET$/heating/readonly.php?DATA="); //     (when you change this, remember to keep the "?DATA=" bit!)        
    sendCommand("set uart mode 2");   //           (i.e. send HTTP request upon receiving UART data)
    sendCommand("set comm time 30");  //           (i.e. how long to wait after opening connection on first UART char)
    sendCommand("set time zone 0");
    sendCommand("set time enable 0"); //           (i.e. how long to wait between trips to the NPT time server)
    sendCommand("set comm size 120"); //           (i.e. flush size - allow more request bytes to be gathered over uart)
    sendCommand("set comm time 100"); //           (i.e. flush time - allow longer delay before deciding that we're not going to get any more request bytes over uart)
    sendCommand("save");
    */
    sendCommand("reboot");
    delay(4000);    
}

 
int HeatingWifly::sendHttpRequest(const char *psMessage_p, const char *psHttpBodySearchString_p, int iHttpBodyCharsToFetch_p, unsigned long lTimeoutMillis_p)
{
    
    debug("wifly.sendHttpRequest:start");
    
    psHttpBodySearchString_i = psHttpBodySearchString_p;
    iHttpBodySearchStringLength_i = strlen(psHttpBodySearchString_i);
    
    debug("wifly2");
    iHttpBodyCharsToFetch_i = iHttpBodyCharsToFetch_p;
    
    if (iHttpBodySearchStringLength_i >= MAX_CIRCULAR_SEARCH_BUFFER_SIZE - 1)
    {
        return ERROR_HTTP_BODY_SEARCH_STRING_TOO_LONG;
    }
    
    if (iHttpBodyCharsToFetch_i >= MAX_HTTP_BODY_BUFFER_SIZE - 1)
    {
        return ERROR_HTTP_BODY_NUM_CHARS_TO_FETCH_TOO_LONG;
    }
    debug("wifly3");
    // Reset all variables
    memset(vsCircularSearchBuffer_i, 0, MAX_CIRCULAR_SEARCH_BUFFER_SIZE);
    memset(vsHttpBodyBuffer_i, 0, MAX_HTTP_BODY_BUFFER_SIZE);
    iCircularSearchBufferPos_i = 0;
    iHttpBodyBufferPos_i = 0;    
    isHttpBodyFound_i = false;   
    
    debug("wifly.sendHttpRequest:about to readAllChars");
    // Make sure there's nothing on the serial buffer before we send our instruction
    readAllCharsOnWifly();
    debug("wifly.sendHttpRequest:done with readAllCHars");
    
    // Send the request data
    delay(270);
    wiflySerial_m.print(psMessage_p);
    delay(270);
    
    
    if (readEverythingUntilTimeout(lTimeoutMillis_p))
    {
        debug("wifly.sendHttpRequest:end (return success)");
        return SUCCESS;
    }
    else
    {
        debug("wifly.sendHttpRequest:end (return timedout");
        return ERROR_TIMED_OUT;
    }
    
    //return ERROR_TIMED_OUT;
}








// True == success, false = timeout
bool HeatingWifly::readEverythingUntilTimeout(unsigned long lTimeoutMillis_p)
{
    unsigned long lStartTimeMillis = millis();
    
    while (true)
    {
        int iNumCharsReadInThisSubLoopIteration = 0;
        
        //Serial.print("Available: ");
        //Serial.println(wiflySerial_m.available(), DEC);
        
        
        while (wiflySerial_m.available())
        {            
            char c = wiflySerial_m.read();
            bool isComplete = processWiflyChar(c);
            
            if (isComplete)
            {                
                return true;
            }
            
            iNumCharsReadInThisSubLoopIteration++;
            
            if (iNumCharsReadInThisSubLoopIteration > 50)
            {
                // Exit our 'if available' loop briefly so we can check the time.
                // We'll go back in again soon.
                break;
            }
        }
        
        if (millisSince(lStartTimeMillis) > lTimeoutMillis_p)
        {
            printDebug("TO,pos");
            printDebug(iHttpBodyBufferPos_i);
            printDebugNewline();
            return false;
        }
    }       
}



bool HeatingWifly::processWiflyChar(char c)
{
    // Assumes that iBufferWritePos_i is never >= MAX_BUFFER_SIZE
	vsCircularSearchBuffer_i[iCircularSearchBufferPos_i++] = c;

    //printDebug(c);

	if (iCircularSearchBufferPos_i >= MAX_CIRCULAR_SEARCH_BUFFER_SIZE)
	{
		/*
		// This is useful to see where the breaks in the buffer are occurring
		char vsPrintBuffer[MAX_CIRCULAR_SEARCH_BUFFER_SIZE+1];
		strncpy(vsPrintBuffer, vsCircularSearchBuffer_i, MAX_CIRCULAR_SEARCH_BUFFER_SIZE);
		vsPrintBuffer[MAX_CIRCULAR_SEARCH_BUFFER_SIZE] = 0;
		printDebug(("Overflow.  ");
		printDebug(isHttpBodyFound_i ? "Y" : "N");
		printDebug(iCircularSearchBufferPos_i);
		printDebug(".  Buffer now contains [");
		printDebug(vsPrintBuffer);
		printDebug("]");
		printDebugNewline();
		*/
		
		iCircularSearchBufferPos_i = 0;
	}

	if (! isHttpBodyFound_i)
	{
		// Check back and see if the last N chars matched our search string 
		// So we want to start with the buffer char at pos iBufferWritePos_i-1 and compare that to psSearchString_i[iSearchStringLength-1]
		int iCharsMatched = 0;

		const char* psSearchStringCursor = psHttpBodySearchString_i + iHttpBodySearchStringLength_i - 1;  // Start at the end of the search string
		int iBufferReadPos = normaliseCircularBufferPos(iCircularSearchBufferPos_i-1);  // Loops it round if necessary

		while (*psSearchStringCursor == vsCircularSearchBuffer_i[iBufferReadPos])
		{
			iCharsMatched++;
			iBufferReadPos = normaliseCircularBufferPos(iBufferReadPos-1);
			psSearchStringCursor--;

			if (iCharsMatched == iHttpBodySearchStringLength_i)
			{
				printDebug("H");
				printDebugNewline();
				isHttpBodyFound_i = true;				
			}
		}
	}
	else
	{
	    vsHttpBodyBuffer_i[iHttpBodyBufferPos_i++] = c;
	    
	    if (iHttpBodyBufferPos_i >= iHttpBodyCharsToFetch_i -1) 
		{
		 	vsHttpBodyBuffer_i[iHttpBodyCharsToFetch_i] = 0;  
			printDebug(vsHttpBodyBuffer_i);
			printDebugNewline();
			
            //processMessageBody();
			isHttpBodyFound_i = false;  // Reset this so we can look for more if we want
			return true;
		}
	}
	
	return false;
}








//=============================================================================
//
//  HELPER FUNCTIONS
//
//=============================================================================



unsigned long HeatingWifly::millisSince(unsigned long lLastMillis_p)
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







int HeatingWifly::normaliseCircularBufferPos(int iBufferPos_p)
{
	while (iBufferPos_p < 0)
	{
		iBufferPos_p += MAX_CIRCULAR_SEARCH_BUFFER_SIZE;
	}

	while (iBufferPos_p >= MAX_CIRCULAR_SEARCH_BUFFER_SIZE)
	{
		iBufferPos_p -= MAX_CIRCULAR_SEARCH_BUFFER_SIZE;
	}

	return iBufferPos_p;
}




void HeatingWifly::readAllCharsOnWifly()
{
    while (wiflySerial_m.available())
    {
        wiflySerial_m.read();
    }
}



void HeatingWifly::printDebug(int iValue_p)
{
    if (NULL != pDebugger_i)
    {            
        pDebugger_i->print(iValue_p, DEC);
    } 
}



void HeatingWifly::printDebug(char* psMessage_p)
{
    if (NULL != pDebugger_i)
    {    
        pDebugger_i->print(psMessage_p);        
    } 
}


void HeatingWifly::printDebugNewline()
{
    printDebug("\r\n");
}


#endif