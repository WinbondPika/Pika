/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_cmd_proc.h
* @brief      Command processor: This file handles processing of flash API commands
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_CMD_PROC_H__
#define __QLIB_CMD_PROC_H__

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
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


#define SSR_MASK__ALL_ERRORS                                                                                               \
    (MASK_FIELD(QLIB_REG_SSR__SES_ERR_S) | MASK_FIELD(QLIB_REG_SSR__INTG_ERR_S) | MASK_FIELD(QLIB_REG_SSR__AUTH_ERR_S) |   \
     MASK_FIELD(QLIB_REG_SSR__PRIV_ERR_S) | MASK_FIELD(QLIB_REG_SSR__IGNORE_ERR_S) | MASK_FIELD(QLIB_REG_SSR__SYS_ERR_S) | \
     MASK_FIELD(QLIB_REG_SSR__FLASH_ERR_S) | MASK_FIELD(QLIB_REG_SSR__MC_ERR) | MASK_FIELD(QLIB_REG_SSR__ERR) |            \
     MASK_FIELD(QLIB_REG_SSR__BUSY))

#define SSR_MASK__IGNORE_INTEG_ERR                                                                                          \
    (MASK_FIELD(QLIB_REG_SSR__AUTH_ERR_S) | MASK_FIELD(QLIB_REG_SSR__PRIV_ERR_S) | MASK_FIELD(QLIB_REG_SSR__IGNORE_ERR_S) | \
     MASK_FIELD(QLIB_REG_SSR__SYS_ERR_S) | MASK_FIELD(QLIB_REG_SSR__FLASH_ERR_S) | MASK_FIELD(QLIB_REG_SSR__MC_ERR) |       \
     MASK_FIELD(QLIB_REG_SSR__BUSY))

#define SSR_MASK__IGNORE_IGNORE_ERR                                                                                       \
    (MASK_FIELD(QLIB_REG_SSR__INTG_ERR_S) | MASK_FIELD(QLIB_REG_SSR__AUTH_ERR_S) | MASK_FIELD(QLIB_REG_SSR__PRIV_ERR_S) | \
     MASK_FIELD(QLIB_REG_SSR__SYS_ERR_S) | MASK_FIELD(QLIB_REG_SSR__FLASH_ERR_S) | MASK_FIELD(QLIB_REG_SSR__MC_ERR) |     \
     MASK_FIELD(QLIB_REG_SSR__BUSY))

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_GET_LAST_SEC_STATUS_FIELD(qlibContext)      ((qlibContext)->ssr.asUint)
#define QLIB_SET_LAST_SEC_STATUS_FIELD(qlibContext, val) ((qlibContext)->ssr.asUint = val)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine returns the Secure status register
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      ssr           Secure status register
 * @param[in]       mask          SSR mask
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_SSR_UNSIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* ssr, U32 mask);

