#include "BoilerController.h"




void BoilerController::init(Print *pDebug_p)
{    
    pDebug_i = pDebug_p;
    isHeatingRelayClosed_i = false;
    isWaterRelayClosed_i = false;
}




/*
Updates the relays based on the time/window/advance statuses
in the given heating info.  Returns 'true' if the advance status
was changed as a result (e.g. we were advanced and have just come
to the start/end of a new window).
*/
bool BoilerController::updateRelays(HeatingInfo *pInfo_p)
{
    // Flash LEDs to show we mean business
    pDebug_i->println("Updating relays");

    
    // Look at the current time, figure out if one or both
    // relays should be on/off, and set the output pins appropriately.   
    byte yDay = pInfo_p->currentDay;
    bool isNowInWindow = false;
    
    for (byte yWindow=0;yWindow < WINDOWS_PER_DAY;yWindow++)
    {
        if (isWithinWindow(pInfo_p, pInfo_p->startWindows[yDay][yWindow], pInfo_p->endWindows[yDay][yWindow]))
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
    
    pDebug_i->print("Was in window: ");
    pDebug_i->print(pInfo_p->wasInWindow);
    pDebug_i->print(", now in window: ");
    pDebug_i->println(isNowInWindow);
    
    
    if (pInfo_p->wasInWindow != isNowInWindow)
    {
        if (ADVANCE_AS_SCHEDULED != pInfo_p->advanceStatus)
        {
            debug("We WERE in a window but now we're not so switching the 'advance' status");
            isAdvanceStatusChanged = true;
        }
        
        debug("Setting 'advance' status back to normal");
        pInfo_p->advanceStatus = ADVANCE_AS_SCHEDULED;
    }                                      
    pInfo_p->wasInWindow = isNowInWindow;
    
    //
    //  Cancel the '+1 hour' setting if it's been active for longer than an hour
    //
    if (ADVANCE_PLUS_ONE_HOUR == pInfo_p->advanceStatus)
    {
        unsigned long lMillisSinceAdvanceStatusChange = millisSince(pInfo_p->lastAdvanceStatusChangeMillis);

        pDebug_i->print("We're advanced +1 hour; that was ");
        pDebug_i->print(lMillisSinceAdvanceStatusChange, DEC);
        pDebug_i->print("ms ago (1 hour is ");
        pDebug_i->print(MILLIS_IN_AN_HOUR, DEC);
        pDebug_i->println(")");
        
        if (lMillisSinceAdvanceStatusChange > MILLIS_IN_AN_HOUR)
        {
            debug("Cancelling the +1 hour");
            pDebug_i->print("Last advance status change was ");
            pDebug_i->print(pInfo_p->lastAdvanceStatusChangeMillis);
            pDebug_i->print(" and millis now is ");
            pDebug_i->println(millis());
         
            isAdvanceStatusChanged = true;
            pInfo_p->advanceStatus = ADVANCE_AS_SCHEDULED;
        }
    }
    
    bool isWindowOverridden = (ADVANCE_AS_SCHEDULED != pInfo_p->advanceStatus);
    bool bActivateRelays = (isNowInWindow != isWindowOverridden);// XOR       
    
    pDebug_i->print("Is window overridden: ");
    pDebug_i->print(isWindowOverridden);
    pDebug_i->print(", activate relays: ");
    pDebug_i->println(bActivateRelays);
    
    //
    //  Now that we know whether the relay(s) should be on, let's
    //  make sure they're both in the correct state.
    //
    if (APPLIANCES_HOT_WATER_ONLY == pInfo_p->appliances)
    {
        isHeatingRelayClosed_i = false;
        isWaterRelayClosed_i = bActivateRelays;
        isEnergised_i = bActivateRelays;
    }
    else if (APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING == pInfo_p->appliances)
    {
        // FORLATER: bool isRoomTooCold = (pInfo_p->ambientTemp < pInfo_p->maxTemp);
        bool isRoomTooCold = true;
        isHeatingRelayClosed_i = (bActivateRelays && isRoomTooCold);
        isWaterRelayClosed_i = bActivateRelays;
        isEnergised_i = bActivateRelays;
    }
    else
    {
        isHeatingRelayClosed_i = false;
        isWaterRelayClosed_i = false;
        isEnergised_i = false;
    }
    
    pDebug_i->print("IsEnergised=");
    pDebug_i->println(isEnergised_i ? "Y" : "N");
        
    return isAdvanceStatusChanged;
}




bool BoilerController::isHeatingRelayClosed()
{
    return isHeatingRelayClosed_i;
}




bool BoilerController::isWaterRelayClosed()
{
    return isWaterRelayClosed_i;
}




bool BoilerController::isEnergised()
{
    return isEnergised_i;
}




bool BoilerController::isWithinWindow(HeatingInfo *pInfo_p, char cStart_p, char cEnd_p)
{
    char cCurrentTime = (pInfo_p->currentTimeHours * 10) + (pInfo_p->currentTimeMins / 10);
    
    return cCurrentTime >= cStart_p  &&  cCurrentTime < cEnd_p;
}




#if UNIT_TEST

void resetHeatingInfo(HeatingInfo *pInfo_p)
{
    pInfo_p->advanceStatus = ADVANCE_AS_SCHEDULED;
    pInfo_p->currentDay = 0;
    pInfo_p->currentTimeHours = 0;
    pInfo_p->currentTimeMins = 0;
    pInfo_p->maxTemp = 20;
    pInfo_p->wasInWindow = false;
    pInfo_p->appliances = APPLIANCES_HOT_WATER_AND_CENTRAL_HEATING;
    
    for (byte yDay=0;yDay < DAYS_IN_A_WEEK;yDay++)
    {
        for (byte yWindow=0;yWindow < WINDOWS_PER_DAY;yWindow++)
        {
            pInfo_p->startWindows[yDay][yWindow] = 0;
            pInfo_p->endWindows[yDay][yWindow] = 0;
        }
    }     
}




bool assertEquals(Print *pDebug_p, char *psTestName_p, char *psFieldName_p, bool bExpectedValue_p, bool bActualValue_p)
{
    if (bExpectedValue_p != bActualValue_p)
    {        
        pDebug_p->print("FAILED: ");
        pDebug_p->print(psTestName_p);
        pDebug_p->print(" - expected '");
        pDebug_p->print(psFieldName_p);
        pDebug_p->print("' to be ");
        pDebug_p->print(bExpectedValue_p);
        pDebug_p->print(" but was ");
        pDebug_p->println(bActualValue_p);
        return false;
    }

    return true;
}




bool checkUpdateRelayResult(Print *pDebug_p, 
                            char *psTestName_p, 
                            HeatingInfo *pTestInfo_p, 
                            bool isExpectedChanged_p, 
                            bool isExpectedHeatingOn_p, 
                            bool isExpectedWaterOn_p,
                            bool iExpectedAdvanceStatus_p)
{
    
    BoilerController testable;
    testable.init(pDebug_p);
    
    bool isAdvanceChanged = testable.updateRelays(pTestInfo_p);
    
    bool isSuccess = true;
    isSuccess = isSuccess  &&  assertEquals(pDebug_p, psTestName_p, "advance changed", isExpectedChanged_p, isAdvanceChanged);
    isSuccess = isSuccess  &&  assertEquals(pDebug_p, psTestName_p, "heating on", isExpectedHeatingOn_p, testable.isHeatingRelayClosed());
    isSuccess = isSuccess  &&  assertEquals(pDebug_p, psTestName_p, "water on", isExpectedWaterOn_p, testable.isWaterRelayClosed());
    
    if (isSuccess)
    {
        pDebug_p->print("PASSED: ");
        pDebug_p->println(psTestName_p);
    }
    
    return isSuccess;
}




bool testOutsideOfWindow(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);

    info.startWindows[0][0] = 30;// 3:00 am
    info.endWindows[0][0] = 41;// 4:10 am
    info.currentTimeHours = 2;
    info.currentTimeMins = 5;
    
    return checkUpdateRelayResult(pDebug_p, "TestOutsideOfWindow", &info, false, false, false, ADVANCE_AS_SCHEDULED);
}




