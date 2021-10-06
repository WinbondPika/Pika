/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sec_regs.h
* @brief      This file contains secure register definitions
*
* ### project qlib
*
************************************************************************************************************/
#ifndef _QLIB_SEC_REGS_H_
#define _QLIB_SEC_REGS_H_

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
#define QLIB_NUM_OF_SECTIONS          8UL
#define QLIB_SEC_READ_PAGE_SIZE_BYTE  _32B_
#define QLIB_SEC_WRITE_PAGE_SIZE_BYTE _32B_
#define QLIB_MIN_STD_ADDR_SIZE        _512KB_

/*---------------------------------------------------------------------------------------------------------*/
/* Section Mapping Register (SMRn) conversions                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_SMRn__BASE_IN_BYTES_TO_TAG(baseBytes) ((baseBytes) >> 16)
#define QLIB_REG_SMRn__BASE_IN_TAG_TO_BYTES(baseTag)   (((U32)(baseTag)) << 16)
#define QLIB_REG_SMRn__LEN_IN_BYTES_TO_TAG(lenBytes)   LOG2((lenBytes) >> 16)
#define QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(lenTag)     (_64KB_ << (lenTag))

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                  TYPES                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define ERASE_T U32 // follow 11.5.6

/*---------------------------------------------------------------------------------------------------------*/
/* Registers Types                                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
typedef U32 GMC_T[5];  ///< 160b
typedef U32 GMT_T[5];  ///< 160b
typedef U32 DEVCFG_T;  ///< 32b
typedef U32 AWDTCFG_T; ///< 32b
typedef U32 AWDTSR_T;  ///< 32b
typedef U32 SCRn_T[5]; ///< 160b
typedef U32 ACLR_T;    ///< 32b
typedef U16 SMRn_T;   ///< 16b
typedef U32 SSPRn_T;  ///< 32b
typedef U32 HW_VER_T; ///< 32b
typedef U32 ACL_STATUS_T[QLIB_NUM_OF_SECTIONS];

/*---------------------------------------------------------------------------------------------------------*/
/* Section Mapping Register (SMRn) fields                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
// SMRn fields (base [0:11] , len [12:14] , enable [15:15])
#define QLIB_REG_SMRn__BASE   0, 12
#define QLIB_REG_SMRn__LEN    12, 3
#define QLIB_REG_SMRn__ENABLE 15, 1

/*---------------------------------------------------------------------------------------------------------*/
/* Global Mapping Table (GMT) fields and access                                                            */
/*---------------------------------------------------------------------------------------------------------*/
// GMT fields (SMR(i) [(16*i):(16*i+15)] , version [128:159])
#define QLIB_REG_GMT_GET_VER(gmt)      (((U32*)(gmt))[4])
#define QLIB_REG_GMT_SET_VER(gmt, val) (((U32*)(gmt))[4]) = val

#define QLIB_REG_GMT_GET_SMRn(gmt, section)      (((SMRn_T*)(gmt))[section])
#define QLIB_REG_GMT_SET_SMRn(gmt, section, val) (((SMRn_T*)(gmt))[section]) = val

#define QLIB_REG_GMT_GET_BASE(gmt, section)   READ_VAR_FIELD(QLIB_REG_GMT_GET_SMRn(gmt, section), QLIB_REG_SMRn__BASE)
#define QLIB_REG_GMT_GET_LEN(gmt, section)    READ_VAR_FIELD(QLIB_REG_GMT_GET_SMRn(gmt, section), QLIB_REG_SMRn__LEN)
#define QLIB_REG_GMT_GET_ENABLE(gmt, section) READ_VAR_FIELD(QLIB_REG_GMT_GET_SMRn(gmt, section), QLIB_REG_SMRn__ENABLE)

#define QLIB_REG_GMT_SET_BASE(gmt, section, val)   SET_VAR_FIELD(QLIB_REG_GMT_GET_SMRn(gmt, section), QLIB_REG_SMRn__BASE, val)
#define QLIB_REG_GMT_SET_LEN(gmt, section, val)    SET_VAR_FIELD(QLIB_REG_GMT_GET_SMRn(gmt, section), QLIB_REG_SMRn__LEN, val)
#define QLIB_REG_GMT_SET_ENABLE(gmt, section, val) SET_VAR_FIELD(QLIB_REG_GMT_GET_SMRn(gmt, section), QLIB_REG_SMRn__ENABLE, val)

