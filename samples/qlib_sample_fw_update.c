/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_fw_update.c
* @brief      This file contains QLIB fw update related functions implementation
*
* @example    qlib_sample_fw_update.c
*
* @page       fw_update FW update sample code
* This sample code shows how to use the FW update feature
*
* @include    samples/qlib_sample_fw_update.c
*
************************************************************************************************************/

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                                  INCLUDES
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
#include <stdio.h>

#include "qlib.h"
#include "qlib_sample_fw_update.h"
#include "qlib_sample_qconf.h"
#include "qlib_utils_crc.h"
#include "qlib_utils_digest.h"

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                         LOCAL FUNCTION DECLARATIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
static QLIB_STATUS_T QLIB_SAMPLE_SecCompare_L(QLIB_CONTEXT_T* qlibContext,
                                              const U8*       fw,
                                              U32             fw_size,
                                              U32             section,
                                              U32             sectionOffset,
                                              BOOL            secure,
                                              BOOL            auth);

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                             INTERFACE FUNCTIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_SAMPLE_FwUpdateRun(void)
{
    QLIB_CONTEXT_T    qlibContext;
    const U8          fw[]              = {'i', ' ', 'a', 'm', ' ', 'a', ' ', 'n', 'e', 'w', ' ', 'f', 'w'};
    KEY_T             sectionKey        = QCONF_FULL_ACCESS_K_3;
    U32               section           = 3; // section configured as rollback protected in configuration sample
    U32               newSectionVersion = 22;
    QLIB_BUS_FORMAT_T busFormat         = QLIB_BUS_FORMAT(QLIB_BUS_MODE_1_1_1, FALSE, FALSE);
    U32               fw32[1 + (sizeof(fw) / 4)];

    memset(fw32, 0xff, sizeof(fw32));
    memcpy(fw32, fw, sizeof(fw));

    /*-------------------------------------------------------------------------------------------------------
        Pre-configured CRC and Digest for the firmware section new sample firmware
    -------------------------------------------------------------------------------------------------------*/
    U64 digest = 0x0a5c3ee298dd239b;
    U32 crc = 0x30fc5598;
    /*-------------------------------------------------------------------------------------------------------
     1) Initialization part
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     Init QLIB  - needed when running QLIB either on a device or on a remote server.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_InitLib(&qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(&qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Init Flash Device - not needed when using QLIB on a remote server
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_InitDevice(&qlibContext, busFormat));

    /*-------------------------------------------------------------------------------------------------------
     Release the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    (void)QLIB_Disconnect(&qlibContext);

    /*-------------------------------------------------------------------------------------------------------
     2) Functional fw update flow part
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     First, we set (load) keys to the section to be updated. The key is necessary to manage secure session to the flash.
     In this sample we update the section full access key,
     otherwise update operation will be blocked by a flash
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_LoadKey(&qlibContext, section, sectionKey, TRUE));

    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_FirmwareUpdate(&qlibContext,
                                                     fw32,
                                                     sizeof(fw32),
                                                     section,
                                                     &newSectionVersion,
                                                     &digest,
                                                     &crc,
                                                     FALSE,
                                                     QLIB_SWAP));
    /*-------------------------------------------------------------------------------------------------------
     Clean the key reference from QLIB. optional
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_RemoveKey(&qlibContext, section, TRUE));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_FirmwareUpdate(QLIB_CONTEXT_T* qlibContext,
                                         const U32*      fw,
                                         U32             fw_size,
                                         U32             section,
                                         U32*            newVersion,
                                         U64*            checkDigest,
                                         U32*            checkCrc,
                                         BOOL            shouldCalcCrcDigest,
                                         QLIB_SWAP_T     swap)
{
    QLIB_STATUS_T ret = QLIB_STATUS__OK;

    /*-------------------------------------------------------------------------------------------------------
     Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
      When ran on embedded target and NOT remotely CRC and DIGEST are pre-calculated
     ------------------------------------------------------------------------------------------------------*/

#ifndef QLIB_SUPPORT_XIP
    /*-------------------------------------------------------------------------------------------------------
     Prepare everything needed to update section - this function will calculate digest/crc if needed.
     If the CRC/Digest are already known, this process is not needed.
    -------------------------------------------------------------------------------------------------------*/
    if (shouldCalcCrcDigest == TRUE)
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_SAMPLE_GetUpdateSectionParams(qlibContext,
                                                                      section,
                                                                      fw,
                                                                      fw_size,
                                                                      checkDigest,
                                                                      checkCrc,
                                                                      NULL),
                                   ret,
                                   disconnect);
    }