bool testMidnight(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);
    
    bool isSuccess = true;
    
    info.currentTimeHours = 23;
    info.currentTimeMins = 59;
    isSuccess = isSuccess  &&  checkUpdateRelayResult(pDebug_p, "testMidnight (1)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    info.currentTimeHours = 0;
    info.currentTimeMins = 0;
    isSuccess = isSuccess  &&  checkUpdateRelayResult(pDebug_p, "testMidnight (2)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    info.currentTimeHours = 0;
    info.currentTimeMins = 1;
    isSuccess = isSuccess  &&  checkUpdateRelayResult(pDebug_p, "testMidnight (3)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    return isSuccess;
}




bool testInAndOutOfWindow(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);
    
    info.startWindows[0][0] = 30;// 3:00 am
    info.endWindows[0][0] = 41;// 4:10 am
    
    bool isSuccess = true;
    
    info.currentTimeHours = 2;
    info.currentTimeMins = 59;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (1)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    info.currentTimeHours = 3;
    info.currentTimeMins = 0;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (2)", &info, false, true, true, ADVANCE_AS_SCHEDULED);
    
    info.currentTimeHours = 3;
    info.currentTimeMins = 0;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (3)", &info, false, true, true, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 3;
    info.currentTimeMins = 1;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (4)", &info, false, true, true, ADVANCE_AS_SCHEDULED);
    
    info.currentTimeHours = 4;
    info.currentTimeMins = 9;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (5)", &info, false, true, true, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (6)", &info, false, false, false, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (7)", &info, false, false, false, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 11;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (8)", &info, false, false, false, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 5;
    info.currentTimeMins = 0;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (9)", &info, false, false, false, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 23;
    info.currentTimeMins = 59;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (10)", &info, false, false, false, ADVANCE_AS_SCHEDULED);

    info.currentDay = 1;
    info.currentTimeHours = 3;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (11)", &info, false, false, false, ADVANCE_AS_SCHEDULED);

    info.currentDay = 6;
    info.currentTimeHours = 3;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testInAndOutOfWindow (12)", &info, false, false, false, ADVANCE_AS_SCHEDULED);

    return isSuccess;
}




bool testFullAdvanceBeforeWindow(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);
    
    info.startWindows[0][0] = 30;// 3:00 am
    info.endWindows[0][0] = 41;// 4:10 am
    
    bool isSuccess = true;
    
    info.currentTimeHours = 2;
    info.currentTimeMins = 0;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceBeforeWindow (1)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    info.advanceStatus = ADVANCED_TO_NEXT_EVENT;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceBeforeWindow (2)", &info, false, true, true, ADVANCED_TO_NEXT_EVENT);
    
    info.currentTimeHours = 2;
    info.currentTimeMins = 59;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceBeforeWindow (3)", &info, false, true, true, ADVANCED_TO_NEXT_EVENT);
    
    info.currentTimeHours = 3;
    info.currentTimeMins = 0;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceBeforeWindow (4)", &info, true, true, true, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 9;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceBeforeWindow (5)", &info, false, true, true, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceBeforeWindow (6)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    return isSuccess;
}   




