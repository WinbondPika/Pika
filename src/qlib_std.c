/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_std.c
* @brief      This file contains QLIB standard flash interface
*
* ### project qlib
*
************************************************************************************************************/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_std.h"

#ifdef QLIB_SUPPORT_QPI
#include "qlib_sec.h"
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                              DEFINITIONS                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define AUTOSENSE_NUM_RETRIES 4
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static QLIB_STATUS_T QLIB_STD_GetStatus_L(QLIB_CONTEXT_T* context, STD_FLASH_STATUS_T* status);
static QLIB_STATUS_T QLIB_STD_execute_std_cmd_L(QLIB_CONTEXT_T* qlibContext,
                                                QLIB_BUS_MODE_T format,
                                                BOOL            dtr,
                                                BOOL            needWriteEnable,
                                                BOOL            waitWhileBusy,
                                                U8              cmd,
                                                const U32*      address,
                                                const U8*       writeData,
                                                U32             writeDataSize,
                                                U32             dummyCycles,
                                                U8*             readData,
                                                U32             readDataSize,
                                                QLIB_REG_SSR_T* ssr) __RAM_SECTION;

static QLIB_STATUS_T QLIB_STD_SetQuadEnable_L(QLIB_CONTEXT_T* qlibContext, BOOL enable);
static QLIB_STATUS_T QLIB_STD_SetResetInEnable_L(QLIB_CONTEXT_T* qlibContext, BOOL enable);
static QLIB_STATUS_T QLIB_STD_SetStatus_L(QLIB_CONTEXT_T*     qlibContext,
                                          STD_FLASH_STATUS_T  statusIn,
                                          STD_FLASH_STATUS_T* statusOut);
#ifndef QLIB_SEC_ONLY
static QLIB_STATUS_T QLIB_STD_PageProgram_L(QLIB_CONTEXT_T* qlibContext,
                                            const U8*       input,
                                            U32             logicalAddr,
                                            U32             size,
                                            BOOL            blocking);
#ifdef QLIB_SUPPORT_QPI
static QLIB_STATUS_T QLIB_STD_EnterExitQPI_L(QLIB_CONTEXT_T* qlibContext, BOOL enter);
#endif // QLIB_SUPPORT_QPI
static U8 QLIB_STD_GetReadCMD_L(QLIB_CONTEXT_T* qlibContext, U32* dummyCycles, QLIB_BUS_MODE_T* format);
static U8 QLIB_STD_GetWriteCMD_L(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_MODE_T* format);
static U8 QLIB_STD_GetReadDummyCyclesCMD_L(QLIB_BUS_MODE_T busMode, BOOL dtr, U32* dummyCycles);
#ifdef QLIB_SUPPORT_QPI
static QLIB_STATUS_T QLIB_STD_CheckWritePrivilege_L(QLIB_CONTEXT_T* qlibContext, U32 logicalAddr);
#endif // QLIB_SUPPORT_QPI
#endif // QLIB_SEC_ONLY


BOOL QLIB_STD_AutoSenseCheckBusMode_L(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_MODE_T busMode);
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#ifndef QLIB_SEC_ONLY
QLIB_STATUS_T QLIB_STD_InitLib(QLIB_CONTEXT_T* qlibContext)
{
    TOUCH(qlibContext);

    return QLIB_STATUS__OK;
}
#endif // QLIB_SEC_ONLY

/************************************************************************************************************
 * @brief       This routine executes standard command using TM interface
 *
 * @param[in]   qlibContext       pointer to qlib context
 * @param[in]   format            SPI transaction format
 * @param[in]   dtr               If TRUE, SPI transaction is using DTR mode
 * @param[in]   needWriteEnable   If TRUE, WriteEnable command is sent prior to the given command
 * @param[in]   waitWhileBusy     If TRUE, busy-wait polling is performed after exec. of command
 * @param[in]   cmd               Command value
 * @param[in]   address           Pointer to address value or NULL if no address available
 * @param[in]   writeData         Pointer to output data or NULL if no output data available
 * @param[in]   writeDataSize     Size of the output data
 * @param[in]   dummyCycles       Delay Cycles between output and input command phases
 * @param[out]  readData          Pointer to input data or NULL if no input data required
 * @param[in]   readDataSize      Size of the input data
 * @param[out]  ssr              Pointer to status register following the transaction
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_execute_std_cmd_L(QLIB_CONTEXT_T* qlibContext,
                                         QLIB_BUS_MODE_T format,
                                         BOOL            dtr,
                                         BOOL            needWriteEnable,
                                         BOOL            waitWhileBusy,
                                         U8              cmd,
                                         const U32*      address,
                                         const U8*       writeData,
                                         U32             writeDataSize,
                                         U32             dummyCycles,
                                         U8*             readData,
                                         U32             readDataSize,
                                         QLIB_REG_SSR_T* ssr)
{
    U32  ssrTemp = 0;
    BOOL poweredDown;
    /*-----------------------------------------------------------------------------------------------------*/
    /* No commands allowed in powered-down state, except of SPI_FLASH_CMD__RELEASE_POWER_DOWN              */
    /*-----------------------------------------------------------------------------------------------------*/
    poweredDown = QLIB_VALUE_BY_FLASH_TYPE(qlibContext,
                                           qlibContext->isPoweredDown,
                                           qlibContext->stdState[qlibContext->activeDie - 1].isPoweredDown);
    if (poweredDown && cmd != SPI_FLASH_CMD__RELEASE_POWER_DOWN
    )
    {
        SET_VAR_FIELD(ssrTemp, QLIB_REG_SSR__IGNORE_ERR_S, 1);
        SET_VAR_FIELD(ssrTemp, QLIB_REG_SSR__ERR, 1);
        if (NULL != ssr)
        {
            ssr->asUint = ssrTemp;
        }
        return QLIB_STATUS__COMMAND_IGNORED;
    }

    QLIB_STATUS_RET_CHECK(QLIB_TM_Standard(qlibContext,
                                           QLIB_BUS_FORMAT(format, dtr, FALSE),
                                           needWriteEnable,
                                           waitWhileBusy,
                                           cmd,
                                           address,
                                           writeData,
                                           writeDataSize,
                                           dummyCycles,
                                           readData,
                                           readDataSize,
                                           ssr));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_AutoSense(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_MODE_T* busMode)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Release power down and check non-QPI mode                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_BUS_MODE_1_1_1,
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__RELEASE_POWER_DOWN,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     NULL,
                                                     0,
                                                     NULL));

    if (QLIB_STD_AutoSenseCheckBusMode_L(qlibContext, QLIB_BUS_MODE_1_4_4))
    {
        *busMode = QLIB_BUS_MODE_1_4_4;
        return QLIB_STATUS__OK;
    }
