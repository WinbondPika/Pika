/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_tm.c
* @brief      This file contains QLIB Transaction Manager (TM) implementation
*
* ### project qlib
*
************************************************************************************************************/
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_tm.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       LOCAL FUNCTION DECLARATIONS                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static _INLINE_ QLIB_STATUS_T QLIB_TM__OP0_get_ssr_L(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* ssr);
static _INLINE_ QLIB_STATUS_T QLIB_TM__OP1_write_ibuf_L(QLIB_CONTEXT_T* qlibContext, U32 ctag, const U32* buf, U32 size);
static _INLINE_ QLIB_STATUS_T QLIB_TM__OP2_read_obuf_L(QLIB_CONTEXT_T* qlibContext, U32* buf, U32 size);
static QLIB_STATUS_T          QLIB_TM_GetStatus_L(QLIB_CONTEXT_T* qlibContext, STD_FLASH_STATUS_T* userStatus) __RAM_SECTION;
static QLIB_STATUS_T          QLIB_TM_WriteEnable_L(QLIB_CONTEXT_T* qlibContext) __RAM_SECTION;
static QLIB_STATUS_T          QLIB_TM_WaitWhileBusySec_L(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* ssr) __RAM_SECTION;

#define SSR__RESP_READY_BIT MASK_FIELD(QLIB_REG_SSR__RESP_READY)
#define SSR__BUSY_BIT       MASK_FIELD(QLIB_REG_SSR__BUSY)
#define SSR__FLASH_BUSY_BIT MASK_FIELD(QLIB_REG_SSR__FLASH_BUSY)
#define SSR__BUSY_BITS      (SSR__BUSY_BIT | SSR__FLASH_BUSY_BIT)
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_TM_Init(QLIB_CONTEXT_T* qlibContext)
{
    qlibContext->busInterface.dtr              = FALSE;
    qlibContext->busInterface.busMode          = QLIB_BUS_MODE_INVALID;
    qlibContext->busInterface.secureCmdsFormat = QLIB_BUS_MODE_INVALID;
    qlibContext->busInterface.busIsLocked      = FALSE;

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_TM_Connect(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_T     ret          = QLIB_STATUS__OK;
    QLIB_INTERFACE_T* busInterface = &qlibContext->busInterface;
    INTERRUPTS_VAR_DECLARE(ints);

    INTERRUPTS_SAVE_DISABLE(ints);

    if (TRUE == busInterface->busIsLocked)
    {
        ret = QLIB_STATUS__DEVICE_BUSY;
    }
    else
    {
        busInterface->busIsLocked = TRUE;
    }

    INTERRUPTS_RESTORE(ints);

    return ret;
}

QLIB_STATUS_T QLIB_TM_Disconnect(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_T     ret          = QLIB_STATUS__OK;
    QLIB_INTERFACE_T* busInterface = &qlibContext->busInterface;
    INTERRUPTS_VAR_DECLARE(ints);

    INTERRUPTS_SAVE_DISABLE(ints);

    if (FALSE == busInterface->busIsLocked)
    {
        ret = QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE;
    }
    else
    {
        busInterface->busIsLocked = FALSE;
    }

    INTERRUPTS_RESTORE(ints);

    return ret;
}

QLIB_STATUS_T QLIB_TM_Standard(QLIB_CONTEXT_T*   qlibContext,
                               QLIB_BUS_FORMAT_T busFormat,
                               BOOL              needWriteEnable,
                               BOOL              waitWhileBusy,
                               U8                cmd,
                               const U32*        address,
                               const U8*         writeData,
                               U32               writeDataSize,
                               U32               dummyCycles,
                               U8*               readData,
                               U32               readDataSize,
                               QLIB_REG_SSR_T*   ssr)
{
    QLIB_STATUS_T   ret      = QLIB_STATUS__OK;
    U32             addr     = 0;
    U32             addrSize = 0;
    QLIB_BUS_MODE_T mode     = QLIB_BUS_FORMAT_GET_MODE(busFormat);
    BOOL            dtr      = QLIB_BUS_FORMAT_GET_DTR(busFormat);
    INTERRUPTS_VAR_DECLARE(ints);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (FALSE == qlibContext->busInterface.busIsLocked)
    {
        return QLIB_STATUS__NOT_CONNECTED;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Handle address                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (address != NULL)
    {
        addr = *address;

        addrSize = 3;
    }
    else
    {
        addr     = 0;
        addrSize = 0;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start atomic transaction                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPTS_SAVE_DISABLE(ints);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform write enable if needed                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (TRUE == needWriteEnable)
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM_WriteEnable_L(qlibContext), ret, exit);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform transaction                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if (cmd != SPI_FLASH_CMD__NONE)
    {
        QLIB_STATUS_RET_CHECK_GOTO(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                                 mode,
                                                                 dtr,
                                                                 cmd,
                                                                 addr,
                                                                 addrSize,
                                                                 writeData,
                                                                 writeDataSize,
                                                                 dummyCycles,
                                                                 readData,
                                                                 readDataSize),
                                   ret,
                                   exit);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform wait while busy                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (TRUE == waitWhileBusy || NULL != ssr)
    {
        QLIB_ACTION_BY_FLASH_TYPE(
            qlibContext,
            { QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM_WaitWhileBusySec_L(qlibContext, ssr), ret, exit); },
            {
                QLIB_ASSERT_WITH_ERROR_GOTO(ssr == NULL, QLIB_STATUS__INVALID_PARAMETER, ret, exit);
                QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM_WaitWhileBusyStd_L(qlibContext), ret, exit);
            });
    }

exit:
    /*-----------------------------------------------------------------------------------------------------*/
    /* End atomic transaction                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPTS_RESTORE(ints);

    return ret;
}

QLIB_STATUS_T QLIB_TM_Secure(QLIB_CONTEXT_T* qlibContext,
                             U32             ctag,
                             const U32*      writeData,
                             U32             writeDataSize,
                             U32*            readData,
                             U32             readDataSize,
                             QLIB_REG_SSR_T* ssr)
{
    QLIB_STATUS_T     ret          = QLIB_STATUS__OK;
    QLIB_INTERFACE_T* busInterface = &qlibContext->busInterface;
#ifdef QLIB_SUPPORT_QPI
    BOOL exitQpi = FALSE;
#endif // QLIB_SUPPORT_QPI
    INTERRUPTS_VAR_DECLARE(ints);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (FALSE == busInterface->busIsLocked)
    {
        return QLIB_STATUS__NOT_CONNECTED;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start atomic transaction                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPTS_SAVE_DISABLE(ints);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform write phase                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if (ctag != 0)
    {
#ifdef QLIB_SUPPORT_QPI
        /*-------------------------------------------------------------------------------------------------*/
        /* OP1 doesn't support QPI, exit for op1                                                           */
        /*-------------------------------------------------------------------------------------------------*/
        if (QLIB_BUS_MODE_4_4_4 == busInterface->secureCmdsFormat)
        {
            exitQpi = TRUE;
            QLIB_STATUS_RET_CHECK_GOTO(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                                     QLIB_BUS_MODE_4_4_4,
                                                                     FALSE,
                                                                     SPI_FLASH_CMD__EXIT_QPI,
                                                                     0,
                                                                     0,
                                                                     NULL,
                                                                     0,
                                                                     0,
                                                                     NULL,
                                                                     0),
                                       ret,
                                       exit);

            // After exit QPI temporarily set the format to quad so the follow transactions execute in quad
            busInterface->secureCmdsFormat = QLIB_BUS_MODE_1_1_4;
        }