/*---------------------------------------------------------------------------------------------------------*/
/* Section Security Policy Register (SSPRn) fields                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_SSPRn__INTG_PROT_CFG 0, 1
#define QLIB_REG_SSPRn__INTG_PROT_AC  1, 1
#define QLIB_REG_SSPRn__WP_EN         2, 1
#define QLIB_REG_SSPRn__ROLLBACK_EN   3, 1
#define QLIB_REG_SSPRn__PA_RD_EN      4, 1
#define QLIB_REG_SSPRn__PA_WR_EN      5, 1
#define QLIB_REG_SSPRn__AUTH_PA       6, 1

/*---------------------------------------------------------------------------------------------------------*/
/* Section Configuration Registers (SCRn) fields                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_SCRn_GET_SSPRn(SCRn)         (((U32*)(SCRn))[0])
#define QLIB_REG_SCRn_SET_SSPRn(SCRn, val)    (((U32*)(SCRn))[0]) = val
#define QLIB_REG_SCRn_GET_CHECKSUM(SCRn)      (((U32*)(SCRn))[1])
#define QLIB_REG_SCRn_GET_CHECKSUM_PTR(SCRn)  &(SCRn)[1]
#define QLIB_REG_SCRn_SET_CHECKSUM(SCRn, val) (((U32*)(SCRn))[1]) = val
#define QLIB_REG_SCRn_GET_DIGEST(SCRn)        (((U64)((SCRn)[2])) | (((U64)((SCRn)[3])) << 32))
#define QLIB_REG_SCRn_GET_DIGEST_PTR(SCRn)    &(SCRn)[2]
#define QLIB_REG_SCRn_SET_DIGEST(SCRn, val)                   \
    {                                                         \
        (SCRn)[2] = (U32)(((U64)(val)) & 0xFFFFFFFF);         \
        (SCRn)[3] = (U32)((((U64)(val)) >> 32) & 0xFFFFFFFF); \
    }
#define QLIB_REG_SCRn_GET_VER(SCRn)      (((U32*)(SCRn))[4])
#define QLIB_REG_SCRn_SET_VER(SCRn, val) (((U32*)(SCRn))[4]) = val

/*---------------------------------------------------------------------------------------------------------*/
/* Global Memory Configuration Register (GMC)                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_GMC_GET_AWDT_DFLT(gmc)      (((U32*)(gmc))[0])
#define QLIB_REG_GMC_SET_AWDT_DFLT(gmc, val) (((U32*)(gmc))[0]) = val
#define QLIB_REG_GMC_GET_DEVCFG(gmc)         (((U32*)(gmc))[1])
#define QLIB_REG_GMC_SET_DEVCFG(gmc, val)    (((U32*)(gmc))[1]) = val
#define QLIB_REG_GMC_GET_VER(gmc)            (((U32*)(gmc))[4])
#define QLIB_REG_GMC_SET_VER(gmc, val)       (((U32*)(gmc))[4]) = val

/*---------------------------------------------------------------------------------------------------------*/
/* Authenticated Watchdog Timer Configuration (AWDTCFG) fields                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_AWDTCFG__AWDT_EN   0, 1
#define QLIB_REG_AWDTCFG__LFOSC_EN  1, 1
#define QLIB_REG_AWDTCFG__SRST_EN   2, 1
#define QLIB_REG_AWDTCFG__AUTH_WDT  3, 1
#define QLIB_REG_AWDTCFG__RSTO_EN   4, 1
#define QLIB_REG_AWDTCFG__RSTI_OVRD 5, 1
#define QLIB_REG_AWDTCFG__RSTI_EN   6, 1
#define QLIB_REG_AWDTCFG__RST_IN_EN 7, 1
#define QLIB_REG_AWDTCFG__KID       8, 4
#define QLIB_REG_AWDTCFG__TH        12, 5
#define QLIB_REG_AWDTCFG__RESERVED1 17, 3
#define QLIB_REG_AWDTCFG__OSC_RATE_FRAC 20, 4
#define QLIB_REG_AWDTCFG__OSC_RATE_KHZ  24, 7
#define QLIB_REG_AWDTCFG__LOCK          31, 1

#define QLIB_REG_AWDTCFG_RESERVED_MASK (MASK_FIELD(QLIB_REG_AWDTCFG__RESERVED1))

#define QLIB_AWDTCFG__OSC_RATE_KHZ_DEFAULT 65

/*---------------------------------------------------------------------------------------------------------*/
/* Authenticated Watchdog Timer Status Register (AWDTSR) fields                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_AWDTSR__AWDT_VAL   0, 20
#define QLIB_REG_AWDTSR__AWDT_RES   20, 11
#define QLIB_REG_AWDTSR__AWDT_EXP_S 31, 1

/*---------------------------------------------------------------------------------------------------------*/
/* Device Configurations (DEVCFG) fields                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#if LOG2(QLIB_MAX_STD_ADDR_SIZE) > 22
#define QLIB_REG_DEVCFG__SECT_SEL   0, 3
#define QLIB_REG_DEVCFG__RESERVED_1 3, 1
#else
#define QLIB_REG_DEVCFG__SECT_SEL   0, 2
#define QLIB_REG_DEVCFG__RESERVED_1 2, 2
#endif
#define QLIB_REG_DEVCFG__RST_RESP_EN 4, 1
#define QLIB_REG_DEVCFG__FB_EN       5, 1
#define QLIB_REG_DEVCFG__CK_SPECUL   6, 1
#define QLIB_REG_DEVCFG__RESERVED_2  7, 1 ///< Reserved. Write as 0.
#define QLIB_REG_DEVCFG__FORMAT_EN   8, 1
#define QLIB_REG_DEVCFG__STM_EN      9, 1
#define QLIB_REG_DEVCFG__RESERVED_3 10, 22

#define QLIB_REG_DEVCFG_RESERVED_MASK \
    (MASK_FIELD(QLIB_REG_DEVCFG__RESERVED_1) | MASK_FIELD(QLIB_REG_DEVCFG__RESERVED_2) | MASK_FIELD(QLIB_REG_DEVCFG__RESERVED_3))
/*---------------------------------------------------------------------------------------------------------*/
/* HW Version Register (HW_VER) fields                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_HW_VER__REVISION 0, 8
#define QLIB_REG_HW_VER__QSF_VER    8, 8
#define QLIB_REG_HW_VER__FLASH_SIZE 16, 4
#define QLIB_REG_HW_VER__FLASH_VER  20, 8
#define QLIB_REG_HW_VER__RESERVED_1 28, 4
#define QLIB_REG_HW_VER__DB         31, 1

/*---------------------------------------------------------------------------------------------------------*/
/* ACLR fields                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_ACLR_WR_LOCK    0, 8
#define QLIB_REG_ACLR_RESERVED_1 8, 8
#define QLIB_REG_ACLR_RD_LOCK    16, 8
#define QLIB_REG_ACLR_RESERVED_2 24, 8

#define QLIB_REG_ACLR_RESERVED_MASK (MASK_FIELD(QLIB_REG_ACLR_RESERVED_1) | MASK_FIELD(QLIB_REG_ACLR_RESERVED_2))


/*---------------------------------------------------------------------------------------------------------*/
/* SSR ('_S' is sticky field)                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_SSR__BUSY 0, 1
#define QLIB_REG_SSR__FLASH_BUSY 1, 1
#define QLIB_REG_SSR__ERR          2, 1
#define QLIB_REG_SSR__SES_READY    4, 1
#define QLIB_REG_SSR__RESP_READY   5, 1
#define QLIB_REG_SSR__POR          6, 1
#define QLIB_REG_SSR__FB_REMAP     7, 1
#define QLIB_REG_SSR__AWDT_EXP     8, 1
#define QLIB_REG_SSR__SES_ERR_S    10, 1
#define QLIB_REG_SSR__INTG_ERR_S   12, 1
#define QLIB_REG_SSR__AUTH_ERR_S   13, 1
#define QLIB_REG_SSR__PRIV_ERR_S   14, 1
#define QLIB_REG_SSR__IGNORE_ERR_S 15, 1
#define QLIB_REG_SSR__SYS_ERR_S    16, 1
#define QLIB_REG_SSR__FLASH_ERR_S  17, 1
#define QLIB_REG_SSR__MC_ERR       19, 1
#define QLIB_REG_SSR__MC_MAINT     20, 2
#define QLIB_REG_SSR__SUSPEND_E    22, 1
#define QLIB_REG_SSR__SUSPEND_W    23, 1
#define QLIB_REG_SSR__STATE        24, 3
#define QLIB_REG_SSR__FULL_PRIV    27, 1
#define QLIB_REG_SSR__KID          28, 4

/*---------------------------------------------------------------------------------------------------------*/
/* QLIB_REG_SSR__STATE values                                                                              */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_REG_SSR__STATE_IN_RESET 0
#define QLIB_REG_SSR__STATE_WORKING  2
#define QLIB_REG_SSR__STATE_LOCKED   4
#define QLIB_REG_SSR__STATE_WORKING_MASK 7

