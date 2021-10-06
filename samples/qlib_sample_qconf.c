/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_qconf.c
* @brief      This file contains QLIB QCONF related functions usage
*
* @example    qlib_sample_qconf.c
*
* @page       qconf sample code
* This sample code shows QCONF API usage
*
* @include    samples/qlib_sample_qconf.c
*
************************************************************************************************************/

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                                  INCLUDES
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/
#include "qlib.h"
#include "qlib_sample_qconf.h"

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                                  DEFINITIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

// FLASH START ADDRESS on the microcontroller
#ifndef FLASH_BASE_ADDR
#define FLASH_BASE_ADDR (0x0)
#endif

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                                  TYPES
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

#ifndef QCONF_NO_DIRECT_FLASH_ACCESS
static const
#endif //WINBOND_DEBUG
    QCONF_T qconf = {
        QCONF_MAGIC_WORD, // magicWord
#ifndef QCONF_NO_DIRECT_FLASH_ACCESS
        FLASH_BASE_ADDR, // flashBaseAddr
#endif                   // QCONF_NO_DIRECT_FLASH_ACCESS
        //otc:
        {
            QCONF_KD,  //deviceMasterKey
            QCONF_KDS, //deviceSecretKey
            // restrictedKeys:
            {
                QCONF_RESTRICTED_K_0,
                QCONF_RESTRICTED_K_1,
                QCONF_RESTRICTED_K_2,
                QCONF_RESTRICTED_K_3,
                QCONF_RESTRICTED_K_4,
                QCONF_RESTRICTED_K_5,
                QCONF_RESTRICTED_K_6,
                QCONF_RESTRICTED_K_7,
            },
            // fullAccessKeys:
            {
                QCONF_FULL_ACCESS_K_0,
                QCONF_FULL_ACCESS_K_1,
                QCONF_FULL_ACCESS_K_2,
                QCONF_FULL_ACCESS_K_3,
                QCONF_FULL_ACCESS_K_4,
                QCONF_FULL_ACCESS_K_5,
                QCONF_FULL_ACCESS_K_6,
                QCONF_FULL_ACCESS_K_7,
            },
            QCONF_SUID, //suid
        },

        // sectionTable:
        {// Section 0 is FW section - holds the boot code
         {
             BOOT_SECTION_BASE, //baseAddr
             BOOT_SECTION_SIZE, //size
             // Section policy structure QLIB_POLICY_T
             {
                 1, // digestIntegrity: If 1, the section is protected by digest integrity
             1, // checksumIntegrity: If 1, the section is protected by CRC integrity. Needed for secure boot feature
                 0, // writeProt: If 1, the section is write protected
                 1, // rollbackProt: If 1, the section is rollback protected. Needed for secure boot feature
                 1, // plainAccessWriteEnable: If 1, the section has plain write access
                 1, // plainAccessReadEnable: If 1, the section has plain read access
                 0, // authPlainAccess: If 1, using plain access requires to use @ref QLIB_OpenSession
             },
         },
         // Section 1: Plain Access section with write protection
         {
             PA_WRITE_PROTECT_SECTION_BASE, //baseAddr
             PA_WRITE_PROTECT_SECTION_SIZE, //size
             //policy
             {
                 1, //digestIntegrity
                 0, //checksumIntegrity
                 0, //writeProt
                 0, //rollbackProt
                 0, //plainAccessWriteEnable
                 1, //plainAccessReadEnable
                 0, //authPlainAccess
             },
         },
         // Section 2: calculate CDI section
         {
             DIGEST_WRITE_PROTECTED_SECTION_BASE, //baseAddr
             DIGEST_WRITE_PROTECTED_SECTION_SIZE, //size
             //policy
             {
                 1, //digestIntegrity
                 0, //checksumIntegrity
                 0, //writeProt
                 1, //rollbackProt
                 0, //plainAccessWriteEnable
                 1, //plainAccessReadEnable
                 0, //authPlainAccess
             },
         },
         // section 3: Rollback protected section
         {
             FW_UPDATE_SECTION_BASE, //baseAddr
             FW_UPDATE_SECTION_SIZE, //size
             //policy
             {
                 0, //digestIntegrity
                 0, //checksumIntegrity
                 0, //writeProt
                 1, //rollbackProt
                 0, //plainAccessWriteEnable
                 1, //plainAccessReadEnable
                 0, //authPlainAccess
             },
         },
         // section 4 : Secure data section
         {
             SECURE_DATA_SECTION_BASE, //baseAddr
             SECURE_DATA_SECTION_SIZE, //size
             //policy
             {
                 0, //digestIntegrity
                 0, //checksumIntegrity
                 0, //writeProt
                 0, //rollbackProt
                 0, //plainAccessWriteEnable
                 0, //plainAccessReadEnable
                 0, //authPlainAccess
             },
         },
         // section 5 : disabled
         {0},
         //EMPTY_SECTION_BASE, //baseAddr
         //EMPTY_SECTION_SIZE, //size
         //policy
         //         {
         //             0, //digestIntegrity
         //             0, //checksumIntegrity
         //             0, //writeProt
         //             0, //rollbackProt
         //             0, //plainAccessWriteEnable
         //             1, //plainAccessReadEnable
         //             0, //authPlainAccess
         //         },
         // section 6
         {
             FW_STORAGE_SECTION_BASE, //baseAddr
             FW_STORAGE_SECTION_SIZE, //size
             //policy
             {
                 0, //digestIntegrity
                 0, //checksumIntegrity
                 0, //writeProt
                 0, //rollbackProt
                 0, //plainAccessWriteEnable
                 1, //plainAccessReadEnable
                 0, //authPlainAccess
             },
         },
         // Section 7 is defined as a fallback section.
         // When section 0 (boot section) integrity fails, W77Q jumps to the fallback section (section 7). In such case, it is important to define the same access policy (write protection / plain access / plain read / auth plain access) both in section 0 and section 7.
         // This is part of the secure boot feature.
         {
             RECOVERY_SECTION_BASE, //baseAddr
             RECOVERY_SECTION_SIZE, //size
             //policy
             {
                 0, //digestIntegrity
                 0, //checksumIntegrity
                 0, //writeProt
                 0, //rollbackProt
                 1, //plainAccessWriteEnable
                 1, //plainAccessReadEnable
                 0, //authPlainAccess
             },
         }},
        //watchdogDefault
        {
            FALSE,                //enable
            TRUE,                 //lfOscEn
            FALSE,                //swResetEn
            FALSE,                //resetInDedicatedEn
            0,                    // sectionID
            QLIB_AWDT_TH_12_DAYS, //threshold
            FALSE,                //lock
            0,                    //oscRateHz - Set to 0 if non-applicable
        },

        //deviceConf
        {{{0}, {0}}, // Reset response is disabled. For more information please refer to the W77Q spec.
         TRUE,       // safeFB: if TRUE, When section 0 integrity fails, W77Q jumps to section 7. Needed for secure boot feature
         TRUE,       // Speculative Cypher Key Generation is disabled
         TRUE,       // nonSecureFormatEn: if TRUE, non-secure FORMAT command is accepted else,
                     // must use SFORMAT with device master key to formats the flash
         {
             QLIB_IO23_MODE__QUAD, // IO2/IO3 pin muxing
             TRUE,                 // dedicated RESET_IN pin enable
         },
#ifndef QLIB_SEC_ONLY
         {
             QLIB_STD_ADDR_LEN__24_BIT,
         }
#endif
        },

};

