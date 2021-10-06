/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_qconf.h
* @brief      This file contains QLIB QCONF sample related headers
*
* ### project qlib_samples
*
************************************************************************************************************/

#ifndef _QLIB_SAMPLE_QCONF__H_
#define _QLIB_SAMPLE_QCONF__H_

#include "../qconf/qconf.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            DEFINITIONS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

// Section address and size
#define BOOT_SECTION_INDEX Q2_BOOT_SECTION
#define BOOT_SECTION_BASE  (0)
#define BOOT_SECTION_SIZE  (_1MB_)

#define PA_WRITE_PROTECT_SECTION_INDEX 1
#define PA_WRITE_PROTECT_SECTION_BASE  (BOOT_SECTION_BASE + BOOT_SECTION_SIZE)
#define PA_WRITE_PROTECT_SECTION_SIZE  (_512KB_)

#define DIGEST_WRITE_PROTECTED_SECTION_INDEX 2
#define DIGEST_WRITE_PROTECTED_SECTION_BASE  (PA_WRITE_PROTECT_SECTION_BASE + PA_WRITE_PROTECT_SECTION_SIZE)
#define DIGEST_WRITE_PROTECTED_SECTION_SIZE  (_256KB_)

#define FW_UPDATE_SECTION_INDEX 3
#define FW_UPDATE_SECTION_BASE  (DIGEST_WRITE_PROTECTED_SECTION_BASE + DIGEST_WRITE_PROTECTED_SECTION_SIZE)
#define FW_UPDATE_SECTION_SIZE  (_512KB_)

#define SECURE_DATA_SECTION_INDEX 4
#define SECURE_DATA_SECTION_BASE  (FW_UPDATE_SECTION_BASE + FW_UPDATE_SECTION_SIZE)
#define SECURE_DATA_SECTION_SIZE  (_256KB_)

#define EMPTY_SECTION_INDEX 5
#define EMPTY_SECTION_BASE  (0)
#define EMPTY_SECTION_SIZE  (0)

#define FW_STORAGE_SECTION_INDEX 6
#define FW_STORAGE_SECTION_BASE  (SECURE_DATA_SECTION_BASE + SECURE_DATA_SECTION_SIZE)
#define FW_STORAGE_SECTION_SIZE  (_1MB_)

#define RECOVERY_SECTION_INDEX Q2_BOOT_SECTION_FALLBACK
#define RECOVERY_SECTION_BASE  (FW_STORAGE_SECTION_BASE + FW_STORAGE_SECTION_SIZE)
#define RECOVERY_SECTION_SIZE  (_512KB_)

// Device Master key
#define QCONF_KD                                       \
    {                                                  \
        0x11111111, 0x12121212, 0x13131313, 0x14141414 \
    }

// Device Secret Key
#define QCONF_KDS                                      \
    {                                                  \
        0x15151515, 0x16161616, 0x17171717, 0x18181818 \
    }

// Secure User ID
#define QCONF_SUID                                     \
    {                                                  \
        0x19191919, 0x10101010, 0x21212121, 0x22222222 \
    }

// Section 0 Full access key
#define QCONF_FULL_ACCESS_K_0                          \
    {                                                  \
        0x23232323, 0x24242424, 0x25252525, 0x26262626 \
    }

// Section 1 Full access key
#define QCONF_FULL_ACCESS_K_1                          \
    {                                                  \
        0x27272727, 0x28282828, 0x29292929, 0x20202020 \
    }

// Section 2 Full access key
#define QCONF_FULL_ACCESS_K_2                          \
    {                                                  \
        0x31313131, 0x32323232, 0x33333333, 0x34343434 \
    }

// Section 3 Full access key
#define QCONF_FULL_ACCESS_K_3                          \
    {                                                  \
        0x35353535, 0x36363636, 0x37373737, 0x38383838 \
    }

// Section 4 Full access key
#define QCONF_FULL_ACCESS_K_4                          \
    {                                                  \
        0x39393939, 0x30303030, 0x41414141, 0x42424242 \
    }

// Section 5 Full access key
#define QCONF_FULL_ACCESS_K_5                          \
    {                                                  \
        0x43434343, 0x44444444, 0x45454545, 0x46464646 \
    }

// Section 6 Full access key
#define QCONF_FULL_ACCESS_K_6                          \
    {                                                  \
        0x47474747, 0x48484848, 0x49494949, 0x40404040 \
    }

// Section 7 Full access key
#define QCONF_FULL_ACCESS_K_7                          \
    {                                                  \
        0x51515151, 0x52525252, 0x53535353, 0x54545454 \
    }

// Section 0 restricted access key
#define QCONF_RESTRICTED_K_0                           \
    {                                                  \
        0x55555555, 0x56565656, 0x57575757, 0x58585858 \
    }

// Section 1 restricted access key
#define QCONF_RESTRICTED_K_1                           \
    {                                                  \
        0x59595959, 0x50505050, 0x61616161, 0x62626262 \
    }

// Section 2 restricted access key
#define QCONF_RESTRICTED_K_2                           \
    {                                                  \
        0x63636363, 0x64646464, 0x65656565, 0x66666666 \
    }

// Section 3 restricted access key
#define QCONF_RESTRICTED_K_3                           \
    {                                                  \
        0x67676767, 0x68686868, 0x69696969, 0x60606060 \
    }

// Section 4 restricted access key
#define QCONF_RESTRICTED_K_4                           \
    {                                                  \
        0x71717171, 0x72727272, 0x73737373, 0x74747474 \
    }

// Section 5 restricted access key
#define QCONF_RESTRICTED_K_5                           \
    {                                                  \
        0x75757575, 0x76767676, 0x77777777, 0x78787878 \
    }

// Section 6 restricted access key
#define QCONF_RESTRICTED_K_6                           \
    {                                                  \
        0x79797979, 0x70707070, 0x81818181, 0x82828282 \
    }

// Section 7 restricted access key
#define QCONF_RESTRICTED_K_7                           \
    {                                                  \
        0x83838383, 0x84848484, 0x85858585, 0x86868686 \
    }

/************************************************************************************************************
 * @brief       This routine shows QCONF configuration flow with initialization sequence
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_QconfConfigRun(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
 * @brief       This routine shows QCONF recovery flow with initialization sequence
 *
 * @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_QconfRecoveryRun(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
* @brief       This routine shows QCONF configuration flow, assuming initialization already executed
*
* @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
*
* @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_QconfConfig(QLIB_CONTEXT_T* qlibContext);

/************************************************************************************************************
* @brief       This routine shows QCONF recovery flow, assuming initialization already executed
*
* @param[out]  qlibContext     [QLIB internal state](md_definitions.html#DEF_CONTEXT)
*
* @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_QconfRecovery(QLIB_CONTEXT_T* qlibContext);

#endif // _QLIB_SAMPLE_QCONF__H_