/************************************************************************************************************
 * @brief       This routine returns assured with signature the Secure status register
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      ssr           Secure status register
 * @param[in]       mask          SSR mask
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_SSR_SIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* ssr, U32 mask);


/*---------------------------------------------------------------------------------------------------------*/
/*                                             STATUS COMMANDS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/************************************************************************************************************
 * @brief       This routine returns Winbond ID (WID)
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      WID           WID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_WID_UNSIGNED(QLIB_CONTEXT_T* qlibContext, _64BIT WID);

/************************************************************************************************************
 * @brief       This routine returns signature assured Winbond ID (WID)
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      WID           WID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_WID_SIGNED(QLIB_CONTEXT_T* qlibContext, _64BIT WID);


/************************************************************************************************************
 * @brief       This routine returns Secret User ID (SUID)
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      SUID          SUID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_SUID_UNSIGNED(QLIB_CONTEXT_T* qlibContext, _128BIT SUID);

/************************************************************************************************************
 * @brief       This routine returns Secret User ID via CALC_SIG command i.e. with a signature
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      SUID          SUID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_SUID_SIGNED(QLIB_CONTEXT_T* qlibContext, _128BIT SUID);

/************************************************************************************************************
 * @brief       This routine returns AWDTSR value
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      AWDTSR        AWDTSR
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_AWDTSR(QLIB_CONTEXT_T* qlibContext, AWDTSR_T* AWDTSR);


/*---------------------------------------------------------------------------------------------------------*/
/*                                         CONFIGURATION COMMANDS                                          */
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine performs Secure Format (SFORMAT)
 *
 * @param[in,out]   qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__SFORMAT(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine performs format
 *
 * @param[in,out]   qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__FORMAT(QLIB_CONTEXT_T* qlibContext);


/************************************************************************************************************
 * @brief       This routine sets new key
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       kid           Key ID
 * @param[in]       key_buff      New key data
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_KEY(QLIB_CONTEXT_T* qlibContext, QLIB_KID_T kid, const KEY_T key_buff);

/************************************************************************************************************
 * @brief       This routine sets the SUID
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       SUID          SUID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_SUID(QLIB_CONTEXT_T* qlibContext, const _128BIT SUID);

/************************************************************************************************************
 * @brief       This routine sets new GMC
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       GMC           GMC
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_GMC(QLIB_CONTEXT_T* qlibContext, const GMC_T GMC);

/************************************************************************************************************
 * @brief       This routine returns GMC
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      GMC           GMC
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_GMC_UNSIGNED(QLIB_CONTEXT_T* qlibContext, GMC_T GMC);

/************************************************************************************************************
 * @brief       This routine returns assured with signature GMC
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      GMC           GMC
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_GMC_SIGNED(QLIB_CONTEXT_T* qlibContext, GMC_T GMC);

/************************************************************************************************************
 * @brief       This routine sets new GMT
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       GMT           GMT
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_GMT(QLIB_CONTEXT_T* qlibContext, const GMT_T GMT);

/************************************************************************************************************
 * @brief       This routine gets GMT
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      GMT           GMT
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_GMT_UNSIGNED(QLIB_CONTEXT_T* qlibContext, GMT_T GMT);

/************************************************************************************************************
 * @brief       This routine gets GMT via CALC_SIG command. Session to any section should be opened
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      GMT           GMT
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_GMT_SIGNED(QLIB_CONTEXT_T* qlibContext, GMT_T GMT);

/************************************************************************************************************
 * @brief       This routine sets watchdog configuration register
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       AWDTCFG       Watchdog configuration
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_AWDT(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T AWDTCFG);

/************************************************************************************************************
 * @brief       This routine gets watchdog configuration register
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      AWDTCFG       Watchdog configuration
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_AWDT_UNSIGNED(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T* AWDTCFG);

/************************************************************************************************************
 * @brief       This routine gets signature assured watchdog configuration register
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      AWDTCFG       Watchdog configuration
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_AWDT_SIGNED(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T* AWDTCFG);

/************************************************************************************************************
 * @brief       This routine performs watchdog touch
 *
 * @param[in,out]   qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__AWDT_TOUCH(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine configures watchdog with plain-access (if allowed)
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       AWDTCFG       Watchdog configuration
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_AWDT_PA(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T AWDTCFG);

/************************************************************************************************************
 * @brief       This routine performs watchdog touch in plain-access
 *
 * @param[in,out]   qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__AWDT_TOUCH_PA(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine sets section configuration
 *
 * @param[in,out]   qlibContext    Context
 * @param[in]       sectionIndex   Section index
 * @param[in]       SCRn           SCR
 * @param[in]       initPA         Initialize PA to the section after swap
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_SCRn(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex, const SCRn_T SCRn, BOOL initPA);

/************************************************************************************************************
 * @brief       This routine sets section configuration with swap
 *
 * @param[in,out]   qlibContext    Context
 * @param[in]       sectionIndex   Section index
 * @param[in]       SCRn           SCR
 * @param[in]       reset          reset the device after swap
 * @param[in]       initPA         Initialize PA to the section after swap
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_SCRn_swap(QLIB_CONTEXT_T* qlibContext,
                                           U32             sectionIndex,
                                           const SCRn_T    SCRn,
                                           BOOL            reset,
                                           BOOL            initPA);

/************************************************************************************************************
 * @brief       This routine gets SCR
 *
 * @param[in,out]   qlibContext    Context
 * @param[in]       sectionIndex   Section index
 * @param[out]      SCRn           SCR
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_SCRn_UNSIGNED(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex, SCRn_T SCRn);

/************************************************************************************************************
 * @brief       This routine gets SCR via secure CALC_SIG command. Session to any section should be opened
 *
 * @param[in,out]   qlibContext    Context
 * @param[in]       sectionIndex   Section index
 * @param[out]      SCRn           SCR
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_SCRn_SIGNED(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex, SCRn_T SCRn);

/************************************************************************************************************
 * @brief       This routine sets new reset response
 *
 * @param[in,out]   qlibContext     Context
 * @param[in]       is_RST_RESP1    if TRUE, its RESP1 otherwise RESP2
 * @param           RST_RESP_half
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_RST_RESP(QLIB_CONTEXT_T* qlibContext, BOOL is_RST_RESP1, const U32* RST_RESP_half);

/************************************************************************************************************
 * @brief       This routine reads 128B reset response
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      RST_RESP      Reset Response
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_RST_RESP(QLIB_CONTEXT_T* qlibContext, U32* RST_RESP);

/************************************************************************************************************
 * @brief       This routine sets ACLR register
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       ACLR          ACLR
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__set_ACLR(QLIB_CONTEXT_T* qlibContext, ACLR_T ACLR);

/************************************************************************************************************
 * @brief       This routine gets ACLR register
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      ACLR          ACLR
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_ACLR(QLIB_CONTEXT_T* qlibContext, ACLR_T* ACLR);


/*---------------------------------------------------------------------------------------------------------*/
/*                                        SESSION CONTROL COMMANDS                                         */
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine returns Flash monotonic counters (TC and DMC)
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      mc            Monotonic Counter
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_MC_UNSIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_MC_T mc);

