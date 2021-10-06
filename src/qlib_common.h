/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_common.h
* @brief      This file contains QLIB common types and definitions (exported to the user)
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_COMMON_H__
#define __QLIB_COMMON_H__

#include "qlib_cfg.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             ERROR CHECKING                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifndef __QLIB_H__
#error "This internal header file should not be included directly. Please include `qlib.h' instead"
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * XIP support
************************************************************************************************************/
// clang-format off
#if defined(QLIB_SUPPORT_XIP)
    #if !defined(__RAM_SECTION)
        #if !defined(QLIB_RAM_SECTION)
            #error "QLIB_RAM_SECTION must be defined in order to support XIP"
        #endif
        #define __RAM_SECTION          __attribute__((noinline))  __attribute__ ((section(QLIB_RAM_SECTION)))
    #endif
#else
    #define __RAM_SECTION
#endif
// clang-format on

/************************************************************************************************************
 * Secure Instructions lines mask
************************************************************************************************************/
#define Q2_SEC_INST__SINGLE 0xA0
#define Q2_SEC_INST__DUAL   0xB0
#define Q2_SEC_INST__QUAD   0xD0

/************************************************************************************************************
* Secure Device ID
************************************************************************************************************/
#define SECURE_DEVICE_ID STD_FLASH_DEVICE_ID__W77Q

/************************************************************************************************************
* Secure Flash Size
************************************************************************************************************/
#define QLIB_STD_ADDR_SIZE(qlibCtx) (qlibCtx->addrSize)
#define QLIB_SEC_MEMORY_TYPE 0x8A
#define QLIB_VALUE_BY_FLASH_TYPE(ctx, secVal, stdVal)   (secVal)
#define QLIB_ACTION_BY_FLASH_TYPE(ctx, sec, std)        sec;
#define QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(ctx, action) action;

#define QLIB_PRNG_RESEED_COUNT 128

/************************************************************************************************************
* Section ID in case of fallback
************************************************************************************************************/
#define Q2_BOOT_SECTION          0
#define Q2_BOOT_SECTION_FALLBACK 7

#define QLIB_FALLBACK_SECTION(qlibContext, sectionID)                                                            \
    (READ_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__FB_REMAP) == 1)                                       \
        ? (sectionID == Q2_BOOT_SECTION ? Q2_BOOT_SECTION_FALLBACK                                               \
                                        : (sectionID == Q2_BOOT_SECTION_FALLBACK ? Q2_BOOT_SECTION : sectionID)) \
        : sectionID

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                               PURE TYPES                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * Flash power state selector
************************************************************************************************************/
typedef enum
{
    QLIB_POWER_FIRST = 0x1145141d,

    QLIB_POWER_UP,
    QLIB_POWER_DOWN,

    QLIB_POWER_LAST,
} QLIB_POWER_T;

/************************************************************************************************************
 * Flash erase type selector
************************************************************************************************************/
typedef enum
{
    QLIB_ERASE_FIRST = 0x156842fd,

    QLIB_ERASE_SECTOR_4K,
    QLIB_ERASE_BLOCK_32K,
    QLIB_ERASE_BLOCK_64K,

    QLIB_ERASE_SECTION,
    QLIB_ERASE_CHIP,

    QLIB_ERASE_LAST,
} QLIB_ERASE_T;

/************************************************************************************************************
 * SPI bus mode
************************************************************************************************************/
typedef enum QLIB_BUS_MODE_T
{
    QLIB_BUS_MODE_INVALID = 0,                         ///< Uninitialized
    QLIB_BUS_MODE_1_1_1   = Q2_SEC_INST__SINGLE | 0x1, ///< Single SPI for command, Single SPI for address and Single SPI for data
    QLIB_BUS_MODE_1_1_2   = Q2_SEC_INST__DUAL | 0x2,   ///< Single SPI for command, Single SPI for address and Dual SPI for data
    QLIB_BUS_MODE_1_2_2   = Q2_SEC_INST__DUAL | 0x3,   ///< Single SPI for command, Dual SPI for address and Dual SPI for data
    QLIB_BUS_MODE_1_1_4   = Q2_SEC_INST__QUAD | 0x4,   ///< Single SPI for command, Single SPI for address and Quad SPI for data
    QLIB_BUS_MODE_1_4_4   = Q2_SEC_INST__QUAD | 0x5,   ///< Single SPI for command, Quad SPI for address and Quad SPI for data
    QLIB_BUS_MODE_4_4_4   = Q2_SEC_INST__QUAD | 0x6,   ///< Quad SPI for command, Quad SPI for address and Quad SPI for data
    QLIB_BUS_MODE_AUTOSENSE,                           ///< Autosense
} QLIB_BUS_MODE_T;

