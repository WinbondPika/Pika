/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_cmd_proc.c
* @brief      Command processor: This file handles processing of flash API commands
*
* ### project qlib
*
************************************************************************************************************/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_cmd_proc.h"
#ifdef PROC_CMD_SIMULATOR
#include "qlib_sec_cmd_proc_tests.h"
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 DEFINE                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_SEC_DMC_MAX_VAL 0x3FFFFFFF

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define QLIB_CMD_PROC_build_encryption_key(qlibContext_p, cipherContext) \
    QLIB_CRYPTO_BuildCipherKey((cipherContext)->hashBuf,                 \
                               (qlibContext_p)->keyMngr.kid,             \
                               DECRYPTION_OF_INPUT_DIR_CODE,             \
                               (cipherContext)->cipherKey);

#define QLIB_CMD_PROC_build_decryption_key(qlibContext_p, cipherContext) \
    QLIB_CRYPTO_BuildCipherKey((cipherContext)->hashBuf,                 \
                               (qlibContext_p)->keyMngr.kid,             \
                               ENCRYPTION_OF_OUTPUT_DIR_CODE,            \
                               (cipherContext)->cipherKey);

#ifdef QLIB_HASH_OPTIMIZATION_ENABLED
#define QLIB_CMD_PROC_build_decryption_key_async(qlibContext_p, cipherContext) \
    QLIB_CRYPTO_BuildCipherKey_Async((cipherContext)->hashBuf,                 \
                                     (qlibContext_p)->keyMngr.kid,             \
                                     ENCRYPTION_OF_OUTPUT_DIR_CODE,            \
                                     (cipherContext)->cipherKey);
#else
#define QLIB_CMD_PROC_build_decryption_key_async(qlibContext_p, cipherContext) \
    QLIB_CMD_PROC_build_decryption_key(qlibContext_p, cipherContext)
#endif

#define QLIB_CMD_PROC_initialize_decryption_key(qlibContext_p, cipherContext) \
    {                                                                         \
        (cipherContext) = &QLIB_KEY_MNGR__CMD_CONTEXT_GET(qlibContext_p);     \
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_refresh_ssk_L(qlibContext_p));    \
        QLIB_CMD_PROC_build_decryption_key(qlibContext_p, (cipherContext));   \
    }


#define QLIB_CMD_PROC_update_decryption_key_async(qlibContext_p, cipherContext)               \
    {                                                                                         \
        QLIB_CRYPTO_put_salt_on_session_key((qlibContext)->mc[TC],                            \
                                            QLIB_HASH_BUF_GET__KEY((cipherContext)->hashBuf), \
                                            (qlibContext_p)->keyMngr.sessionKey);             \
        QLIB_CMD_PROC_build_decryption_key_async(qlibContext, (cipherContext));               \
    }

#ifdef QLIB_HASH_OPTIMIZATION_ENABLED
#define QLIB_CMD_PROC_update_decryption_key_async_wait_till_ready(qlibContext) PLAT_HASH_Async_WaitWhileBusy()
#else
#define QLIB_CMD_PROC_update_decryption_key_async_wait_till_ready(qlibContext)
#endif

#define QLIB_CMD_PROC_encrypt_address(enc_addr, addr, key)         \
    {                                                              \
        (enc_addr) = (addr) ^ (QLIB_CRYPTO_CreateAddressKey(key)); \
        (enc_addr) = Q2_SWAP_24BIT_ADDR_FOR_SPI(enc_addr);         \
    }

#define QLIB_CMD_PROC_execute_sec_cmd(qlibContext, ctag, writeData, writeDataSize, readData, readDataSize, ssr) \
    QLIB_TM_Secure(qlibContext, ctag, writeData, writeDataSize, readData, readDataSize, ssr)

#define QLIB_CMD_PROC_execute_sec_cmd_write_read(qlibContext, ctag, writeData, writeDataSize, readData, readDataSize) \
    QLIB_CMD_PROC_execute_sec_cmd(qlibContext, ctag, writeData, writeDataSize, readData, readDataSize, &(qlibContext)->ssr)

#define QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext, ctag) \
    QLIB_CMD_PROC_execute_sec_cmd_write_read(qlibContext, ctag, NULL, 0, NULL, 0)

#define QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext, ctag, writeData, writeDataSize) \
    QLIB_CMD_PROC_execute_sec_cmd_write_read(qlibContext, ctag, writeData, writeDataSize, NULL, 0)

#define QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, ctag, readData, readDataSize) \
    QLIB_CMD_PROC_execute_sec_cmd_write_read(qlibContext, ctag, NULL, 0, readData, readDataSize)

#define QLIB_CMD_PROC__OP1_only(qlibContext, ctag)                QLIB_CMD_PROC_execute_sec_cmd(qlibContext, ctag, NULL, 0, NULL, 0, NULL)
#define QLIB_CMD_PROC__OP0_busy_wait(qlibContext)                 QLIB_CMD_PROC_execute_sec_cmd_write_read(qlibContext, 0, NULL, 0, NULL, 0)
#define QLIB_CMD_PROC__OP0_busy_wait_OP2(qlibContext, data, size) QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, 0, data, size)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       LOCAL FUNCTION DECLARATIONS                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static QLIB_STATUS_T QLIB_CMD_PROC_use_mc_L(QLIB_CONTEXT_T* qlibContext, _64BIT mc);
static QLIB_STATUS_T QLIB_CMD_PROC_refresh_ssk_L(QLIB_CONTEXT_T* qlibContext);

static QLIB_STATUS_T QLIB_CMD_PROC__sign_data_L(QLIB_CONTEXT_T* qlibContext,
                                                U32             plain_ctag,
                                                const U32*      data_up_to256bit,
                                                U32             data_size,
                                                U32*            signature,
                                                BOOL            refresh_ssk);

static QLIB_STATUS_T QLIB_CMD_PROC__prepare_signed_setter_cmd_L(QLIB_CONTEXT_T* qlibContext,
                                                                U32*            ctag,
                                                                const U32*      data,
                                                                U32             data_size,
                                                                U32*            signature,
                                                                U32*            encrypted_data,
                                                                U32*            addr);

static QLIB_STATUS_T QLIB_CMD_PROC__execute_signed_setter_L(QLIB_CONTEXT_T* qlibContext,
                                                            U32             ctag,
                                                            const U32*      data,
                                                            U32             data_size,
                                                            BOOL            encrypt_data,
                                                            U32*            addr);

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*                                            STATUS COMMANDS                                              */
/*---------------------------------------------------------------------------------------------------------*/
QLIB_STATUS_T QLIB_CMD_PROC__get_SSR_UNSIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* ssr, U32 mask)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_NONE)));

    if (ssr != NULL)
    {
        ssr->asUint = QLIB_GET_LAST_SEC_STATUS_FIELD(qlibContext);
    }
    return QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, mask);
}

