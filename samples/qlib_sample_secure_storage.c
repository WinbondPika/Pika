/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2020 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_secure_storage.c
* @brief      This file contains QLIB secure storage sample code
*
* @example    qlib_sample_secure_storage.c
*
* @page       secure_storage Secure storage sample code
* This sample code shows how to perform secure storage.\n
* The first function shows erase/write/read from W77Q as legacy flash.\n
* The second function shows erase/write/read from W77Q as secure flash using full key.\n
* The third function shows erase/write/read from W77Q as secure flash using restricted key.\n
*
* @include    samples/qlib_sample_secure_storage.c
*
************************************************************************************************************/

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                                  INCLUDES
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
#include <stdio.h>

#include "qlib.h"
#include "qlib_sample_secure_storage.h"
#include "qlib_sample_qconf.h"

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                             INTERFACE FUNCTIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
QLIB_STATUS_T QLIB_SAMPLE_SecureStorage(void)
{
    QLIB_CONTEXT_T    qlibContext;
    QLIB_STATUS_T     status    = QLIB_STATUS__OK;
    QLIB_BUS_FORMAT_T busFormat = QLIB_BUS_FORMAT(QLIB_BUS_MODE_1_1_1, FALSE, FALSE);

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
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_InitDevice(&qlibContext, busFormat), status, disconnect);

    /*-------------------------------------------------------------------------------------------------------
     Run sample code
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     secure section with plain read access
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_SAMPLE_SecureSectionWithPlainAccessRead(&qlibContext), status, disconnect);

    /*-------------------------------------------------------------------------------------------------------
     fully secure section
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_SAMPLE_SecureSectionFullKey(&qlibContext), status, disconnect);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_SAMPLE_SecureSectionRestrictedKey(&qlibContext), status, disconnect);

disconnect:
    (void)QLIB_Disconnect(&qlibContext);
    return status;
}

QLIB_STATUS_T QLIB_SAMPLE_SecureSectionWithPlainAccessRead(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_T status = QLIB_STATUS__OK;
    U32   section = PA_WRITE_PROTECT_SECTION_INDEX; //section (1) is pre-configured as secure section with plain read access
    KEY_T key     = QCONF_FULL_ACCESS_K_1;
    U8    writeBuf[FLASH_PAGE_SIZE];
    U8    readBuf[FLASH_PAGE_SIZE];
    U8    eraseBuf[FLASH_PAGE_SIZE];

    memset(readBuf, 0, sizeof(writeBuf));
    memset(writeBuf, 0xa5, sizeof(writeBuf));
    memset(eraseBuf, 0xFF, sizeof(eraseBuf));

    /*-------------------------------------------------------------------------------------------------------
     Load the key for the secure write operations
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_LoadKey(qlibContext, section, key, TRUE));

    /*-------------------------------------------------------------------------------------------------------
     Open full access secure session for the secure commands
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL), status, remove_key);

    /*-------------------------------------------------------------------------------------------------------
     Secure erase
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Erase(qlibContext, section, 0, FLASH_SECTOR_SIZE, TRUE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Secure write
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Write(qlibContext, writeBuf, section, 0, sizeof(writeBuf), TRUE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Non-secure read
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Read(qlibContext, readBuf, section, 0, sizeof(readBuf), FALSE, FALSE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Verify data
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(0 == memcmp(readBuf, writeBuf, sizeof(writeBuf)), QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Non-secure erase is not allowed
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ALLOW_TO_FAIL__START();
    status = QLIB_Erase(qlibContext, section, 0, FLASH_SECTOR_SIZE, FALSE);
    QLIB_ALLOW_TO_FAIL__END();
    QLIB_ASSERT_WITH_ERROR_GOTO(status == QLIB_STATUS__DEVICE_PRIVILEGE_ERR, QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Non-secure read
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Read(qlibContext, readBuf, section, 0, sizeof(readBuf), FALSE, FALSE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Verify data
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(0 == memcmp(readBuf, writeBuf, sizeof(writeBuf)), QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Secure erase
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Erase(qlibContext, section, 0, FLASH_SECTOR_SIZE, TRUE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Non-secure write is not allowed
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ALLOW_TO_FAIL__START();
    status = QLIB_Write(qlibContext, writeBuf, section, 0, sizeof(writeBuf), FALSE);
    QLIB_ALLOW_TO_FAIL__END();
    QLIB_ASSERT_WITH_ERROR_GOTO(status == QLIB_STATUS__DEVICE_PRIVILEGE_ERR, QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Non-secure read
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Read(qlibContext, readBuf, section, 0, sizeof(readBuf), FALSE, FALSE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Verify sector is cleared
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(0 == memcmp(readBuf, eraseBuf, sizeof(eraseBuf)), QLIB_STATUS__TEST_FAIL, status, close_session);

    status = QLIB_STATUS__OK;

close_session:
    (void)QLIB_CloseSession(qlibContext, section);

remove_key:
    (void)QLIB_RemoveKey(qlibContext, section, TRUE);

    return status;
}

QLIB_STATUS_T QLIB_SAMPLE_SecureSectionFullKey(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_T status  = QLIB_STATUS__OK;
    U32           section = SECURE_DATA_SECTION_INDEX; //section (4) is pre-configured as fully secure section
    KEY_T         key     = QCONF_FULL_ACCESS_K_4;
    U8            writeBuf[FLASH_PAGE_SIZE];
    U8            readBuf[FLASH_PAGE_SIZE];
    U8            eraseBuf[FLASH_PAGE_SIZE];

    memset(readBuf, 0, sizeof(writeBuf));
    memset(writeBuf, 0xa5, sizeof(writeBuf));
    memset(eraseBuf, 0xFF, sizeof(eraseBuf));

    /*-------------------------------------------------------------------------------------------------------
     Load the key for the secure write operations
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_LoadKey(qlibContext, section, key, TRUE));

    /*-------------------------------------------------------------------------------------------------------
     Open secure session for the secure commands
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL), status, remove_key);

    /*-------------------------------------------------------------------------------------------------------
     Secure erase
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Erase(qlibContext, section, 0, FLASH_SECTOR_SIZE, TRUE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Secure write
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Write(qlibContext, writeBuf, section, 0, sizeof(writeBuf), TRUE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Non-secure read is not allowed
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ALLOW_TO_FAIL__START();
    status = QLIB_Read(qlibContext, readBuf, section, 0, sizeof(readBuf), FALSE, FALSE);
    QLIB_ALLOW_TO_FAIL__END();
    QLIB_ASSERT_WITH_ERROR_GOTO(status == QLIB_STATUS__DEVICE_PRIVILEGE_ERR, QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Verify data
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(0 != memcmp(readBuf, writeBuf, sizeof(writeBuf)), QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Non-secure erase is not allowed
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ALLOW_TO_FAIL__START();
    status = QLIB_Erase(qlibContext, section, 0, FLASH_SECTOR_SIZE, FALSE);
    QLIB_ALLOW_TO_FAIL__END();
    QLIB_ASSERT_WITH_ERROR_GOTO(status == QLIB_STATUS__DEVICE_PRIVILEGE_ERR, QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Secure read
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Read(qlibContext, readBuf, section, 0, sizeof(readBuf), TRUE, FALSE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Verify data
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(0 == memcmp(readBuf, writeBuf, sizeof(writeBuf)), QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Note: user can verify section data with integrity check as well by using CALC_SIG command and comparing
     the digest with an expected digest value
    -------------------------------------------------------------------------------------------------------*/

    status = QLIB_STATUS__OK;