/************************************************************************************************************
 * @brief       This routine returns signature assured Flash monotonic counters (TC and DMC)
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      mc            Monotonic Counter
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_MC_SIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_MC_T mc);

/************************************************************************************************************
 * @brief       This routine synchronizes MC counters (TC and DMC) from HW to RAM
 *
 * @param[in,out]   qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__synch_MC(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine performs MC maintenance
 *
 * @param[in,out]   qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__MC_MAINT(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine is used to open a Session and establish a Secure Channel
 *
 * @param[in,out]   qlibContext         Context
 * @param[in]       kid                 Key ID
 * @param[in]       key                 Key data
 * @param[in]       includeWID          if TRUE, WID is used in session opening
 * @param[in]       ignoreScrValidity   if TRUE, SCR validity is ignored
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__Session_Open(QLIB_CONTEXT_T* qlibContext,
                                          QLIB_KID_T      kid,
                                          const KEY_T     key,
                                          BOOL            includeWID,
                                          BOOL            ignoreScrValidity);

/************************************************************************************************************
 * @brief       This routine is used to close a Session
 *
 * @param[in,out]   qlibContext         Context
 * @param[in]       kid                 Key ID
 * @param[in]       revokePA            If TRUE, revoke plain-access privileges
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__Session_Close(QLIB_CONTEXT_T* qlibContext, QLIB_KID_T kid, BOOL revokePA);

/************************************************************************************************************
 * @brief           This function performs INIT_SECTION_PA command
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       sectionIndex   [Section index](md_definitions.html#DEF_SECTION)
 *
 * @return          QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__init_section_PA(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex);

/************************************************************************************************************
 * @brief       This routine returns CDI value
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       mode          CDI mode
 * @param[out]      cdi           CDI value
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__CALC_CDI(QLIB_CONTEXT_T* qlibContext, U32 mode, _256BIT cdi);

/************************************************************************************************************
 * @brief       This routine forces integrity check on given section
 *
 * @param[in,out]   qlibContext    Context
 * @param[in]       sectionIndex   Section index
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__Check_Integrity(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex);

/*---------------------------------------------------------------------------------------------------------*/
/*                                        SECURE TRANSPORT COMMANDS                                        */
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine returns TC counter
 *
 * @param[in,out]   qlibContext     Context
 * @param[out]      tc              Transaction Counter
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_TC(QLIB_CONTEXT_T* qlibContext, U32* tc);