QLIB_STATUS_T QLIB_CMD_PROC__get_SSR_SIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_REG_SSR_T* ssr, U32 mask)
{
    QLIB_REG_SSR_T ssrHw;
    QLIB_STATUS_RET_CHECK(
        QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_SSR, 0, (U32*)&ssrHw, sizeof(QLIB_REG_SSR_T), NULL));
    if (ssr != NULL)
    {
        ssr->asUint = ssrHw.asUint;
    }
    return QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, mask);
}


QLIB_STATUS_T QLIB_CMD_PROC__get_WID_UNSIGNED(QLIB_CONTEXT_T* qlibContext, _64BIT WID)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    QLIB_ASSERT_RET(WID != NULL, QLIB_STATUS__INVALID_PARAMETER);
    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_WID), WID, sizeof(_64BIT));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));

    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_WID_SIGNED(QLIB_CONTEXT_T* qlibContext, _64BIT WID)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;
    QLIB_ASSERT_RET(WID != NULL, QLIB_STATUS__INVALID_PARAMETER);
    ret = QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_WID, 0, WID, sizeof(_64BIT), NULL);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}


QLIB_STATUS_T QLIB_CMD_PROC__get_SUID_UNSIGNED(QLIB_CONTEXT_T* qlibContext, _128BIT SUID)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_SUID), SUID, sizeof(_128BIT));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_SUID_SIGNED(QLIB_CONTEXT_T* qlibContext, _128BIT SUID)
{
    QLIB_STATUS_T ret = QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_SUID, 0, SUID, sizeof(_128BIT), NULL);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_AWDTSR(QLIB_CONTEXT_T* qlibContext, AWDTSR_T* AWDTSR)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext,
                                             QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_AWDTSR),
                                             AWDTSR,
                                             sizeof(AWDTSR_T));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                         CONFIGURATION COMMANDS                                          */
/*---------------------------------------------------------------------------------------------------------*/