#endif // QLIB_SUPPORT_QPI

        QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM__OP1_write_ibuf_L(qlibContext, ctag, writeData, writeDataSize), ret, exit);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait while busy                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    if (ssr != NULL)
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM_WaitWhileBusySec_L(qlibContext, ssr), ret, exit);
        if (QLIB_CMD_PROC__CTAG_GET_CMD(ctag) == QLIB_CMD_SEC_SAWR)
        {
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM_WaitWhileBusySec_L(qlibContext, ssr), ret, exit);
        }
        if (QLIB_CMD_PROC__CTAG_GET_CMD(ctag) == QLIB_CMD_SEC_CALC_SIG)
        {
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM_WaitWhileBusySec_L(qlibContext, ssr), ret, exit);
        }
    }

#ifdef QLIB_SUPPORT_QPI
    if (exitQpi == TRUE)
    {
        QLIB_STATUS_RET_CHECK_GOTO(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                                 QLIB_BUS_MODE_1_1_1,
                                                                 FALSE,
                                                                 SPI_FLASH_CMD__ENTER_QPI,
                                                                 0,
                                                                 0,
                                                                 NULL,
                                                                 0,
                                                                 0,
                                                                 NULL,
                                                                 0),
                                   ret,
                                   exit);

        // After entering back to QPI set the format back to quad
        busInterface->secureCmdsFormat = QLIB_BUS_MODE_4_4_4;
    }
