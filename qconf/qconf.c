/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qconf.c
* @brief      This file contains (QCONF)[qconf.html] code
*
* ### project qlib
*
-----------------------------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                                  INCLUDES
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
#include "qconf.h"
#include "qlib_utils_crc.h"

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                         LOCAL FUNCTION DECLARATIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
static QLIB_STATUS_T QCONF_UpdateSectionIntegrity_L(QLIB_CONTEXT_T*              qlibContext,
                                                    U64                          flashBaseAddr,
                                                    const QLIB_SECTION_CONFIG_T* sectionConfig,
                                                    U32                          section);

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                             INTERFACE FUNCTIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QCONF_Config(QLIB_CONTEXT_T* qlibContext, const QCONF_T* configHeader)
{
    QLIB_STATUS_T               ret;
    QCONF_T                     ramConfigHeader;
    U32                         section;
    QLIB_SECTION_CONFIG_TABLE_T initSectionTable;

    /*-------------------------------------------------------------------------------------------------------
     Verify the header
    -------------------------------------------------------------------------------------------------------*/
    if (QCONF_MAGIC_WORD == configHeader->magicWord)
    {
        QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "Prepare flash configuration at the first time\r\n");
    }
    else
    {
        QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "flash is already configured\r\n");
        return QLIB_STATUS__OK;
    }

    /*-------------------------------------------------------------------------------------------------------
     Read the flash configuration header and save in RAM
    -------------------------------------------------------------------------------------------------------*/
    memcpy(&ramConfigHeader, configHeader, sizeof(QCONF_T));

    /*-------------------------------------------------------------------------------------------------------
     Variables initialization
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
     Load all keys
    -------------------------------------------------------------------------------------------------------*/
    for (section = 0; section < QLIB_NUM_OF_SECTIONS; section++)
    {
        if (0 != ramConfigHeader.sectionTable[section].size)
        {
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_LoadKey(qlibContext, section, ramConfigHeader.otc.fullAccessKeys[section], TRUE),
                                       ret,
                                       exit);
        }
    }

#ifndef QCONF_NO_DIRECT_FLASH_ACCESS
    /*-------------------------------------------------------------------------------------------------------
     Wipe out all the keys and the magic number in the header. This makes the header sealed.
    -------------------------------------------------------------------------------------------------------*/
    QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "Wipe keys from flash configuration header");
    {
        QCONF_T wipedConfigurations = {0};
        U32     headerOffsetInFlash;

        QLIB_ASSERT_RET(MAX_U32 > (((UPTR)configHeader) - ramConfigHeader.flashBaseAddr), QLIB_STATUS__INVALID_PARAMETER);
        headerOffsetInFlash = (U32)(((UPTR)configHeader) - ramConfigHeader.flashBaseAddr);

        QLIB_STATUS_RET_CHECK_GOTO(QLIB_Write(qlibContext,
                                              (U8*)&wipedConfigurations,
                                              0,
                                              headerOffsetInFlash,
                                              sizeof(QCONF_T),
                                              FALSE),
                                   ret,
                                   exit);

        /*---------------------------------------------------------------------------------------------------
         Verify that the keys were removed (might fail if cache is enabled)
        ---------------------------------------------------------------------------------------------------*/
        if (0 != memcmp(&wipedConfigurations, configHeader, sizeof(QCONF_T)))
        {
            QLIB_DEBUG_PRINT(QLIB_VERBOSE_FATAL, "Qconf wipe keys from header fail");
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_STATUS__COMMAND_FAIL, ret, exit);
        }
    }