bool testFullAdvanceDuringWindow(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);
    
    info.startWindows[0][0] = 30;// 3:00 am
    info.endWindows[0][0] = 41;// 4:10 am
    
    bool isSuccess = true;
    
    info.currentTimeHours = 3;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceDuringWindow (1)", &info, false, true, true, ADVANCE_AS_SCHEDULED);
    
    info.advanceStatus = ADVANCED_TO_NEXT_EVENT;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceDuringWindow (2)", &info, false, false, false, ADVANCED_TO_NEXT_EVENT);
    
    info.currentTimeHours = 4;
    info.currentTimeMins = 9;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceDuringWindow (3)", &info, false, false, false, ADVANCED_TO_NEXT_EVENT);
    
    info.currentTimeHours = 4;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceDuringWindow (4)", &info, true, false, false, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 11;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testFullAdvanceDuringWindow (4)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    return isSuccess;
}

    


bool testHourlyAdvanceWithoutWindow(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);
    
    info.startWindows[0][0] = 30;// 3:00 am
    info.endWindows[0][0] = 41;// 4:10 am
    
    bool isSuccess = true;
    
    info.currentTimeHours = 1;
    info.currentTimeMins = 0;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceWithoutWindow (1)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    info.advanceStatus = ADVANCE_PLUS_ONE_HOUR;
    info.lastAdvanceStatusChangeMillis = millis() - 100;// i.e. just now
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceWithoutWindow (2)", &info, false, true, true, ADVANCE_PLUS_ONE_HOUR);
    
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceWithoutWindow (3)", &info, false, true, true, ADVANCE_PLUS_ONE_HOUR);
    
    info.lastAdvanceStatusChangeMillis = millis() - (MILLIS_IN_AN_HOUR - 100);// i.e. hour almost up
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceWithoutWindow (4)", &info, false, true, true, ADVANCE_PLUS_ONE_HOUR);
    
    info.lastAdvanceStatusChangeMillis = millis() - (MILLIS_IN_AN_HOUR + 1);// i.e. just up
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceWithoutWindow (5)", &info, true, false, false, ADVANCE_AS_SCHEDULED);
        
    info.lastAdvanceStatusChangeMillis = millis() - (MILLIS_IN_AN_HOUR * 5);// i.e. up ages ago
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceWithoutWindow (6)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    return isSuccess;
}




