/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sec_cmds.h
* @brief      This file contains secure command definitions
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_SEC_CMDS_H__
#define __QLIB_SEC_CMDS_H__

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

/*---------------------------------------------------------------------------------------------------------*/
/* Secure Instructions                                                                                     */
/*---------------------------------------------------------------------------------------------------------*/
#define Q2_SEC_INST__LINES_MASK 0xF0
#define Q2_SEC_INST__OP0        0x0
#define Q2_SEC_INST__OP1        0x1
#define Q2_SEC_INST__OP2        0x2

#define Q2_SEC_INST__MAKE(inst, format, dtr_bit) ((inst) | (Q2_SEC_INST__LINES_MASK & (format)) | ((dtr_bit) << 2))

#define Q2_SEC_INST__WR_IBUF_START 0x80
#define Q2_SEC_INST__WR_IBUF_END   0x91

/*---------------------------------------------------------------------------------------------------------*/
/* Dummy cycles                                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#ifndef Q2_OP0_DUMMY_CYCLES_DTR
#define Q2_OP0_DUMMY_CYCLES_DTR Q2_OP0_DUMMY_CYCLES
#endif
#define Q2_SEC_INST_DUMMY_CYCLES__OP0(dtr) ((dtr == TRUE) ? Q2_OP0_DUMMY_CYCLES_DTR : Q2_OP0_DUMMY_CYCLES)
#define Q2_SEC_INST_DUMMY_CYCLES__OP2      8

/*---------------------------------------------------------------------------------------------------------*/
/* Sizes                                                                                                   */
/*---------------------------------------------------------------------------------------------------------*/
#define Q2_CTAG_SIZE_BYTE     (4)       // size of CTAG on SPI
#define Q2_MAX_IBUF_SIZE_BYTE (128 + 8) // includes data and signature

/************************************************************************************************************
 * Secure commands
************************************************************************************************************/
typedef enum
{
    QLIB_CMD_SEC_NONE = 0x00,
    QLIB_CMD_SEC_GET_WID       = 0x14,
    QLIB_CMD_SEC_GET_SUID      = 0x15,
    QLIB_CMD_SEC_GET_AWDTSR    = 0x18,
    QLIB_CMD_SEC_SFORMAT       = 0x20,
    QLIB_CMD_SEC_FORMAT        = 0x30,
    QLIB_CMD_SEC_SET_KEY       = 0x21,
    QLIB_CMD_SEC_SET_SUID      = 0x22,
    QLIB_CMD_SEC_SET_GMC       = 0x24,
    QLIB_CMD_SEC_GET_GMC       = 0x34,
    QLIB_CMD_SEC_SET_GMT       = 0x25,
    QLIB_CMD_SEC_GET_GMT       = 0x35,
    QLIB_CMD_SEC_SET_AWDT      = 0x26,
    QLIB_CMD_SEC_GET_AWDT      = 0x36,
    QLIB_CMD_SEC_AWDT_TOUCH    = 0x27,
    QLIB_CMD_SEC_SET_AWDT_PA   = 0x2D,
    QLIB_CMD_SEC_AWDT_TOUCH_PA = 0x2E,
    QLIB_CMD_SEC_SET_SCR       = 0x28,
    QLIB_CMD_SEC_SET_SCR_SWAP  = 0x29,
    QLIB_CMD_SEC_GET_SCR       = 0x38,
    QLIB_CMD_SEC_SET_RST_RESP  = 0x2B,
    QLIB_CMD_SEC_GET_RST_RESP  = 0x3B,
    QLIB_CMD_SEC_SET_ACLR      = 0x2C,
    QLIB_CMD_SEC_GET_ACLR      = 0x3C,
    QLIB_CMD_SEC_GET_MC        = 0x40,
    QLIB_CMD_SEC_MC_MAINT      = 0x41,
    QLIB_CMD_SEC_SESSION_OPEN  = 0x44,
    QLIB_CMD_SEC_INIT_SECTION_PA = 0x47,
    QLIB_CMD_SEC_CALC_CDI        = 0x48,
    QLIB_CMD_SEC_VER_INTG        = 0x49,
    QLIB_CMD_SEC_GET_TC          = 0x50,
    QLIB_CMD_SEC_CALC_SIG        = 0x52,
    QLIB_CMD_SEC_SRD             = 0x60,
    QLIB_CMD_SEC_SARD            = 0x61,
    QLIB_CMD_SEC_SAWR            = 0x64,
    QLIB_CMD_SEC_SERASE_4        = 0x68,
    QLIB_CMD_SEC_SERASE_32       = 0x69,
    QLIB_CMD_SEC_SERASE_64       = 0x6A,
    QLIB_CMD_SEC_SERASE_SEC      = 0x6B,
    QLIB_CMD_SEC_SERASE_ALL      = 0x6C,
    QLIB_CMD_SEC_ERASE_SECT_PA   = 0x6F,
    QLIB_CMD_SEC_GET_VERSION     = 0xF0,
    QLIB_CMD_SEC_AWDT_EXPIRE     = 0xFE,
    QLIB_CMD_SEC_SLEEP           = 0xF8,
} QLIB_SEC_CMD_T;