#endif // QLIB_SUPPORT_QPI

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read phase if needed                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    if (0 != readDataSize)
    {
        if ((ssr != NULL) &&
            (0 == READ_VAR_FIELD(ssr->asUint, QLIB_REG_SSR__RESP_READY))
        )
        {
            if (QLIB_CMD_PROC__CTAG_GET_CMD(ctag) == QLIB_CMD_SEC_CALC_SIG)
            {
                QLIB_REG_SSR_T tempSSR = *ssr;

                while ((READ_VAR_FIELD(tempSSR.asUint, QLIB_REG_SSR__ERR) == 0) &&
                       (READ_VAR_FIELD(tempSSR.asUint, QLIB_REG_SSR__RESP_READY) == 0))

                {
                    QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM__OP0_get_ssr_L(qlibContext, &tempSSR), ret, exit);
                }

                SET_VAR_FIELD(ssr->asUint, QLIB_REG_SSR__RESP_READY, READ_VAR_FIELD(tempSSR.asUint, QLIB_REG_SSR__RESP_READY));
                SET_VAR_FIELD(ssr->asUint, QLIB_REG_SSR__ERR, READ_VAR_FIELD(tempSSR.asUint, QLIB_REG_SSR__ERR));

                if (READ_VAR_FIELD(ssr->asUint, QLIB_REG_SSR__ERR) == 1)
                {
                    goto exit;
                }
            }
            else
            {
                /*-----------------------------------------------------------------------------------------*/
                /* Emulate HW error                                                                        */
                /*-----------------------------------------------------------------------------------------*/
                SET_VAR_FIELD(ssr->asUint, QLIB_REG_SSR__ERR, 1);
                goto exit;
            }
        }
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM__OP2_read_obuf_L(qlibContext, readData, readDataSize), ret, exit);
    }

    if (qlibContext->multiTransactionCmd == FALSE) //if TRUE this comes from Read/Write commands
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Perform init PA after SET_SCR to restore plain access                                           */
        /*-------------------------------------------------------------------------------------------------*/
        if ((BYTE(ctag, 0) == QLIB_CMD_SEC_SET_SCR || BYTE(ctag, 0) == QLIB_CMD_SEC_SET_SCR_SWAP) && NULL != ssr &&
            0 == READ_VAR_FIELD(ssr->asUint, QLIB_REG_SSR__ERR))
        {
            // Check if need post command operations
            BOOL needToReset = (ctag & QLIB_TM_CTAG_SCR_NEED_RESET_MASK) ? TRUE : FALSE;
            BOOL needInitPA  = (ctag & QLIB_TM_CTAG_SCR_NEED_INIT_PA_MASK) ? TRUE : FALSE;

            if (TRUE == needInitPA)
            {
                ctag = MAKE_32_BIT(QLIB_CMD_SEC_INIT_SECTION_PA, BYTE(ctag, 1), 0, 0);

                QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM__OP1_write_ibuf_L(qlibContext, ctag, writeData, writeDataSize), ret, exit);

                QLIB_STATUS_RET_CHECK_GOTO(QLIB_TM_WaitWhileBusySec_L(qlibContext, ssr), ret, exit);
            }

            /*---------------------------------------------------------------------------------------------*/
            /* Check if we need to reset now                                                               */
            /*---------------------------------------------------------------------------------------------*/
            if (TRUE == needToReset)
            {
                CORE_RESET();
            }
        }
    }

