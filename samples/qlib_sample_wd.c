/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_wd.c
* @brief      This file contains QLIB WD related functions implementation
*
* @example    qlib_sample_wd.c
*
* @page       watchdog watchdog sample code
* This sample code shows watchdog protection
*
* @include    samples/qlib_sample_wd.c
*
************************************************************************************************************/

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                                  INCLUDES
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
#include "qlib.h"
#include "qlib_sample_wd.h"
#include "qlib_sample_qconf.h"

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                             INTERFACE FUNCTIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_SAMPLE_WatchDogConfig(QLIB_CONTEXT_T* qlibContext,
                                         BOOL            enable,
                                         BOOL            secure,
                                         QLIB_AWDT_TH_T  threshold,
                                         U32             section)
{
    QLIB_WATCHDOG_CONF_T watchdogCFG;
    QLIB_STATUS_T        status = QLIB_STATUS__OK;

    watchdogCFG.enable        = enable;
    watchdogCFG.authenticated = secure;
    watchdogCFG.sectionID     = section;
    watchdogCFG.threshold     = threshold;

    /*-------------------------------------------------------------------------------------------------------
     The rest of WD features are not among the parameters of this sample and are board related.
     This is why we use constant values.
    -------------------------------------------------------------------------------------------------------*/
    watchdogCFG.lfOscEn   = TRUE;
    watchdogCFG.swResetEn = FALSE;
    watchdogCFG.lock      = FALSE;
    watchdogCFG.oscRateHz = 0;

    /*-------------------------------------------------------------------------------------------------------
     Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     In case of secure watchdog we need to open session to the section WD which it is associated with.
    -------------------------------------------------------------------------------------------------------*/
    if (secure)
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL), status, disconnect);
    }

    /*-------------------------------------------------------------------------------------------------------
     Configure the watchdog with the parameters defined above.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_ConfigSet(qlibContext, &watchdogCFG), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Now WD is configured and the counter on flash starts to count to 2^^threshold, incrementing every
     second ( not accurate ). Once it gets there reset pin is set, so you need to touch (pet) the
     watchdog to zero the timer.
    -------------------------------------------------------------------------------------------------------*/

close_session:
    if (TRUE == secure)
    {
        (void)QLIB_CloseSession(qlibContext, section);
    }

disconnect:
    (void)QLIB_Disconnect(qlibContext);

    return status;
}

QLIB_STATUS_T QLIB_SAMPLE_WatchDogTouch(QLIB_CONTEXT_T* qlibContext, BOOL secure, U32 section)
{
    QLIB_STATUS_T status = QLIB_STATUS__OK;

    /*-------------------------------------------------------------------------------------------------------
     Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    if (secure)
    {
        /*---------------------------------------------------------------------------------------------------
         In case of secure watchdog we need to open session to the section WD which it is associated with.
        ---------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL), status, disconnect);
    }

    /*-------------------------------------------------------------------------------------------------------
     Touch the watchdog to zero the flash counter.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_Touch(qlibContext), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Now the counter on flash is zeroed. This prevents from the counter to reach the threshold and
     reset the chip.
    -------------------------------------------------------------------------------------------------------*/
close_session:
    if (TRUE == secure)
    {
        (void)QLIB_CloseSession(qlibContext, section);
    }

disconnect:
    (void)QLIB_Disconnect(qlibContext);

    return status;
}

QLIB_STATUS_T QLIB_SAMPLE_WatchDogGet(QLIB_CONTEXT_T* qlibContext, BOOL secure, U32 section, U32* time, BOOL* expired)
{
    QLIB_STATUS_T status = QLIB_STATUS__OK;

    /*-------------------------------------------------------------------------------------------------------
     take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    if (secure)
    {
        /*---------------------------------------------------------------------------------------------------
         in case of secure watchdog you we need to open session to the section WD is associated with
        ---------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL), status, disconnect);
    }

    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_Get(qlibContext, time, NULL, expired), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     if *expired is TRUE then WD was not touched and already expired, in this case time value you get
     is irrelevant
    -------------------------------------------------------------------------------------------------------*/
close_session:
    if (TRUE == secure)
    {
        (void)QLIB_CloseSession(qlibContext, section);
    }

disconnect:
    (void)QLIB_Disconnect(qlibContext);

    return status;
}