QLIB_STATUS_T QLIB_CMD_PROC__SFORMAT(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* OP1, CTAG (32b), SIG (64b)                                                                          */
    /* CTAG = CMD (8b), 24'b0                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->keyMngr.kid == QLIB_KID__DEVICE_MASTER, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext,
                                                                 QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SFORMAT),
                                                                 NULL,
                                                                 0,
                                                                 FALSE,
                                                                 NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__FORMAT(QLIB_CONTEXT_T* qlibContext)
{
    U32 ver = 0xA5A5A5A5;
    QLIB_STATUS_RET_CHECK(
        QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_FORMAT), &ver, sizeof(U32)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_KEY(QLIB_CONTEXT_T* qlibContext, QLIB_KID_T kid, const KEY_T key_buff)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* OP1, CTAG (32b), Encrypted-Key (128b), SIG (64b)                                                    */
    /* CTAG = CMD (8b), KID (8b), 16'b0                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    U32 ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_SET_KEY, kid, 0, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* set key is never done on provisioning key id                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(!QLIB_KEY_ID_IS_PROVISIONING(kid), QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* check if the session opened is provisioning session                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_KEY_ID_IS_PROVISIONING(qlibContext->keyMngr.kid), QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Execute command                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext, ctag, key_buff, sizeof(_128BIT), TRUE, NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__IGNORE_IGNORE_ERR));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_SUID(QLIB_CONTEXT_T* qlibContext, const _128BIT SUID)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext,
                                                                 QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SET_SUID),
                                                                 SUID,
                                                                 sizeof(_128BIT),
                                                                 FALSE,
                                                                 NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_GMC(QLIB_CONTEXT_T* qlibContext, const GMC_T GMC)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* OP1, CTAG (32b), GMC (160b), SIG (64b                                                               */
    /* CTAG = CMD (8b), 24'b0                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->keyMngr.kid == QLIB_KID__DEVICE_MASTER, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext,
                                                                 QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SET_GMC),
                                                                 GMC,
                                                                 sizeof(GMC_T),
                                                                 FALSE,
                                                                 NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_GMC_UNSIGNED(QLIB_CONTEXT_T* qlibContext, GMC_T GMC)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_GMC), GMC, sizeof(GMC_T));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_GMC_SIGNED(QLIB_CONTEXT_T* qlibContext, GMC_T GMC)
{
    QLIB_STATUS_T ret = QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_GMC, 0, GMC, sizeof(GMC_T), NULL);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_GMT(QLIB_CONTEXT_T* qlibContext, const GMT_T GMT)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* OP1, CTAG (32b), GMT (160b), SIG (64b)                                                              */
    /* CTAG = CMD (8b), 24'b0                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->keyMngr.kid == QLIB_KID__DEVICE_MASTER, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext,
                                                                 QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SET_GMT),
                                                                 GMT,
                                                                 sizeof(GMT_T),
                                                                 FALSE,
                                                                 NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_GMT_UNSIGNED(QLIB_CONTEXT_T* qlibContext, GMT_T GMT)
{
    QLIB_STATUS_T ret =
        QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_GMT), GMT, sizeof(GMT_T));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_GMT_SIGNED(QLIB_CONTEXT_T* qlibContext, GMT_T GMT)
{
    QLIB_STATUS_T ret = QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_GMT, 0, GMT, sizeof(GMT_T), NULL);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_AWDT(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T AWDTCFG)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* OP1, CTAG (32b), AWDTCFG (32b), SIG                                                                 */
    /* CMD (8b), 24'b0                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext,
                                                                 QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SET_AWDT),
                                                                 &AWDTCFG,
                                                                 sizeof(AWDTCFG_T),
                                                                 FALSE,
                                                                 NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_AWDT_UNSIGNED(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T* AWDTCFG)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext,
                                             QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_AWDT),
                                             AWDTCFG,
                                             sizeof(AWDTCFG_T));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_AWDT_SIGNED(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T* AWDTCFG)
{
    QLIB_STATUS_T ret = QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_AWDTCFG, 0, AWDTCFG, sizeof(AWDTCFG_T), NULL);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__AWDT_TOUCH(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext,
                                                                 QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_AWDT_TOUCH),
                                                                 NULL,
                                                                 0,
                                                                 FALSE,
                                                                 NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_AWDT_PA(QLIB_CONTEXT_T* qlibContext, AWDTCFG_T AWDTCFG)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext,
                                                              QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SET_AWDT_PA),
                                                              &AWDTCFG,
                                                              sizeof(AWDTCFG_T)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__AWDT_TOUCH_PA(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_AWDT_TOUCH_PA)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_SCRn(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex, const SCRn_T SCRn, BOOL initPA)
{
    U32 ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_SET_SCR, (U8)sectionIndex, 0, 0);

    if (TRUE == initPA)
    {
        ctag = ctag | QLIB_TM_CTAG_SCR_NEED_INIT_PA_MASK;
    }

    QLIB_ASSERT_RET(qlibContext->keyMngr.kid == QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__FULL_ACCESS_SECTION, sectionIndex),
                    QLIB_STATUS__COMMAND_IGNORED);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext, ctag, SCRn, sizeof(SCRn_T), FALSE, NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_SCRn_swap(QLIB_CONTEXT_T* qlibContext,
                                           U32             sectionIndex,
                                           const SCRn_T    SCRn,
                                           BOOL            reset,
                                           BOOL            initPA)
{
    U32 ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_SET_SCR_SWAP, (U8)sectionIndex, 0, 0);

    if (TRUE == reset)
    {
        ctag = ctag | QLIB_TM_CTAG_SCR_NEED_RESET_MASK;
    }

    if (TRUE == initPA)
    {
        ctag = ctag | QLIB_TM_CTAG_SCR_NEED_INIT_PA_MASK;
    }

    QLIB_ASSERT_RET(qlibContext->keyMngr.kid == QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__FULL_ACCESS_SECTION, sectionIndex),
                    QLIB_STATUS__COMMAND_IGNORED);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext, ctag, SCRn, sizeof(SCRn_T), FALSE, NULL));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_SCRn_UNSIGNED(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex, SCRn_T SCRn)
{
    U32           ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_GET_SCR, (U8)sectionIndex, 0, 0);
    QLIB_STATUS_T ret  = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, ctag, SCRn, sizeof(SCRn_T));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_SCRn_SIGNED(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex, SCRn_T SCRn)
{
    QLIB_STATUS_T ret =
        QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_SECTION_CONFIG, sectionIndex, SCRn, sizeof(SCRn_T), NULL);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_RST_RESP(QLIB_CONTEXT_T* qlibContext, BOOL is_RST_RESP1, const U32* RST_RESP_half)
{
    _256BIT hash_output;
    U32     dataOutBuf[(_64B_ + sizeof(_64BIT)) / sizeof(U32)]; // RST_RESP_half + signature
    U32     ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_SET_RST_RESP, is_RST_RESP1 == TRUE ? 0 : 1, 0, 0);

    QLIB_ASSERT_RET(qlibContext->keyMngr.kid == QLIB_KID__DEVICE_MASTER, QLIB_STATUS__COMMAND_IGNORED);
    PLAT_HASH(hash_output, RST_RESP_half, _64B_);
    QLIB_STATUS_RET_CHECK(
        QLIB_CMD_PROC__sign_data_L(qlibContext, ctag, hash_output, sizeof(_256BIT), &dataOutBuf[_64B_ / sizeof(U32)], TRUE));
    memcpy(dataOutBuf, RST_RESP_half, _64B_);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext, ctag, dataOutBuf, sizeof(dataOutBuf)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_RST_RESP(QLIB_CONTEXT_T* qlibContext, U32* RST_RESP)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_RST_RESP), RST_RESP, _128B_);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__set_ACLR(QLIB_CONTEXT_T* qlibContext, ACLR_T ACLR)
{
    QLIB_STATUS_RET_CHECK(
        QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SET_ACLR), &ACLR, sizeof(ACLR_T)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_ACLR(QLIB_CONTEXT_T* qlibContext, ACLR_T* ACLR)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_ACLR), ACLR, sizeof(ACLR_T));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}


/*---------------------------------------------------------------------------------------------------------*/
/*                                        Session Control Commands                                         */
/*---------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_CMD_PROC__get_MC_UNSIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_MC_T mc)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    {
        ret =
            QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_MC), mc, sizeof(QLIB_MC_T));
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
        QLIB_STATUS_RET_CHECK(ret);
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_MC_SIGNED(QLIB_CONTEXT_T* qlibContext, QLIB_MC_T mc)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    {
        ret = QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_MC, 0, mc, sizeof(QLIB_MC_T), NULL);
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
        QLIB_STATUS_RET_CHECK(ret);
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__synch_MC(QLIB_CONTEXT_T* qlibContext)
{
    if (TRUE == qlibContext->mcInSync)
    {
        return QLIB_STATUS__OK;
    }

    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_MC_UNSIGNED(qlibContext, qlibContext->mc));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Verify MC                                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->mc[TC] >= 0x10, QLIB_STATUS__DEVICE_MC_ERR);
    QLIB_ASSERT_RET(qlibContext->mc[TC] < 0xFFFFFFFF, QLIB_STATUS__DEVICE_MC_ERR);
    QLIB_ASSERT_RET(qlibContext->mc[DMC] < QLIB_SEC_DMC_MAX_VAL, QLIB_STATUS__DEVICE_MC_ERR);

    qlibContext->mcInSync = TRUE;

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__MC_MAINT(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_MC_MAINT)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__Session_Open(QLIB_CONTEXT_T* qlibContext,
                                          QLIB_KID_T      kid,
                                          const KEY_T     key,
                                          BOOL            includeWID,
                                          BOOL            ignoreScrValidity)
{
    QLIB_WID_T    id;
    QLIB_MC_T     mc;
    U64           nonce = PLAT_GetNONCE();
    QLIB_STATUS_T ret   = QLIB_STATUS__COMMAND_FAIL;
    U8            mode  = 0;
    U32           ctag  = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* OP1, CTAG (32b), NONCE (64b), SIG (64b)                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    U32 buff[(sizeof(_64BIT) + sizeof(_64BIT)) / sizeof(U32)];

    /*-----------------------------------------------------------------------------------------------------*/
    /* CTAG = CMD (8b), KID (8b), MODE (8b), 8'b0                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_SEC_OPEN_CMD_MODE_SET(mode, includeWID, ignoreScrValidity);
    ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_SESSION_OPEN, kid, mode, 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* NONCE (64b)                                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    memcpy(buff, &nonce, sizeof(U64));

    /*-----------------------------------------------------------------------------------------------------*/
    /* SIG (64b)                                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    if (includeWID == TRUE)
    {
        memcpy(id, qlibContext->wid, sizeof(QLIB_WID_T));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Key                                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if (!QLIB_KEY_MNGR__IS_KEY_VALID(key))
    {
        U32  sectionID  = QLIB_KEY_MNGR__GET_KEY_SECTION(kid);
        BOOL fullAccess = QLIB_KEY_MNGR__GET_KEY_TYPE(kid) == QLIB_KID__FULL_ACCESS_SECTION ? TRUE : FALSE;
        QLIB_SEC_KEYMNGR_GetKey(&qlibContext->keyMngr, &key, sectionID, fullAccess);
        QLIB_ASSERT_RET(QLIB_KEY_MNGR__IS_KEY_VALID(key), QLIB_STATUS__INVALID_PARAMETER);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get, sync and advance MC                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_use_mc_L(qlibContext, mc));

    /*-----------------------------------------------------------------------------------------------------*/
    /* now we are ready to create the signature and current session key                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CRYPTO_SessionKeyAndSignature(key,
                                       ctag,
                                       mc,
                                       buff,
                                       (includeWID == TRUE) ? id : NULL,
                                       qlibContext->keyMngr.sessionKey,
                                       &buff[2],
                                       (U32*)&qlibContext->prng.state);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set session KID                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    qlibContext->keyMngr.kid = kid;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform open session command                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext, ctag, buff, sizeof(_64BIT) + sizeof(_64BIT)),
                               ret,
                               error);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Verify errors                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_KEY_MNGR__GET_KEY_TYPE(kid) == QLIB_KID__FULL_ACCESS_SECTION)
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__IGNORE_INTEG_ERR), ret, error);
    }
    else
    {
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS), ret, error);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Verify session is open wth correct KID                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_WITH_ERROR_GOTO(READ_VAR_FIELD(QLIB_GET_LAST_SEC_STATUS_FIELD(qlibContext), QLIB_REG_SSR__SES_READY) == 1,
                                QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE,
                                ret,
                                error);
    QLIB_ASSERT_WITH_ERROR_GOTO(READ_VAR_FIELD(QLIB_GET_LAST_SEC_STATUS_FIELD(qlibContext), QLIB_REG_SSR__KID) ==
                                    QLIB_KEY_MNGR__GET_KEY_SECTION(kid),
                                QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE,
                                ret,
                                error);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set SSK                                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    memcpy(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[0].hashBuf), qlibContext->keyMngr.sessionKey, sizeof(KEY_T));
    memcpy(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[1].hashBuf), qlibContext->keyMngr.sessionKey, sizeof(KEY_T));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set OK                                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    ret = QLIB_STATUS__OK;

    goto exit;

error:
    qlibContext->keyMngr.kid = QLIB_KID__INVALID;
    memset(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[0].hashBuf), 0xFF, sizeof(KEY_T));
    memset(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[1].hashBuf), 0xFF, sizeof(KEY_T));

exit:
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__Session_Close(QLIB_CONTEXT_T* qlibContext, QLIB_KID_T kid, BOOL revokePA)
{
    QLIB_STATUS_T ret = QLIB_STATUS__OK;

    if (revokePA == TRUE)
    {
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__init_section_PA(qlibContext, QLIB_KEY_MNGR__GET_KEY_SECTION(kid)));
        qlibContext->sectionsState[QLIB_KEY_MNGR__GET_KEY_SECTION(kid)].plainEnabled = 0;
        qlibContext->keyMngr.kid                                                     = QLIB_KID__INVALID;
    }
    else
    {
        QLIB_MC_T mc;
        U64       nonce = PLAT_GetNONCE();
        U8        mode  = 0;
        U32       ctag  = 0;
        KEY_T     key   = {0};
        /*-------------------------------------------------------------------------------------------------*/
        /* OP1, CTAG (32b), NONCE (64b), SIG (64b)                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        U32 buff[(sizeof(_64BIT) + sizeof(_64BIT)) / sizeof(U32)];

        /*-------------------------------------------------------------------------------------------------*/
        /* CTAG = CMD (8b), KID (8b), MODE (8b), 8'b0                                                      */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_SEC_OPEN_CMD_MODE_SET(mode, FALSE, FALSE);
        ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_SESSION_OPEN, kid, mode, 0);

        /*-------------------------------------------------------------------------------------------------*/
        /* NONCE (64b)                                                                                     */
        /*-------------------------------------------------------------------------------------------------*/
        memcpy(buff, &nonce, sizeof(U64));

        /*-------------------------------------------------------------------------------------------------*/
        /* Get, sync and advance MC                                                                        */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_use_mc_L(qlibContext, mc));

        /*-------------------------------------------------------------------------------------------------*/
        /* now we are ready to create the signature and current session key                                */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_CRYPTO_SessionKeyAndSignature(key,
                                           ctag,
                                           mc,
                                           buff,
                                           NULL,
                                           qlibContext->keyMngr.sessionKey,
                                           &buff[2],
                                           (U32*)&qlibContext->prng.state);

        /*-------------------------------------------------------------------------------------------------*/
        /* Perform bad open session command                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext, ctag, buff, sizeof(_64BIT) + sizeof(_64BIT)),
                                   ret,
                                   error);

        qlibContext->keyMngr.kid = QLIB_KID__INVALID;
        /*-------------------------------------------------------------------------------------------------*/
        /* Clear errors                                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, 0), ret, error);

        /*-------------------------------------------------------------------------------------------------*/
        /* Verify session is close with no errors                                                          */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_SEC__get_SSR(qlibContext, NULL, SSR_MASK__ALL_ERRORS), ret, error);

        QLIB_ASSERT_WITH_ERROR_GOTO(READ_VAR_FIELD(QLIB_GET_LAST_SEC_STATUS_FIELD(qlibContext), QLIB_REG_SSR__SES_READY) == 0,
                                    QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE,
                                    ret,
                                    error);
    }
    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear context                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    memset(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[0].hashBuf), 0xFF, sizeof(KEY_T));
    memset(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[1].hashBuf), 0xFF, sizeof(KEY_T));