/************************************************************************************************************
 * This enumeration defines the Authenticated watchdog threshold
************************************************************************************************************/
typedef enum QLIB_AWDT_TH_T
{
    QLIB_AWDT_TH_1_SEC      = 0,
    QLIB_AWDT_TH_2_SEC      = 1,
    QLIB_AWDT_TH_4_SEC      = 2,
    QLIB_AWDT_TH_8_SEC      = 3,
    QLIB_AWDT_TH_16_SEC     = 4,
    QLIB_AWDT_TH_32_SEC     = 5,
    QLIB_AWDT_TH_1_MINUTES  = 6,
    QLIB_AWDT_TH_2_MINUTES  = 7,
    QLIB_AWDT_TH_4_MINUTES  = 8,
    QLIB_AWDT_TH_8_MINUTES  = 9,
    QLIB_AWDT_TH_17_MINUTES = 10,
    QLIB_AWDT_TH_34_MINUTES = 11,
    QLIB_AWDT_TH_1_HOURS    = 12,
    QLIB_AWDT_TH_2_HOURS    = 13,
    QLIB_AWDT_TH_4_HOURS    = 14,
    QLIB_AWDT_TH_9_HOURS    = 15,
    QLIB_AWDT_TH_18_HOURS   = 16,
    QLIB_AWDT_TH_36_HOURS   = 17,
    QLIB_AWDT_TH_72_HOURS   = 18,
    QLIB_AWDT_TH_6_DAYS     = 19,
    QLIB_AWDT_TH_12_DAYS    = 20,
} QLIB_AWDT_TH_T;

/************************************************************************************************************
 * This enumeration defines the session integrity check type
************************************************************************************************************/
typedef enum QLIB_INTEGRITY_T
{
    QLIB_INTEGRITY_CRC,    ///< CRC type integrity
    QLIB_INTEGRITY_DIGEST, ///< digest type integrity
} QLIB_INTEGRITY_T;

/************************************************************************************************************
 * This enumeration defines the session access type to open
************************************************************************************************************/
typedef enum QLIB_SESSION_ACCESS_T
{
    QLIB_SESSION_ACCESS_RESTRICTED,  ///< Open session using restricted key
    QLIB_SESSION_ACCESS_CONFIG_ONLY, ///< Open session just for configuration - ignoring the validity of SCRi
    QLIB_SESSION_ACCESS_FULL,        ///< Open session using full key
} QLIB_SESSION_ACCESS_T;

/************************************************************************************************************
 * Key type
************************************************************************************************************/
typedef enum
{
    QLIB_KID__RESTRICTED_ACCESS_SECTION = 0x00,
    QLIB_KID__FULL_ACCESS_SECTION       = 0x10,
    QLIB_KID__SECTION_PROVISIONING      = 0x20,
    QLIB_KID__DEVICE_SECRET = 0x8F,
    QLIB_KID__DEVICE_MASTER = 0x9F,
    QLIB_KID__DEVICE_KEY_PROVISIONING = 0xAF,
    QLIB_KID__INVALID                 = 0xFF,
} QLIB_KID_TYPE_T;

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             BASIC INCLUDES                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define __QLIB_PLATFORM_INCLUDED__
#include "qlib_platform.h"
#include "qlib_defs.h"
#include "qlib_errors.h"
#include "qlib_debug.h"
#include "qlib_sec_regs.h"
#include "qlib_sec_cmds.h"
#include "qlib_std_cmds.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             DEPENDENT TYPES                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * SPI bus format including dtr
************************************************************************************************************/
typedef U32 QLIB_BUS_FORMAT_T;

#define QLIB_BUS_FORMAT(mode, dtr, enter_exit_qpi) \
    MAKE_32_BIT((mode), ((dtr) == TRUE ? 1 : 0), ((enter_exit_qpi) == TRUE ? 1 : 0), 0)
#define QLIB_BUS_FORMAT_AUTOSENSE                     QLIB_BUS_FORMAT(QLIB_BUS_MODE_AUTOSENSE, FALSE, TRUE)
#define QLIB_BUS_FORMAT_GET_MODE(busFormat)           ((QLIB_BUS_MODE_T)BYTE((busFormat), 0))
#define QLIB_BUS_FORMAT_GET_DTR(busFormat)            (1 == BYTE((busFormat), 1) ? TRUE : FALSE)
#define QLIB_BUS_FORMAT_GET_ENTER_EXIT_QPI(busFormat) (1 == BYTE((busFormat), 2) ? TRUE : FALSE)