exit:

    /*-----------------------------------------------------------------------------------------------------*/
    /* End atomic transaction                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    INTERRUPTS_RESTORE(ints);

    return ret;
}

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine retrieve SSR (Secure Status Register)
 *
 * @param[in]   qlibContext    Context
 * @param[out]  ssr            SSR (Secure Status Register) value (32bit)
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
static _INLINE_ QLIB_STATUS_T QLIB_TM__OP0_get_ssr_L(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* ssr)
{
    QLIB_BUS_MODE_T format = qlibContext->busInterface.secureCmdsFormat;
    U8              op0    = qlibContext->busInterface.op0;
    BOOL            dtr    = qlibContext->busInterface.dtr;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform GET_SSR command                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                        format,
                                                        dtr,
                                                        op0,
                                                        0,
                                                        0,
                                                        NULL,
                                                        0,
                                                        Q2_SEC_INST_DUMMY_CYCLES__OP0(qlibContext->busInterface.dtr),
                                                        (U8*)&(ssr->asUint),
                                                        sizeof(U32)));

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This routine write secure command to chip input buffer
 *
 * @param[in]   qlibContext    Context
 * @param[in]   ctag           Secure Command CTAG value
 * @param[in]   buf            data buffer
 * @param       size           data buffer size (in bytes)
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
static _INLINE_ QLIB_STATUS_T QLIB_TM__OP1_write_ibuf_L(QLIB_CONTEXT_T* qlibContext, U32 ctag, const U32* buf, U32 size)
{
    U32             ctagStream = MAKE_32_BIT(BYTE(ctag, 3), BYTE(ctag, 2), BYTE(ctag, 1), BYTE(ctag, 0));
    U32             ctagSize   = Q2_CTAG_SIZE_BYTE;
    QLIB_BUS_MODE_T format     = qlibContext->busInterface.secureCmdsFormat;
    U8              op1        = qlibContext->busInterface.op1;
    QLIB_STATUS_T   ret        = QLIB_STATUS__OK;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(size <= Q2_MAX_IBUF_SIZE_BYTE, QLIB_STATUS__INVALID_DATA_SIZE);
#if defined QLIB_MAX_SPI_OUTPUT_SIZE
    // sending OP1 + CTAG + DATA on SPI
    QLIB_ASSERT_RET(1 + Q2_CTAG_SIZE_BYTE + size <= QLIB_MAX_SPI_OUTPUT_SIZE, QLIB_STATUS__INVALID_DATA_SIZE);
#endif

#ifdef QLIB_SUPPORT_DUAL_SPI
    /*-----------------------------------------------------------------------------------------------------*/
    /* The bellow code block bypasses OP1 specific limitations                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* OP1 doesn't support DUAL                                                                        */
        /*-------------------------------------------------------------------------------------------------*/
        if ((format == QLIB_BUS_MODE_1_1_2) || (format == QLIB_BUS_MODE_1_2_2))
        {
            format = QLIB_BUS_MODE_1_1_1; // Bus-mode: single
            op1    = 0xA1;                // OP code:  single
        }
    }
#endif


        /*-------------------------------------------------------------------------------------------------*/
        /* Perform Write IBUF command                                                                      */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK_GOTO(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                                 format,
                                                                 FALSE, // OP1 doesn't support DTR
                                                                 op1,
                                                                 ctagStream,
                                                                 ctagSize,
                                                                 (const U8*)buf,
                                                                 size,
                                                                 0,
                                                                 NULL,
                                                                 0),
                                   ret,
                                   exit);


exit:
    return ret;
}

