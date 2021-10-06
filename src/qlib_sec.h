/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sec.h
* @brief      This file contains security features definitions
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_SEC_H__
#define __QLIB_SEC_H__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 DEFINES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define QLIB_EXECUTE_SIGNED_GET(contextP)                                                             \
    ((QLIB_KID__RESTRICTED_ACCESS_SECTION == QLIB_KEY_MNGR__GET_KEY_TYPE((contextP)->keyMngr.kid)) || \
     (QLIB_KID__FULL_ACCESS_SECTION == QLIB_KEY_MNGR__GET_KEY_TYPE((contextP)->keyMngr.kid)))

#define QLIB_SEC__get_GMT(contextP, gmt)                                                  \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_GMT_SIGNED((contextP), (gmt)) \
                                       : QLIB_CMD_PROC__get_GMT_UNSIGNED((contextP), (gmt)))

#define QLIB_SEC__get_SCRn(contextP, sec, scrn)                                                    \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_SCRn_SIGNED((contextP), (sec), (scrn)) \
                                       : QLIB_CMD_PROC__get_SCRn_UNSIGNED((contextP), (sec), (scrn)))

#define QLIB_SEC__get_SUID(contextP, suid)                                                  \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_SUID_SIGNED((contextP), (suid)) \
                                       : QLIB_CMD_PROC__get_SUID_UNSIGNED((contextP), (suid)))

#define QLIB_SEC__get_GMC(contextP, gmc)                                                  \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_GMC_SIGNED((contextP), (gmc)) \
                                       : QLIB_CMD_PROC__get_GMC_UNSIGNED((contextP), (gmc)))

#define QLIB_SEC__get_HW_VER(contextP, hwVer)                                                  \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_HW_VER_SIGNED((contextP), (hwVer)) \
                                       : QLIB_CMD_PROC__get_HW_VER_UNSIGNED((contextP), (hwVer)))

#define QLIB_SEC__get_SSR(contextP, ssr, mask)                                                    \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_SSR_SIGNED((contextP), (ssr), (mask)) \
                                       : QLIB_CMD_PROC__get_SSR_UNSIGNED((contextP), (ssr), (mask)))

#define QLIB_SEC__get_MC(contextP, mc)                                                  \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_MC_SIGNED((contextP), (mc)) \
                                       : QLIB_CMD_PROC__get_MC_UNSIGNED((contextP), (mc)))

#define QLIB_SEC__get_WID(contextP, wid)                                                  \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_WID_SIGNED((contextP), (wid)) \
                                       : QLIB_CMD_PROC__get_WID_UNSIGNED((contextP), (wid)))

#define QLIB_SEC__get_AWDTCNFG(contextP, awdtCnfg)                                              \
    (QLIB_EXECUTE_SIGNED_GET(contextP) ? QLIB_CMD_PROC__get_AWDT_SIGNED((contextP), (awdtCnfg)) \
                                       : QLIB_CMD_PROC__get_AWDT_UNSIGNED((contextP), (awdtCnfg)))

