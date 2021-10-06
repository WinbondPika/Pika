/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib.h
* @brief      This file contains QLIB main interface
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_H__
#define __QLIB_H__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_common.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            INTERFACE MACROS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This macro returns logical address from section ID and offset
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   offset        [Section offset](md_definitions.html#DEF_OFFSET)
 *
 * @return
 * logical address from section ID and offset
************************************************************************************************************/
#define QLIB_MAKE_LOGICAL_ADDRESS(qlibContext, sectionID, offset) \
    _QLIB_MAKE_LOGICAL_ADDRESS(sectionID, offset, (qlibContext)->addrSize)

/************************************************************************************************************
 * @brief       This macro performs flash read operation from given logical address
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  buf             Output buffer to read the data to
 * @param[in]   logicalAddr     Logical address.
 * @param[in]   size            [Size](md_definitions.html#DEF_SIZE)
 * @param[in]   secure          Perform secure read
 * @param[in]   auth            Perform authenticated read
 *
 * @return
 * 0 in no error occurred, or QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
#define QLIB_Read_LA(qlibContext, buf, logicalAddr, size, secure, auth)                 \
    QLIB_Read(qlibContext,                                                              \
              buf,                                                                      \
              _QLIB_SECTION_FROM_LOGICAL_ADDRESS(logicalAddr, (qlibContext)->addrSize), \
              _QLIB_OFFSET_FROM_LOGICAL_ADDRESS(logicalAddr, (qlibContext)->addrSize),  \
              size,                                                                     \
              secure,                                                                   \
              auth)

/************************************************************************************************************
 * @brief       This macro performs flash write operation to given logical address
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   buf             input buffer with the data to write
 * @param[in]   logicalAddr     Logical address
 * @param[in]   size            [Size](md_definitions.html#DEF_SIZE)
 * @param[in]   secure          Perform secure write
 *
 * @return
 * 0 in no error occurred, or QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
#define QLIB_Write_LA(qlibContext, buf, logicalAddr, size, secure)                       \
    QLIB_Write(qlibContext,                                                              \
               buf,                                                                      \
               _QLIB_SECTION_FROM_LOGICAL_ADDRESS(logicalAddr, (qlibContext)->addrSize), \
               _QLIB_OFFSET_FROM_LOGICAL_ADDRESS(logicalAddr, (qlibContext)->addrSize),  \
               size,                                                                     \
               secure)

/************************************************************************************************************
 * @brief       This macro performs flash erase operation from given logical address
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   logicalAddr     Logical address
 * @param[in]   size            [Size](md_definitions.html#DEF_SIZE)
 * @param[in]   secure          Perform secure erase
 *
 * @return
 * 0 in no error occurred, or QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
#define QLIB_Erase_LA(qlibContext, logicalAddr, size, secure)                            \
    QLIB_Erase(qlibContext,                                                              \
               _QLIB_SECTION_FROM_LOGICAL_ADDRESS(logicalAddr, (qlibContext)->addrSize), \
               _QLIB_OFFSET_FROM_LOGICAL_ADDRESS(logicalAddr, (qlibContext)->addrSize),  \
               size,                                                                     \
               secure)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This function performs QLIB library initialization.

 * The initialized state is contained in @p qlibContext.
 * This initialization must be performed before usage of other QLIB functions"
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0              - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER   - @p qlibContext is NULL\n
 * QLIB_STATUS__(ERROR)             - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_InitLib(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function initializes the communication with W77Q.
 *
 * This function must be called before other QLIB API functions who communicate with W77Q\n
 * This function:\n
 *  1. Resumes and completes suspended erase/write operations.\n
 *  2. If the flash is in power down mode, the power down mode is released.\n
 *  3. If Multi-die chip includes flash with size over 128Mb, switch these dies to 4 bytes address mode.\n
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   busFormat     Flash interface format, consists of bus mode and dtr.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__DEVICE_SYSTEM_ERR       - One of the sections has wrong configuration (e.g rollback and size is not correct)\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE  - @p busFormat is @ref QLIB_BUS_MODE_4_4_4 and QLIB_SUPPORT_QPI is not defined\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_InitDevice(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat);

/************************************************************************************************************
 * @brief       This function establishes a communication channel with the flash and sets the active die to 0
 *
 * This function must run before any QLIB API that performs communication with the flash.\n
 * Trying to perform operation without being connected will return error.\n
 * If already connected, QLIB_STATUS__DEVICE_BUSY is returned.
 *
 * @param[out]  qlibContext         [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__DEVICE_BUSY             - already connected, need to disconnect using @ref QLIB_Disconnect first\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Connect(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function closes the communication channel with the flash created by @ref QLIB_Connect and sets the active die to 0
 *
 * @param[out]  qlibContext         [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                      - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p qlibContext is NULL\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE   - can not disconnect while session is open, first close the session by calling @ref QLIB_CloseSession
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE   - not connected yet\n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Disconnect(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function loads an access key used for QLIB secure operations
 *
 * This function loads restricted/full access keys of a section into QLIB. This key will be used in secure
 * operations such as @ref QLIB_OpenSession function.\n
 * There are two types of keys that can be configured: 'restricted access key' and 'full access key'.
 * Each key has a different security properties. \n
 * The permissions configured are volatile and expire after flash reset or POR.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   key           Section key value. NULL or zero value key is not supported.
 * @param[in]   fullAccess    If TRUE, the given section key is full access key. Otherwise the
 *                            given key considered a restricted key
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p key is NULL or invalid (zero key)\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_LoadKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, const KEY_T key, BOOL fullAccess);

/************************************************************************************************************
 * @brief       This function removes the  key loaded with  @ref QLIB_LoadKey
 *
 * This function removes the loaded restricted/full access keys of a section from qlib.\n
 * This function can be used, if the required key is not used in current open session.\n
 * Secure operations, such as @ref QLIB_OpenSession won't work if the required key is removed.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   fullAccess    If TRUE, the given section key is full access key. Otherwise the
 *                            given key considered a restricted key
 *
 * @return
 * QLIB_STATUS__OK = 0                    - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p sectionID is invalid\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE - The key is used in currently open session and can't be removed
 * QLIB_STATUS__(ERROR)                   - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_RemoveKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL fullAccess);

/************************************************************************************************************
 * @brief       This function opens a secure session with a given section.
 *              Note that if section is configured as auth plain access, this function will also grant plain
 *              access to this section, like if @ref QLIB_AuthPlainAccess_Grant was used
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID       [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   sessionAccess   Session access type. Should be in range of enumeration QLIB_SESSION_ACCESS_T.
 *
 * @return
 * QLIB_STATUS__OK = 0                      - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER           - session access is illegal\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE   - session is already opened\n
 * QLIB_STATUS__DEVICE_ERR                  - Section is disabled\n
 * QLIB_STATUS__INVALID_PARAMETER           - the section key is not loaded. use @ref QLIB_LoadKey \n
 * QLIB_STATUS__DEVICE_AUTHENTICATION_ERR   - wrong section key is loaded\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE   - Session is opened with incorrect KID\n
 * QLIB_STATUS__DEVICE_INTEGRITY_ERR        - Section CRC integrity failed\n
 * QLIB_STATUS__NOT_CONNECTED               - Need to perform connect using @ref QLIB_Connect function \n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_OpenSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_SESSION_ACCESS_T sessionAccess);

/************************************************************************************************************
 * @brief       This routine closes the secure session opened by @ref QLIB_OpenSession.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
 * QLIB_STATUS__INVALID_PARAMETER           - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p sectionID is invalid\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE   - session is not opened\n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_CloseSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID);

/************************************************************************************************************
 * @brief       This function grants access to the authenticated plain access section
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID       [Section index](md_definitions.html#DEF_SECTION)
 *
 * @return
 * QLIB_STATUS__OK = 0                      - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER           - section not configured as authenticated plain read is illegal\n
 * QLIB_STATUS__DEVICE_ERR                  - Section is disabled\n
 * QLIB_STATUS__INVALID_PARAMETER           - the section key is not loaded. use @ref QLIB_LoadKey \n
 * QLIB_STATUS__DEVICE_AUTHENTICATION_ERR   - wrong section key is loaded\n
 * QLIB_STATUS__NOT_CONNECTED               - Need to perform connect using @ref QLIB_Connect function \n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_AuthPlainAccess_Grant(QLIB_CONTEXT_T* qlibContext, U32 sectionID);

/************************************************************************************************************
 * @brief       This routine revokes access to the section granted by @ref QLIB_AuthPlainAccess_Grant.
 *              If the section is open, the function closes it.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
 * QLIB_STATUS__INVALID_PARAMETER           - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER           - section not configured as authenticated plain read\n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_AuthPlainAccess_Revoke(QLIB_CONTEXT_T* qlibContext, U32 sectionID);

/************************************************************************************************************
 * @brief       This function enables plain access explicitly.
 *
 * This function is optional. The plain read/write/erase functions will execute this function implicitly
 * if plain access is not enabled. This function can be optionally used to time the `plain access enable`
 * event. This event can sometime take considerable amount of time due to integrity verification, if
 * the given section is integrity protected.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 *
 * @return:
 * QLIB_STATUS__INVALID_PARAMETER         - @p qlibContext is NULL\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE - Chip is not in working state\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE - Section is not enabled\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR      - Plain access is not enabled according to the section policy\n
 * QLIB_STATUS__(ERROR)                   - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_PlainAccessEnable(QLIB_CONTEXT_T* qlibContext, U32 sectionID);

/************************************************************************************************************
 * @brief       This function erase the flash device, including data of all sections, configurations and keys (optional).
 *
 * If @p deviceMasterKey is available then SFORMAT (Secure Format) command is used, followed by flash reset.
 * The SFORMAT command is used to securely erase the entire device, including data of all sections, configurations and keys.\n
 * If @p deviceMasterKey is NULL non-secure format (FORMAT) command is used, followed by flash reset.
 * In order to use non-secure format, FORMAT_EN must be set in DEVCFG register.\n
 * If @p eraseDataOnly is TRUE then SERASE (Secure Erase) command is used. The SERASE command is used to securely
 * erase the data of all sections, configurations and keys do not change.
 * If @p eraseDataOnly is TRUE then @p deviceMasterKey must me available.
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   deviceMasterKey Device Master Key value for secure format, or NULL for non-secure format
 * @param[in]   eraseDataOnly   If TRUE, only the data is erased, else erased also configurations and keys
 *
 * @return
 * QLIB_STATUS__OK = 0                      - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p eraseDataOnly == TRUE and @p deviceMasterKey == NULL\n
 * QLIB_STATUS__DEVICE_AUTHENTICATION_ERR   - @p deviceMasterKey is not correct and not NULL\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR        - @p deviceMasterKey == NULL and @p eraseOnlyData == FALSE in case that secure format should be executed (configured by DEVCFG.FORMAT_EN)\n
 * QLIB_STATUS__NOT_CONNECTED               - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Format(QLIB_CONTEXT_T* qlibContext, const KEY_T deviceMasterKey, BOOL eraseDataOnly);

/************************************************************************************************************
 * @brief       This function deploys new security configuration to the flash
 *
 * This function should be used on a clean formatted flash in case it configures the keys.\n
 * In order to re-configure the flash keys, it need to be formatted with @ref QLIB_Format function.\n
 * Section configuration includes section size. When the section size equals to zero, the section is disabled.
 * Section policy can be reconfigured using @ref QLIB_ConfigSection function.\n
 * The 'restrictedKeys' and 'fullAccessKeys' can be NULL or contain empty keys (zero value) elements in the
 * array. In such case all or the appropriate key locations will not be programmed to the chip,
 * and remain un-programmed. Note: When the keys are un-programmed all related security operations are
 * disabled
 * The function can receive optional 'suid' (Secure User ID) parameter, which can add
 * additional user-specific unique ID, in addition to Winbond WID. \n
 * Any section with PA enabled will stay enable after calling this function.
 * When trying to re-configure keys that are different from the previously configured keys for a specific section
 * and the section is disabled, no error will be returned.
 *
 * @param[out]  qlibContext         [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   deviceMasterKey     Device Master Key value. NULL or empty (zero) key is not supported.
 * @param[in]   deviceSecretKey     Device Secret Key value. The device secret key is used to configure the Root Of Trust feature. If NULL or zero, Device Secret value will not be programmed.
 * @param[in]   sectionTable        Section configuration table. See @ref QLIB_SECTION_CONFIG_TABLE_T for further information
 * @param[in]   restrictedKeys      Array of restricted keys for each section. If NULL, restricted keys will not be programmed.
 * @param[in]   fullAccessKeys      Array of full-access keys for each section. If NULL, full-access keys will not be programmed.
 * @param[in]   watchdogDefault     Watchdog and rest configurations.If NULL, GMC.AWDT_DFLT register will not be programmed.
 * @param[in]   deviceConf          General device configurations. If NULL, GMC.DEVCFG register will not be programmed.
 * @param[in]   suid                Secure User ID. If NULL, Secure Unique Device ID will not be programmed.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred \n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL \n
 * QLIB_STATUS__INVALID_PARAMETER       - @p deviceMasterKey is NULL or include zero value \n
 * QLIB_STATUS__INVALID_PARAMETER       - One of the section's address or size is wrong \n
 * QLIB_STATUS__INVALID_PARAMETER       - One of the section's policy includes plain access but the section (or part of it) can not be accessed with standard commands due to SECT_SEL configuration (When QLIB_SEC_ONLY is not defined)\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE  - One of the section exceeds the flash size \n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionTable has an enabled section (positive size) that has no valid key in the @p fullAccessKeys \n
 * QLIB_STATUS__DEVICE_SYSTEM_ERR       - One of the sections has wrong configuration e.g. rollback and size is not correct \n
 * QLIB_STATUS__NOT_CONNECTED           - Need to connect using @ref QLIB_Connect function \n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_ConfigDevice(QLIB_CONTEXT_T*                   qlibContext,
                                const KEY_T                       deviceMasterKey,
                                const KEY_T                       deviceSecretKey,
                                const QLIB_SECTION_CONFIG_TABLE_T sectionTable,
                                const KEY_ARRAY_T                 restrictedKeys,
                                const KEY_ARRAY_T                 fullAccessKeys,
                                const QLIB_WATCHDOG_CONF_T*       watchdogDefault,
                                const QLIB_DEVICE_CONF_T*         deviceConf,
                                const _128BIT                     suid);

/************************************************************************************************************
 * @brief       This function configures a section.
 *
 * This function configures one flash section. The section start address and size can't be changed.\n
 * The parameters @p policy, @p digest, @p crc, @p newVersion are optional.\n
 * This function requires that a session with full access is opened to the section which is configured see @ref QLIB_OpenSession
 *
 * Since section 7 is the fallback section for the boot section (section 0) in case of integrity failure, It is
 * important to define the same access policy in both sections 0 and 7 when fallback is enabled.
 *
 * Note that @ref QLIB_ConfigSection consists of two stages: configuring section and reopening the session again. \n
 * This is done since when section is configured, HW closes current session. In case that section is configured to authenticated plain read
 * access will be blocked till session is reopened. This means that if XIP is used and the code is located on w77q,
 * the session could not be reopened as the code itself is not accessible and thus, MCU fetch will fail. In such case ensure that
 * all @ref QLIB_ConfigSection code is located NOT on W77q but in RAM or another memory. Note that this includes also crypto primitives
 * which should run NOT from w77q. In other words if user wants to reconfigure flash he runs from, he must ensure that the code is always accessible for the MCU.
 *
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID       [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   policy          New section configuration policy. If NULL, the current section policy will not be changed.
 * @param[in]   digest          New section digest, or NULL if digest is not available
 * @param[in]   crc             New section CRC, or NULL if CRC is not available
 * @param[in]   newVersion      New section version value, or NULL if the version has not changed
 * @param[in]   swap            Swap configuration for rollback-protected sections. See @ref QLIB_SWAP_T
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p digest is not sent according to the section policy\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p crc is not sent according to the section policy\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p policy includes plain access but section size is beyond standard address maximal offset (When QLIB_SEC_ONLY is not defined)\n
 * QLIB_STATUS__INVALID_PARAMETER       - section is configured with checksumIntegrity and rollbackProt enabled\n
 * QLIB_STATUS__DEVICE_SESSION_ERR      - No open session\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR    - No open session with full privileges\n
 * QLIB_STATUS__DEVICE_INTEGRITY_ERR    - @p digest or @p crc are wrongly calculated\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_ConfigSection(QLIB_CONTEXT_T*      qlibContext,
                                 U32                  sectionID,
                                 const QLIB_POLICY_T* policy,
                                 const U64*           digest,
                                 const U32*           crc,
                                 const U32*           newVersion,
                                 QLIB_SWAP_T          swap);

/************************************************************************************************************
 * @brief       This function retrieve a section configuration by the section number @p sectionID.
 *
 * This function retrieves full or partial section configuration parameters. The output
 * parameters can be NULL if not required.
 * When the section size in @p size parameter is returned as zero, it means that the section is disabled and all the other parameters are unpredicted.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[out]  baseAddr      Section base address. If NULL, section base address is not returned.
 * @param[out]  size          Section size in bytes. If NULL, section size is not returned.
 * @param[out]  policy        Section configuration policy. If NULL, section policy is not returned.
 * @param[out]  digest        Section digest. If NULL, section digest is not returned.
 * @param[out]  crc           Section CRC. If NULL, section CRC is not returned.
 * @param[out]  version       Section version value. If NULL, section version is not returned.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER       - One of the section's address or size is wrong\n
 * QLIB_STATUS__DEVICE_SYSTEM_ERR       - One of the sections has wrong configuration e.g. rollback and size is not correct\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_GetSectionConfiguration(QLIB_CONTEXT_T* qlibContext,
                                           U32             sectionID,
                                           U32*            baseAddr,
                                           U32*            size,
                                           QLIB_POLICY_T*  policy,
                                           U64*            digest,
                                           U32*            crc,
                                           U32*            version);

/************************************************************************************************************
 * @brief       This function sets the flash SPI bus mode
 * *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   busFormat     Flash interface format, which consists of QLIB_BUS_MODE_T, DTR and ENTER_EXIT_QPI \n
 *                                  QLIB_BUS_MODE_T - SPI bus mode \n
 *                                  DTR             - if TRUE, DTR mode will be used \n
 *                                  ENTER_EXIT_QPI  - if TRUE, the function will set QPI mode (on/off). \n
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - if QLIB_SEC_ONLY is defined and QPI switch is required
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE  - Illegal @p format parameter\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE  - @p format parameter is @ref QLIB_BUS_MODE_4_4_4 and QLIB_SUPPORT_QPI is not defined\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_SetInterface(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat);

/************************************************************************************************************
 * @brief       This function configures volatile access permissions to a given section.
 *
 * This function configures the read and write permissions of a section in ACLR register.\n
 * The ACLR control section Read/Write permission on top of the section policy meaning it has higher priority.\n
 * The permissions configured are volatile and expire on flash reset or POR.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   readEnable    If TRUE, read permission is enabled for @p sectionID section
 * @param[in]   writeEnable   If TRUE, write permission is enabled for @p sectionID section
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionID is invalid\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_ConfigAccess(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL readEnable, BOOL writeEnable);

/************************************************************************************************************
 * @brief       This function reads data from the flash
 *
 * The data is read into @p buf. from offset in sectionId.\n
 * It is possible to select if the data is read in secure or standard (legacy) mode and in authenticated or
 * non-authenticated fashion.\n
 * If plain access is needed and it is not opened, it will be opened automatically by this routine.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  buf           The data read
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   offset        [Section offset](md_definitions.html#DEF_OFFSET)
 * @param[in]   size          [Size](md_definitions.html#DEF_SIZE)
 * @param[in]   secure        If TRUE then secure read, else standard read.
 * @param[in]   auth          If TRUE performs authenticated read.
 *
 * @return
 * QLIB_STATUS__OK = 0                    - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p buf is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p sectionID is invalid\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE    - @p Offset + size > maximal section offset, according to the SECT_SEL in case of standard read\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE    - @p size == 0 in case of standard read only\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE    - @p Offset + @p size > section size\n
 * QLIB_STATUS__DEVICE_SESSION_ERR        - No open session for secure access\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE - Chip is not in working state\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE - Section is not enabled\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR      - Section is defined without plain read access enabled in case of standard read\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR      - Section has no read access permission in ACLR register\n
 * QLIB_STATUS__NOT_CONNECTED             - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                   - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Read(QLIB_CONTEXT_T* qlibContext, U8* buf, U32 sectionID, U32 offset, U32 size, BOOL secure, BOOL auth);

/************************************************************************************************************
 * @brief       This function writes data to the flash
 *
 * The data is written from @p buf to offset in sectionId.\n
 * The data can be written in secure mode or standard modes.\n
 * If plain access is needed and it is not opened, it will be opened automatically by this routine.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   buf           The data to write
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   offset        [Section offset](md_definitions.html#DEF_OFFSET)
 * @param[in]   size          [Size](md_definitions.html#DEF_SIZE)
 * @param[in]   secure        If TRUE then secure write, else standard write.
 *
 * @return
 * QLIB_STATUS__OK = 0                                              - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER                                   - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER                                   - @p buf is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER                                   - @p sectionID is invalid\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE                              - @p Offset + size > maximal section offset, according to the SECT_SEL in case of standard write\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE                              - @p Offset + @p size > section size\n
 * QLIB_STATUS__DEVICE_SESSION_ERR                                  - No open session for secure access\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE                           - Chip is not in working state\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE                           - Section is not enabled\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR or QLIB_STATUS__DEVICE_ERR     - Section is defined without plain write access enabled in case of standard write\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR or QLIB_STATUS__DEVICE_ERR     - Section has no write access permission in ACLR register\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR or QLIB_STATUS__DEVICE_ERR     - Section is not write permitted\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR or QLIB_STATUS__DEVICE_ERR     - Section policy does not allow writing - the section is write protected\n
 * QLIB_STATUS__NOT_CONNECTED                                       - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                                             - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Write(QLIB_CONTEXT_T* qlibContext, const U8* buf, U32 sectionID, U32 offset, U32 size, BOOL secure);

/************************************************************************************************************
 * @brief       This function erases the given memory range.
 *
 * The minimal memory range size is defined by FLASH_SECTOR_SIZE.
 * When W77Q is configured as standard flash, in order to fully erase the flash call @ref QLIB_EraseSection with @p sectionID = 0.\n
 * If plain access is needed and it is not opened, it will be opened automatically by this routine.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   offset        [Section offset](md_definitions.html#DEF_OFFSET). The offset must be aligned to FLASH_SECTOR_SIZE.
 * @param[in]   size          [Size](md_definitions.html#DEF_SIZE). The size must be aligned to FLASH_SECTOR_SIZE.
 * @param[in]   secure        If TRUE then secure erase, else standard erase.
 *
 * @return
 * QLIB_STATUS__OK = 0                    - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p buf is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER         - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_DATA_ALIGNMENT    - @p offset is not aligned to sector size\n
 * QLIB_STATUS__INVALID_DATA_SIZE         - @p size is not aligned to sector size in case of secure erase\n
 * QLIB_STATUS__DEVICE_SESSION_ERR        - No open session for secure access, in case of secure erase\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR      - Section opened without full access key, in case of secure erase\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE - Chip is not in working state\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE - Section is not enabled\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE    - @p Offset/Offset + size > maximal section offset, according to the SECT_SEL in case of standard erase\n
 * QLIB_STATUS__PARAMETER_OUT_OF_RANGE    - @p Offset + @p size > section size\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR      - Section is defined without plain access write enabled in case of standard erase\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR      - Section has no write access permission in ACLR register\n
 * QLIB_STATUS__NOT_CONNECTED             - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                   - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Erase(QLIB_CONTEXT_T* qlibContext, U32 sectionID, U32 offset, U32 size, BOOL secure);

/************************************************************************************************************
 * @brief       This function erases a full flash section.
 *
 * In case of secure erase, an open session with full access key must be opened for the desired section.\n
 * A session can be opened using @ref QLIB_OpenSession() function
 * When W77Q is configured as standard flash, in order to fully erase the flash call @ref QLIB_EraseSection with @p sectionID = 0
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID     [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   secure        If TRUE then secure erase, else standard erase.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionID is invalid\n
 * QLIB_STATUS__DEVICE_SESSION_ERR      - No open session for secure access, in case of secure erase\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR    - No open session for plain access, in case of plain erase\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR    - Section opened without full access key, in case of secure erase\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR    - Section is defined without plain access write enabled in case of standard erase\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR    - Section has no write access permission in ACLR register\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_EraseSection(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL secure);

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * @brief       This function suspends an ongoing erase or write operations.
 *
 * Only standard erase operation or standard write operations can be suspended.\n
 * Once standard erase is suspended the user can perform read operation from all the flash except for the section which was erased.\n
 * Once standard write is suspended the user can perform read/write operation from all the flash except for the address which was written.\n
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__COMMAND_IGNORED         - No standard erase/write in progress
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Suspend(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function resumes the operation, suspended by @ref QLIB_Suspend
 *
 * Only standard erase operation or standard write operations can be suspended and thus resumed.\n
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Resume(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function controls flash power states
 *
 * Power down can be used before entering sleep mode to save power. During power down all flash commands are
 * ignored. The only command that works is power-up command.\n
 * Power up can be used upon exiting sleep mode.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   power         Power up / Power down. power value should be in range of enumeration QLIB_POWER_T.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Power(QLIB_CONTEXT_T* qlibContext, QLIB_POWER_T power);
#endif // QLIB_SEC_ONLY

/************************************************************************************************************
 * @brief       This function resets the flash.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_ResetFlash(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function check if there are notifications from the device
 *
 * This function should be used to check whether QLIB notifications are available.\n
 * Such notifications might suggest there are actions to be executed on the device.\n
 * @ref QLIB_PerformMaintenance should be called if mcMaintenance bit is set to 1 in reply.\n
 * Device reset is required if resetDevice bit is set to 1 in reply, as TC is close to its maximal value. (see also @ref QLIB_SEC_DMC_EOL_THRESHOLD)\n
 * If replaceDevice bit is set, the device is close to its End of Life and user should replace it. (see also @ref QLIB_SEC_TC_RESET_THRESHOLD)\n
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  notifs        Bitmap of active notifications.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p notifs is NULL\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_GetNotifications(QLIB_CONTEXT_T* qlibContext, QLIB_NOTIFICATIONS_T* notifs);

/************************************************************************************************************
 * @brief       This function perform QLIB maintenance.
 *
 * In order to speed-up the time-critical boot sequence and session open process, QLIB maintenance is
 * performed in iterations.\n
 * It is recommended to call @ref QLIB_PerformMaintenance when the SW is in idle state.\n
 * It is recommended to call @ref QLIB_PerformMaintenance only when maintenance is needed
 * (@ref QLIB_GetNotifications returns mcMaintenance bit 1).\n
 * Doing maintenance in idle time can save process time.\n
 * Recommended flow:
 * @code{.c}
       QLIB_NOTIFICATIONS_T notifications;
       QLIB_STATUS_RET_CHECK(QLIB_GetNotifications(qlibContext, &notifications));
       while (1 == notifications.mcMaintenance)
       {
           QLIB_STATUS_RET_CHECK(QLIB_PerformMaintenance(qlibContext));
           QLIB_STATUS_RET_CHECK(QLIB_GetNotifications(qlibContext, &notifications));
       }
 * @endcode
 *
 * @param[out]  qlibContext [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_PerformMaintenance(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function forces an integrity check on a given section
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   sectionID       [Section index](md_definitions.html#DEF_SECTION)
 * @param[in]   integrityType   The integrity type to perform. Integrity type should be in range of enumeration QLIB_INTEGRITY_T.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p integrityType is invalid\n
 * QLIB_STATUS__DEVICE_SESSION_ERR      - Session is closed. Need to open session using @ref QLIB_OpenSession
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__DEVICE_INTEGRITY_ERR    - The @p sectionID CRC is incorrect\n
 * QLIB_STATUS__SECURITY_ERR            - The @p sectionId digest is incorrect\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_CheckIntegrity(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_INTEGRITY_T integrityType);

/************************************************************************************************************
 * @brief       This function calculates the CDI value.
 *
 * This function can be executed only if no section were accessed or enabled beside section 0 after the last
 * POR. Note that section id the CDI is associated with has to be have policy with digest integrity
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  nextCdi         CDI value produced by this module
 * @param[in]   prevCdi         CDI value obtained from previous module
 * @param[in]   sectionId       section number CDI associated with
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p sectionID is invalid\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p section's policy has no digest integrity\n
 * QLIB_STATUS__DEVICE_SESSION_ERR      - Session is closed. Need to open session using @ref QLIB_OpenSession
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_CalcCDI(QLIB_CONTEXT_T* qlibContext, _256BIT nextCdi, _256BIT prevCdi, U32 sectionId);

/************************************************************************************************************
 * @brief       This function configures the Secure Watchdog.
 *
 * The key used for the Watchdog functionality, is the key used in the current open session.\n
 * To use this function the session must be open with full-access key
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   watchdogCFG   Watchdog and rest configurations. NULL is not supported.
 *
 * @return
 * QLIB_STATUS__OK = 0                      - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER           - @p watchdogCFG is NULL\n
 * QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE   - AWDTCFG register is locked\n
 * QLIB_STATUS__DEVICE_SESSION_ERR          - Watchdog is configured as secured and session is not opened\n
 * QLIB_STATUS__DEVICE_PRIVILEGE_ERR        - Watchdog is configured as secured and full privileges session is not opened\n\n
 * QLIB_STATUS__NOT_CONNECTED               - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Watchdog_ConfigSet(QLIB_CONTEXT_T* qlibContext, const QLIB_WATCHDOG_CONF_T* watchdogCFG);

/************************************************************************************************************
 * @brief       This function read the Secure Watchdog configuration.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  watchdogCFG   Watchdog and rest configurations. NULL is not supported.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Watchdog_ConfigGet(QLIB_CONTEXT_T* qlibContext, QLIB_WATCHDOG_CONF_T* watchdogCFG);

/************************************************************************************************************
 * @brief       This function touches (resets) the Secure Watchdog.
 *
 * A session must be open with appropriate key to allow successful execution of this function.\n
 * They key should have same 'sectionID' as the key used for watchdog configuration.\n
 * It can be either full-access or restricted key.
 *
 * @param[out]  qlibContext         [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__DEVICE_SESSION_ERR      - Watchdog is configured as secured and session is not opened\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Watchdog_Touch(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function triggers (force expire) the Secure Watchdog.
 *
 * @param[out]  qlibContext         [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Watchdog_Trigger(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function returns the current status of the Secure watchdog timer.
 *
 * @p secondsPassed, @p ticsResidue and @p expired parameters can be NULL and thus the function does not return them.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  secondsPassed The current value of the AWD timer (in seconds). If NULL, time is not returned.
 * @param[out]  ticsResidue   The residue of the AWD timer (in units of 64 tics of LF_OSC). If NULL, time is not returned.
 * @param[out]  expired       TRUE if AWD timer has expired. If NULL, expired indication is not returned.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_Watchdog_Get(QLIB_CONTEXT_T* qlibContext, U32* secondsPassed, U32* ticsResidue, BOOL* expired);

/************************************************************************************************************
 * @brief       This function return the flash IDs
 *
 * Flash contain several ID values described in @ref QLIB_ID_T
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  id_p          Pointer to struct contains all the unique IDs types of flash. NULL is not supported.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p id_p is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_GetId(QLIB_CONTEXT_T* qlibContext, QLIB_ID_T* id_p);

/************************************************************************************************************
 * @brief       This function return the flash hardware versions
 *
 * Flash hardware information is described in @ref QLIB_HW_VER_T
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  hwVer         Pointer to struct contains all the flash hardware version info. NULL is not supported.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p id_p is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_GetHWVersion(QLIB_CONTEXT_T* qlibContext, QLIB_HW_VER_T* hwVer);

/************************************************************************************************************
 * @brief       This function reads errors from the SSR (Secure Status Register).
 *
 * If error in SSR is found it is returned else QLIB_STATUS__OK is returned.
 *
 * @param[out]  qlibContext         [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__NOT_CONNECTED           - Need to perform connect using @ref QLIB_Connect function\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_GetStatus(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function sets a user specific data associated with @p qlibContext
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   userData      Pointer to data associated with this qlib instance. NULL is not supported.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
void QLIB_SetUserData(QLIB_CONTEXT_T* qlibContext, void* userData);

/************************************************************************************************************
 * @brief       This function return the user specific data set by @ref QLIB_SetUserData
 *
 * @param[out]  qlibContext         [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
void* QLIB_GetUserData(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function generates a 'synchronization object' to synchronize between remote QLIB and TM layers
 *
 * This function generates a 'synchronization object' that is used to synchronize states between remote
 * QLIB and TM layers.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[out]  syncObject    Pointer to synchronization object to be filled with sync data. NULL is not supported.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p syncObject is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_ExportState(QLIB_CONTEXT_T* qlibContext, QLIB_SYNC_OBJ_T* syncObject);

/************************************************************************************************************
 * @brief       This function applies 'synchronization object' and updates the QLIB state.
 *
 * This function is used to apply the 'synchronization object' generated by @ref QLIB_ExportState function.\n
 * It is used in scenario when QLIB and TM layers run on different platforms.
 *
 * @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 * @param[in]   syncObject    Pointer to synchronization object. NULL is not supported.
 *
 * @return
 * QLIB_STATUS__OK = 0                  - no error occurred\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
 * QLIB_STATUS__INVALID_PARAMETER       - @p syncObject is NULL\n
 * QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_ImportState(QLIB_CONTEXT_T* qlibContext, const QLIB_SYNC_OBJ_T* syncObject);

/************************************************************************************************************
 * @brief       This function returns QLIB version
 *
 * @return      U32 represents QLIB version in the format MAJOR.MINOR.PATCH.INTERNAL
 *              For example version 1.2.3.4 is returned as 0x01020304
************************************************************************************************************/
U32 QLIB_GetVersion(void);



/************************************************************************************************************
* @brief       This function returns the reset status
*
* @param[out]  qlibContext   [QLIB internal state](md_definitions.html#DEF_CONTEXT)
* @param[out]  resetStatus   Reset status

* @return
* QLIB_STATUS__OK = 0                  - no error occurred\n
* QLIB_STATUS__INVALID_PARAMETER       - @p qlibContext is NULL\n
* QLIB_STATUS__INVALID_PARAMETER       - @p resetStatus is NULL\n
* QLIB_STATUS__(ERROR)                 - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_GetResetStatus(QLIB_CONTEXT_T* qlibContext, QLIB_RESET_STATUS_T* resetStatus);

#ifdef __cplusplus
}
#endif

#else
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERNAL RE-INCLUDE                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_defs.h"
#include "qlib_errors.h"
#endif // __QLIB_H__
