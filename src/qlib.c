/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib.c
* @brief      This file contains QLIB main interface
*
* ### project qlib
*
************************************************************************************************************/
#define __QLIB_C__
// Prevent unused macro warning

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_DEVICE_INITIALIZED(qlibContext) ((qlibContext)->busInterface.busMode != QLIB_BUS_MODE_INVALID)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                        LOCAL FUNCTION PROTOTYPES                                        */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
QLIB_STATUS_T QLIB_IsFlashSecure_L(QLIB_CONTEXT_T* qlibContext, BOOL* secure);
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_InitLib(QLIB_CONTEXT_T* qlibContext)
{
    void* userData;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* We must keep user data untouched since could be set before calling QLIB_InitLib                     */
    /*-----------------------------------------------------------------------------------------------------*/
    userData = QLIB_GetUserData(qlibContext);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear globals                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    memset(qlibContext, 0, sizeof(QLIB_CONTEXT_T));


#ifndef QLIB_SEC_ONLY
    /*-----------------------------------------------------------------------------------------------------*/
    /* Initiate the standard module                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_InitLib(qlibContext));
#endif // QLIB_SEC_ONLY

    /*-----------------------------------------------------------------------------------------------------*/
    /* Initiate the secure module                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_InitLib(qlibContext));


    /*-----------------------------------------------------------------------------------------------------*/
    /* Initiate the transport manager module with configured user data                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_SetUserData(qlibContext, userData);
    QLIB_STATUS_RET_CHECK(QLIB_TM_Init(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SetInterface(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Change the bus interface                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_SetInterface(qlibContext, busFormat));
    QLIB_STATUS_RET_CHECK(QLIB_SEC_SetInterface(qlibContext, busFormat));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_InitDevice(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat)
{
    QLIB_BUS_MODE_T currentMode;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Make sure to be synchronized with the flash device, exit power down mode,                           */
    /* detect SPI mode, make sure flash is ready and there is no connectivity issues                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if (qlibContext->busInterface.busMode == QLIB_BUS_MODE_INVALID)
    {
        BOOL secure;
        QLIB_STATUS_T status;
        do
        {
            status = QLIB_STD_AutoSense(qlibContext, &currentMode);
        } while (status == QLIB_STATUS__CONNECTIVITY_ERR);
        QLIB_STATUS_RET_CHECK(status);


        // autosense again to identify best mode, since device might have been busy during first autosense
        QLIB_STATUS_RET_CHECK(QLIB_STD_AutoSense(qlibContext, &currentMode));

        QLIB_STATUS_RET_CHECK(QLIB_SetInterface(qlibContext, QLIB_BUS_FORMAT(currentMode, FALSE, FALSE)));

        // Clear any error indication from invalid command during auto-sensing
        QLIB_STATUS_RET_CHECK(QLIB_IsFlashSecure_L(qlibContext, &secure));
        if (secure)
        {
            // Read SSR to clear all sticky (ROC) error bits in SSR but do not consider any bit as error.
            // This function will also implicit perform OP1 to clear the Non-sticky ERR bit if exist
            QLIB_STATUS_RET_CHECK(QLIB_SEC__get_SSR(qlibContext, NULL, 0));

            // Read SSR again to verify all errors have been cleared
            QLIB_STATUS_RET_CHECK(QLIB_SEC__get_SSR(qlibContext, NULL, SSR_MASK__ALL_ERRORS));
        }
    }
    if (QLIB_BUS_FORMAT_GET_MODE(busFormat) != QLIB_BUS_MODE_AUTOSENSE)
    {
        QLIB_STATUS_RET_CHECK(QLIB_SetInterface(qlibContext, busFormat));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Make sure bus interface has initialized                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->busInterface.busMode != QLIB_BUS_MODE_INVALID, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    QLIB_ASSERT_RET(qlibContext->busInterface.secureCmdsFormat != QLIB_BUS_MODE_INVALID, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

#ifndef QLIB_SEC_ONLY
    /*-----------------------------------------------------------------------------------------------------*/
    /* Resume power-down and any suspended erase / write command                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_Power(qlibContext, QLIB_POWER_UP));
    QLIB_STATUS_RET_CHECK(QLIB_STD_EraseResume(qlibContext, TRUE));
#endif // QLIB_SEC_ONLY

    /*-----------------------------------------------------------------------------------------------------*/
    /* synchronize the lib state with the flash secure module state                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_SyncState(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_Read(QLIB_CONTEXT_T* qlibContext, U8* buf, U32 sectionID, U32 offset, U32 size, BOOL secure, BOOL auth)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(0 < size, QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
    QLIB_ASSERT_RET((offset + size) >= size, QLIB_STATUS__INVALID_PARAMETER);


    if (TRUE == secure)
    {
        QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);
        QLIB_ASSERT_RET((offset + size) <= QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(qlibContext->sectionsState[sectionID].sizeTag),
                        QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        return QLIB_SEC_Read(qlibContext, buf, sectionID, offset, size, auth);
    }
    else
    {
#ifndef QLIB_SEC_ONLY
        U32 q2Section = QLIB_VALUE_BY_FLASH_TYPE(qlibContext, QLIB_FALLBACK_SECTION(qlibContext, sectionID), sectionID);
        QLIB_ASSERT_RET((offset + size) <= _QLIB_MAX_LEGACY_OFFSET(qlibContext), QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        QLIB_ASSERT_RET(sectionID < _QLIB_MAX_LEGACY_SECTION_ID(qlibContext), QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(qlibContext,
                                           QLIB_ASSERT_RET((offset + size) <= QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(
                                                                                  qlibContext->sectionsState[q2Section].sizeTag),
                                                           QLIB_STATUS__PARAMETER_OUT_OF_RANGE));
        QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(
            qlibContext,
            if (qlibContext->sectionsState[q2Section].plainEnabled == 0) {
                QLIB_STATUS_RET_CHECK(QLIB_PlainAccessEnable(qlibContext, q2Section));
            })

        return QLIB_STD_Read(qlibContext, buf, _QLIB_MAKE_LOGICAL_ADDRESS(sectionID, offset, qlibContext->addrSize), size);
#else
        return QLIB_STATUS__NOT_SUPPORTED;
#endif // QLIB_SEC_ONLY
    }
}

QLIB_STATUS_T QLIB_Write(QLIB_CONTEXT_T* qlibContext, const U8* buf, U32 sectionID, U32 offset, U32 size, BOOL secure)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(0 < size, QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
    QLIB_ASSERT_RET((offset + size) >= size, QLIB_STATUS__INVALID_PARAMETER);


    if (TRUE == secure)
    {
        QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);
        QLIB_ASSERT_RET((offset + size) <= QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(qlibContext->sectionsState[sectionID].sizeTag),
                        QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        return QLIB_SEC_Write(qlibContext, buf, sectionID, offset, size);
    }
    else
    {
#ifndef QLIB_SEC_ONLY
        U32 q2Section = QLIB_VALUE_BY_FLASH_TYPE(qlibContext, QLIB_FALLBACK_SECTION(qlibContext, sectionID), sectionID);
        QLIB_ASSERT_RET((offset + size) <= _QLIB_MAX_LEGACY_OFFSET(qlibContext), QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        QLIB_ASSERT_RET(sectionID < _QLIB_MAX_LEGACY_SECTION_ID(qlibContext), QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(qlibContext,
                                           QLIB_ASSERT_RET((offset + size) <= QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(
                                                                                  qlibContext->sectionsState[q2Section].sizeTag),
                                                           QLIB_STATUS__PARAMETER_OUT_OF_RANGE));
        QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(
            qlibContext,
            if (qlibContext->sectionsState[q2Section].plainEnabled == 0) {
                QLIB_STATUS_RET_CHECK(QLIB_PlainAccessEnable(qlibContext, q2Section));
            });
        return QLIB_STD_Write(qlibContext, buf, _QLIB_MAKE_LOGICAL_ADDRESS(sectionID, offset, qlibContext->addrSize), size);
#else
        return QLIB_STATUS__NOT_SUPPORTED;
#endif // QLIB_SEC_ONLY
    }
}

QLIB_STATUS_T QLIB_Erase(QLIB_CONTEXT_T* qlibContext, U32 sectionID, U32 offset, U32 size, BOOL secure)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(0 < size, QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
    QLIB_ASSERT_RET((offset + size) >= size, QLIB_STATUS__INVALID_PARAMETER);


    if (TRUE == secure)
    {
        QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        QLIB_ASSERT_RET((offset + size) <= QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(qlibContext->sectionsState[sectionID].sizeTag),
                        QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        return QLIB_SEC_Erase(qlibContext, sectionID, offset, size);
    }
    else
    {
#ifndef QLIB_SEC_ONLY
        U32 q2Section = QLIB_VALUE_BY_FLASH_TYPE(qlibContext, QLIB_FALLBACK_SECTION(qlibContext, sectionID), sectionID);
        QLIB_ASSERT_RET((offset + size) <= _QLIB_MAX_LEGACY_OFFSET(qlibContext), QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        QLIB_ASSERT_RET(sectionID < _QLIB_MAX_LEGACY_SECTION_ID(qlibContext), QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(qlibContext,
                                           QLIB_ASSERT_RET((offset + size) <= QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(
                                                                                  qlibContext->sectionsState[q2Section].sizeTag),
                                                           QLIB_STATUS__PARAMETER_OUT_OF_RANGE));

        QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(
            qlibContext,
            if (qlibContext->sectionsState[q2Section].plainEnabled == 0) {
                QLIB_STATUS_RET_CHECK(QLIB_PlainAccessEnable(qlibContext, q2Section));
            });
        return QLIB_STD_Erase(qlibContext, _QLIB_MAKE_LOGICAL_ADDRESS(sectionID, offset, qlibContext->addrSize), size);
#else
        return QLIB_STATUS__NOT_SUPPORTED;
#endif // QLIB_SEC_ONLY
    }
}

QLIB_STATUS_T QLIB_EraseSection(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL secure)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);

#ifndef QLIB_SEC_ONLY
    if (FALSE == secure)
    {
        if (qlibContext->sectionsState[sectionID].plainEnabled == 0)
        {
            QLIB_STATUS_RET_CHECK(QLIB_PlainAccessEnable(qlibContext, sectionID));
        }
    }
#else
    if (FALSE == secure)
    {
        return QLIB_STATUS__NOT_SUPPORTED;
    }
#endif // QLIB_SEC_ONLY

    return QLIB_SEC_EraseSection(qlibContext, sectionID, secure);
}

#ifndef QLIB_SEC_ONLY
QLIB_STATUS_T QLIB_Suspend(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    QLIB_STATUS_RET_CHECK(QLIB_STD_EraseSuspend(qlibContext));

    QLIB_ACTION_BY_FLASH_TYPE(qlibContext,
                              qlibContext->isSuspended                                      = TRUE,
                              qlibContext->stdState[qlibContext->activeDie - 1].isSuspended = TRUE);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_Resume(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(qlibContext->isSuspended == TRUE, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    QLIB_STATUS_RET_CHECK(QLIB_STD_EraseResume(qlibContext, FALSE));

    QLIB_ACTION_BY_FLASH_TYPE(
        qlibContext,
        {
            qlibContext->isSuspended = FALSE;
            qlibContext->mcInSync    = FALSE;
        },
        qlibContext->stdState[qlibContext->activeDie - 1].isSuspended = FALSE);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_Power(QLIB_CONTEXT_T* qlibContext, QLIB_POWER_T power)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_STATUS_RET_CHECK(QLIB_STD_Power(qlibContext, power));

    /*-----------------------------------------------------------------------------------------------------*/
    /* During power down SW could try to communicate and increment counters                                */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(
        qlibContext,
        if (QLIB_POWER_UP == power) { qlibContext->mcInSync = FALSE; });

    return QLIB_STATUS__OK;
}
#endif // QLIB_SEC_ONLY