/************************************************************************************************************
 * swap type
************************************************************************************************************/
typedef enum
{
    QLIB_SWAP_NO        = FALSE,            ///< do not perform swap after applying configuration
    QLIB_SWAP           = TRUE,             ///< perform swap after applying configuration
    QLIB_SWAP_AND_RESET = FALSE + TRUE + 1, ///< perform swap and then perform CPU reset
} QLIB_SWAP_T;

/************************************************************************************************************
 * This type contains the value of Q2 WID
************************************************************************************************************/
typedef _64BIT QLIB_WID_T;

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * Flash ID - Standard part
************************************************************************************************************/
typedef struct
{
    U64 uniqueID; ///< unique ID
} QLIB_STD_ID_T;
#endif // QLIB_SEC_ONLY

/************************************************************************************************************
 * Flash ID - secure part
************************************************************************************************************/
typedef struct
{
    _128BIT    suid; ///< secure user ID
    QLIB_WID_T wid;  ///< Winbond ID
} QLIB_SEC_ID_T;

/************************************************************************************************************
 * Flash monotonic counter
************************************************************************************************************/
typedef U32 QLIB_MC_T[2];

/************************************************************************************************************
 * Flash monotonic counter offsets
************************************************************************************************************/
typedef enum
{
    TC  = 0, ///< Offset to TC part in @ref QLIB_MC_T
    DMC = 1, ///< Offset to DMC part in @ref QLIB_MC_T
} QLIB_MC_OFFSET_T;

/************************************************************************************************************
 * Flash ID
************************************************************************************************************/
typedef struct
{
#ifndef QLIB_SEC_ONLY
    QLIB_STD_ID_T std; ///< standard ID
#endif                 // QLIB_SEC_ONLY
    QLIB_SEC_ID_T sec; ///< secure ID
} QLIB_ID_T;

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * Hardware Version - Standard part
************************************************************************************************************/
typedef struct
{
    U8 manufacturerID; ///< Manufacture ID
    U8 memoryType;     ///< Memory type
    U8 capacity;       ///< Capacity
    U8 deviceID;       ///< Device ID
} QLIB_STD_HW_VER_T;
#endif

/************************************************************************************************************
 * Hardware Version - Secure part
************************************************************************************************************/
typedef struct
{
    U8 flashVersion;    ///< Flash version
    U8 securityVersion; ///< Security Protocol Version
    U8 revision;        ///< Hardware revision
    U8 flashSize;       ///< Flash size
} QLIB_SEC_HW_VER_T;

/************************************************************************************************************
 * Hardware Version
************************************************************************************************************/
typedef struct
{
#ifndef QLIB_SEC_ONLY
    QLIB_STD_HW_VER_T std; ///< standard HW information
#endif
    QLIB_SEC_HW_VER_T sec; ///< secure HW version
} QLIB_HW_VER_T;

/*---------------------------------------------------------------------------------------------------------*/
/* Keys                                                                                                    */
/*---------------------------------------------------------------------------------------------------------*/
typedef _128BIT       KEY_T;
typedef KEY_T         KEY_ARRAY_T[QLIB_NUM_OF_SECTIONS];
typedef const U32*    CONST_KEY_P_T;
typedef CONST_KEY_P_T KEY_P_ARRAY_T[QLIB_NUM_OF_SECTIONS];

/************************************************************************************************************
 * Key ID
************************************************************************************************************/
typedef U8 QLIB_KID_T;

/************************************************************************************************************
 * Secure read data structure
************************************************************************************************************/
typedef union QLIB_SEC_READ_DATA_T
{
    U32 asArray[(sizeof(U32) + sizeof(_256BIT) + sizeof(_64BIT)) / sizeof(U32)];
    struct
    {
        U32     tc;
        _256BIT data;
        _64BIT  sig;
    } asStruct;
} QLIB_SEC_READ_DATA_T;

/************************************************************************************************************
 * HASH buffer data structure
************************************************************************************************************/
typedef U32 QLIB_HASH_BUF_T[15]; // Add extra U32 for signature of SARD command