/*-----------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------------
                                             INTERFACE FUNCTIONS
-------------------------------------------------------------------------------------------------------------
-----------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_SAMPLE_QconfConfigRun(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_CONTEXT_T    qlibContextOnStack;
    QLIB_BUS_FORMAT_T busFormat = QLIB_BUS_FORMAT(QLIB_BUS_MODE_1_1_1, FALSE, FALSE);

    if (qlibContext == NULL)
    {
        qlibContext = &qlibContextOnStack;
    }

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
    QLIB_STATUS_RET_CHECK(QLIB_Disconnect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
     Flash will obtain secure configuration according to qconf structure
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_QconfConfig(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_QconfRecoveryRun(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_CONTEXT_T    qlibContextOnStack;
    QLIB_BUS_FORMAT_T busFormat = QLIB_BUS_FORMAT(QLIB_BUS_MODE_1_1_1, FALSE, FALSE);

#ifdef QLIB_SUPPORT_XIP
    /*-------------------------------------------------------------------------------------------------------
      recovery functionality is designed to run from RAM only since it formats the flash
    -------------------------------------------------------------------------------------------------------*/
    return QLIB_STATUS__NOT_SUPPORTED;
#endif

    if (qlibContext == NULL)
    {
        qlibContext = &qlibContextOnStack;
    }

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
    QLIB_STATUS_RET_CHECK(QLIB_Disconnect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
    Flash will obtain legacy configuration, otc contains keys needed for reconfiguration
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_QconfRecovery(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_QconfConfig(QLIB_CONTEXT_T* qlibContext)
{
    /*-------------------------------------------------------------------------------------------------------
    Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
    Flash will obtain secure configuration according to qconf structure
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QCONF_Config(qlibContext, &qconf));

    /*-------------------------------------------------------------------------------------------------------
    User FW should start here...
    -------------------------------------------------------------------------------------------------------*/

    /*-------------------------------------------------------------------------------------------------------
    Release the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Disconnect(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_QconfRecovery(QLIB_CONTEXT_T* qlibContext)
{
    /*-------------------------------------------------------------------------------------------------------
    Take the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Connect(qlibContext));

    /*-------------------------------------------------------------------------------------------------------
    Flash will obtain legacy configuration, otc contains keys needed for reconfiguration
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QCONF_Recovery(qlibContext, &qconf.otc));

    /*-------------------------------------------------------------------------------------------------------
    Release the ownership of flash communication channel (it belongs to local MCU or to remote server, exclusively)
    -------------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_Disconnect(qlibContext));

    return QLIB_STATUS__OK;
}