#ifdef QLIB_SUPPORT_DUAL_SPI
    else if (QLIB_STD_AutoSenseCheckBusMode_L(qlibContext, QLIB_BUS_MODE_1_2_2))
    {
        *busMode = QLIB_BUS_MODE_1_2_2;
        return QLIB_STATUS__OK;
    }
#endif
    else if (QLIB_STD_AutoSenseCheckBusMode_L(qlibContext, QLIB_BUS_MODE_1_1_1))
    {
        *busMode = QLIB_BUS_MODE_1_1_1;
        return QLIB_STATUS__OK;
    }

#ifdef QLIB_SUPPORT_QPI
    /*-----------------------------------------------------------------------------------------------------*/
    /* Release power down and  check QPI mode                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_BUS_MODE_4_4_4,
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__RELEASE_POWER_DOWN,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     NULL,
                                                     0,
                                                     NULL));

    if (QLIB_STD_AutoSenseCheckBusMode_L(qlibContext, QLIB_BUS_MODE_4_4_4))
    {
        *busMode = QLIB_BUS_MODE_4_4_4;
        return QLIB_STATUS__OK;
    }
#endif // QLIB_SUPPORT_QPI

    return QLIB_STATUS__CONNECTIVITY_ERR;
}

QLIB_STATUS_T QLIB_STD_SetInterface(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat)
{
#ifdef QLIB_SUPPORT_QPI
    QLIB_BUS_MODE_T formatCurrent = qlibContext->busInterface.busMode;
#endif // QLIB_SUPPORT_QPI
    QLIB_BUS_MODE_T format         = QLIB_BUS_FORMAT_GET_MODE(busFormat);
    BOOL            dtr            = QLIB_BUS_FORMAT_GET_DTR(busFormat);
    BOOL            enter_exit_qpi = QLIB_BUS_FORMAT_GET_ENTER_EXIT_QPI(busFormat);

#ifdef QLIB_SUPPORT_QPI
    if ((enter_exit_qpi == TRUE) && (format != formatCurrent))
    {
#ifndef QLIB_SEC_ONLY
        /*-------------------------------------------------------------------------------------------------*/
        /* Enter / Exit QPI if needed                                                                      */
        /*-------------------------------------------------------------------------------------------------*/
        if (QLIB_BUS_MODE_4_4_4 == format)
        {
            // enter QPI mode
            QLIB_STATUS_RET_CHECK(QLIB_STD_EnterExitQPI_L(qlibContext, TRUE));
        }
        else if (QLIB_BUS_MODE_4_4_4 == formatCurrent)
        {
            QLIB_STATUS_RET_CHECK(QLIB_STD_EnterExitQPI_L(qlibContext, FALSE));
        }
#else  // QLIB_SEC_ONLY
        return QLIB_STATUS__INVALID_PARAMETER;
#endif // QLIB_SEC_ONLY
    }
#else  // QLIB_SUPPORT_QPI
    QLIB_ASSERT_RET(FALSE == enter_exit_qpi, QLIB_STATUS__NOT_SUPPORTED);
    QLIB_ASSERT_RET(QLIB_BUS_MODE_4_4_4 != format, QLIB_STATUS__NOT_SUPPORTED);