/*---------------------------------------------------------------------------------------------------------*/
/* Every time Transaction counter is USED it is incremented. Should be used once for transaction           */
/* Check TC < FFFFFFFFh. Otherwise, the device should be reset before any secure function can be used.     */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_TRANSACTION_CNTR_USE(context_p) \
    (context_p)->mc[TC]++;                   \
    QLIB_ASSERT_RET((context_p)->mc[TC] != 0, QLIB_STATUS__DEVICE_MC_ERR)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This function initiate the globals of the secure module.
 *              This function should be called once at the beginning of every host (local or remote)
 *
 * @param[in,out]   qlibContext   qlib context object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_InitLib(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function synchronize the Qlib state with the flash secure module state,
 *              both Qlib and the flash state may change during this function
 *
 * @param[in,out]   qlibContext   qlib context object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_SyncState(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function formats the flash device, including keys and sections configurations.
 *              If deviceMasterKey is available then secure format (SFORMAT) command is used,
 *              otherwise, if deviceMasterKey is NULL, non-secure format (FORMAT) command is used
 *              In order to use non-secure format, FORMAT_EN must be set in DEVCFG.
 *
 * @param[in,out]   qlibContext      qlib context object
 * @param[in]       deviceMasterKey  Device Master Key value, or NULL for non-secure format
 * @param[in]       eraseDataOnly    If TRUE, only the data is erased, else also all the configurations will be erased
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Format(QLIB_CONTEXT_T* qlibContext, const KEY_T deviceMasterKey, BOOL eraseDataOnly);

/************************************************************************************************************
 * @brief       This function sets the opcodes for secure operations
 *
 * @param       qlibContext   QLIB state object
  *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_configOPS(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function check if there are notifications from the device.
 *
 * @param[in,out]   qlibContext   qlib context object
 * @param[out]      notifs        Bitmap of notifications. A bit is set to 1 if the notification exists.
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_GetNotifications(QLIB_CONTEXT_T* qlibContext, QLIB_NOTIFICATIONS_T* notifs);

/************************************************************************************************************
 * @brief       This function perform one monotonic counter maintenance iteration.
 *              To speed-up the time-critical boot sequence and Session Open process,
 *              it is recommended to call this function when the SW is in idle while
 *              \ref QLIB_GetNotifications returns mcMaintenance value f 1.
 *              This function is part of the Qlib maintenance flow perform by \ref QLIB_PerformMaintenance
 *
 * @param[in,out]   qlibContext   qlib context object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_PerformMCMaint(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function re-configures the flash. It assume clean/formatted flash if keys are provided.
 *              The 'restrictedKeys' and 'fullAccessKeys' can be NULL or contain invalid (zero) key elements
 *              in the array. In such case all or the appropriate key locations will not be programmed to
 *              the chip, and remain un-programmed. Note: un-programmed keys have security implications.
 *              The function can receive optional 'suid' (Secure User ID) parameter, which can add
 *              additional user-specific unique ID, in addition to Winbond WID.
 *
 * @param[in,out]   qlibContext         QLIB state object
 * @param[in]       deviceMasterKey     Device Master Key value
 * @param[in]       deviceSecretKey     Device Secret Key value
 * @param[in]       sectionTable        Section configuration table
 * @param[in]       restrictedKeys      Array of restricted keys for each section
 * @param[in]       fullAccessKeys      Array of full-access keys for each section
 * @param[in]       watchdogDefault     Watchdog and rest configurations
 * @param[in]       deviceConf          General device configurations
 * @param[in]       suid                Secure User ID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_ConfigDevice(QLIB_CONTEXT_T*                   qlibContext,
                                    const KEY_T                       deviceMasterKey,
                                    const KEY_T                       deviceSecretKey,
                                    const QLIB_SECTION_CONFIG_TABLE_T sectionTable,
                                    const KEY_ARRAY_T                 restrictedKeys,
                                    const KEY_ARRAY_T                 fullAccessKeys,
                                    const QLIB_WATCHDOG_CONF_T*       watchdogDefault,
                                    const QLIB_DEVICE_CONF_T*         deviceConf,
                                    const _128BIT                     suid);

/************************************************************************************************************
 * @brief       This function retrieve the section configuration.
 *              All configuration parameters are optional (can be NULL).
 *
 * @param       qlibContext   QLIB state object
 * @param       sectionID     Section number to configure
 * @param       policy        section configuration policy
 * @param       baseAddr      section base address
 * @param       size          section size in bytes
 * @param       digest        section digest
 * @param       crc           section CRC
 * @param       version       Section version value
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_GetSectionConfiguration(QLIB_CONTEXT_T* qlibContext,
                                               U32             sectionID,
                                               U32*            baseAddr,
                                               U32*            size,
                                               QLIB_POLICY_T*  policy,
                                               U64*            digest,
                                               U32*            crc,
                                               U32*            version);

/************************************************************************************************************
 * @brief       This function updates section configuration.
 *              All configuration parameters ('policy','digest','crc', 'newVersion') are optional
 *              (can be NULL). This function requires the session to be opened to the relevant section
 *              with Full Access
 *
 * @param[in,out]   qlibContext     qlib context object
 * @param[in]       sectionID       Section number to configure
 * @param[in]       policy          New section configuration policy
 * @param[in]       digest          New section digest, or NULL if not digest available
 * @param[in]       crc             New section CRC, or NULL if no CRC available
 * @param[in]       newVersion      new section version value, or NULL if version not changed
 * @param[in]       swap            swap the code and reset
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_ConfigSection(QLIB_CONTEXT_T*      qlibContext,
                                     U32                  sectionID,
                                     const QLIB_POLICY_T* policy,
                                     const U64*           digest,
                                     const U32*           crc,
                                     const U32*           newVersion,
                                     QLIB_SWAP_T          swap);

/************************************************************************************************************
 * @brief       This function opens a session to a given section
 *
 * @param       qlibContext     QLIB state object
 * @param       sectionID       Section index
 * @param       sessionAccess   Session access type
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_OpenSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_SESSION_ACCESS_T sessionAccess);

/************************************************************************************************************
 * @brief       This function closes a session to a given section
 *
 * @param       qlibContext QLIB state object
 * @param       sectionID   Section index
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_CloseSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID);

/************************************************************************************************************
 * @brief       This function grants plain access to a authenticated plain access section
 *
 * @param       qlibContext     QLIB state object
 * @param       sectionID       Section index
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_AuthPlainAccess_Grant(QLIB_CONTEXT_T* qlibContext, U32 sectionID);

/************************************************************************************************************
 * @brief       This function revokes plain access from a authenticated plain access section
 *              If the section is open, then the function closes it.
 *
 * @param       qlibContext QLIB state object
 * @param       sectionID   Section index
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_AuthPlainAccess_Revoke(QLIB_CONTEXT_T* qlibContext, U32 sectionID);

/************************************************************************************************************
 * @brief       This function perform secure reads data from the flash.
 *              It can generate Secure and Secure + Authenticated reads. A session must be open first
 *
 * @param       qlibContext-   QLIB state object
 * @param       buf            Pointer to output buffer
 * @param       sectionID      Section index
 * @param       offset         Section offset
 * @param       size           Size of read data
 * @param       auth           if TRUE, read operation will be authenticated
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Read(QLIB_CONTEXT_T* qlibContext, U8* buf, U32 sectionID, U32 offset, U32 size, BOOL auth);

/************************************************************************************************************
 * @brief       This function perform secure write data to the flash.
 *
 * @param       qlibContext   QLIB state object
 * @param       buf           Pointer to input buffer
 * @param       sectionID     Section index
 * @param       offset        Section offset
 * @param       size          Data size
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Write(QLIB_CONTEXT_T* qlibContext, const U8* buf, U32 sectionID, U32 offset, U32 size);

/************************************************************************************************************
 * @brief       This function erases data from the flash
 *
 * @param       qlibContext-   QLIB state object
 * @param       sectionID      Section index
 * @param       offset         Section offset
 * @param       size           Data size
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Erase(QLIB_CONTEXT_T* qlibContext, U32 sectionID, U32 offset, U32 size);

/************************************************************************************************************
 * @brief       This function erases the entire section with either plain-text or secure command
 *
 * @param[in,out]   qlibContext   qlib context object
 * @param[in]       sectionID     Section ID
 * @param[in]       secure        if TRUE, secure erase is performed
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_EraseSection(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL secure);

/************************************************************************************************************
 * @brief This function configures volatile access permissions to given section.
 *
 * @param qlibContext   qlib context object
 * @param sectionID     Section index
 * @param readEnable    if TRUE, read will be enabled for this section
 * @param writeEnable   if TRUE, write will be enabled for this section
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_ConfigAccess(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL readEnable, BOOL writeEnable);

/************************************************************************************************************
 * @brief       This function allows loading of the section keys
 *
 * @param[in,out]   qlibContext     qlib context object
 * @param[in]       sectionID       Section index
 * @param[in]       key             Section key value
 * @param[in]       fullAccess      If TRUE, the given section key is full access key. Otherwise the
 *                                  given key considered a restricted key
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_LoadKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, const KEY_T key, BOOL fullAccess);

/************************************************************************************************************
 * @brief       The function allows removal of keys from QLib
 *
 * @param       qlibContext  QLIB state object
 * @param       sectionID    Section index
 * @param       fullAccess   If TRUE, the given section key is full access key. Otherwise the given
 *                           key considered a restricted key
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_RemoveKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL fullAccess);

/************************************************************************************************************
 * @brief       This function forces an integrity check on specific section
 *
 * @param       qlibContext     QLIB state object
 * @param       sectionID       Section index
 * @param       integrityType   The integrity type to perform
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_CheckIntegrity(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_INTEGRITY_T integrityType);

/************************************************************************************************************
 * @brief       This function returns CDI value
 *
 * @param       qlibContext     QLIB state object
 * @param[out]  nextCdi         CDI value produced by this module
 * @param[in]   prevCdi         CDI value obtained from previous module
 * @param[in]   sectionId       section number CDI associated with
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_CalcCDI(QLIB_CONTEXT_T* qlibContext, _256BIT nextCdi, _256BIT prevCdi, U32 sectionId);

/************************************************************************************************************
 * @brief       This function configures the Secure Watchdog functionality.
 *              The key used for the Watchdog functionality, is the key used in currently open session.
 *
 * @param       qlibContext   QLIB state object
 * @param       watchdogCFG   Watchdog and rest configurations
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Watchdog_ConfigSet(QLIB_CONTEXT_T* qlibContext, const QLIB_WATCHDOG_CONF_T* watchdogCFG);

/************************************************************************************************************
 * @brief       This function read the Secure Watchdog configuration.
 *
 * @param       qlibContext   QLIB state object
 * @param       watchdogCFG   Watchdog and rest configurations
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Watchdog_ConfigGet(QLIB_CONTEXT_T* qlibContext, QLIB_WATCHDOG_CONF_T* watchdogCFG);

/************************************************************************************************************
 * @brief       This function touches (resets) the Secure Watchdog. A session must be open with
 *              appropriate key to allow successful execution of this function.
 *              They key should have same 'sectionID' as key used for watchdog configuration.
 *              It can be either full-access or restricted key.
 *
 * @param       qlibContext   QLIB state object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Watchdog_Touch(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function triggers (force expire) the Secure Watchdog.
 *
 * @param       qlibContext   QLIB state object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Watchdog_Trigger(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function read the current status of the Secure watchdog timer.
 *              All output parameters can be NULL
 *
 * @param[in,out]   qlibContext   qlib context object
 * @param[out]      secondsPassed The current value of the AWD timer (in seconds)
 * @param[out]      ticsResidue   The residue of the AWD timer (in units of 64 tics of LF_OSC). If NULL, time is not returned.
 * @param[out]      expired       TRUE if AWD timer has expired
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_Watchdog_Get(QLIB_CONTEXT_T* qlibContext, U32* secondsPassed, U32* ticsResidue, BOOL* expired);

/************************************************************************************************************
 * @brief       This function updates the RAM copy of WID and optionally returns it
 *
 * @param       qlibContext   QLIB state object
 * @param[out]  id            Winbond ID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_GetWID(QLIB_CONTEXT_T* qlibContext, QLIB_WID_T id);

/************************************************************************************************************
 * @brief       This routine returns the Hardware version of the flash
 *
 * @param[in,out]   qlibContext   qlib context object
 * @param[out]      hwVer         pointer to struct to be filled with the output
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_GetHWVersion(QLIB_CONTEXT_T* qlibContext, QLIB_SEC_HW_VER_T* hwVer);

/************************************************************************************************************
 * @brief       This routine returns the WID and SUID of the flash
 *
 * @param[in,out]   qlibContext   qlib context object
 * @param[out]      id_p          pointer to struct to be filled with the output
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_GetId(QLIB_CONTEXT_T* qlibContext, QLIB_SEC_ID_T* id_p);

/************************************************************************************************************
 * @brief       This routine reads the secure status register and returns QLIB_STATUS__OK if no error occur,
 *              or the corresponding QLIB_STATUS__[ERROR] if an error occur
 *
 * @param       qlibContext   qlib context object
 *
 * @return      returns QLIB_STATUS__OK if no error occur found or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_GetStatus(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine configures Flash interface configuration for secure commands.
 *              This function do not handle switching from or to QPI mode,
 *              this is assumed to be completed by the standard module, see @ref QLIB_STD_SetInterface.
 *
 * @param       qlibContext   qlib context object
 * @param[in]   busFormat     SPI bus format
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_SetInterface(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat);

/************************************************************************************************************
 * @brief       This function synchronize the host monotonic counter with the flash monotonic counter,
 *              and refresh the out-dated information in the lib context.
 *              This function mast be called after every reset or if secure flash commands has
 *              executed without using this lib.
 *
 * @param       qlibContext   QLIB state object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_SyncAfterFlashReset(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This function enables plain access to the given section
 *
 * @param       qlibContext     QLIB state object
 * @param       sectionID       Section index
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_EnablePlainAccess(QLIB_CONTEXT_T* qlibContext, U32 sectionID);



#ifdef __cplusplus
}
#endif

#endif // __QLIB_SEC_H__