QLIB_STATUS_T QLIB_SAMPLE_WatchDogRun(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_CONTEXT_T    qlibContextOnStack;
    KEY_T             sectionKey = QCONF_FULL_ACCESS_K_1;
    U32               section    = PA_WRITE_PROTECT_SECTION_INDEX;
    U32               time;
    BOOL              expired;
    QLIB_BUS_FORMAT_T busFormat = QLIB_BUS_FORMAT(QLIB_BUS_MODE_1_1_1, FALSE, FALSE);

    if (qlibContext == NULL)
    {
        qlibContext = &qlibContextOnStack;
    }

    /*-------------------------------------------------------------------------------------------------------
     1) Initialization part
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     Init QLIB  - needed when running QLIB either on a device or on a remote server.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_InitLib(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Init Flash Device - not needed when using QLIB on a remote server
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_InitDevice(qlibContext, busFormat));

    /*-------------------------------------------------------------------------------------------------------
     Release the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    (void)QLIB_Disconnect(qlibContext);

    /*-------------------------------------------------------------------------------------------------------
     2) Functional WD usage flow part
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     Note that there is a default WD configuration stored in section configuration loaded on power up.
     Be careful with it, since if you configure default WD threshold which is very short, flash may signal reset
     even before board initialization is over.
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     first, we set (load) keys to the section. If secured WD is used it is associated with the section number
     and we must have correct full access key to that section.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_LoadKey(qlibContext, section, sectionKey, TRUE));

    /*-------------------------------------------------------------------------------------------------------
     We configure WD to signal every 4 minutes (approximately) . Now its counter starts to count up, and if not
     "touched", then after 4 minutes the reset bit will be raised.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_WatchDogConfig(qlibContext, TRUE, TRUE, QLIB_AWDT_TH_4_MINUTES, section));

    /*-------------------------------------------------------------------------------------------------------
     Check whether the WD counter is expired
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_WatchDogGet(qlibContext, TRUE, section, &time, &expired));

    /*-------------------------------------------------------------------------------------------------------
     Touching the WD will zero the counter and WD will bark only after 4 minutes again...
     Configure this in timer loop to continuously pet the WD.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_WatchDogTouch(qlibContext, TRUE, section));

    /*-------------------------------------------------------------------------------------------------------
     Disable WD
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_WatchDogConfig(qlibContext, FALSE, TRUE, QLIB_AWDT_TH_4_MINUTES, section));

    /*-------------------------------------------------------------------------------------------------------
     Clean the key reference from QLIB. optional
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_RemoveKey(qlibContext, section, TRUE));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_WatchDogCalibrate(QLIB_CONTEXT_T* qlibContext, const KEY_T deviceMasterKey)
{
    QLIB_STATUS_T        status = QLIB_STATUS__OK;
    QLIB_WATCHDOG_CONF_T watchdogOriginalCFG;
    QLIB_WATCHDOG_CONF_T watchdogTempCFG;
    U32                  measurementTime_ms = 1000;
    U32                  timeSec[2];
    U32                  timeRes[2];
    U32                  measuredOsc;
    U32                  measuredTime_ms;

    /*-------------------------------------------------------------------------------------------------------
     This calibration flow is based on W77Q spec section 8.4.4: AWDT Calibration
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Read original WD configurations and enable the timer
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_ConfigGet(qlibContext, &watchdogOriginalCFG), status, disconnect);

    memcpy(&watchdogTempCFG, &watchdogOriginalCFG, sizeof(QLIB_WATCHDOG_CONF_T));
    watchdogTempCFG.enable    = TRUE;
    watchdogTempCFG.threshold = QLIB_AWDT_TH_12_DAYS;
    watchdogTempCFG.lfOscEn   = TRUE;
    watchdogTempCFG.oscRateHz = QLIB_AWDTCFG__OSC_RATE_KHZ_DEFAULT << 10; // nominal frequency of LF-OSC

    /*-------------------------------------------------------------------------------------------------------
     In case of secure watchdog we need to open session to the section WD which it is associated with.
    -------------------------------------------------------------------------------------------------------*/
    if (TRUE == watchdogTempCFG.authenticated)
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, watchdogTempCFG.sectionID, QLIB_SESSION_ACCESS_FULL),
                                   status,
                                   disconnect);
    }

    /*-------------------------------------------------------------------------------------------------------
     Enable the WD timer
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_ConfigSet(qlibContext, &watchdogTempCFG), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Read the AWDT counter twice within constant interval time of "measurementTime_ms"
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_Get(qlibContext, &timeSec[0], &timeRes[0], NULL), status, close_session);
    QLIB_SAMPLE_DELAY_MSEC(measurementTime_ms);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_Get(qlibContext, &timeSec[1], &timeRes[1], NULL), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Calculate the LF-OSC frequency
    -------------------------------------------------------------------------------------------------------*/
    measuredOsc = (((timeSec[1] - timeSec[0]) * watchdogTempCFG.oscRateHz + timeRes[1] * (U32)64) - timeRes[0] * (U32)64) /
                  (measurementTime_ms / 1000);
    watchdogTempCFG.oscRateHz = measuredOsc & 0xFFFFFFC0; // LF-OSC calibration have 64 Hz resolution
    QLIB_ASSERT_WITH_ERROR_GOTO(0 < watchdogTempCFG.oscRateHz, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE, status, disconnect);

    /*-------------------------------------------------------------------------------------------------------
     Configure the calibration parameters to AWDTCFG based on the calculated LF-OSC frequency
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_ConfigSet(qlibContext, &watchdogTempCFG), status, close_session);

    if (TRUE == watchdogTempCFG.authenticated)
    {
        (void)QLIB_CloseSession(qlibContext, watchdogTempCFG.sectionID);
    }

    if (NULL != deviceMasterKey)
    {
        /*---------------------------------------------------------------------------------------------------
         If device master key is available, also update the default WD configuration for next POR
        ---------------------------------------------------------------------------------------------------*/
        watchdogOriginalCFG.oscRateHz = watchdogTempCFG.oscRateHz;
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_ConfigDevice(qlibContext,
                                                     deviceMasterKey,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     NULL,
                                                     &watchdogOriginalCFG,
                                                     NULL,
                                                     NULL),
                                   status,
                                   disconnect);
    }

    /*-------------------------------------------------------------------------------------------------------
     Read the AWDT counter again to validate the calibration is accurate
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_Get(qlibContext, &timeSec[0], &timeRes[0], NULL), status, disconnect);
    QLIB_SAMPLE_DELAY_MSEC(measurementTime_ms);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Watchdog_Get(qlibContext, &timeSec[1], &timeRes[1], NULL), status, disconnect);

    measuredTime_ms = ((timeSec[1] - timeSec[0]) * 1000) + ((timeRes[1] * 64 * 1000) / watchdogTempCFG.oscRateHz) -
                      ((timeRes[0] * 64 * 1000) / watchdogTempCFG.oscRateHz);

    QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO,
                     "WD LF-OSC frequency is %dHz, after calibration the WD count to %d mSec after %d mSec",
                     watchdogTempCFG.oscRateHz,
                     measuredTime_ms,
                     measurementTime_ms);

    // Allow maximum error of 1%
    QLIB_ASSERT_WITH_ERROR_GOTO(measuredTime_ms / 100 >=
                                    (MAX(measuredTime_ms, measurementTime_ms) - MIN(measuredTime_ms, measurementTime_ms)),
                                QLIB_STATUS__TEST_FAIL,
                                status,
                                disconnect);

    /*-------------------------------------------------------------------------------------------------------
     Now WD is calibrated.
    -------------------------------------------------------------------------------------------------------*/
    goto disconnect;