#define QLIB_HASH_BUF_OFFSET__KEY       0
#define QLIB_HASH_BUF_OFFSET__CTAG      4
#define QLIB_HASH_BUF_OFFSET__DATA      5
#define QLIB_HASH_BUF_OFFSET__CTRL      13
#define QLIB_HASH_BUF_OFFSET__READ_PAGE (QLIB_HASH_BUF_OFFSET__DATA - 1)
#define QLIB_HASH_BUF_OFFSET__READ_TC   QLIB_HASH_BUF_OFFSET__READ_PAGE
#define QLIB_HASH_BUF_OFFSET__READ_SIG  (QLIB_HASH_BUF_OFFSET__DATA + 8)

#define QLIB_HASH_BUF_GET__KEY(buf)       (&((buf)[QLIB_HASH_BUF_OFFSET__KEY]))
#define QLIB_HASH_BUF_GET__CTAG(buf)      ((buf)[QLIB_HASH_BUF_OFFSET__CTAG])
#define QLIB_HASH_BUF_GET__DATA(buf)      (&((buf)[QLIB_HASH_BUF_OFFSET__DATA]))
#define QLIB_HASH_BUF_GET__CTRL(buf)      ((buf)[QLIB_HASH_BUF_OFFSET__CTRL])
#define QLIB_HASH_BUF_GET__READ_PAGE(buf) (&((buf)[QLIB_HASH_BUF_OFFSET__READ_PAGE]))
#define QLIB_HASH_BUF_GET__READ_TC(buf)   ((buf)[QLIB_HASH_BUF_OFFSET__READ_TC])
#define QLIB_HASH_BUF_GET__READ_SIG(buf)  (&((buf)[QLIB_HASH_BUF_OFFSET__READ_SIG]))

/************************************************************************************************************
 * crypto context of a command
************************************************************************************************************/
typedef struct QLIB_CRYPTO_CONTEXT_T
{
    QLIB_HASH_BUF_T hashBuf;
    _256BIT         cipherKey;
} QLIB_CRYPTO_CONTEXT_T;

/************************************************************************************************************
 * PRNG state
************************************************************************************************************/
typedef struct
{
    U64 state;
    U8  count;
} QLIB_PRNG_STATE_T;

/************************************************************************************************************
 * key manager state
************************************************************************************************************/
typedef struct QLIB_KEY_MNGR_T
{
    KEY_P_ARRAY_T restrictedKeys; ///< Array of restricted keys
    KEY_P_ARRAY_T fullAccessKeys; ///< Array of full access keys
    KEY_T         sessionKey;     ///< Session key
    U8            kid;            ///< Session KID (key ID)

    U8                    cmdContexIndex;
    QLIB_CRYPTO_CONTEXT_T cmdContexArr[2];

} QLIB_KEY_MNGR_T;

#define QLIB_KEY_MNGR__CMD_CONTEXT_GET(qlibContext) ((qlibContext)->keyMngr.cmdContexArr[(qlibContext)->keyMngr.cmdContexIndex])
#define QLIB_KEY_MNGR__CMD_CONTEXT_GET_NEXT(qlibContext) \
    ((qlibContext)->keyMngr.cmdContexArr[(qlibContext)->keyMngr.cmdContexIndex ^ 0x1])
#define QLIB_KEY_MNGR__CMD_CONTEXT_ADVANCE(qlibContext) ((qlibContext)->keyMngr.cmdContexIndex ^= 0x1)

/************************************************************************************************************
 * This type contains section policy configuration
************************************************************************************************************/
typedef struct QLIB_POLICY_T
{
    U8 digestIntegrity : 1;        ///< perform digest integrity check
    U8 checksumIntegrity : 1;      ///< perform checksum integrity check
    U8 writeProt : 1;              ///< write protected section
    U8 rollbackProt : 1;           ///< rollback protected section
    U8 plainAccessWriteEnable : 1; ///< allow plain write access
    U8 plainAccessReadEnable : 1;  ///< allow plain read access
    U8 authPlainAccess : 1;        ///< plain access require @ref QLIB_OpenSession
} QLIB_POLICY_T;

/************************************************************************************************************
 * This type contains a section configuration
************************************************************************************************************/
typedef struct QLIB_SECTION_CONFIG_T
{
    U32           baseAddr; ///< flash physical address
    U32           size;     ///< section size. When the size equals to 0 the section is disabled.
    QLIB_POLICY_T policy;   ///< section policy
} QLIB_SECTION_CONFIG_T;

/************************************************************************************************************
 * Section configuration table
************************************************************************************************************/
typedef QLIB_SECTION_CONFIG_T QLIB_SECTION_CONFIG_TABLE_T[QLIB_NUM_OF_SECTIONS];