QLIB_STATUS_T QLIB_ResetFlash(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform reset                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_ResetFlash(qlibContext, TRUE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Sync secure side after reset                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_SyncAfterFlashReset(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_Format(QLIB_CONTEXT_T* qlibContext, const KEY_T deviceMasterKey, BOOL eraseDataOnly)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(FALSE == eraseDataOnly || QLIB_KEY_MNGR__IS_KEY_VALID(deviceMasterKey), QLIB_STATUS__INVALID_PARAMETER);

    return QLIB_SEC_Format(qlibContext, deviceMasterKey, eraseDataOnly);
}

QLIB_STATUS_T QLIB_GetNotifications(QLIB_CONTEXT_T* qlibContext, QLIB_NOTIFICATIONS_T* notifs)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != notifs, QLIB_STATUS__INVALID_PARAMETER);

    return QLIB_SEC_GetNotifications(qlibContext, notifs);
}

QLIB_STATUS_T QLIB_PerformMaintenance(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);

    return QLIB_SEC_PerformMCMaint(qlibContext);
}

QLIB_STATUS_T QLIB_ConfigDevice(QLIB_CONTEXT_T*                   qlibContext,
                                const KEY_T                       deviceMasterKey,
                                const KEY_T                       deviceSecretKey,
                                const QLIB_SECTION_CONFIG_TABLE_T sectionTable,
                                const KEY_ARRAY_T                 restrictedKeys,
                                const KEY_ARRAY_T                 fullAccessKeys,
                                const QLIB_WATCHDOG_CONF_T*       watchdogDefault,
                                const QLIB_DEVICE_CONF_T*         deviceConf,
                                const _128BIT                     suid)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure the device                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_ConfigDevice(qlibContext,
                                                deviceMasterKey,
                                                deviceSecretKey,
                                                sectionTable,
                                                restrictedKeys,
                                                fullAccessKeys,
                                                watchdogDefault,
                                                deviceConf,
                                                suid));
    /*-----------------------------------------------------------------------------------------------------*/
    /* Re-sync the device                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_SyncState(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_GetSectionConfiguration(QLIB_CONTEXT_T* qlibContext,
                                           U32             sectionID,
                                           U32*            baseAddr,
                                           U32*            size,
                                           QLIB_POLICY_T*  policy,
                                           U64*            digest,
                                           U32*            crc,
                                           U32*            version)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_GetSectionConfiguration(qlibContext, sectionID, baseAddr, size, policy, digest, crc, version);
}

QLIB_STATUS_T QLIB_ConfigSection(QLIB_CONTEXT_T*      qlibContext,
                                 U32                  sectionID,
                                 const QLIB_POLICY_T* policy,
                                 const U64*           digest,
                                 const U32*           crc,
                                 const U32*           newVersion,
                                 QLIB_SWAP_T          swap)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform Section Config                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_ConfigSection(qlibContext, sectionID, policy, digest, crc, newVersion, swap));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reopen the session to this section, after 'set_SCRn' revokes the access privileges to the section   */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSession(qlibContext, sectionID, QLIB_SESSION_ACCESS_FULL));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_ConfigAccess(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL readEnable, BOOL writeEnable)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform configuration                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    return QLIB_SEC_ConfigAccess(qlibContext, sectionID, readEnable, writeEnable);
}