#endif // QCONF_NO_DIRECT_FLASH_ACCESS

    /*-------------------------------------------------------------------------------------------------------
     Write the configuration to the flash device
    -------------------------------------------------------------------------------------------------------*/
    QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "Write new configurations");

    /*-------------------------------------------------------------------------------------------------------
     Initial section configuration is without integrity checks
    -------------------------------------------------------------------------------------------------------*/
    memcpy(initSectionTable, ramConfigHeader.sectionTable, sizeof(QLIB_SECTION_CONFIG_TABLE_T));
    for (section = 0; section < QLIB_NUM_OF_SECTIONS; section++)
    {
        initSectionTable[section].policy.digestIntegrity   = 0;
        initSectionTable[section].policy.checksumIntegrity = 0;
    }
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_ConfigDevice(qlibContext,
                                                 ramConfigHeader.otc.deviceMasterKey,
                                                 ramConfigHeader.otc.deviceSecretKey,
                                                 initSectionTable,
                                                 (const KEY_T*)ramConfigHeader.otc.restrictedKeys,
                                                 (const KEY_T*)ramConfigHeader.otc.fullAccessKeys,
                                                 &(ramConfigHeader.watchdogDefault),
                                                 &(ramConfigHeader.deviceConf),
                                                 ramConfigHeader.otc.suid),
                               ret,
                               exit);

    /*-------------------------------------------------------------------------------------------------------
     Re-configure sections with integrity checks
    -------------------------------------------------------------------------------------------------------*/
    for (section = 0; section < QLIB_NUM_OF_SECTIONS; section++)
    {
#ifdef QCONF_NO_DIRECT_FLASH_ACCESS
        QLIB_STATUS_RET_CHECK_GOTO(QCONF_UpdateSectionIntegrity_L(qlibContext,
                                                                  0,
                                                                  &(ramConfigHeader.sectionTable[section]),
                                                                  section),
                                   ret,
                                   exit);
#else
        QLIB_STATUS_RET_CHECK_GOTO(QCONF_UpdateSectionIntegrity_L(qlibContext,
                                                                  ramConfigHeader.flashBaseAddr,
                                                                  &(ramConfigHeader.sectionTable[section]),
                                                                  section),
                                   ret,
                                   exit);
#endif
    }

    QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "Device configurations has finish\r\n");

exit:
    /*-------------------------------------------------------------------------------------------------------
     Close the session on error
    -------------------------------------------------------------------------------------------------------*/
    if (QLIB_STATUS__OK != ret)
    {
        (void)QLIB_CloseSession(qlibContext, section);
    }

    /*-------------------------------------------------------------------------------------------------------
     Remove all keys
    -------------------------------------------------------------------------------------------------------*/
    memset(&ramConfigHeader, 0x0, sizeof(QCONF_T));
    for (section = 0; section < QLIB_NUM_OF_SECTIONS; section++)
    {
        (void)QLIB_RemoveKey(qlibContext, section, TRUE);
    }

    return ret;
}

QLIB_STATUS_T QCONF_Recovery(QLIB_CONTEXT_T* qlibContext, const QCONF_OTC_T* otc)
{
    QCONF_OTC_T ramOtc;
    GMC_T       GMC;
    BOOL        nonSecureFormatEn;

    // map section 0 for legacy access to all the flash size
    const QLIB_SECTION_CONFIG_TABLE_T sectionTable = {
        {
            // Section 0
            0,                   // baseAddr
            QLIB_SEC_FLASH_SIZE, // size
            {
                // policy
                0, // digestIntegrity
                0, // checksumIntegrity
                0, // writeProt
                0, // rollbackProt
                1, // plainAccessWriteEnable
                1, // plainAccessReadEnable
                0, // authPlainAccess
            },
        },
        {0}, // Section 1
        {0}, // Section 2
        {0}, // Section 3
        {0}, // Section 4
        {0}, // Section 5
        {0}, // Section 6
        {0}, // Section 7
    };

    QLIB_WATCHDOG_CONF_T watchdogDefault = {
        FALSE,                // enable
        FALSE,                // lfOscEn
        FALSE,                // swResetEn
        FALSE,                // authenticated
        0,                    // sectionID
        QLIB_AWDT_TH_12_DAYS, // threshold
        FALSE,                // lock
        0,                    // oscRateHz
    };

    QLIB_DEVICE_CONF_T deviceConf = {{{0}, {0}}, // resetResp
                                     FALSE,      // safeFB
                                     FALSE,      // speculCK
                                     TRUE,       // nonSecureFormatEn
                                     {
                                         QLIB_IO23_MODE__QUAD, // IO2 and IO3 mux
                                         TRUE,                 // Dedicated reset-in mux
                                     },
#ifndef QLIB_SEC_ONLY
                                     {
#if LOG2(QLIB_SEC_FLASH_SIZE) > 22
                                         QLIB_STD_ADDR_LEN__27_BIT,
#else
                                         QLIB_STD_ADDR_LEN__25_BIT,
#endif
                                     }
#endif
    };

    /*-------------------------------------------------------------------------------------------------------
     make a local copy of the OTC (in case that configurations are in flash)
    -------------------------------------------------------------------------------------------------------*/
    memcpy(&ramOtc, otc, sizeof(QCONF_OTC_T));

    /*-------------------------------------------------------------------------------------------------------
     Ensure device can be formatted and configure if not
     ------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_GMC(qlibContext, GMC));
    nonSecureFormatEn = READ_VAR_FIELD(QLIB_REG_GMC_GET_DEVCFG(GMC), QLIB_REG_DEVCFG__FORMAT_EN) ? TRUE : FALSE;
    if (FALSE == nonSecureFormatEn)
    {
        QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "configuring device for formatting (QLIB_REG_DEVCFG__FORMAT_EN=1)\r\n");
        QLIB_STATUS_RET_CHECK(
            QLIB_ConfigDevice(qlibContext, ramOtc.deviceMasterKey, NULL, NULL, NULL, NULL, NULL, &(deviceConf), NULL));
    }

    /*-------------------------------------------------------------------------------------------------------
     Format the flash - clean from all it's data and configurations
    -------------------------------------------------------------------------------------------------------*/
    QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "perform device format\r\n");
    QLIB_STATUS_RET_CHECK(QLIB_Format(qlibContext, NULL, FALSE));

    /*-------------------------------------------------------------------------------------------------------
     Write the configuration to the flash device
    -------------------------------------------------------------------------------------------------------*/
    QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "Write new configurations");
    QLIB_STATUS_RET_CHECK(QLIB_ConfigDevice(qlibContext,
                                            ramOtc.deviceMasterKey,
                                            ramOtc.deviceSecretKey,
                                            sectionTable,
                                            (const KEY_T*)ramOtc.restrictedKeys,
                                            (const KEY_T*)ramOtc.fullAccessKeys,
                                            &(watchdogDefault),
                                            &(deviceConf),
                                            ramOtc.suid));


    return QLIB_STATUS__OK;
}