/************************************************************************************************************
 * This type contains Authenticated watchdog configuration
************************************************************************************************************/
typedef struct QLIB_WATCHDOG_CONF_T
{
    BOOL           enable;        ///< Authenticated watchdog enable
    BOOL           lfOscEn;       ///< Low frequency oscillator enable
    BOOL           swResetEn;     ///< Software reset enable
    BOOL           authenticated; ///< Requires authentication (signature) for its configurations and reset
    U32            sectionID;     ///< Section ID of the key associated with the AWDT function
    QLIB_AWDT_TH_T threshold;     ///< Watchdog expires when the timer reaches this value
    BOOL           lock;          ///< AWDTCFG register is locked
    U32            oscRateHz;     ///< Frequency of the internal oscillator, used for calibration. Set to 0 if non-applicable
} QLIB_WATCHDOG_CONF_T;

/************************************************************************************************************
 * This type contains IO2 muxing options
************************************************************************************************************/
typedef enum QLIB_IO23_MODE_T
{
    QLIB_IO23_MODE__LEGACY_WP_HOLD,
    QLIB_IO23_MODE__RESET_IN_OUT,
    QLIB_IO23_MODE__QUAD,
} QLIB_IO23_MODE_T;

/************************************************************************************************************
 * This type contains W77Q pin muxing options
************************************************************************************************************/
typedef struct QLIB_PIN_MUX_T
{
    QLIB_IO23_MODE_T io23Mux;            // Set IO2 and IO3 mux type (LEGACY/RESET/QUAD)
    BOOL             dedicatedResetInEn; // Set TRUE to enable dedicated RSTIN#
} QLIB_PIN_MUX_T;

/************************************************************************************************************
* This type contains reset response buffers
************************************************************************************************************/
typedef struct QLIB_RESET_RESPONSE_T
{
    U32 response1[_64B_ / sizeof(U32)];
    U32 response2[_64B_ / sizeof(U32)];
} QLIB_RESET_RESPONSE_T;

/************************************************************************************************************
* Standard commands address mode
************************************************************************************************************/
typedef enum
{
    QLIB_STD_ADDR_MODE__3_BYTE, ///< 3 bytes address mode
} QLIB_STD_ADDR_MODE_T;

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
* Standard logical address length
************************************************************************************************************/
typedef enum
{
    QLIB_STD_ADDR_LEN__22_BIT = 19, ///< address is 3 bits of SID and offset of 19 bits
    QLIB_STD_ADDR_LEN__23_BIT = 20, ///< address is 3 bits of SID and offset of 20 bits
    QLIB_STD_ADDR_LEN__24_BIT = 21, ///< address is 3 bits of SID and offset of 21 bits
    QLIB_STD_ADDR_LEN__25_BIT = 22, ///< address is 3 bits of SID and offset of 22 bits
#if LOG2(QLIB_MAX_STD_ADDR_SIZE) > 22
    QLIB_STD_ADDR_LEN__26_BIT = 23, ///< address is 3 bits of SID and offset of 23 bits
    QLIB_STD_ADDR_LEN__27_BIT = 24, ///< address is 3 bits of SID and offset of 24 bits
#endif
    QLIB_STD_ADDR_LEN__LAST
} QLIB_STD_ADDR_LEN_T;
#define QLIB_STD_ADDR_LEN_TO_ADDRESS_OFFSET_SIZE(len) ((U32)len)

/************************************************************************************************************
* This type contains the standard commands address size
************************************************************************************************************/
typedef struct QLIB_STD_ADDR_SIZE_T
{
    QLIB_STD_ADDR_LEN_T addrLen;
} QLIB_STD_ADDR_SIZE_T;
#endif //QLIB_SEC_ONLY
/************************************************************************************************************
 * This type contains general device configuration
************************************************************************************************************/
typedef struct QLIB_DEVICE_CONF_T
{
    /********************************************************************************************************
     * Reset response (1 & 2) value to be sent following the Flash power on reset.
     * If value is all zeros, reset response is disabled
     *******************************************************************************************************/
    QLIB_RESET_RESPONSE_T resetResp;
    BOOL                  safeFB;            ///< Safe fall back Enabled
    BOOL                  speculCK;          ///< Speculative Cypher Key Generation
    BOOL                  nonSecureFormatEn; ///< Non-secure FORMAT enable
    QLIB_PIN_MUX_T        pinMux;            ///< Pin muxing configuration
#ifndef QLIB_SEC_ONLY
    QLIB_STD_ADDR_SIZE_T stdAddrSize; ///< Address size configuration
#endif
} QLIB_DEVICE_CONF_T;