close_session:
    (void)QLIB_CloseSession(qlibContext, section);

remove_key:
    (void)QLIB_RemoveKey(qlibContext, section, TRUE);

    return status;
}

QLIB_STATUS_T QLIB_SAMPLE_SecureSectionRestrictedKey(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_T status              = QLIB_STATUS__OK;
    U32           section             = SECURE_DATA_SECTION_INDEX; //section (4) is pre-configured as fully secure section
    KEY_T         fullAccessKey       = QCONF_FULL_ACCESS_K_4;
    KEY_T         restrictedAccessKey = QCONF_RESTRICTED_K_4;
    U8            writeBuf[FLASH_PAGE_SIZE];
    U8            readBuf[FLASH_PAGE_SIZE];
    U8            eraseBuf[FLASH_PAGE_SIZE];
    BOOL          isFullAccess = TRUE;

    memset(readBuf, 0, sizeof(writeBuf));
    memset(writeBuf, 0xa5, sizeof(writeBuf));
    memset(eraseBuf, 0xFF, sizeof(eraseBuf));

    /*-------------------------------------------------------------------------------------------------------
     Set flash data at start of test with full key
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_LoadKey(qlibContext, section, fullAccessKey, TRUE));
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL), status, remove_key);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Erase(qlibContext, section, 0, FLASH_SECTOR_SIZE, TRUE), status, close_session);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Write(qlibContext, writeBuf, section, 0, sizeof(writeBuf), TRUE), status, close_session);
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_CloseSession(qlibContext, section), status, remove_key);
    QLIB_STATUS_RET_CHECK(QLIB_RemoveKey(qlibContext, section, TRUE));

    /*-------------------------------------------------------------------------------------------------------
     Load the restricted key for the secure read operations
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_LoadKey(qlibContext, section, restrictedAccessKey, FALSE));
    isFullAccess = FALSE;

    /*-------------------------------------------------------------------------------------------------------
     Open secure session for the secure commands
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_RESTRICTED), status, remove_key);

    /*-------------------------------------------------------------------------------------------------------
     Secure read
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Read(qlibContext, readBuf, section, 0, sizeof(readBuf), TRUE, FALSE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Verify data
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(0 == memcmp(readBuf, writeBuf, sizeof(writeBuf)), QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Secure erase is not allowed
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ALLOW_TO_FAIL__START();
    status = QLIB_Erase(qlibContext, section, 0, FLASH_SECTOR_SIZE, TRUE);
    QLIB_ALLOW_TO_FAIL__END();
    QLIB_ASSERT_WITH_ERROR_GOTO(status == QLIB_STATUS__DEVICE_PRIVILEGE_ERR, QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Secure read
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_Read(qlibContext, readBuf, section, 0, sizeof(readBuf), TRUE, FALSE), status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Verify data
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(0 == memcmp(readBuf, writeBuf, sizeof(writeBuf)), QLIB_STATUS__TEST_FAIL, status, close_session);

    /*-------------------------------------------------------------------------------------------------------
     Secure write is not allowed
    -------------------------------------------------------------------------------------------------------*/
    QLIB_ALLOW_TO_FAIL__START();
    status = QLIB_Write(qlibContext, writeBuf, section, 0, sizeof(writeBuf), TRUE);
    QLIB_ALLOW_TO_FAIL__END();
    QLIB_ASSERT_WITH_ERROR_GOTO(status == QLIB_STATUS__DEVICE_PRIVILEGE_ERR, QLIB_STATUS__TEST_FAIL, status, close_session);

    status = QLIB_STATUS__OK;

close_session:
    (void)QLIB_CloseSession(qlibContext, section);

remove_key:
    (void)QLIB_RemoveKey(qlibContext, section, isFullAccess);

    return status;
}