error:
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__init_section_PA(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex)
{
    QLIB_STATUS_RET_CHECK(
        QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext,
                                           QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_INIT_SECTION_PA, sectionIndex, 0, 0)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__IGNORE_INTEG_ERR));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__CALC_CDI(QLIB_CONTEXT_T* qlibContext, U32 mode, _256BIT cdi)
{
    U32                    buff[(sizeof(U32) + sizeof(_256BIT)) / sizeof(U32)];
    QLIB_STATUS_T          ret = QLIB_STATUS__SECURITY_ERR;
    QLIB_CRYPTO_CONTEXT_T* cryptContext;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Prepare decryption key                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_initialize_decryption_key(qlibContext, cryptContext);

    /*-----------------------------------------------------------------------------------------------------*/
    /* execute the command                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext,
                                             QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_CALC_CDI, mode ? 1 : 0, 0, 0),
                                             buff,
                                             sizeof(buff));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    QLIB_STATUS_RET_CHECK(ret);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Decrypt the data                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CRYPTO_EncryptData(cdi, &buff[1], cryptContext->cipherKey, QLIB_SEC_READ_PAGE_SIZE_BYTE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* verify the transaction counter  (HW returns previous transaction number)                            */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(buff[0] == (qlibContext->mc[TC] - 1), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__Check_Integrity(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex)
{
    QLIB_STATUS_RET_CHECK(
        QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext,
                                           QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_VER_INTG, sectionIndex, 0, 0)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/*                                       Secure Transport Commands                                         */
/*---------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_CMD_PROC__get_TC(QLIB_CONTEXT_T* qlibContext, U32* tc)
{
    {
        QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

        ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_TC), tc, sizeof(U32));
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
        QLIB_STATUS_RET_CHECK(ret);
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__CALC_SIG(QLIB_CONTEXT_T*         qlibContext,
                                      QLIB_SIGNED_DATA_TYPE_T dataType,
                                      U32                     section,
                                      U32*                    data,
                                      U32                     size,
                                      _64BIT                  signature)
{
    U32 buff[(sizeof(U32) + QLIB_SIGNED_DATA_MAX_SIZE + sizeof(_64BIT)) / sizeof(U32)];
    U32 tc_counter_HW = 0;
    U32 tc_counter_SW = 0;

    U32 data_size = QLIB_SIGNED_DATA_TYPE_GET_SIZE(dataType);
    U8  data_id   = QLIB_SIGNED_DATA_TYPE_GET_ID(dataType, section);

    U32           ctag = QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_CALC_SIG, data_id, 0, 0);
    _64BIT        calculated_signature;
    QLIB_STATUS_T ret = QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(size >= data_size, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Make sure MC is in sync before CALC_SIG command since MC is used in the sign stage                  */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__synch_MC(qlibContext));

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext, ctag, buff, sizeof(buff));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    QLIB_STATUS_RET_CHECK(ret);

    /*-----------------------------------------------------------------------------------------------------*/
    /* verify the signature                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__sign_data_L(qlibContext, ctag, &buff[1], data_size, calculated_signature, TRUE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* verify the transaction counter (HW returns previous transaction number)                             */
    /*-----------------------------------------------------------------------------------------------------*/
    tc_counter_SW = qlibContext->mc[TC];
    memcpy(&tc_counter_HW, buff, sizeof(U32));
    QLIB_ASSERT_RET(tc_counter_HW == (tc_counter_SW - 1), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check signature                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    if (memcmp(calculated_signature, (U8*)buff + sizeof(U32) + data_size, sizeof(_64BIT)))
    {
        ret = QLIB_STATUS__SECURITY_ERR;
    }
    else
    {
        ret = QLIB_STATUS__OK;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Parameters back to user                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (signature != NULL)
    {
        memcpy(signature, ((U8*)buff + sizeof(U32) + data_size), sizeof(_64BIT));
    }
    if (data != NULL)
    {
        memcpy(data, ((U8*)buff + sizeof(U32)), data_size);
    }

    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__SRD(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data32B)
{
    QLIB_CRYPTO_CONTEXT_T* cryptContext;
    QLIB_STATUS_T          ret = QLIB_STATUS__SECURITY_ERR;
#ifdef Q2_DEBUG_ASSERTS // 5-LS bits are ignored, so we do not want to fail this in field
    QLIB_ASSERT_RET(addr == ROUND_DOWN(addr, QLIB_SEC_READ_PAGE_SIZE_BYTE), QLIB_STATUS__INVALID_PARAMETER);
#endif

    /*-----------------------------------------------------------------------------------------------------*/
    /* Build decryption cipher key                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_initialize_decryption_key(qlibContext, cryptContext);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Randomize address                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    addr = addr ^ QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(QLIB_SEC_READ_PAGE_SIZE_BYTE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Encrypt address                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_encrypt_address(addr, addr, cryptContext->cipherKey);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform read                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext,
                                             QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SRD, addr),
                                             QLIB_HASH_BUF_GET__READ_PAGE(cryptContext->hashBuf),
                                             sizeof(U32) + QLIB_SEC_READ_PAGE_SIZE_BYTE);

    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    QLIB_STATUS_RET_CHECK(ret);

    /*-----------------------------------------------------------------------------------------------------*/
    /* verify the transaction counter (HW returns previous transaction number)                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET((QLIB_HASH_BUF_GET__READ_TC(cryptContext->hashBuf) + 1) == qlibContext->mc[TC],
                    QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Decrypt the data                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CRYPTO_EncryptData(data32B,
                            QLIB_HASH_BUF_GET__DATA(cryptContext->hashBuf),
                            cryptContext->cipherKey,
                            QLIB_SEC_READ_PAGE_SIZE_BYTE);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__SARD(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data32B)
{
    QLIB_CRYPTO_CONTEXT_T* cryptContext;
    U32                    enc_addr;

    _64BIT        received_signature;
    _64BIT        calculated_signature;
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;


    /*-----------------------------------------------------------------------------------------------------*/
    /* Build decryption cipher key                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_initialize_decryption_key(qlibContext, cryptContext);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Randomize address                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    addr = addr ^ QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(QLIB_SEC_READ_PAGE_SIZE_BYTE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Encrypt address                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_encrypt_address(enc_addr, addr, cryptContext->cipherKey);

    /*-----------------------------------------------------------------------------------------------------*/
    /* execute the command                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/

    ret =
        QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext,
                                           QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SARD, enc_addr),
                                           QLIB_HASH_BUF_GET__READ_PAGE(cryptContext->hashBuf),
                                           sizeof(U32) + QLIB_SEC_READ_PAGE_SIZE_BYTE + sizeof(_64BIT)); // TC + DATA + Signature
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    QLIB_STATUS_RET_CHECK(ret);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Decrypt the data                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CRYPTO_EncryptData(QLIB_HASH_BUF_GET__DATA(cryptContext->hashBuf),
                            QLIB_HASH_BUF_GET__DATA(cryptContext->hashBuf),
                            cryptContext->cipherKey,
                            QLIB_SEC_READ_PAGE_SIZE_BYTE);
    memcpy(data32B, QLIB_HASH_BUF_GET__DATA(cryptContext->hashBuf), QLIB_SEC_READ_PAGE_SIZE_BYTE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* verify the transaction counter (HW returns previous transaction number)                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET((QLIB_HASH_BUF_GET__READ_TC(cryptContext->hashBuf) + 1) == qlibContext->mc[TC],
                    QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Backup the signature so we can reuse the data buffer                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    received_signature[0] = QLIB_HASH_BUF_GET__READ_SIG(cryptContext->hashBuf)[0];
    received_signature[1] = QLIB_HASH_BUF_GET__READ_SIG(cryptContext->hashBuf)[1];

    /*-----------------------------------------------------------------------------------------------------*/
    /* calculate the signature                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_HASH_BUF_GET__CTAG(cryptContext->hashBuf) = QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SARD, addr);
    QLIB_CRYPTO_CalcAuthSignature(cryptContext->hashBuf, qlibContext->keyMngr.kid, calculated_signature);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Signature check                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((calculated_signature[0] == received_signature[0]) && (calculated_signature[1] == received_signature[1]))
    {
        ret = QLIB_STATUS__OK;
    }
    else
    {
        memset(data32B, 0, QLIB_SEC_READ_PAGE_SIZE_BYTE);
        ret = QLIB_STATUS__SECURITY_ERR;
    }

    return ret;
}

#ifndef QLIB_SUPPORT_XIP
QLIB_STATUS_T QLIB_CMD_PROC__SRD_Multi(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data, U32 size)
{
    QLIB_CRYPTO_CONTEXT_T* cryptContext_old = NULL;
    QLIB_CRYPTO_CONTEXT_T* cryptContext_new = NULL;
    QLIB_STATUS_T          ret              = QLIB_STATUS__SECURITY_ERR;
    U32                    enc_addr         = 0;
    U32                    rand             = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Build decryption cipher key                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_initialize_decryption_key(qlibContext, cryptContext_old);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Randomize address                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    rand = QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(QLIB_SEC_READ_PAGE_SIZE_BYTE));
    addr = addr ^ rand;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Encrypt address                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_encrypt_address(enc_addr, addr, cryptContext_old->cipherKey);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start read transaction (non-blocking)                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__OP1_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SRD, enc_addr)));

    do
    {
        if (size > QLIB_SEC_READ_PAGE_SIZE_BYTE)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Build next cipher without advancing TC (in case it is not used)                             */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_KEY_MNGR__CMD_CONTEXT_ADVANCE(qlibContext);
            cryptContext_new = &QLIB_KEY_MNGR__CMD_CONTEXT_GET(qlibContext);
            QLIB_CMD_PROC_update_decryption_key_async(qlibContext, cryptContext_new);

            /*---------------------------------------------------------------------------------------------*/
            /* Generate random                                                                             */
            /*---------------------------------------------------------------------------------------------*/
            rand = QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(QLIB_SEC_READ_PAGE_SIZE_BYTE));

            /*---------------------------------------------------------------------------------------------*/
            /* Calculate next address                                                                      */
            /*---------------------------------------------------------------------------------------------*/
            addr += QLIB_SEC_READ_PAGE_SIZE_BYTE;
            addr = addr ^ rand;

            /*---------------------------------------------------------------------------------------------*/
            /* Increment TC                                                                                */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_TRANSACTION_CNTR_USE(qlibContext);
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Wait while busy, check for errors and read data                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        ret = QLIB_CMD_PROC__OP0_busy_wait_OP2(qlibContext,
                                               QLIB_HASH_BUF_GET__READ_PAGE(cryptContext_old->hashBuf),
                                               sizeof(U32) + QLIB_SEC_READ_PAGE_SIZE_BYTE);

        /*-------------------------------------------------------------------------------------------------*/
        /* Start next command                                                                              */
        /*-------------------------------------------------------------------------------------------------*/
        if (size > QLIB_SEC_READ_PAGE_SIZE_BYTE)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Wait till cipher is ready                                                                   */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_CMD_PROC_update_decryption_key_async_wait_till_ready(qlibContext);

            /*---------------------------------------------------------------------------------------------*/
            /* Encrypt next address                                                                        */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_CMD_PROC_encrypt_address(enc_addr, addr, cryptContext_new->cipherKey);

            /*---------------------------------------------------------------------------------------------*/
            /* Start next read transaction (non-blocking)                                                  */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_STATUS_RET_CHECK(
                QLIB_CMD_PROC__OP1_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SRD, enc_addr)));
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Check errors after starting new command                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
        QLIB_STATUS_RET_CHECK(ret);

        /*-------------------------------------------------------------------------------------------------*/
        /* Decrypt with old cipher                                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_CRYPTO_EncryptData_INLINE(data, QLIB_HASH_BUF_GET__DATA(cryptContext_old->hashBuf), cryptContext_old->cipherKey, 8);

        /*-------------------------------------------------------------------------------------------------*/
        /* Update variables                                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        if (size > QLIB_SEC_READ_PAGE_SIZE_BYTE)
        {
            data += QLIB_SEC_READ_PAGE_SIZE_BYTE / sizeof(U32);
            size -= QLIB_SEC_READ_PAGE_SIZE_BYTE;
            cryptContext_old = cryptContext_new;
        }
        else
        {
            size = 0; // exit loop
        }
    } while (size);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__SARD_Multi(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data, U32 size)
{
    QLIB_CRYPTO_CONTEXT_T* cryptContext_old = NULL;
    QLIB_CRYPTO_CONTEXT_T* cryptContext_new = NULL;
    _64BIT                 received_signature;
    _64BIT                 calculated_signature;
    QLIB_STATUS_T          ret      = QLIB_STATUS__SECURITY_ERR;
    U32                    enc_addr = 0;
    U32                    rand     = 0;
    U32                    new_addr = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Build decryption cipher key                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CMD_PROC_initialize_decryption_key(qlibContext, cryptContext_old);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Encrypt address                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    rand = QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(QLIB_SEC_READ_PAGE_SIZE_BYTE));
    addr = addr ^ rand;
    QLIB_CMD_PROC_encrypt_address(enc_addr, addr, cryptContext_old->cipherKey);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start read transaction (non-blocking)                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__OP1_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SARD, enc_addr)));

    do
    {
        if (size > QLIB_SEC_READ_PAGE_SIZE_BYTE)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Build next cipher without advancing TC (in case it is not used)                             */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_KEY_MNGR__CMD_CONTEXT_ADVANCE(qlibContext);
            cryptContext_new = &QLIB_KEY_MNGR__CMD_CONTEXT_GET(qlibContext);
            QLIB_CMD_PROC_update_decryption_key_async(qlibContext, cryptContext_new);

            /*---------------------------------------------------------------------------------------------*/
            /* Generate random                                                                             */
            /*---------------------------------------------------------------------------------------------*/
            rand = QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(QLIB_SEC_READ_PAGE_SIZE_BYTE));

            /*---------------------------------------------------------------------------------------------*/
            /* Calculate next address                                                                      */
            /*---------------------------------------------------------------------------------------------*/
            new_addr = addr + QLIB_SEC_READ_PAGE_SIZE_BYTE;
            new_addr = new_addr ^ rand;

            /*---------------------------------------------------------------------------------------------*/
            /* Increment TC                                                                                */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_TRANSACTION_CNTR_USE(qlibContext);
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Wait while busy, check for errors and read data                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        ret = QLIB_CMD_PROC__OP0_busy_wait_OP2(qlibContext,
                                               QLIB_HASH_BUF_GET__READ_PAGE(cryptContext_old->hashBuf),
                                               sizeof(U32) + QLIB_SEC_READ_PAGE_SIZE_BYTE + sizeof(U64));

        /*-------------------------------------------------------------------------------------------------*/
        /* Start next command                                                                              */
        /*-------------------------------------------------------------------------------------------------*/
        if (size > QLIB_SEC_READ_PAGE_SIZE_BYTE)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Wait till cipher is ready                                                                   */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_CMD_PROC_update_decryption_key_async_wait_till_ready(qlibContext);

            /*---------------------------------------------------------------------------------------------*/
            /* Encrypt next address                                                                        */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_CMD_PROC_encrypt_address(enc_addr, new_addr, cryptContext_new->cipherKey);

            /*---------------------------------------------------------------------------------------------*/
            /* Start next read transaction (non-blocking)                                                  */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_STATUS_RET_CHECK(
                QLIB_CMD_PROC__OP1_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SARD, enc_addr)));
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Check errors after starting new command                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
        QLIB_STATUS_RET_CHECK(ret);

        /*-------------------------------------------------------------------------------------------------*/
        /* Decrypt with old cipher                                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_CRYPTO_EncryptData_INLINE(QLIB_HASH_BUF_GET__DATA(cryptContext_old->hashBuf),
                                       QLIB_HASH_BUF_GET__DATA(cryptContext_old->hashBuf),
                                       cryptContext_old->cipherKey,
                                       8);
        ARRAY_COPY_INLINE(data, QLIB_HASH_BUF_GET__DATA(cryptContext_old->hashBuf), 8);

        /*-------------------------------------------------------------------------------------------------*/
        /* Verify signature                                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        received_signature[0] = QLIB_HASH_BUF_GET__READ_SIG(cryptContext_old->hashBuf)[0];
        received_signature[1] = QLIB_HASH_BUF_GET__READ_SIG(cryptContext_old->hashBuf)[1];

        /*-------------------------------------------------------------------------------------------------*/
        /* calculate the signature                                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_HASH_BUF_GET__CTAG(cryptContext_old->hashBuf) = QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SARD, addr);
        QLIB_CRYPTO_CalcAuthSignature(cryptContext_old->hashBuf, qlibContext->keyMngr.kid, calculated_signature);

        /*-------------------------------------------------------------------------------------------------*/
        /* Signature check                                                                                 */
        /*-------------------------------------------------------------------------------------------------*/
        if ((calculated_signature[0] != received_signature[0]) || (calculated_signature[1] != received_signature[1]))
        {
            /*---------------------------------------------------------------------------------------------*/
            /*                               Signature verification failed!                                */
            /*---------------------------------------------------------------------------------------------*/

            /*---------------------------------------------------------------------------------------------*/
            /* Wait till transaction is finished                                                           */
            /*---------------------------------------------------------------------------------------------*/
            (void)QLIB_CMD_PROC__OP0_busy_wait(qlibContext);

            /*---------------------------------------------------------------------------------------------*/
            /* Clear data                                                                                  */
            /*---------------------------------------------------------------------------------------------*/
            memset(data, 0, size);

            return QLIB_STATUS__SECURITY_ERR;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Update variables                                                                                */
        /*-------------------------------------------------------------------------------------------------*/
        if (size > QLIB_SEC_READ_PAGE_SIZE_BYTE)
        {
            data += QLIB_SEC_READ_PAGE_SIZE_BYTE / sizeof(U32);
            size -= QLIB_SEC_READ_PAGE_SIZE_BYTE;
            addr             = new_addr;
            cryptContext_old = cryptContext_new;
        }
        else
        {
            size = 0; // exit loop
        }
    } while (size);

    return QLIB_STATUS__OK;
}
#endif // QLIB_SUPPORT_XIP