#endif // QLIB_SUPPORT_QPI

    /*-----------------------------------------------------------------------------------------------------*/
    /* update rest of the globals                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    qlibContext->busInterface.busMode = format;
    qlibContext->busInterface.dtr     = dtr;

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_SetQuadEnable(QLIB_CONTEXT_T* qlibContext, BOOL enable)
{
    return QLIB_STD_SetQuadEnable_L(qlibContext, enable);
}

QLIB_STATUS_T QLIB_STD_SetResetInEnable(QLIB_CONTEXT_T* qlibContext, BOOL enable)
{
    return QLIB_STD_SetResetInEnable_L(qlibContext, enable);
}

#ifndef QLIB_SEC_ONLY
QLIB_STATUS_T QLIB_STD_Read(QLIB_CONTEXT_T* qlibContext, U8* output, U32 logicalAddr, U32 size)
{
    U8              readCMD     = 0;
    U32             dummyCycles = 0;
    QLIB_BUS_MODE_T format      = QLIB_BUS_MODE_INVALID;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(output != NULL, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get read command                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    readCMD = QLIB_STD_GetReadCMD_L(qlibContext, &dummyCycles, &format);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/

    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     format,
                                                     qlibContext->busInterface.dtr,
                                                     FALSE,
                                                     TRUE,
                                                     readCMD,
                                                     &logicalAddr,
                                                     NULL,
                                                     0,
                                                     dummyCycles,
                                                     output,
                                                     size,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(qlibContext,
                                       QLIB_STATUS_RET_CHECK(
                                           QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS)));


    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_Write(QLIB_CONTEXT_T* qlibContext, const U8* input, U32 logicalAddr, U32 size)
{
    U32 size_tmp = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(input != NULL, QLIB_STATUS__INVALID_PARAMETER);

    while (0 < size)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Calculate the iteration size                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        size_tmp = FLASH_PAGE_SIZE - (logicalAddr % FLASH_PAGE_SIZE);
        if (size < size_tmp)
        {
            size_tmp = size;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* One page program                                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_STD_PageProgram_L(qlibContext, input, logicalAddr, size_tmp, TRUE));

        /*-------------------------------------------------------------------------------------------------*/
        /* Update pointers for next iteration                                                              */
        /*-------------------------------------------------------------------------------------------------*/
        size -= size_tmp;
        logicalAddr += size_tmp;
        input += size_tmp;
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_PerformErase(QLIB_CONTEXT_T* qlibContext, QLIB_ERASE_T eraseType, U32 logicalAddr, BOOL blocking)
{
    U8 cmd = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check aligned address & calculate command                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    switch (eraseType)
    {
        case QLIB_ERASE_SECTOR_4K:
            cmd = SPI_FLASH_CMD__ERASE_SECTOR;
            break;

        case QLIB_ERASE_BLOCK_32K:
            cmd = SPI_FLASH_CMD__ERASE_BLOCK_32;
            break;

        case QLIB_ERASE_BLOCK_64K:
            cmd = SPI_FLASH_CMD__ERASE_BLOCK_64;
            break;

        case QLIB_ERASE_CHIP:
            cmd = SPI_FLASH_CMD__ERASE_CHIP;
            break;

        default:
            return QLIB_STATUS__INVALID_PARAMETER;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Send erase command (with write enable and wait while busy after)                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_ERASE_CHIP == eraseType)
    {
        QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                         QLIB_STD_GET_BUS_MODE(qlibContext),
                                                         FALSE,
                                                         TRUE,
                                                         blocking,
                                                         cmd,
                                                         NULL,
                                                         NULL,
                                                         0,
                                                         0,
                                                         NULL,
                                                         0,
                                                         QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));
    }
    else
    {
        QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                         QLIB_STD_GET_BUS_MODE(qlibContext),
                                                         FALSE,
                                                         TRUE,
                                                         blocking,
                                                         cmd,
                                                         &logicalAddr,
                                                         NULL,
                                                         0,
                                                         0,
                                                         NULL,
                                                         0,
                                                         QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ACTION_BY_FLASH_TYPE(
        qlibContext,
        { QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS)); },
        {
            if (blocking)
            {
                STD_FLASH_STATUS_T status;
                QLIB_STATUS_RET_CHECK(QLIB_STD_GetStatus_L(qlibContext, &status));
                QLIB_ASSERT_RET(0 == READ_VAR_FIELD(status.SR1.asUint, SPI_FLASH__STATUS_1_FIELD__WEL),
                                QLIB_STATUS__COMMAND_IGNORED);
            }
        });

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_Erase(QLIB_CONTEXT_T* qlibContext, U32 logicalAddr, U32 size)
{
    U32          eraseSize = 0;
    QLIB_ERASE_T eraseType = QLIB_ERASE_FIRST;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(0 == (logicalAddr % FLASH_SECTOR_SIZE), QLIB_STATUS__INVALID_DATA_ALIGNMENT);
    QLIB_ASSERT_RET(0 == (size % FLASH_SECTOR_SIZE), QLIB_STATUS__INVALID_DATA_ALIGNMENT);

#ifdef QLIB_SUPPORT_QPI
    QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(
        qlibContext,
        if (qlibContext->busInterface.busMode == QLIB_BUS_MODE_4_4_4) {
            QLIB_STATUS_RET_CHECK(QLIB_STD_CheckWritePrivilege_L(qlibContext, logicalAddr));
        });
#endif // QLIB_SUPPORT_QPI

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start erasing with optimal command                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    while (0 < size)
    {
        if (_64KB_ <= size && 0 == (logicalAddr % _64KB_))
        {
            eraseSize = _64KB_;
            eraseType = QLIB_ERASE_BLOCK_64K;
        }
        else if (_32KB_ <= size && 0 == (logicalAddr % _32KB_))
        {
            eraseSize = _32KB_;
            eraseType = QLIB_ERASE_BLOCK_32K;
        }
        else
        {
            eraseSize = FLASH_SECTOR_SIZE;
            eraseType = QLIB_ERASE_SECTOR_4K;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Start erase                                                                                     */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_STD_PerformErase(qlibContext, eraseType, logicalAddr, TRUE));

        /*-------------------------------------------------------------------------------------------------*/
        /* erase success                                                                                   */
        /*-------------------------------------------------------------------------------------------------*/
        logicalAddr += eraseSize;
        size -= eraseSize;
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_EraseSuspend(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform flash suspend and wait for flash device to finish                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     TRUE,
                                                     SPI_FLASH_CMD__SUSPEND,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     NULL,
                                                     0,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/

    QLIB_ACTION_BY_FLASH_TYPE(
        qlibContext,
        {
            QLIB_ASSERT_RET(0 == READ_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__IGNORE_ERR_S),
                            QLIB_STATUS__COMMAND_IGNORED);
        },
        {
            STD_FLASH_STATUS_T status;
            QLIB_STATUS_RET_CHECK(QLIB_STD_GetStatus_L(qlibContext, &status));
            QLIB_ASSERT_RET(1 == READ_VAR_FIELD(status.SR2.asUint, SPI_FLASH__STATUS_2_FIELD__SUS), QLIB_STATUS__COMMAND_IGNORED);
        });

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_EraseResume(QLIB_CONTEXT_T* qlibContext, BOOL blocking)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform flash resume                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     blocking,
                                                     SPI_FLASH_CMD__RESUME,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     NULL,
                                                     0,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ACTION_BY_FLASH_TYPE(
        qlibContext,
        {
            QLIB_ASSERT_RET(0 == READ_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__IGNORE_ERR_S),
                            QLIB_STATUS__COMMAND_IGNORED);
            QLIB_ASSERT_RET(0 == READ_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__SUSPEND_E), QLIB_STATUS__COMMAND_IGNORED);
            QLIB_ASSERT_RET(0 == READ_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__SUSPEND_W), QLIB_STATUS__COMMAND_IGNORED);
        },
        {
            STD_FLASH_STATUS_T status = {0};
            QLIB_STATUS_RET_CHECK(QLIB_STD_GetStatus_L(qlibContext, &status));
            QLIB_ASSERT_RET(0 == READ_VAR_FIELD(status.SR2.asUint, SPI_FLASH__STATUS_2_FIELD__SUS), QLIB_STATUS__COMMAND_IGNORED);
        });

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_Power(QLIB_CONTEXT_T* qlibContext, QLIB_POWER_T power)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Select power state                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    switch (power)
    {
        case QLIB_POWER_UP:
            QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                             QLIB_STD_GET_BUS_MODE(qlibContext),
                                                             FALSE,
                                                             FALSE,
                                                             TRUE,
                                                             SPI_FLASH_CMD__RELEASE_POWER_DOWN,
                                                             NULL,
                                                             NULL,
                                                             0,
                                                             0,
                                                             NULL,
                                                             0,
                                                             QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

            QLIB_ACTION_BY_FLASH_TYPE(qlibContext,
                                      qlibContext->isPoweredDown                                      = FALSE,
                                      qlibContext->stdState[qlibContext->activeDie - 1].isPoweredDown = FALSE);

            /*---------------------------------------------------------------------------------------------*/
            /* Error checking                                                                              */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(qlibContext,
                                               QLIB_STATUS_RET_CHECK(
                                                   QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS)));

            break;

        case QLIB_POWER_DOWN:
            QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                             QLIB_STD_GET_BUS_MODE(qlibContext),
                                                             FALSE,
                                                             FALSE,
                                                             FALSE,
                                                             SPI_FLASH_CMD__POWER_DOWN,
                                                             NULL,
                                                             NULL,
                                                             0,
                                                             0,
                                                             NULL,
                                                             0,
                                                             NULL));

            QLIB_ACTION_BY_FLASH_TYPE(qlibContext,
                                      qlibContext->isPoweredDown                                      = TRUE,
                                      qlibContext->stdState[qlibContext->activeDie - 1].isPoweredDown = TRUE);


            break;

        default:
            return QLIB_STATUS__INVALID_PARAMETER;
    }

    return QLIB_STATUS__OK;
}
#endif // QLIB_SEC_ONLY