/************************************************************************************************************
 * Bus interface
************************************************************************************************************/
PACKED_START
typedef struct QLIB_INTERFACE_T
{
    BOOL            dtr;              ///< Double Transfer Rate enable
    QLIB_BUS_MODE_T busMode;          ///< General bus mode to use, not all the commands support all the bus modes
    QLIB_BUS_MODE_T secureCmdsFormat; ///< Secure commands transfer format
    U8              op0;              ///< Opcode for secure "get SSR"
    U8              op1;              ///< Opcode for secure "write input buffer"
    U8              op2;              ///< Opcode for secure "read output buffer"
    U8              padding;          ///
    BOOL            busIsLocked;      ///< Mutex - Marks the physical link to the flash is in use
} PACKED QLIB_INTERFACE_T;
PACKED_END

/************************************************************************************************************
 * Session state
************************************************************************************************************/
typedef struct QLIB_SECTION_STATE_T
{
    U8 sizeTag : 3;      ///< Section size tag representing the section size
    U8 enabled : 1;      ///< Section is enabled
    U8 plainEnabled : 1; ///< Section is plain read or plain write enabled
} QLIB_SECTION_STATE_T;


/************************************************************************************************************
 * QLIB Reset Status
************************************************************************************************************/
typedef struct QLIB_RESET_STATUS_T
{
    U32 powerOnReset : 1;      // 1==HW reset, 0==SW reset (by software command)
    U32 fallbackRemapping : 1; // 1==Remapping occurred
    U32 watchdogReset : 1;     // 1==Watchdog reset occurred
} QLIB_RESET_STATUS_T;

/************************************************************************************************************
 * QLIB context structure\n
 * [QLIB internal state](md_definitions.html#DEF_CONTEXT)
************************************************************************************************************/
typedef struct
{
    QLIB_INTERFACE_T busInterface;            ///< Bus interface configuration
    QLIB_WID_T       wid;                     ///< Winbond ID
    QLIB_MC_T        mc;                      ///< Monotonic counter
    U32              mcInSync : 1;            ///< Monotonic counter is synced indication
    U32              watchdogIsSecure : 1;    ///< Watchdog is secure indication
    U32              isSuspended : 1;         ///< The flash is in suspended state indication
    U32              isPoweredDown : 1;       ///< The flash is in powered-down state indication
    U32              multiTransactionCmd : 1; ///< Qlib runs SecureRead/Write commands which can cause many hw transactions
    U32              watchdogSectionId;       ///< Section key used for Secure Watchdog
    U32              addrSize;                ///< Standard address size
    QLIB_KEY_MNGR_T      keyMngr;                             ///< Key manager
    QLIB_REG_SSR_T       ssr;                                 ///< Last SSR
    void*                userData;                            ///< Saved user data pointer
    QLIB_SECTION_STATE_T sectionsState[QLIB_NUM_OF_SECTIONS]; ///< section state and configuration
    QLIB_PRNG_STATE_T    prng;                                ///< PRNG state
    QLIB_RESET_STATUS_T resetStatus; ///< Last Reset status
} QLIB_CONTEXT_T;

/************************************************************************************************************
 * Synchronization object
************************************************************************************************************/
PACKED_START
typedef struct QLIB_SYNC_OBJ_T
{
    QLIB_INTERFACE_T busInterface;
    QLIB_WID_T       wid;
    QLIB_RESET_STATUS_T resetStatus;
} PACKED QLIB_SYNC_OBJ_T;
PACKED_END


/************************************************************************************************************
 * QLIB notifications
************************************************************************************************************/
typedef struct QLIB_NOTIFICATIONS_T
{
    U8 mcMaintenance : 1; ///< Monotonic Counter maintenance is needed
    U8 resetDevice : 1;   ///< Device reset is needed since Transaction Counter is close to its max value
    U8 replaceDevice : 1; ///< Device replacement is needed since DMC is close to its max value
} QLIB_NOTIFICATIONS_T;

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            DEPENDENT INCLUDES                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_std.h"
#include "qlib_sec.h"
#include "qlib_cmd_proc.h"
#include "qlib_tm.h"
#include "qlib_crypto.h"
#include "qlib_version.h"
#include "qlib_key_mngr.h"
#include "qlib.h"

#endif // __QLIB_COMMON_H__