/************************************************************************************************************
 * Register data types
************************************************************************************************************/

typedef enum
{
    QLIB_SIGNED_DATA_ID_SECTION_DIGEST = 0x00,
    QLIB_SIGNED_DATA_ID_WID            = 0x10,
    QLIB_SIGNED_DATA_ID_SUID           = 0x14,
    QLIB_SIGNED_DATA_ID_HW_VER         = 0x18,
    QLIB_SIGNED_DATA_ID_SSR            = 0x20,
    QLIB_SIGNED_DATA_ID_AWDTCFG        = 0x24,
    QLIB_SIGNED_DATA_ID_MC             = 0x28,
    QLIB_SIGNED_DATA_ID_GMC            = 0x30,
    QLIB_SIGNED_DATA_ID_GMT            = 0x32,
    QLIB_SIGNED_DATA_ID_SECTION_CONFIG = 0x40,
} QLIB_SIGNED_DATA_ID_T;

typedef enum
{
    /*-----------------------------------------------------------------------------------------------------*/
    /*                                                      ID     SIZE                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_SIGNED_DATA_TYPE_SECTION_DIGEST = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_SECTION_DIGEST, BITS_TO_BYTES(64)),
    QLIB_SIGNED_DATA_TYPE_WID            = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_WID, BITS_TO_BYTES(64)),
    QLIB_SIGNED_DATA_TYPE_SUID           = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_SUID, BITS_TO_BYTES(128)),
    QLIB_SIGNED_DATA_TYPE_HW_VER         = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_HW_VER, BITS_TO_BYTES(32)),
    QLIB_SIGNED_DATA_TYPE_SSR            = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_SSR, BITS_TO_BYTES(32)),
    QLIB_SIGNED_DATA_TYPE_AWDTCFG        = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_AWDTCFG, BITS_TO_BYTES(32)),
    QLIB_SIGNED_DATA_TYPE_MC             = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_MC, BITS_TO_BYTES(64)),
    QLIB_SIGNED_DATA_TYPE_GMC            = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_GMC, BITS_TO_BYTES(160)),
    QLIB_SIGNED_DATA_TYPE_GMT            = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_GMT, BITS_TO_BYTES(160)),
    QLIB_SIGNED_DATA_TYPE_SECTION_CONFIG = MAKE_16_BIT(QLIB_SIGNED_DATA_ID_SECTION_CONFIG, BITS_TO_BYTES(160)),
} QLIB_SIGNED_DATA_TYPE_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Maximum data size                                                                                       */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_SIGNED_DATA_MAX_SIZE BITS_TO_BYTES(160)

/*---------------------------------------------------------------------------------------------------------*/
/* Get data size                                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_SIGNED_DATA_TYPE_GET_SIZE(data_type) BYTE(data_type, 1)

/*---------------------------------------------------------------------------------------------------------*/
/* Get data ID                                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_SIGNED_DATA_TYPE_GET_ID(data_type, section)                                                               \
    (BYTE(data_type, 0) +                                                                                              \
     ((((data_type) == QLIB_SIGNED_DATA_TYPE_SECTION_DIGEST) || ((data_type) == QLIB_SIGNED_DATA_TYPE_SECTION_CONFIG)) \
          ? ((U8)(0x07 & section))                                                                                     \
          : 0))

/*---------------------------------------------------------------------------------------------------------*/
/* open session command mode field                                                                         */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_SEC_CMD_OPEN_MODE_FIELD_INC_WID 0, 1
#define QLIB_SEC_CMD_OPEN_MODE_FIELD_IGN_SCR 1, 1

#define QLIB_SEC_OPEN_CMD_MODE_SET(mode, includeWID, ignoreScrValidity)                                   \
    {                                                                                                     \
        SET_VAR_FIELD(mode, QLIB_SEC_CMD_OPEN_MODE_FIELD_INC_WID, (includeWID == TRUE) ? 0x1 : 0);        \
        SET_VAR_FIELD(mode, QLIB_SEC_CMD_OPEN_MODE_FIELD_IGN_SCR, (ignoreScrValidity == TRUE) ? 0x1 : 0); \
    }

#endif //__QLIB_SEC_CMDS_H__