/************************************************************************************************************
 * @brief       This routine returns signed data of registers or sector hash
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       dataType      Data type
 * @param[in]       section       Section number
 * @param[out]      data          Data output or NULL
 * @param[in]       size          Output data size
 * @param[out]      signature     Output signature or NULL
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__CALC_SIG(QLIB_CONTEXT_T*         qlibContext,
                                      QLIB_SIGNED_DATA_TYPE_T dataType,
                                      U32                     section,
                                      U32*                    data,
                                      U32                     size,
                                      _64BIT                  signature);

/************************************************************************************************************
 * @brief       This routine performs secure read
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       addr          Address
 * @param[out]      data32B       Read data buffer
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__SRD(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data32B);

/************************************************************************************************************
 * @brief       This routine performs secure authenticated read
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       addr          Address
 * @param[out]      data32B       Read data buffer
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__SARD(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data32B);

#ifndef QLIB_SUPPORT_XIP
/************************************************************************************************************
 * @brief       This routine performs multi-block secure read
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       addr          Address
 * @param[out]      data          Read data buffer
 * @param[in]       size          Read data size in bytes
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__SRD_Multi(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data, U32 size);

/************************************************************************************************************
 * @brief       This routine performs multi-block secure authenticated read
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       addr          Address
 * @param[out]      data          Read data buffer
 * @param[in]       size          Read data size in bytes
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__SARD_Multi(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data, U32 size);
#endif // QLIB_SUPPORT_XIP

/************************************************************************************************************
 * @brief       This routine performs secure authenticated write
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       addr          Address
 * @param[in]       data          Data to write (32Bytes)
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__SAWR(QLIB_CONTEXT_T* qlibContext, U32 addr, const U32* data);

/************************************************************************************************************
 * @brief       This routine performs all kind of secure erase commands
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       type          Secure erase type
 * @param[in]       addr          Secure erase address
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__SERASE(QLIB_CONTEXT_T* qlibContext, QLIB_ERASE_T type, U32 addr);

/************************************************************************************************************
 * @brief       This routine performs non-secure (plain) sector erase
 *
 * @param[in,out]   qlibContext     Context
 * @param[in]       sectionIndex    sector index
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__ERASE_SECT_PA(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex);

/*---------------------------------------------------------------------------------------------------------*/
/*                                              AUX COMMANDS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/************************************************************************************************************
 * @brief       This routine returns HW version
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      hwVer        HW version
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_HW_VER_UNSIGNED(QLIB_CONTEXT_T* qlibContext, HW_VER_T* hwVer);

/************************************************************************************************************
 * @brief       This routine returns HW version assured by signature
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      hwVer         HW version
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__get_HW_VER_SIGNED(QLIB_CONTEXT_T* qlibContext, HW_VER_T* hwVer);

/************************************************************************************************************
 * @brief       This routine force-triggers the watchdog timer
 *
 * @param[in,out]   qlibContext         Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__AWDT_expire(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine puts the Flash into sleep mode
 *
 * @param[in,out]       qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__sleep(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine check if there is some error in the last SSR reed from flash and return it
 *              This routine do not reads SSR rather use a local version from last command
 *              Note: if an error found, this routine automatically execute OP1 to clear the error bits
 *
 * @param[in,out]   qlibContext   Context
 * @param[in]       mask          SSR mask
 *
 * @return      QLIB_STATUS__OK if no error occurred (according to ssr error bits)
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__checkLastSsrErrors(QLIB_CONTEXT_T* qlibContext, U32 mask);


#ifdef __cplusplus
}
#endif

#endif // __QLIB_CMD_PROC_H__