QLIB_STATUS_T QLIB_CMD_PROC__SAWR(QLIB_CONTEXT_T* qlibContext, U32 addr, const U32* data)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------------------------------------*/
    /* Randomize 5-LS bits to prevent zero bits encryption                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    addr = addr ^ QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(QLIB_SEC_WRITE_PAGE_SIZE_BYTE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Send the command                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext,
                                                                 QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SAWR, addr),
                                                                 data,
                                                                 QLIB_SEC_WRITE_PAGE_SIZE_BYTE,
                                                                 TRUE,
                                                                 &addr));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__SERASE(QLIB_CONTEXT_T* qlibContext, QLIB_ERASE_T type, U32 addr)
{
    U32  ctag   = 0;
    U32* addr_p = NULL;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Select erase type and randomize LS bits to prevent zero bits encryption                             */
    /*-----------------------------------------------------------------------------------------------------*/
    switch (type)
    {
        case QLIB_ERASE_SECTOR_4K:

            addr   = addr ^ QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(_4KB_));
            ctag   = QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SERASE_4, addr);
            addr_p = &addr;
            break;

        case QLIB_ERASE_BLOCK_32K:
            addr   = addr ^ QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(_32KB_));
            ctag   = QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SERASE_32, addr);
            addr_p = &addr;
            break;

        case QLIB_ERASE_BLOCK_64K:
            addr   = addr ^ QLIB_CRYPTO_GetRandBits(&qlibContext->prng, LOG2(_64KB_));
            ctag   = QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_SEC_SERASE_64, addr);
            addr_p = &addr;
            break;

        case QLIB_ERASE_SECTION:
            ctag = QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SERASE_SEC);
            break;

        case QLIB_ERASE_CHIP:
            ctag = QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SERASE_ALL);
            break;

        default:
            return QLIB_STATUS__INVALID_PARAMETER;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Send the command                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__execute_signed_setter_L(qlibContext, ctag, NULL, 0, FALSE, addr_p));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__ERASE_SECT_PA(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex)
{
    QLIB_STATUS_RET_CHECK(
        QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext,
                                           QLIB_CMD_PROC__MAKE_CTAG_PARAMS(QLIB_CMD_SEC_ERASE_SECT_PA, sectionIndex, 0, 0)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/*                                           Auxiliary Commands                                            */
/*---------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_CMD_PROC__get_HW_VER_UNSIGNED(QLIB_CONTEXT_T* qlibContext, HW_VER_T* hwVer)
{
    QLIB_STATUS_T ret = QLIB_STATUS__SECURITY_ERR;

    ret = QLIB_CMD_PROC_execute_sec_cmd_read(qlibContext,
                                             QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_VERSION),
                                             (U32*)hwVer,
                                             sizeof(U32));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__get_HW_VER_SIGNED(QLIB_CONTEXT_T* qlibContext, HW_VER_T* hwVer)
{
    QLIB_STATUS_T ret = QLIB_CMD_PROC__CALC_SIG(qlibContext, QLIB_SIGNED_DATA_TYPE_HW_VER, 0, hwVer, sizeof(HW_VER_T), NULL);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return ret;
}

QLIB_STATUS_T QLIB_CMD_PROC__AWDT_expire(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_AWDT_EXPIRE)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__sleep(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_execute_sec_cmd_only(qlibContext, QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_SLEEP)));
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__checkLastSsrErrors(qlibContext, SSR_MASK__ALL_ERRORS));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_CMD_PROC__checkLastSsrErrors(QLIB_CONTEXT_T* qlibContext, U32 mask)
{
    U32           ssrReg       = QLIB_GET_LAST_SEC_STATUS_FIELD(qlibContext);
    U32           ssrRegMasked = ssrReg & mask;
    QLIB_STATUS_T ssrError     = QLIB_STATUS__OK;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check for errors                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    if (ssrRegMasked)
    {
        if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__SES_ERR_S))
        {
            ssrError = QLIB_STATUS__DEVICE_SESSION_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__INTG_ERR_S))
        {
            ssrError = QLIB_STATUS__DEVICE_INTEGRITY_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__AUTH_ERR_S))
        {
            ssrError = QLIB_STATUS__DEVICE_AUTHENTICATION_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__PRIV_ERR_S))
        {
            ssrError = QLIB_STATUS__DEVICE_PRIVILEGE_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__IGNORE_ERR_S))
        {
            ssrError = QLIB_STATUS__COMMAND_IGNORED;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__SYS_ERR_S))
        {
            ssrError = QLIB_STATUS__DEVICE_SYSTEM_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__FLASH_ERR_S))
        {
            ssrError = QLIB_STATUS__DEVICE_FLASH_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__MC_ERR))
        {
            ssrError = QLIB_STATUS__DEVICE_MC_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__ERR))
        {
            ssrError = QLIB_STATUS__DEVICE_ERR;
        }
        else if (READ_VAR_FIELD(ssrRegMasked, QLIB_REG_SSR__BUSY))
        {
            ssrError = QLIB_STATUS__DEVICE_BUSY;
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Clear SSR if error occurred and synch MC                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    if (READ_VAR_FIELD(ssrReg, QLIB_REG_SSR__ERR))
    {
        QLIB_REG_SSR_T ssr;
        QLIB_STATUS_T  ret;

        if (qlibContext->multiTransactionCmd == TRUE)
        {
            qlibContext->multiTransactionCmd = FALSE;

#ifdef QLIB_SPI_OPTIMIZATION_ENABLED
            PLAT_SPI_MultiTransactionStop();
#endif //QLIB_SPI_OPTIMIZATION_ENABLED
        }
        ret = QLIB_CMD_PROC_execute_sec_cmd(qlibContext,
                                            QLIB_CMD_PROC__MAKE_CTAG(QLIB_CMD_SEC_GET_MC),
                                            NULL,
                                            0,
                                            qlibContext->mc,
                                            sizeof(QLIB_MC_T),
                                            &ssr);

        if (QLIB_STATUS__OK != ret || READ_VAR_FIELD(ssr.asUint, QLIB_REG_SSR__ERR))
        {
            qlibContext->mcInSync = FALSE;
        }
    }

    return ssrError;
}


/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                             LOCAL FUNCTIONS                                             */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine returns current sw MC counter which is synchronized to hw if needed
 *
 * @param[in,out]   qlibContext   Context
 * @param[out]      mc           Full MC
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC_use_mc_L(QLIB_CONTEXT_T* qlibContext, QLIB_MC_T mc)
{
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__synch_MC(qlibContext));

    mc[TC]  = QLIB_TRANSACTION_CNTR_USE(qlibContext);
    mc[DMC] = qlibContext->mc[DMC];

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This routine refreshes SSK
 *
 * @param[in,out]   qlibContext   Context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC_refresh_ssk_L(QLIB_CONTEXT_T* qlibContext)
{
    _64BIT mc;
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Get MC                                                                                          */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_use_mc_L(qlibContext, mc));
        QLIB_CRYPTO_put_salt_on_session_key(mc[TC],
                                            QLIB_HASH_BUF_GET__KEY(QLIB_KEY_MNGR__CMD_CONTEXT_GET(qlibContext).hashBuf),
                                            qlibContext->keyMngr.sessionKey);
    }

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This routine signs the given data
 *
 * @param[in]   qlibContext        Context
 * @param[in]   plain_ctag         CTAG
 * @param[in]   data_up_to256bit   Data to sign
 * @param[in]   data_size          Data size
 * @param[out]  signature          Data signature
 * @param[in]   refresh_ssk        if TRUE, SSK will be refreshed
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__sign_data_L(QLIB_CONTEXT_T* qlibContext,
                                         U32             plain_ctag,
                                         const U32*      data_up_to256bit,
                                         U32             data_size,
                                         U32*            signature,
                                         BOOL            refresh_ssk)
{
    QLIB_CRYPTO_CONTEXT_T* cryptContext = &QLIB_KEY_MNGR__CMD_CONTEXT_GET(qlibContext);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Refresh SSK key if required                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    if (TRUE == refresh_ssk)
    {
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC_refresh_ssk_L(qlibContext));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform signing                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_HASH_BUF_GET__CTAG(cryptContext->hashBuf) = plain_ctag;
    memcpy(QLIB_HASH_BUF_GET__DATA(cryptContext->hashBuf), data_up_to256bit, data_size);
    memset((((U8*)QLIB_HASH_BUF_GET__DATA(cryptContext->hashBuf)) + data_size), 0, sizeof(_256BIT) - data_size);

    QLIB_CRYPTO_CalcAuthSignature(cryptContext->hashBuf, qlibContext->keyMngr.kid, signature);

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This routine signs and encrypts the given data & address
 *
 * @param[in]       qlibContext                 Context
 * @param[out]      ctag                        CTAG pointer. If address is available, returns CTAG with encrypted address
 * @param[in]       data                        Data to sign/encrypt
 * @param[in]       data_size                   Data size
 * @param[out]      signature                   Data signature
 * @param[out]      encrypted_data              Returned encrypted data-in not NULL if data encryption is not required
 * @param[in,out]   addr                        Address to encrypt or NULL if address encryption is not required
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__prepare_signed_setter_cmd_L(QLIB_CONTEXT_T* qlibContext,
                                                         U32*            ctag,
                                                         const U32*      data,
                                                         U32             data_size,
                                                         U32*            signature,
                                                         U32*            encrypted_data,
                                                         U32*            addr)
{
    QLIB_CRYPTO_CONTEXT_T* cryptContext;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Create signature of plain-text data                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__sign_data_L(qlibContext, *ctag, data, data_size, signature, TRUE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Shall we encrypt?                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((encrypted_data != NULL) || (addr != NULL))
    {
        cryptContext = &QLIB_KEY_MNGR__CMD_CONTEXT_GET(qlibContext);

        /*-------------------------------------------------------------------------------------------------*/
        /* Prepare encryption key                                                                          */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_CMD_PROC_build_encryption_key(qlibContext, cryptContext);

        /*-------------------------------------------------------------------------------------------------*/
        /* encrypt address and patch CTAG                                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        if (addr != NULL)
        {
            QLIB_CMD_PROC_encrypt_address(*addr, *addr, cryptContext->cipherKey);
            *ctag = QLIB_CMD_PROC__MAKE_CTAG_ADDR(QLIB_CMD_PROC__CTAG_GET_CMD(*ctag), *addr);
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Encrypt data if needed                                                                          */
        /*-------------------------------------------------------------------------------------------------*/
        if (encrypted_data != NULL)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Encrypt data                                                                                */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_CRYPTO_EncryptData(encrypted_data, data, cryptContext->cipherKey, data_size);
        }
    }

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This routine executes a given command with data and optionally encrypts it
 *
 * @param[in]       qlibContext    Context
 * @param[in]       ctag           CTAG
 * @param[in]       data           Data
 * @param[in]       data_size      Data size
 * @param[in]       encrypt_data   if TRUE, data will be sent encrypted
 * @param[in,out]   addr           if not NULL, address is encrypted into ctag
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_CMD_PROC__execute_signed_setter_L(QLIB_CONTEXT_T* qlibContext,
                                                     U32             ctag,
                                                     const U32*      data,
                                                     U32             data_size,
                                                     BOOL            encrypt_data,
                                                     U32*            addr)
{
    U32 dataOutBuf[(sizeof(_256BIT) + sizeof(_64BIT)) / sizeof(U32)]; // Maximum data + signature

    /*-----------------------------------------------------------------------------------------------------*/
    /* Encrypt and sign the command                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__prepare_signed_setter_cmd_L(qlibContext,
                                                                     &ctag,
                                                                     data,
                                                                     data_size,
                                                                     &dataOutBuf[data_size / sizeof(U32)],
                                                                     encrypt_data == TRUE ? dataOutBuf : NULL,
                                                                     addr));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Copy the data manually to the output buffer is not encrypted                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    if (FALSE == encrypt_data)
    {
        memcpy(dataOutBuf, data, data_size);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Execute command                                                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    return QLIB_CMD_PROC_execute_sec_cmd_write(qlibContext, ctag, dataOutBuf, data_size + sizeof(_64BIT));
}