#else
    QLIB_ASSERT_WITH_ERROR_GOTO(shouldCalcCrcDigest == FALSE, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE, ret, disconnect);
#endif

    ret = QLIB_SAMPLE_SectionUpdate(qlibContext, (const U8*)fw, fw_size, section, checkDigest, checkCrc, newVersion, swap);

    /*-------------------------------------------------------------------------------------------------------
     If swap is set to QLIB_SWAP_AND_RESET device should undergo reset -> if this code runs locally we reset here
     so the following code relevant for remote execution only
    -------------------------------------------------------------------------------------------------------*/
    if ((swap == QLIB_SWAP_AND_RESET) && (ret == QLIB_STATUS__COMMUNICATION_ERR))
    {
        /*---------------------------------------------------------------------------------------------------
         Device should undergo reset and therefore does not answer. Let QLIB_STATUS__COMMUNICATION_ERR
         be the error that is returned by remote server TM layer, when there is no answer from IoT device.
        ---------------------------------------------------------------------------------------------------*/
        ret = QLIB_STATUS__OK;
        goto exit;
    }

    QLIB_ASSERT_WITH_ERROR_GOTO(ret == QLIB_STATUS__OK, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE, ret, disconnect);

    /*-------------------------------------------------------------------------------------------------------
     Verify the FW update operation. Securely read from the active half.
    -------------------------------------------------------------------------------------------------------*/
    if (swap == QLIB_SWAP)
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_SAMPLE_SecCompare_L(qlibContext, (const U8*)fw, fw_size, section, 0, TRUE, TRUE),
                                   ret,
                                   disconnect);
    }