QLIB_STATUS_T QLIB_STD_ResetFlash(QLIB_CONTEXT_T* qlibContext, BOOL forceReset)
{
    STD_FLASH_STATUS_T status;
    QLIB_BUS_MODE_T    preResetFormat = QLIB_STD_GET_BUS_MODE(qlibContext);
    QLIB_STATUS_T      ret            = QLIB_STATUS__OK;
    INTERRUPTS_VAR_DECLARE(ints);

    if (TRUE == forceReset)
    {
#ifndef QLIB_SEC_ONLY
        /*-------------------------------------------------------------------------------------------------*/
        /* Release from power down in case of power down (no error checking is needed)                     */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STD_Power(qlibContext, QLIB_POWER_UP);
#endif // QLIB_SEC_ONLY
    }
    else
    {
#ifndef QLIB_SEC_ONLY
            /*---------------------------------------------------------------------------------------------*/
            /* Release from power down in case of power down (no error checking is needed)                 */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_STD_Power(qlibContext, QLIB_POWER_UP);
#endif // QLIB_SEC_ONLY
            /*---------------------------------------------------------------------------------------------*/
            /* check that the device is not in a critical state (avoid corrupted data)                     */
            /*---------------------------------------------------------------------------------------------*/
            do
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Read status from flash                                                                  */
                /*-----------------------------------------------------------------------------------------*/
                QLIB_STATUS_RET_CHECK_GOTO(QLIB_STD_GetStatus_L(qlibContext, &status), ret, error);

            } while (0 != READ_VAR_FIELD(status.SR1.asUint, SPI_FLASH__STATUS_1_FIELD__BUSY) ||
                     0 != READ_VAR_FIELD(status.SR2.asUint, SPI_FLASH__STATUS_2_FIELD__SUS));

    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform the reset flow ("reset enable" & "reset device") without interruptions                      */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPTS_SAVE_DISABLE(ints);

    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     preResetFormat,
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__RESET_ENABLE,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     NULL,
                                                     0,
                                                     NULL));

#ifdef QLIB_SUPPORT_QPI
    if (QLIB_BUS_MODE_4_4_4 == preResetFormat)
    {
        // Set the bus mode to quad because the reset command exits the QPI mode
        qlibContext->busInterface.busMode          = QLIB_BUS_MODE_1_4_4;
        qlibContext->busInterface.secureCmdsFormat = QLIB_BUS_MODE_1_1_4;
    }