/************************************************************************************************************
 * @brief       This routine reads secure command response from the chip output buffer
 *
 * @param[in]   qlibContext    Context
 * @param[out]  buf            data buffer
 * @param[in]   size           data buffer size (in bytes)
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
static _INLINE_ QLIB_STATUS_T QLIB_TM__OP2_read_obuf_L(QLIB_CONTEXT_T* qlibContext, U32* buf, U32 size)
{
#ifndef QLIB_MAX_SPI_INPUT_SIZE // no limitation of input buffer for SPI
    return PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                         qlibContext->busInterface.secureCmdsFormat,
                                         qlibContext->busInterface.dtr,
                                         qlibContext->busInterface.op2,
                                         0,
                                         0,
                                         NULL,
                                         0,
                                         Q2_SEC_INST_DUMMY_CYCLES__OP2,
                                         (U8*)buf,
                                         size);
#else
    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform multiple small Read OBUF command for the case that SIP bus not supporting big transactions  */
    /*-----------------------------------------------------------------------------------------------------*/
    U8* tempBuf = (U8*)buf;

    while (0 != size)
    {
        U32 transactionSize = MIN(size, ROUND_DOWN(QLIB_MAX_SPI_INPUT_SIZE, 4));

        QLIB_STATUS_RET_CHECK(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                            qlibContext->busInterface.secureCmdsFormat,
                                                            qlibContext->busInterface.dtr,
                                                            qlibContext->busInterface.op2,
                                                            0,
                                                            0,
                                                            NULL,
                                                            0,
                                                            Q2_SEC_INST_DUMMY_CYCLES__OP2,
                                                            tempBuf,
                                                            transactionSize));

        // prepare next iteration
        tempBuf += transactionSize;
        size -= transactionSize;
    }

    return QLIB_STATUS__OK;
#endif // QLIB_MAX_SPI_INPUT_SIZE
}

/************************************************************************************************************
 * @brief       This routine waits till Flash becomes not busy, and optionally returns the Flash
 *              secure status
 *
 * @param[in]   qlibContext    Pointer to the qlib context
 * @param[out]  userSsr        Output SSR value (can be NULL)
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_TM_WaitWhileBusySec_L(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* userSsr)
{
    QLIB_REG_SSR_T  ssr   = {0};
    QLIB_REG_SSR_T* ssr_p = (userSsr != NULL) ? userSsr : &ssr;

#ifdef QLIB_SUPPORT_QPI
    BOOL exitQpi = FALSE;

    /*-----------------------------------------------------------------------------------------------------*/
    /* OP0 doesn't support QPI, exit QPI for op0                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_BUS_MODE_4_4_4 == qlibContext->busInterface.secureCmdsFormat)
    {
        STD_FLASH_STATUS_T status;
        do
        {
            QLIB_STATUS_RET_CHECK(QLIB_TM_GetStatus_L(qlibContext, &status));
        } while (1 == READ_VAR_FIELD(status.SR1.asUint, SPI_FLASH__STATUS_1_FIELD__BUSY));

        exitQpi = TRUE;
        QLIB_STATUS_RET_CHECK(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                            QLIB_BUS_MODE_4_4_4,
                                                            FALSE,
                                                            SPI_FLASH_CMD__EXIT_QPI,
                                                            0,
                                                            0,
                                                            NULL,
                                                            0,
                                                            0,
                                                            NULL,
                                                            0));

        // After exit QPI temporarily set the format to quad so the follow transactions execute in quad
        qlibContext->busInterface.secureCmdsFormat = QLIB_BUS_MODE_1_1_4;
    }
#endif // QLIB_SUPPORT_QPI

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait while busy by polling SSR                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    do
    {

        /*-------------------------------------------------------------------------------------------------*/
        /* Read next SSR                                                                                   */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_TM__OP0_get_ssr_L(qlibContext, ssr_p));

        /*-------------------------------------------------------------------------------------------------*/
        /* Check if flash is alive                                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        if (ssr_p->asUint == MAX_U32 || ssr_p->asUint == 0)
        {
#ifdef QLIB_SUPPORT_QPI
            // in case of power-down mode all commands are ignored and we are still in QPI
            if (exitQpi == TRUE)
            {
                qlibContext->busInterface.secureCmdsFormat = QLIB_BUS_MODE_4_4_4;
            }
#endif // QLIB_SUPPORT_QPI
            return QLIB_STATUS__CONNECTIVITY_ERR;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* if response is there we do not keep on waiting till busy goes off, We can use op2 right away    */
        /*-------------------------------------------------------------------------------------------------*/
        if (ssr_p->asUint & SSR__RESP_READY_BIT)
        {
            ssr_p->asUint &= ~SSR__BUSY_BIT;
        }

    } while (ssr_p->asUint & SSR__BUSY_BITS);