/*---------------------------------------------------------------------------------------------------------*/
/* SSR bit-field structure                                                                                 */
/*---------------------------------------------------------------------------------------------------------*/
typedef struct
{
    U32 BUSY : 1;
#ifdef QLIB_SSR_FLASH_BUSY_BIT
    U32 FLASH_BUSY : 1;
#else
    U32 RESERVED_0 : 1;
#endif
    U32 ERR : 1;
    U32 RESERVED_1 : 1;
    U32 SES_READY : 1;
    U32 RESP_READY : 1;
    U32 POR : 1;
    U32 FB_REMAP : 1;
    U32 AWDT_EXP : 1;
    U32 RESERVED_2 : 1;
    U32 SES_ERR_S : 1;
    U32 RESERVED_3 : 1;
    U32 INTG_ERR_S : 1;
    U32 AUTH_ERR_S : 1;
    U32 PRIV_ERR_S : 1;
    U32 IGNORE_ERR_S : 1;
    U32 SYS_ERR_S : 1;
    U32 FLASH_ERR_S : 1;
    U32 RESERVED_4 : 1;
    U32 MC_ERR : 1;
    U32 MC_MAINT : 2;
    U32 SUSPEND_E : 1;
    U32 SUSPEND_W : 1;
    U32 STATE : 3;
    U32 FULL_PRIV : 1;
    U32 KID : 4;
} QLIB_REG_SSR_STRUCT_T;

/*---------------------------------------------------------------------------------------------------------*/
/* SSR bitfield-U32 union                                                                                  */
/*---------------------------------------------------------------------------------------------------------*/
typedef union
{
    U32                   asUint;
    QLIB_REG_SSR_STRUCT_T asStruct;
} QLIB_REG_SSR_T;


#endif //_QLIB_SEC_REGS_H_