#endif // QLIB_SUPPORT_QPI

    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     preResetFormat,
                                                     FALSE,
                                                     FALSE,
                                                     TRUE,
                                                     SPI_FLASH_CMD__RESET_DEVICE,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     NULL,
                                                     0,
                                                     NULL));


    INTERRUPTS_RESTORE(ints);

#ifdef QLIB_SUPPORT_QPI
    /*-----------------------------------------------------------------------------------------------------*/
    /* Reset will automatic exits QPI mode, enter back if was in QPI before reset                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_BUS_MODE_4_4_4 == preResetFormat)
    {
        // Set the bus mode back to quad because the reset command exits the QPI mode
        QLIB_STATUS_RET_CHECK(QLIB_STD_ENTER_EXIT_QPI(qlibContext, TRUE));
        qlibContext->busInterface.busMode          = QLIB_BUS_MODE_4_4_4;
        qlibContext->busInterface.secureCmdsFormat = QLIB_BUS_MODE_4_4_4;
    }
#endif // QLIB_SUPPORT_QPI

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read SSR and check for errors (the above commands are executed without reading SSR)                 */
    /*-----------------------------------------------------------------------------------------------------*/
    //QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_SSR(qlibContext, NULL, SSR_MASK__ALL_ERRORS));

    goto exit;

error:
exit:
    return ret;
}