#ifdef QLIB_SUPPORT_QPI
    /*-----------------------------------------------------------------------------------------------------*/
    /* OP0 doesn't support QPI, if we exit QPI for op0 now need to enter back                              */
    /*-----------------------------------------------------------------------------------------------------*/
    if (exitQpi == TRUE)
    {
        QLIB_STATUS_RET_CHECK(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                            QLIB_BUS_MODE_1_1_1,
                                                            FALSE,
                                                            SPI_FLASH_CMD__ENTER_QPI,
                                                            0,
                                                            0,
                                                            NULL,
                                                            0,
                                                            0,
                                                            NULL,
                                                            0));

        // After entering back to QPI set the format back to quad
        qlibContext->busInterface.secureCmdsFormat = QLIB_BUS_MODE_4_4_4;
    }
#endif // QLIB_SUPPORT_QPI
    return QLIB_STATUS__OK;
}


/************************************************************************************************************
 * @brief       This routine returns Flash status register
 *
 * @param[in]   qlibContext   pointer to qlib context
 * @param[out]  status        Flash status
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_TM_GetStatus_L(QLIB_CONTEXT_T* qlibContext, STD_FLASH_STATUS_T* status)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read status register 1                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                        QLIB_STD_GET_BUS_MODE(qlibContext),
                                                        FALSE,
                                                        SPI_FLASH_CMD__READ_STATUS_REGISTER_1,
                                                        0,
                                                        0,
                                                        NULL,
                                                        0,
                                                        0,
                                                        &(status->SR1.asUint),
                                                        1));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read status register 2                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                        QLIB_STD_GET_BUS_MODE(qlibContext),
                                                        FALSE,
                                                        SPI_FLASH_CMD__READ_STATUS_REGISTER_2,
                                                        0,
                                                        0,
                                                        NULL,
                                                        0,
                                                        0,
                                                        &(status->SR2.asUint),
                                                        1));

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This routine sends standard write enable command, and optionally returns the Flash
 *              status
 *
 * @param[in]   qlibContext   pointer to qlib context
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_TM_WriteEnable_L(QLIB_CONTEXT_T* qlibContext)
{
    STD_FLASH_STATUS_T status;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform Write Enable                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(PLAT_SPI_WriteReadTransaction(qlibContext->userData,
                                                        QLIB_STD_GET_BUS_MODE(qlibContext),
                                                        FALSE,
                                                        SPI_FLASH_CMD__WRITE_ENABLE,
                                                        0,
                                                        0,
                                                        NULL,
                                                        0,
                                                        0,
                                                        NULL,
                                                        0));

    /*-----------------------------------------------------------------------------------------------------*/
    /* wait for the command to finish                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ACTION_BY_FLASH_TYPE(
        qlibContext,
        { QLIB_STATUS_RET_CHECK(QLIB_TM_WaitWhileBusySec_L(qlibContext, NULL)); },
        { QLIB_STATUS_RET_CHECK(QLIB_TM_WaitWhileBusyStd_L(qlibContext)); });

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check that write is enabled                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_TM_GetStatus_L(qlibContext, &status));
    if (0 == READ_VAR_FIELD(status.SR1.asUint, SPI_FLASH__STATUS_1_FIELD__WEL))
    {
        return QLIB_STATUS__COMMAND_FAIL;
    }

    return QLIB_STATUS__OK;
}