bool testHourlyAdvanceJustBeforeWindowStart(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);
    
    info.startWindows[0][0] = 30;// 3:00 am
    info.endWindows[0][0] = 41;// 4:10 am
    
    bool isSuccess = true;
    
    info.currentTimeHours = 2;
    info.currentTimeMins = 59;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustBeforeWindowStart (1)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
    
    info.advanceStatus = ADVANCE_PLUS_ONE_HOUR;
    info.lastAdvanceStatusChangeMillis = millis() - 100;// i.e. just now
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustBeforeWindowStart (2)", &info, false, true, true, ADVANCE_PLUS_ONE_HOUR);
    
    info.currentTimeHours = 3;
    info.currentTimeMins = 0;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustBeforeWindowStart (3)", &info, true, true, true, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 9;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustBeforeWindowStart (4)", &info, false, true, true, ADVANCE_AS_SCHEDULED);

    info.currentTimeHours = 4;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustBeforeWindowStart (5)", &info, false, false, false, ADVANCE_AS_SCHEDULED);
}



bool testHourlyAdvanceJustAfterWindowStart(Print *pDebug_p)
{    
    HeatingInfo info;
    resetHeatingInfo(&info);
    
    info.startWindows[0][0] = 30;// 3:00 am
    info.endWindows[0][0] = 41;// 4:10 am
    
    bool isSuccess = true;
    
    info.currentTimeHours = 3;
    info.currentTimeMins = 1;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (1)", &info, false, true, true, ADVANCE_AS_SCHEDULED);
    
    info.advanceStatus = ADVANCE_PLUS_ONE_HOUR;
    info.lastAdvanceStatusChangeMillis = millis() - 100;// i.e. just now
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (2)", &info, false, false, false, ADVANCE_PLUS_ONE_HOUR);
    
    info.lastAdvanceStatusChangeMillis = millis() - (MILLIS_IN_AN_HOUR - 100);// i.e. almost up
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (3)", &info, false, false, false, ADVANCE_PLUS_ONE_HOUR);
    
    info.lastAdvanceStatusChangeMillis = millis() - (MILLIS_IN_AN_HOUR + 1);// i.e. just up
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (4)", &info, true, true, true, ADVANCE_AS_SCHEDULED);
    
    //
    //  Try again, letting this one go to the end of the window
    //
    info.advanceStatus = ADVANCE_PLUS_ONE_HOUR;
    info.lastAdvanceStatusChangeMillis = millis() - 100;// i.e. just now
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (5)", &info, false, false, false, ADVANCE_PLUS_ONE_HOUR);
        
    info.currentTimeHours = 4;
    info.currentTimeMins = 9;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (6)", &info, false, false, false, ADVANCE_PLUS_ONE_HOUR);

    info.currentTimeHours = 4;
    info.currentTimeMins = 10;
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (7)", &info, true, false, false, ADVANCE_AS_SCHEDULED);
    
    info.lastAdvanceStatusChangeMillis = millis() - (MILLIS_IN_AN_HOUR + 1);// i.e. just up
    isSuccess = isSuccess &&  checkUpdateRelayResult(pDebug_p, "testHourlyAdvanceJustAfterWindowStart (8)", &info, false, false, false, ADVANCE_AS_SCHEDULED);    
}




bool BoilerController::runTests()
{
    bool isSuccess = true;
    isSuccess = isSuccess  &&  testOutsideOfWindow(pDebug_i);
    isSuccess = isSuccess  &&  testMidnight(pDebug_i);
    isSuccess = isSuccess  &&  testInAndOutOfWindow(pDebug_i);
    isSuccess = isSuccess  &&  testFullAdvanceBeforeWindow(pDebug_i);
    isSuccess = isSuccess  &&  testFullAdvanceDuringWindow(pDebug_i);
    isSuccess = isSuccess  &&  testHourlyAdvanceWithoutWindow(pDebug_i);
    isSuccess = isSuccess  &&  testHourlyAdvanceJustBeforeWindowStart(pDebug_i);
    isSuccess = isSuccess  &&  testHourlyAdvanceJustAfterWindowStart(pDebug_i);
    
    pDebug_i->println("Finished BoilerController tests");
    return isSuccess;
}
#endif