#ifndef QLIB_SEC_ONLY
QLIB_STATUS_T QLIB_STD_GetHwVersion(QLIB_CONTEXT_T* qlibContext, QLIB_STD_HW_VER_T* hwVer)
{
    U8 jedecId[3];

    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__READ_JEDEC,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     jedecId,
                                                     sizeof(jedecId),
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    hwVer->manufacturerID = jedecId[0];
    hwVer->memoryType     = jedecId[1];
    hwVer->capacity       = jedecId[2];

    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__DEVICE_ID,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext) == QLIB_BUS_MODE_4_4_4
                                                         ? SPI_FLASH_DUMMY_CYCLES__DEVICE_ID__4_4_4
                                                         : SPI_FLASH_DUMMY_CYCLES__DEVICE_ID__1_1_1,
                                                     &(hwVer->deviceID),
                                                     (sizeof(hwVer->deviceID)),
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(qlibContext,
                                       QLIB_STATUS_RET_CHECK(
                                           QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS)));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_STD_GetId(QLIB_CONTEXT_T* qlibContext, QLIB_STD_ID_T* id_p)
{
    QLIB_STATUS_T ret = QLIB_STATUS__OK;
    U32           dummyCycles;

#ifdef QLIB_SUPPORT_QPI
    BOOL exitQpi = FALSE;
    /*-----------------------------------------------------------------------------------------------------*/
    /* SPI_FLASH_CMD__READ_UNIQUE_ID is supported only in SPI mode                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_BUS_MODE_4_4_4 == qlibContext->busInterface.busMode)
    {
        QLIB_STATUS_RET_CHECK(
            QLIB_SetInterface(qlibContext, QLIB_BUS_FORMAT(QLIB_BUS_MODE_1_1_1, qlibContext->busInterface.dtr, TRUE)));
        exitQpi = TRUE;
    }
#endif //QLIB_SUPPORT_QPI

    dummyCycles = SPI_FLASH_DUMMY_CYCLES__UNIQUE_ID;

    QLIB_STATUS_RET_CHECK_GOTO(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                          QLIB_BUS_MODE_1_1_1,
                                                          FALSE,
                                                          FALSE,
                                                          FALSE,
                                                          SPI_FLASH_CMD__READ_UNIQUE_ID,
                                                          NULL,
                                                          NULL,
                                                          0,
                                                          dummyCycles,
                                                          (U8*)&(id_p->uniqueID),
                                                          sizeof(id_p->uniqueID),
                                                          QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)),
                               ret,
                               error);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_EXECUTE_FOR_SECURE_FLASH_ONLY(qlibContext,
                                       QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext,
                                                                                                    SSR_MASK__ALL_ERRORS),
                                                                  ret,
                                                                  error));

error:
#ifdef QLIB_SUPPORT_QPI
    if (exitQpi)
    {
        QLIB_STATUS_RET_CHECK(
            QLIB_SetInterface(qlibContext, QLIB_BUS_FORMAT(QLIB_BUS_MODE_4_4_4, qlibContext->busInterface.dtr, TRUE)));
    }
#endif
    return ret;
}
#endif // QLIB_SEC_ONLY



QLIB_STATUS_T QLIB_STD_GetMemoryType(QLIB_CONTEXT_T* qlibContext, U8* memType)
{
    U8 id[2];

    // Get device ID
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__READ_JEDEC,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     id,
                                                     sizeof(id),
                                                     NULL));

    *memType = id[1];
    return QLIB_STATUS__OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine sets the quad enable bit in the status register
 *
 * @param       qlibContext   qlib context object
 * @param[in]   enable        TRUE for enable, FALSE for disable
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_SetQuadEnable_L(QLIB_CONTEXT_T* qlibContext, BOOL enable)
{
    STD_FLASH_STATUS_T status;
    STD_FLASH_STATUS_T statusCheck;
    U32                quadEnableBit = ((TRUE == enable) ? 1 : 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read the status registers                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_GetStatus_L(qlibContext, &status));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the quad enable bit                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_FIELD(status.SR2.asUint, SPI_FLASH__STATUS_2_FIELD__QE, quadEnableBit);
    QLIB_STATUS_RET_CHECK(QLIB_STD_SetStatus_L(qlibContext, status, &statusCheck));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check that the command succeed                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(READ_VAR_FIELD(statusCheck.SR2.asUint, SPI_FLASH__STATUS_2_FIELD__QE) == quadEnableBit,
                    QLIB_STATUS__COMMAND_IGNORED);

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief           This function sets non-volatile QE (QuadEnable) bit value
 *
 * @param[out]      qlibContext     qlib context object
 * @param[in]       enable          QE value
 *
 * @return          0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_SetResetInEnable_L(QLIB_CONTEXT_T* qlibContext, BOOL enable)
{
    STD_FLASH_STATUS_T status;
    STD_FLASH_STATUS_T statusCheck;
    U32                resetInEn = ((TRUE == enable) ? 1 : 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read the status registers                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_GetStatus_L(qlibContext, &status));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the reset in enable bit                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_FIELD(status.SR3.asUint, SPI_FLASH__STATUS_3_FIELD__HOLD_RST, resetInEn);
    QLIB_STATUS_RET_CHECK(QLIB_STD_SetStatus_L(qlibContext, status, &statusCheck));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check that the command succeed                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(READ_VAR_FIELD(statusCheck.SR3.asUint, SPI_FLASH__STATUS_3_FIELD__HOLD_RST) == resetInEn,
                    QLIB_STATUS__COMMAND_IGNORED);

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This routine returns the Flash status registers
 *
 * @param       qlibContext   qlib context object
 * @param[out]  status        Flash status
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_GetStatus_L(QLIB_CONTEXT_T* qlibContext, STD_FLASH_STATUS_T* status)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read status register 1                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__READ_STATUS_REGISTER_1,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     &status->SR1.asUint,
                                                     1,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read status register 2                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__READ_STATUS_REGISTER_2,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     &status->SR2.asUint,
                                                     1,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read status register 3                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     FALSE,
                                                     FALSE,
                                                     SPI_FLASH_CMD__READ_STATUS_REGISTER_3,
                                                     NULL,
                                                     NULL,
                                                     0,
                                                     0,
                                                     &status->SR3.asUint,
                                                     1,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    return QLIB_STATUS__OK;
}

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * @brief       This routine performs blocking / non-blocking page program
 *
 * @param       qlibContext   qlib context object
 * @param[in]   input         buffer of data to write
 * @param[in]   logicalAddr   logical flash address
 * @param[in]   size          data size to write (page-size bytes max)
 * @param[in]   blocking      if TRUE, this function is blocking till the program is finish
 *
 * @return      0 in no error occurred, or QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_PageProgram_L(QLIB_CONTEXT_T* qlibContext, const U8* input, U32 logicalAddr, U32 size, BOOL blocking)
{
    U8              writeCMD = 0;
    QLIB_BUS_MODE_T format   = QLIB_BUS_MODE_INVALID;
#ifdef QLIB_SUPPORT_QPI
#define BYPASS_MIN_WRITE_SIZE 16
    U8 buffer[BYPASS_MIN_WRITE_SIZE];
#endif // QLIB_SUPPORT_QPI

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(0 < size, QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
    QLIB_ASSERT_RET(size <= FLASH_PAGE_SIZE, QLIB_STATUS__INVALID_DATA_SIZE);
    QLIB_ASSERT_RET(((logicalAddr % FLASH_PAGE_SIZE) + size) <= FLASH_PAGE_SIZE, QLIB_STATUS__PARAMETER_OUT_OF_RANGE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get write command                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    writeCMD = QLIB_STD_GetWriteCMD_L(qlibContext, &format);
    // TODO - make this calculation in Config time and store the command in the globals

#ifdef QLIB_SUPPORT_QPI

    if (format == QLIB_BUS_MODE_4_4_4 && writeCMD == SPI_FLASH_CMD__PAGE_PROGRAM && size < BYPASS_MIN_WRITE_SIZE)
    {
        memset(buffer, 0xFF, BYPASS_MIN_WRITE_SIZE);

        if ((logicalAddr % FLASH_PAGE_SIZE) >= (BYPASS_MIN_WRITE_SIZE - size))
        {
            //read extra bytes b4 the beginning instead of the end
            logicalAddr = logicalAddr - (BYPASS_MIN_WRITE_SIZE - size);
            memcpy(buffer + (BYPASS_MIN_WRITE_SIZE - size), input, size);
        }
        else
        {
            memcpy(buffer, input, size);
        }

        input = buffer;
        size  = BYPASS_MIN_WRITE_SIZE;
    }
#endif // QLIB_SUPPORT_QPI

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform write                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     format,
                                                     FALSE,
                                                     TRUE,
                                                     blocking,
                                                     writeCMD,
                                                     &logicalAddr,
                                                     input,
                                                     size,
                                                     0,
                                                     NULL,
                                                     0,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ACTION_BY_FLASH_TYPE(
        qlibContext,
        { QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS)); },
        {
            if (blocking)
            {
                STD_FLASH_STATUS_T status;
                QLIB_STATUS_RET_CHECK(QLIB_STD_GetStatus_L(qlibContext, &status));
                QLIB_ASSERT_RET(0 == READ_VAR_FIELD(status.SR1.asUint, SPI_FLASH__STATUS_1_FIELD__WEL),
                                QLIB_STATUS__COMMAND_IGNORED);
            }
        });

    return QLIB_STATUS__OK;
}

#ifdef QLIB_SUPPORT_QPI
/************************************************************************************************************
 * @brief       This routine switch the flash device from standard SPI to QPI mode
 *              or from QPI to standard SPI
 *
 * @param       qlibContext   qlib context object
 * @param[in]   enter         TRUE for enter QPI, FALSE for go back to standard SPI
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_EnterExitQPI_L(QLIB_CONTEXT_T* qlibContext, BOOL enter)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (TRUE == enter)
    {
        QLIB_ASSERT_RET(QLIB_BUS_MODE_4_4_4 != qlibContext->busInterface.busMode, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    }
    else
    {
        QLIB_ASSERT_RET(QLIB_BUS_MODE_4_4_4 == qlibContext->busInterface.busMode, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enter / Exit QPI                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_ENTER_EXIT_QPI(qlibContext, enter));

    return QLIB_STATUS__OK;
}
#endif // QLIB_SUPPORT_QPI

U8 QLIB_STD_GetReadCMD_L(QLIB_CONTEXT_T* qlibContext, U32* dummyCycles, QLIB_BUS_MODE_T* format)
{
    *format = qlibContext->busInterface.busMode;
    if (TRUE == qlibContext->busInterface.dtr)
    {
        if (QLIB_BUS_MODE_1_1_4 == qlibContext->busInterface.busMode
#ifdef QLIB_SUPPORT_DUAL_SPI
            || QLIB_BUS_MODE_1_1_2 == qlibContext->busInterface.busMode
#endif //QLIB_SUPPORT_DUAL_SPI
        )
        {
            *format = QLIB_BUS_MODE_1_1_1;
        }
    }
    return QLIB_STD_GetReadDummyCyclesCMD_L(*format, qlibContext->busInterface.dtr, dummyCycles);
}

/************************************************************************************************************
 * @brief       This routine calculates Write Command
 *
 * @param       qlibContext    internal context object
 * @param[out]  format         Interface format to use for performing the command
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
U8 QLIB_STD_GetWriteCMD_L(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_MODE_T* format)
{
    switch (qlibContext->busInterface.busMode)
    {
        case QLIB_BUS_MODE_1_1_4:
        case QLIB_BUS_MODE_1_4_4:
            *format = QLIB_BUS_MODE_1_1_4;
            return SPI_FLASH_CMD__PAGE_PROGRAM_1_1_4;

        case QLIB_BUS_MODE_4_4_4:
            *format = QLIB_BUS_MODE_4_4_4;
            return SPI_FLASH_CMD__PAGE_PROGRAM;

        default:
            *format = QLIB_BUS_MODE_1_1_1;
            return SPI_FLASH_CMD__PAGE_PROGRAM;
    }
}
#endif // QLIB_SEC_ONLY

QLIB_STATUS_T QLIB_STD_SetStatus_L(QLIB_CONTEXT_T* qlibContext, STD_FLASH_STATUS_T statusIn, STD_FLASH_STATUS_T* statusOut)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Write status register                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     TRUE,
                                                     TRUE,
                                                     SPI_FLASH_CMD__WRITE_STATUS_REGISTER_1,
                                                     NULL,
                                                     &statusIn.SR1.asUint,
                                                     1,
                                                     0,
                                                     NULL,
                                                     0,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     TRUE,
                                                     TRUE,
                                                     SPI_FLASH_CMD__WRITE_STATUS_REGISTER_2,
                                                     NULL,
                                                     &statusIn.SR2.asUint,
                                                     1,
                                                     0,
                                                     NULL,
                                                     0,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    QLIB_STATUS_RET_CHECK(QLIB_STD_execute_std_cmd_L(qlibContext,
                                                     QLIB_STD_GET_BUS_MODE(qlibContext),
                                                     FALSE,
                                                     TRUE,
                                                     TRUE,
                                                     SPI_FLASH_CMD__WRITE_STATUS_REGISTER_3,
                                                     NULL,
                                                     &statusIn.SR3.asUint,
                                                     1,
                                                     0,
                                                     NULL,
                                                     0,
                                                     QLIB_VALUE_BY_FLASH_TYPE(qlibContext, &qlibContext->ssr, NULL)));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read back status register                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_STD_GetStatus_L(qlibContext, statusOut));

    return QLIB_STATUS__OK;
}

#ifndef QLIB_SEC_ONLY
static U8 QLIB_STD_GetReadDummyCyclesCMD_L(QLIB_BUS_MODE_T busMode, BOOL dtr, U32* dummyCycles)
{
    if (FALSE == dtr)
    {
        switch (busMode)
        {
            case QLIB_BUS_MODE_1_1_1:
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_1_X;
                return SPI_FLASH_CMD__READ_FAST__1_1_1;

#ifdef QLIB_SUPPORT_DUAL_SPI
            case QLIB_BUS_MODE_1_1_2:
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_1_X;
                return SPI_FLASH_CMD__READ_FAST__1_1_2;

            case QLIB_BUS_MODE_1_2_2:
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_2_2;
                return SPI_FLASH_CMD__READ_FAST__1_2_2;
#endif
            case QLIB_BUS_MODE_1_1_4:
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_1_X;
                return SPI_FLASH_CMD__READ_FAST__1_1_4;

            case QLIB_BUS_MODE_1_4_4:
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ__1_4_4;
                return SPI_FLASH_CMD__READ_FAST__1_4_4;

#ifdef QLIB_SUPPORT_QPI
            case QLIB_BUS_MODE_4_4_4:
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ__4_4_4; ///< 2 clocks default (configurable with 0xC0 command)
                return SPI_FLASH_CMD__READ_FAST__4_4_4;                  ///< QPI
#endif
            default:
                return SPI_FLASH_CMD__READ_DATA__1_1_1;
        }
    }
    else
    {
        switch (busMode)
        {
            case QLIB_BUS_MODE_1_1_1:
#ifdef QLIB_SUPPORT_DUAL_SPI
            case QLIB_BUS_MODE_1_1_2: // dtr is not support 1_1_2 format, fall back to single
#endif
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__1_1_1;
                return SPI_FLASH_CMD__READ_FAST_DTR__1_1_1;

#ifdef QLIB_SUPPORT_DUAL_SPI
            case QLIB_BUS_MODE_1_2_2:
#endif
            case QLIB_BUS_MODE_1_1_4: // dtr is not support 1_1_4 format, fall back to single
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__1_2_2;
                return SPI_FLASH_CMD__READ_FAST_DTR__1_2_2;

            case QLIB_BUS_MODE_1_4_4:
                *dummyCycles = SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__1_4_4;
                return SPI_FLASH_CMD__READ_FAST_DTR__1_4_4;

#ifdef QLIB_SUPPORT_QPI
            case QLIB_BUS_MODE_4_4_4:
                *dummyCycles =
                    SPI_FLASH_DUMMY_CYCLES__FAST_READ_DTR__4_4_4; ///< 2 clocks default (configurable with 0xC0 command)
                return SPI_FLASH_CMD__READ_FAST_DTR__4_4_4;       ///< QPI
#endif

            default:
                return SPI_FLASH_CMD__READ_DATA__1_1_1;
        }
    }
}

#ifdef QLIB_SUPPORT_QPI
static QLIB_STATUS_T QLIB_STD_CheckWritePrivilege_L(QLIB_CONTEXT_T* qlibContext, U32 logicalAddr)
{
    QLIB_POLICY_T policy;
    U32           sectionSize;
    U32           sectionID = _QLIB_SECTION_FROM_LOGICAL_ADDRESS(logicalAddr, qlibContext->addrSize);
    ACLR_T        aclr;
    U32           sectionWriteLock;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get section ID in case of fallback                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    sectionID = QLIB_FALLBACK_SECTION(qlibContext, sectionID);

    QLIB_STATUS_RET_CHECK(
        QLIB_SEC_GetSectionConfiguration(qlibContext, sectionID, NULL, &sectionSize, &policy, NULL, NULL, NULL));
    QLIB_ASSERT_RET(sectionSize > 0, QLIB_STATUS__INVALID_PARAMETER);

    QLIB_ASSERT_RET(policy.plainAccessWriteEnable, QLIB_STATUS__DEVICE_PRIVILEGE_ERR);
    QLIB_ASSERT_RET(!policy.writeProt, QLIB_STATUS__DEVICE_PRIVILEGE_ERR);
    if (policy.rollbackProt)
    {
        QLIB_ASSERT_RET(_QLIB_OFFSET_FROM_LOGICAL_ADDRESS(logicalAddr, qlibContext->addrSize) >= (sectionSize / 2),
                        QLIB_STATUS__DEVICE_PRIVILEGE_ERR);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check ACLR write lock                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_ACLR(qlibContext, &aclr));
    sectionWriteLock = READ_VAR_BIT(READ_VAR_FIELD(aclr, QLIB_REG_ACLR_WR_LOCK), sectionID);
    QLIB_ASSERT_RET(!sectionWriteLock, QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

    return QLIB_STATUS__OK;
}
#endif // QLIB_SUPPORT_QPI
#endif // QLIB_SEC_ONLY


BOOL QLIB_STD_AutoSenseCheckBusMode_L(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_MODE_T busMode)
{
    U8  deviceManufacturerId;
    U32 address = 0;
    U8  cmd;
    U32 dummy;
    switch (busMode)
    {
        case QLIB_BUS_MODE_1_4_4:
            cmd   = SPI_FLASH_CMD__MANUFACTURER_AND_DEVICE_ID_QUAD;
            dummy = SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__1_4_4;
            break;
#ifdef QLIB_SUPPORT_DUAL_SPI
        case QLIB_BUS_MODE_1_2_2:
            cmd   = SPI_FLASH_CMD__MANUFACTURER_AND_DEVICE_ID_DUAL;
            dummy = SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__1_2_2;
            break;
#endif // QLIB_SUPPORT_DUAL_SPI
        case QLIB_BUS_MODE_1_1_1:
            cmd   = SPI_FLASH_CMD__MANUFACTURER_AND_DEVICE_ID;
            dummy = SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__1_1_1;
            break;
#ifdef QLIB_SUPPORT_QPI
        case QLIB_BUS_MODE_4_4_4:
            cmd   = SPI_FLASH_CMD__MANUFACTURER_AND_DEVICE_ID;
            dummy = SPI_FLASH_DUMMY_CYCLES__MANUFACTURER_AND_DEVICE_ID__4_4_4;
            break;
#endif // QLIB_SUPPORT_QPI

        default:
            return FALSE;
    }
    // verify a few times to make sure we did not get the correct manufacturer ID by accident
    for (int i = 0; i < AUTOSENSE_NUM_RETRIES; i++)
    {
        deviceManufacturerId = 0;
        (void)QLIB_STD_execute_std_cmd_L(qlibContext,
                                         busMode,
                                         FALSE,
                                         FALSE,
                                         FALSE,
                                         cmd,
                                         &address,
                                         NULL,
                                         0,
                                         dummy,
                                         &deviceManufacturerId,
                                         1,
                                         NULL);

        if (STD_FLASH_MANUFACTURER__WINBOND_SERIAL_FLASH != deviceManufacturerId)
        {
            return FALSE;
        }
    }

    return TRUE;
}