/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                               LOCAL FUNCTIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine check if the section is configured with integrity check,
 *              and if so calculate the digest & crc values and configure the flash accordingly
 *
 * @param[out] qlibContext    [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]  flashBaseAddr  The flash base address
 * @param[in]  sectionConfig  Pointer to the section configuration struct
 * @param[in]  section        The section to be updated
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
static QLIB_STATUS_T QCONF_UpdateSectionIntegrity_L(QLIB_CONTEXT_T*              qlibContext,
                                                    U64                          flashBaseAddr,
                                                    const QLIB_SECTION_CONFIG_T* sectionConfig,
                                                    U32                          section)
{
    QLIB_STATUS_T ret = QLIB_STATUS__OK;

    if (0 != sectionConfig->size && (sectionConfig->policy.checksumIntegrity || sectionConfig->policy.digestIntegrity))
    {
        U64  digest      = {0};
        U32  crc         = {0};
        U64* digest_p    = NULL;
        U32* crc_p       = NULL;
        U32  sectionSize = sectionConfig->policy.rollbackProt ? sectionConfig->size / 2 : sectionConfig->size;

        QLIB_STATUS_RET_CHECK(QLIB_OpenSession(qlibContext, section, QLIB_SESSION_ACCESS_FULL));

        if (sectionConfig->policy.checksumIntegrity)
        {
            QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "Calculate CRC for section %d", section);
            crc_p = &crc;

#if !defined QCONF_NO_DIRECT_FLASH_ACCESS && !defined(Q2_SIMULATOR)
            if (sectionConfig->policy.plainAccessReadEnable)
            {
                UPTR sectionLegacyAddr;
                sectionLegacyAddr = (UPTR)(flashBaseAddr + QLIB_MAKE_LOGICAL_ADDRESS(qlibContext, section, 0));
                QLIB_STATUS_RET_CHECK_GOTO(QLIB_UTILS_CalcCRCWithPadding((U32*)sectionLegacyAddr, sectionSize, 0, 0, crc_p),
                                           ret,
                                           close_session);
            }
            else
#endif
            {
                QLIB_STATUS_RET_CHECK_GOTO(QLIB_UTILS_CalcCRCForSection(qlibContext, section, 0, sectionSize, crc_p),
                                           ret,
                                           close_session);
            }
        }

        if (sectionConfig->policy.digestIntegrity)
        {
            QLIB_DEBUG_PRINT(QLIB_VERBOSE_INFO, "Calculate digest for section %d", section);
            digest_p = &digest;
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__CALC_SIG(qlibContext,
                                                               QLIB_SIGNED_DATA_TYPE_SECTION_DIGEST,
                                                               section,
                                                               (U32*)digest_p,
                                                               sizeof(digest),
                                                               NULL),
                                       ret,
                                       close_session);
        }
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_ConfigSection(qlibContext,
                                                      section,
                                                      &sectionConfig->policy,
                                                      digest_p,
                                                      crc_p,
                                                      NULL,
                                                      QLIB_SWAP_NO),
                                   ret,
                                   close_session);

        QLIB_STATUS_RET_CHECK(QLIB_CloseSession(qlibContext, section));
    }

    return QLIB_STATUS__OK;

close_session:
    QLIB_STATUS_RET_CHECK(QLIB_CloseSession(qlibContext, section));
    return ret;
}