QLIB_STATUS_T QLIB_LoadKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, const KEY_T key, BOOL fullAccess)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_LoadKey(qlibContext, sectionID, key, fullAccess);
}

QLIB_STATUS_T QLIB_RemoveKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL fullAccess)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_RemoveKey(qlibContext, sectionID, fullAccess);
}

QLIB_STATUS_T QLIB_Connect(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_STATUS_RET_CHECK(QLIB_TM_Connect(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_Disconnect(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(FALSE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    if (QLIB_DEVICE_INITIALIZED(qlibContext))
    {
    }

    QLIB_STATUS_RET_CHECK(QLIB_TM_Disconnect(qlibContext));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_OpenSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_SESSION_ACCESS_T sessionAccess)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_OpenSession(qlibContext, sectionID, sessionAccess);
}

QLIB_STATUS_T QLIB_CloseSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_CloseSession(qlibContext, sectionID);
}

QLIB_STATUS_T QLIB_PlainAccessEnable(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(sectionID < QLIB_NUM_OF_SECTIONS, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if we need to enable plain access                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (qlibContext->sectionsState[sectionID].plainEnabled == 0)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Enable plain access                                                                             */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_SEC_EnablePlainAccess(qlibContext, sectionID));

        /*-------------------------------------------------------------------------------------------------*/
        /* Mark plain access is enabled                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        qlibContext->sectionsState[sectionID].plainEnabled = 1;
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_AuthPlainAccess_Grant(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    QLIB_POLICY_T policy;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);


    /*-----------------------------------------------------------------------------------------------------*/
    /* check if the section is authenticated plain read                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_GetSectionConfiguration(qlibContext, sectionID, NULL, NULL, &policy, NULL, NULL, NULL));
    QLIB_ASSERT_RET(policy.authPlainAccess, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(policy.plainAccessReadEnable || policy.plainAccessWriteEnable, QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

    if (qlibContext->sectionsState[sectionID].plainEnabled == 0)
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC_AuthPlainAccess_Grant(qlibContext, sectionID));
    }
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_AuthPlainAccess_Revoke(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    QLIB_POLICY_T policy;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);


    /*-----------------------------------------------------------------------------------------------------*/
    /* check if the section is authenticated plain read                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_GetSectionConfiguration(qlibContext, sectionID, NULL, NULL, &policy, NULL, NULL, NULL));
    QLIB_ASSERT_RET(policy.authPlainAccess, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(policy.plainAccessReadEnable || policy.plainAccessWriteEnable, QLIB_STATUS__INVALID_PARAMETER);

    if (qlibContext->sectionsState[sectionID].plainEnabled == 1)
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC_AuthPlainAccess_Revoke(qlibContext, sectionID));
    }
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CheckIntegrity(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_INTEGRITY_T integrityType)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_CheckIntegrity(qlibContext, sectionID, integrityType);
}

QLIB_STATUS_T QLIB_CalcCDI(QLIB_CONTEXT_T* qlibContext, _256BIT nextCdi, _256BIT prevCdi, U32 sectionId)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != nextCdi, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionId, QLIB_STATUS__INVALID_PARAMETER);

    return QLIB_SEC_CalcCDI(qlibContext, nextCdi, prevCdi, sectionId);
}

QLIB_STATUS_T QLIB_Watchdog_ConfigSet(QLIB_CONTEXT_T* qlibContext, const QLIB_WATCHDOG_CONF_T* watchdogCFG)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != watchdogCFG, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_Watchdog_ConfigSet(qlibContext, watchdogCFG);
}

QLIB_STATUS_T QLIB_Watchdog_ConfigGet(QLIB_CONTEXT_T* qlibContext, QLIB_WATCHDOG_CONF_T* watchdogCFG)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != watchdogCFG, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_Watchdog_ConfigGet(qlibContext, watchdogCFG);
}

QLIB_STATUS_T QLIB_Watchdog_Touch(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_Watchdog_Touch(qlibContext);
}

QLIB_STATUS_T QLIB_Watchdog_Trigger(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_Watchdog_Trigger(qlibContext);
}

QLIB_STATUS_T QLIB_Watchdog_Get(QLIB_CONTEXT_T* qlibContext, U32* secondsPassed, U32* ticsResidue, BOOL* expired)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    return QLIB_SEC_Watchdog_Get(qlibContext, secondsPassed, ticsResidue, expired);
}

QLIB_STATUS_T QLIB_GetId(QLIB_CONTEXT_T* qlibContext, QLIB_ID_T* id_p)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != id_p, QLIB_STATUS__INVALID_PARAMETER);

#ifndef QLIB_SEC_ONLY
    QLIB_STATUS_RET_CHECK(QLIB_STD_GetId(qlibContext, &(id_p->std)));
#endif // QLIB_SEC_ONLY

    QLIB_ACTION_BY_FLASH_TYPE(qlibContext,
                              QLIB_STATUS_RET_CHECK(QLIB_SEC_GetId(qlibContext, &(id_p->sec))),
                              memset(&(id_p->sec), 0xFF, sizeof(id_p->sec)));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_GetHWVersion(QLIB_CONTEXT_T* qlibContext, QLIB_HW_VER_T* hwVer)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != hwVer, QLIB_STATUS__INVALID_PARAMETER);

#ifndef QLIB_SEC_ONLY
    QLIB_STATUS_RET_CHECK(QLIB_STD_GetHwVersion(qlibContext, &(hwVer->std)));
#endif // QLIB_SEC_ONLY

    QLIB_ACTION_BY_FLASH_TYPE(qlibContext,
                              QLIB_STATUS_RET_CHECK(QLIB_SEC_GetHWVersion(qlibContext, &(hwVer->sec))),
                              memset(&(hwVer->sec), 0xFF, sizeof(hwVer->sec)));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_GetStatus(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);

    return QLIB_SEC_GetStatus(qlibContext);
}

void* QLIB_GetUserData(QLIB_CONTEXT_T* qlibContext)
{
    if (qlibContext == NULL)
    {
        return NULL;
    }
    else
    {
        return qlibContext->userData;
    }
}

void QLIB_SetUserData(QLIB_CONTEXT_T* qlibContext, void* userData)
{
    if (qlibContext != NULL)
    {
        qlibContext->userData = userData;
    }
}

QLIB_STATUS_T QLIB_ExportState(QLIB_CONTEXT_T* qlibContext, QLIB_SYNC_OBJ_T* syncObject)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != syncObject, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Fill the synchronization object                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    memset(syncObject, 0, sizeof(QLIB_SYNC_OBJ_T));
    memcpy(&syncObject->busInterface, &qlibContext->busInterface, sizeof(qlibContext->busInterface));
    syncObject->busInterface.busIsLocked = FALSE;
    memcpy(syncObject->wid, qlibContext->wid, sizeof(QLIB_WID_T));
    syncObject->resetStatus = qlibContext->resetStatus;

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_ImportState(QLIB_CONTEXT_T* qlibContext, const QLIB_SYNC_OBJ_T* syncObject)
{
    BOOL busIsLocked = FALSE;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != syncObject, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Update the lib context                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    busIsLocked = qlibContext->busInterface.busIsLocked;
    memcpy(&qlibContext->busInterface, &syncObject->busInterface, sizeof(qlibContext->busInterface));
    qlibContext->busInterface.busIsLocked = busIsLocked;
    memcpy(qlibContext->wid, syncObject->wid, sizeof(QLIB_WID_T));
    qlibContext->resetStatus = syncObject->resetStatus;

    return QLIB_STATUS__OK;
}

U32 QLIB_GetVersion(void)
{
    return QLIB_VERSION;
}



QLIB_STATUS_T QLIB_GetResetStatus(QLIB_CONTEXT_T* qlibContext, QLIB_RESET_STATUS_T* resetStatus)
{
    QLIB_ASSERT_RET(NULL != qlibContext, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(NULL != resetStatus, QLIB_STATUS__INVALID_PARAMETER);

    *resetStatus = qlibContext->resetStatus;

    return QLIB_STATUS__OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/************************************************************************************************************
* @brief       This routine returns whether flash chip is secure
*
* @param       qlibContext   qlib context object
* @param[out]  secure        TRUE if active flash is secure, FALSE otherwise
*
* @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_IsFlashSecure_L(QLIB_CONTEXT_T* qlibContext, BOOL* secure)
{
    (void)qlibContext;
    *secure = TRUE;
    return QLIB_STATUS__OK;
}

