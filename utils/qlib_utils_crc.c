/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2021 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_utils_crc.c
* @brief      This file contains CRC utility functions
*
* ### project qlib
*
************************************************************************************************************/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_utils_crc.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                DEFINITIONS                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define QLIB_UTILS_CRC_READ_BUFFER_SIZE _256B_
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                CONSTANTS                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static const U32 table32[] = {
    0xb8bc6765, 0xaa09c88b, 0x8f629757, 0xc5b428ef, 0x5019579f, 0xa032af3e, 0x9b14583d, 0xed59b63b,
    0x01c26a37, 0x0384d46e, 0x0709a8dc, 0x0e1351b8, 0x1c26a370, 0x384d46e0, 0x709a8dc0, 0xe1351b80,
    0x191b3141, 0x32366282, 0x646cc504, 0xc8d98a08, 0x4ac21251, 0x958424a2, 0xf0794f05, 0x3b83984b,
    0x77073096, 0xee0e612c, 0x076dc419, 0x0edb8832, 0x1db71064, 0x3b6e20c8, 0x76dc4190, 0xedb88320,
};

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                          FORWARD DECLARATION                                            */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static U32 QLIB_UTILS_CRC_churn32_L(U32 x);

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
QLIB_STATUS_T QLIB_UTILS_CalcCRCWithPadding(const U32* buf, U32 size, U32 padValue, U32 padSize, U32* crc)
{
    U32 res = 0xFFFFFFFFL;
    U32 i   = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (NULL == crc)
    {
        return QLIB_STATUS__INVALID_PARAMETER;
    }

    if (NULL == buf)
    {
        *crc = 0;
        return QLIB_STATUS__INVALID_PARAMETER;
    }

    //size is multiple of 4 bytes
    QLIB_ASSERT_RET(0 == (size % (sizeof(U32))), QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(0 == (padSize % (sizeof(U32))), QLIB_STATUS__INVALID_PARAMETER);

    for (i = 0; i < size / sizeof(U32); ++i)
    {
        res = QLIB_UTILS_CRC_churn32_L(buf[i] ^ res);
    }

    for (i = 0; i < padSize / sizeof(U32); ++i)
    {
        res = QLIB_UTILS_CRC_churn32_L(padValue ^ res);
    }

    *crc = (res ^ 0xFFFFFFFF);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_UTILS_CalcCRCForSection(QLIB_CONTEXT_T* qlibContext, U32 sectionId, U32 offset, U32 dataSize, U32* crc)
{
    U32 res = 0xFFFFFFFFL;
    U32 i   = 0;
    U32 readBuf[QLIB_UTILS_CRC_READ_BUFFER_SIZE / sizeof(U32)];
    U32 readSize;

    while (dataSize > 0)
    {
        readSize = MIN(dataSize, sizeof(readBuf));
        QLIB_STATUS_RET_CHECK(QLIB_Read(qlibContext, (U8*)readBuf, sectionId, offset, readSize, TRUE, FALSE));
        for (i = 0; i < (readSize / sizeof(U32)); i++)
        {
            res  = QLIB_UTILS_CRC_churn32_L(readBuf[i] ^ res);
        }
        dataSize -= readSize;
        offset += readSize;
    }
    *crc = (res ^ 0xFFFFFFFF);

    return QLIB_STATUS__OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static U32 QLIB_UTILS_CRC_churn32_L(U32 x)
{
    U32 res = 0;
    U32 i   = 0;
    for (i = 0; i < 32; i++)
    {
        if ((x & 1) == 1)
        {
            res ^= table32[i];
        }
        x >>= 1;
    }
    return res;
}