close_session:
    if (TRUE == watchdogTempCFG.authenticated)
    {
        (void)QLIB_CloseSession(qlibContext, watchdogTempCFG.sectionID);
    }

disconnect:
    (void)QLIB_Disconnect(qlibContext);

    return status;
}

QLIB_STATUS_T QLIB_SAMPLE_WatchDogCalibrateRun(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_CONTEXT_T       qlibContextOnStack;
    KEY_T                deviceMasterKey = QCONF_KD;
    U32                  section         = BOOT_SECTION_INDEX;
    QLIB_BUS_FORMAT_T    busFormat       = QLIB_BUS_FORMAT(QLIB_BUS_MODE_1_1_1, FALSE, FALSE);
    QLIB_WATCHDOG_CONF_T watchdogCFG;
    const KEY_T          sectionKey[QLIB_NUM_OF_SECTIONS] = {
        QCONF_FULL_ACCESS_K_0,
        QCONF_FULL_ACCESS_K_1,
        QCONF_FULL_ACCESS_K_2,
        QCONF_FULL_ACCESS_K_3,
        QCONF_FULL_ACCESS_K_4,
        QCONF_FULL_ACCESS_K_5,
        QCONF_FULL_ACCESS_K_6,
        QCONF_FULL_ACCESS_K_7,

    };

    if (qlibContext == NULL)
    {
        qlibContext = &qlibContextOnStack;
    }

    /*-------------------------------------------------------------------------------------------------------
     1) Initialization part
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     Init QLIB  - needed when running QLIB either on a device or on a remote server.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_InitLib(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Init Flash Device - not needed when using QLIB on a remote server
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_InitDevice(qlibContext, busFormat));

    /*-------------------------------------------------------------------------------------------------------
     2) Calibrate WD flow part
    -------------------------------------------------------------------------------------------------------*/

    QLIB_STATUS_RET_CHECK(QLIB_Watchdog_ConfigGet(qlibContext, &watchdogCFG));
    section = watchdogCFG.sectionID;

    /*-------------------------------------------------------------------------------------------------------
     Release the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    (void)QLIB_Disconnect(qlibContext);

    /*-------------------------------------------------------------------------------------------------------
     first, we set (load) keys to the section. If secured WD is used it is associated with the section number
     and we must have correct full access key to that section.
    -------------------------------------------------------------------------------------------------------*/
    if (watchdogCFG.authenticated)
    {
        QLIB_STATUS_RET_CHECK(QLIB_LoadKey(qlibContext, section, sectionKey[section], TRUE));
    }

    /*-------------------------------------------------------------------------------------------------------
     Calibrate the WD
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_WatchDogCalibrate(qlibContext, deviceMasterKey));

    /*-------------------------------------------------------------------------------------------------------
     Clean the key reference from QLIB. optional
    -------------------------------------------------------------------------------------------------------*/
    if (watchdogCFG.authenticated)
    {
        QLIB_STATUS_RET_CHECK(QLIB_RemoveKey(qlibContext, section, TRUE));
    }

    return QLIB_STATUS__OK;
}