disconnect:
    /*-------------------------------------------------------------------------------------------------------
     Release the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    (void)QLIB_Disconnect(qlibContext);

exit:

    return ret;
}
#ifndef QLIB_SUPPORT_XIP

QLIB_STATUS_T QLIB_SAMPLE_GetUpdateSectionParams(QLIB_CONTEXT_T* qlibContext,
                                                 U32             section,
                                                 const U32*      buff,
                                                 U32             buffSize,
                                                 U64*            digestIntegrity,
                                                 U32*            checksumIntegrity,
                                                 U32*            oldVersion)
{
    U32           sectionLen;
    QLIB_POLICY_T policy;
    QLIB_STATUS_T ret;

    QLIB_STATUS_RET_CHECK(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL));

    /*-------------------------------------------------------------------------------------------------------
     To calculate crc/digest section length.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_GetSectionConfiguration(qlibContext,
                                                            section,
                                                            NULL,
                                                            &sectionLen,
                                                            &policy,
                                                            NULL,
                                                            NULL,
                                                            oldVersion),
                               ret,
                               close_session);

    /*-------------------------------------------------------------------------------------------------------
     Get section information and check that the section is defined as rollback protected section.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(1 == policy.rollbackProt, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE, ret, close_session);

    if (digestIntegrity || checksumIntegrity)
    {
        if (digestIntegrity)
        {
#ifndef LPC54005_SERIES
            static U32 sectionBuf[QLIB_SAMPLE_MAX_FW_SIZE / 4];
            memset(sectionBuf, 0xFF, sectionLen / 2);
            memcpy(sectionBuf, buff, buffSize);
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_UTILS_CalcDigest(sectionBuf, sectionLen / 2, digestIntegrity), ret, close_session);
#else
            // there is not enough RAM to hold all the section buffer
            QLIB_STATUS_RET_CHECK(QLIB_STATUS__NOT_SUPPORTED);
#endif
        }

        if (checksumIntegrity)
        {
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_UTILS_CalcCRCWithPadding(buff,
                                                                     buffSize,
                                                                     0xFFFFFFFF,
                                                                     (sectionLen / 2) - buffSize,
                                                                     checksumIntegrity),
                                       ret,
                                       close_session);
        }
    }

close_session:

    (void)QLIB_CloseSession(qlibContext, section);

    return ret;
}
#endif // QLIB_SUPPORT_XIP

QLIB_STATUS_T QLIB_SAMPLE_SectionUpdate(QLIB_CONTEXT_T* qlibContext,
                                        const U8*       buff,
                                        U32             buffSize,
                                        U32             section,
                                        U64*            digestIntegrity,
                                        U32*            checksumIntegrity,
                                        U32*            newVersion,
                                        QLIB_SWAP_T     swap)
{
    QLIB_STATUS_T ret = QLIB_STATUS__OK;
    U32           sectionLen;
    QLIB_POLICY_T policy;

    /*-------------------------------------------------------------------------------------------------------
     Open the session. Assumed that the key to that section is already loaded.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL));

    /*-------------------------------------------------------------------------------------------------------
     Check that the section is defined as rollback protected and error exit if not.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_GetSectionConfiguration(qlibContext, section, NULL, &sectionLen, &policy, NULL, NULL, NULL),
                               ret,
                               close_session);

    QLIB_ASSERT_WITH_ERROR_GOTO(1 == policy.rollbackProt, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE, ret, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Perform secure write operations to write DATA into inactive partition of the section (high).
     The buffer size should be not bigger than half of the section length as in case of rollback protected section,
     the section is divided to two parts.
     Note: If section plain write is enabled, non-secure erase/write operations might be used instead.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(buffSize <= (sectionLen / 2), QLIB_STATUS__INVALID_DATA_SIZE, ret, close_session);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Erase(qlibContext, section, (sectionLen / 2), (sectionLen / 2), TRUE), ret, close_session);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Write(qlibContext, buff, section, (sectionLen / 2), buffSize, TRUE), ret, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Now, in order to finish we want to swap the new fw with the current one.
     If configured as CRC/digest protected, we need to give correct digest/crc to that command.
    -------------------------------------------------------------------------------------------------------*/
    policy.checksumIntegrity = checksumIntegrity != NULL ? 1 : 0;
    policy.digestIntegrity   = digestIntegrity != NULL ? 1 : 0;

    if (0 == policy.digestIntegrity)
    {
        QLIB_DEBUG_PRINT(QLIB_VERBOSE_WARNING, "Warning: Update section with digest integrity disabled");
    }

    if (0 == policy.checksumIntegrity)
    {
        QLIB_DEBUG_PRINT(QLIB_VERBOSE_WARNING, "Warning: Update section with checksum (CRC) integrity disabled");
    }

    /*-------------------------------------------------------------------------------------------------------
     Last command to finish, note if section updated is section 0 - (the one we run from):
     QLIB_SWAP_AND_RESET should be used as a last parameter in order for reset to occur so we can run from
     the new updated FW.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_PRINT_CMD("\nSENDING QLIB_ConfigSection with crc 0x%x \r\n", checksumIntegrity == NULL ? 0 : *checksumIntegrity);

    QLIB_STATUS_RET_CHECK_GOTO(QLIB_ConfigSection(qlibContext,
                                                  section,
                                                  &policy,
                                                  digestIntegrity,
                                                  checksumIntegrity,
                                                  newVersion,
                                                  swap),
                               ret,
                               close_session);

close_session:

    /*-------------------------------------------------------------------------------------------------------
     even when the device is after reset, close section will work as needed from remote since it does not
     involve TM transactions
    -------------------------------------------------------------------------------------------------------*/
    (void)QLIB_CloseSession(qlibContext, section);

    return ret;
}

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                               LOCAL FUNCTIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine make memcmp over secure flash
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   fw              FW buffer
 * @param[in]   fw_size         FW buffer size in bytes
 * @param[in]   section         [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   sectionOffset   [Section offset](md_definitions.html#DEF_OFFSET)
 * @param[in]   secure          If read is secure
 * @param[in]   auth            If read is authenticated
 *
 * @return      0 if fw[fw size] is equal to flash content from section[sectionOffset]
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SAMPLE_SecCompare_L(QLIB_CONTEXT_T* qlibContext,
                                              const U8*       fw,
                                              U32             fw_size,
                                              U32             section,
                                              U32             sectionOffset,
                                              BOOL            secure,
                                              BOOL            auth)
{
    QLIB_STATUS_T ret      = QLIB_STATUS__OK;
    U32           compared = 0;
    U32           takeSize = 0;
    U32           take[4];

    QLIB_STATUS_RET_CHECK(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL));

    while (compared < fw_size)
    {
        takeSize = MIN(fw_size - compared, sizeof(take));
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_Read(qlibContext, (U8*)take, section, sectionOffset, takeSize, secure, auth),
                                   ret,
                                   close_session);
        QLIB_ASSERT_WITH_ERROR_GOTO(0 == memcmp(&fw[compared], (U8*)take, takeSize),
                                    QLIB_STATUS__COMMAND_FAIL,
                                    ret,
                                    close_session);
        compared += takeSize;
    }

close_session:

    (void)QLIB_CloseSession(qlibContext, section);
    return ret;
}
