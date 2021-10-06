/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sec.c
* @brief      This file contains security features implementation
*
* ### project qlib
*
************************************************************************************************************/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_sec.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                              DEFINITIONS                                                */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * Threshold value for DMC.
 * @ref QLIB_GetNotifications will return replaceDevice notification if DMC reach this value.
 * User may choose to define a different value .
************************************************************************************************************/
#ifndef QLIB_SEC_DMC_EOL_THRESHOLD
#define QLIB_SEC_DMC_EOL_THRESHOLD 0x3FFFF000
#endif
/************************************************************************************************************
 * Threshold value for TC.
 * @ref QLIB_GetNotifications will report resetDevice notification if TC reach this value.
 * User may choose to define a different value .
************************************************************************************************************/
#ifndef QLIB_SEC_TC_RESET_THRESHOLD
#define QLIB_SEC_TC_RESET_THRESHOLD 0xFFFFFFF0
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                  TYPES                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
typedef QLIB_STATUS_T (*QLIB_READ_FUNC_T)(QLIB_CONTEXT_T* qlibContext, U32 addr, U32* data32B);

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                          FORWARD DECLARATION                                            */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
static QLIB_STATUS_T QLIB_SEC_EnablePlainAccess_L(QLIB_CONTEXT_T* qlibContext, U32 sectionID);
static QLIB_STATUS_T QLIB_SEC_KeyProvisioning_L(QLIB_CONTEXT_T* qlibContext,
                                                const KEY_T     Kd,
                                                QLIB_KID_T      new_kid,
                                                const KEY_T     new_key);
static QLIB_STATUS_T QLIB_SEC_setAllKeys_L(QLIB_CONTEXT_T*   qlibContext,
                                           const KEY_T       deviceMasterKey,
                                           const KEY_T       deviceSecretKey,
                                           const KEY_ARRAY_T restrictedKeys,
                                           const KEY_ARRAY_T fullAccessKeys);
static QLIB_STATUS_T QLIB_SEC_configGMC_L(QLIB_CONTEXT_T*             qlibContext,
                                          const KEY_T                 deviceMasterKey,
                                          const QLIB_WATCHDOG_CONF_T* watchdogDefault,
                                          const QLIB_DEVICE_CONF_T*   deviceConf);
static QLIB_STATUS_T QLIB_SEC_configGMT_L(QLIB_CONTEXT_T*                   qlibContext,
                                          const KEY_T                       deviceMasterKey,
                                          const QLIB_SECTION_CONFIG_TABLE_T sectionTable);
static QLIB_STATUS_T QLIB_SEC_OpenSessionInternal_L(QLIB_CONTEXT_T* qlibContext,
                                                    QLIB_KID_T      kid,
                                                    const KEY_T     keyBuf,
                                                    BOOL            ignoreScrValidity);
static QLIB_STATUS_T QLIB_SEC_CloseSessionInternal_L(QLIB_CONTEXT_T* qlibContext, BOOL revoke);
static QLIB_STATUS_T QLIB_SEC_GetWID_L(QLIB_CONTEXT_T* qlibContext, QLIB_WID_T id);
static QLIB_STATUS_T QLIB_SEC_GetStdAddrSize_L(QLIB_CONTEXT_T* qlibContext);
static QLIB_STATUS_T QLIB_SEC_GetSectionsSize_L(QLIB_CONTEXT_T* qlibContext);
static QLIB_STATUS_T QLIB_SEC_GetWatchdogConfig_L(QLIB_CONTEXT_T* qlibContext);
static QLIB_STATUS_T QLIB_SEC_MarkSessionClose_L(QLIB_CONTEXT_T* qlibContext);
static QLIB_STATUS_T QLIB_SEC_ConfigInitialSectionPolicy_L(QLIB_CONTEXT_T*      qlibContext,
                                                           U32                  sectionIndex,
                                                           const QLIB_POLICY_T* policy,
                                                           const KEY_T          fullAccessKey);
static BOOL          QLIB_SEC_ConfigSectionPolicyAfterGMT_L(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex);
static QLIB_STATUS_T QLIB_SEC_GrantRevokePA_L(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL grant);
#ifndef QLIB_SEC_ONLY
static QLIB_STATUS_T QLIB_SEC_VerifyAddressSizeConfig_L(QLIB_CONTEXT_T* qlibContext, const QLIB_STD_ADDR_SIZE_T* addrSizeConf);
#endif
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                          INTERFACE FUNCTIONS                                            */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_SEC_InitLib(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure the globals                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    qlibContext->isSuspended         = FALSE;
    qlibContext->isPoweredDown       = FALSE;
    qlibContext->multiTransactionCmd = FALSE;
    qlibContext->mcInSync            = FALSE;
    qlibContext->watchdogIsSecure    = FALSE;
    qlibContext->watchdogSectionId   = QLIB_NUM_OF_SECTIONS;

    // Set the cached SSR busy bit in order to mark it as invalid
    SET_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__BUSY, 1);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Init the key manager module                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_KEYMNGR_Init(&qlibContext->keyMngr));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_SyncState(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Sync after reset                                                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_SyncAfterFlashReset(qlibContext));

    /*-----------------------------------------------------------------------------------------------------*/
    /* read WID on reset once                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_GetWID_L(qlibContext, qlibContext->wid));

    /*-----------------------------------------------------------------------------------------------------*/
    /* read standard address size                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_GetStdAddrSize_L(qlibContext));

    /*-----------------------------------------------------------------------------------------------------*/
    /* read Global Mapping Table                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_GetSectionsSize_L(qlibContext));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get watchdog configuration                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_GetWatchdogConfig_L(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_Format(QLIB_CONTEXT_T* qlibContext, const KEY_T deviceMasterKey, BOOL eraseDataOnly)
{
    QLIB_STATUS_T ret = QLIB_STATUS__COMMAND_FAIL;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    if (NULL == deviceMasterKey)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Perform non-secure full device format                                                           */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_ASSERT_RET(FALSE == eraseDataOnly, QLIB_STATUS__INVALID_PARAMETER);
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__FORMAT(qlibContext));
    }
    else
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Perform secure full device format                                                               */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_ASSERT_RET(QLIB_KEY_MNGR__IS_KEY_VALID(deviceMasterKey), QLIB_STATUS__INVALID_PARAMETER);
        QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, QLIB_KID__DEVICE_MASTER, deviceMasterKey, FALSE));
        if (TRUE == eraseDataOnly)
        {
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__SERASE(qlibContext, QLIB_ERASE_CHIP, 0), ret, error_session);
        }
        else
        {
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__SFORMAT(qlibContext), ret, error_session);
        }
        QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));
    }

    if (FALSE == eraseDataOnly)
    {
        U32 sectionID;
        /*-------------------------------------------------------------------------------------------------*/
        /* All sections disabled after format                                                              */
        /*-------------------------------------------------------------------------------------------------*/
        for (sectionID = 0; sectionID < QLIB_NUM_OF_SECTIONS; sectionID++)
        {
            qlibContext->sectionsState[sectionID].enabled = 0;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Perform reset to close opened session and update context                                        */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_ResetFlash(qlibContext));
    }
    ret = QLIB_STATUS__OK;

    goto exit;

error_session:
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

exit:
    return ret;
}

QLIB_STATUS_T QLIB_SEC_configOPS(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_INTERFACE_T* busInterface_p = &(qlibContext->busInterface);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Build OP codes                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    busInterface_p->op0 = Q2_SEC_INST__MAKE(Q2_SEC_INST__OP0, busInterface_p->secureCmdsFormat, busInterface_p->dtr);
    busInterface_p->op1 = Q2_SEC_INST__MAKE(Q2_SEC_INST__OP1, busInterface_p->secureCmdsFormat, FALSE);
    busInterface_p->op2 = Q2_SEC_INST__MAKE(Q2_SEC_INST__OP2, busInterface_p->secureCmdsFormat, busInterface_p->dtr);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_GetNotifications(QLIB_CONTEXT_T* qlibContext, QLIB_NOTIFICATIONS_T* notifs)
{
    memset(notifs, 0, sizeof(QLIB_NOTIFICATIONS_T));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if Monotonic Counter maintenance is needed                                                    */
    /*-----------------------------------------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------------------------------------*/
    /* GET_SSR command is ignored if power is down                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* SSR is not valid while busy, if busy, update the cached SSR                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    if (1 == READ_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__BUSY))
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC__get_SSR(qlibContext, &qlibContext->ssr, SSR_MASK__ALL_ERRORS));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* check from the last status                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if (0 != READ_VAR_FIELD(qlibContext->ssr.asUint, QLIB_REG_SSR__MC_MAINT))
    {
        notifs->mcMaintenance = 1;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if device replacement is needed since DMC is close to its maximal value                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__synch_MC(qlibContext));
    if (qlibContext->mc[DMC] >= QLIB_SEC_DMC_EOL_THRESHOLD)
    {
        notifs->replaceDevice = 1;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if device reset is needed since TC is close to its maximal value                              */
    /*-----------------------------------------------------------------------------------------------------*/
    if (qlibContext->mc[TC] >= QLIB_SEC_TC_RESET_THRESHOLD)
    {
        notifs->resetDevice = 1;
    }
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_PerformMCMaint(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__MC_MAINT(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_ConfigDevice(QLIB_CONTEXT_T*                   qlibContext,
                                    const KEY_T                       deviceMasterKey,
                                    const KEY_T                       deviceSecretKey,
                                    const QLIB_SECTION_CONFIG_TABLE_T sectionTable,
                                    const KEY_ARRAY_T                 restrictedKeys,
                                    const KEY_ARRAY_T                 fullAccessKeys,
                                    const QLIB_WATCHDOG_CONF_T*       watchdogDefault,
                                    const QLIB_DEVICE_CONF_T*         deviceConf,
                                    const _128BIT                     suid)
{
    U8            sectionIndex = 0;
    QLIB_STATUS_T ret          = QLIB_STATUS__COMMAND_FAIL;
    _128BIT       suid_test;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);


    /*-----------------------------------------------------------------------------------------------------*/
    /* Write all keys                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    if (deviceMasterKey != NULL)
    {
        QLIB_STATUS_RET_CHECK(
            QLIB_SEC_setAllKeys_L(qlibContext, deviceMasterKey, deviceSecretKey, restrictedKeys, fullAccessKeys));
    }


    /*-----------------------------------------------------------------------------------------------------*/
    /* Set Secure User ID                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    if (suid != NULL)
    {
        _128BIT suidTest;

        // Check if already programmed
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_SEC__get_SUID(qlibContext, suidTest), ret, error_session);

        if (0 != memcmp(suid, suidTest, sizeof(_128BIT)))
        {
            QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, QLIB_KID__DEVICE_MASTER, deviceMasterKey, FALSE));
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__set_SUID(qlibContext, suid), ret, error_session);
            QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));
            QLIB_STATUS_RET_CHECK(QLIB_SEC__get_SUID(qlibContext, suid_test));
            QLIB_ASSERT_RET(0 == memcmp(suid, suid_test, sizeof(_128BIT)), QLIB_STATUS__COMMAND_FAIL);
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* configure Global Configuration Register (GMC)                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((deviceConf != NULL) || (watchdogDefault != NULL))
    {
        if ((deviceConf != NULL) && (deviceConf->safeFB == TRUE) &&
            (qlibContext->sectionsState[Q2_BOOT_SECTION_FALLBACK].enabled == 1) && (sectionTable == NULL))
        {
            QLIB_POLICY_T policy;
            QLIB_STATUS_RET_CHECK(
                QLIB_SEC_GetSectionConfiguration(qlibContext, Q2_BOOT_SECTION_FALLBACK, NULL, NULL, &policy, NULL, NULL, NULL));

            QLIB_ASSERT_RET(policy.checksumIntegrity == 0 || policy.rollbackProt == 0, QLIB_STATUS__INVALID_PARAMETER);
        }
#ifndef QLIB_SEC_ONLY
        if (deviceConf != NULL && sectionTable == NULL)
        {
            QLIB_STATUS_RET_CHECK(QLIB_SEC_VerifyAddressSizeConfig_L(qlibContext, &deviceConf->stdAddrSize));
        }
#endif
        QLIB_STATUS_RET_CHECK(QLIB_SEC_configGMC_L(qlibContext, deviceMasterKey, watchdogDefault, deviceConf));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set Section Configuration Registers (SCRn) for sections that need to be configured prior to GMT     */
    /*-----------------------------------------------------------------------------------------------------*/
    if (sectionTable != NULL)
    {
        for (sectionIndex = 0; sectionIndex < QLIB_NUM_OF_SECTIONS; sectionIndex++)
        {
            if (sectionTable[sectionIndex].size != 0)
            {
                // set size on context before it is set in HW so section configuration checks will test new size
                qlibContext->sectionsState[sectionIndex].sizeTag =
                    QLIB_REG_SMRn__LEN_IN_BYTES_TO_TAG(sectionTable[sectionIndex].size);

                if (!QLIB_SEC_ConfigSectionPolicyAfterGMT_L(qlibContext, sectionIndex))
                {
                    QLIB_STATUS_RET_CHECK(QLIB_SEC_ConfigInitialSectionPolicy_L(qlibContext,
                                                                                (U32)sectionIndex,
                                                                                &sectionTable[sectionIndex].policy,
                                                                                fullAccessKeys[sectionIndex]));
                }
            }
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* configure Global Mapping Table Register (GMT)                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    if (sectionTable != NULL)
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC_configGMT_L(qlibContext, deviceMasterKey, sectionTable));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform reset                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_ResetFlash(qlibContext));


    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure Reset Response                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    if (deviceConf != NULL)
    {
        QLIB_RESET_RESPONSE_T resetResp_test = {0};

        if (0 != memcmp(&deviceConf->resetResp, &resetResp_test, sizeof(QLIB_RESET_RESPONSE_T)))
        {
            // Reset response should be set if non zero
            QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, QLIB_KID__DEVICE_MASTER, deviceMasterKey, FALSE));
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__set_RST_RESP(qlibContext, TRUE, deviceConf->resetResp.response1),
                                       ret,
                                       error_session);
            QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__set_RST_RESP(qlibContext, FALSE, deviceConf->resetResp.response2),
                                       ret,
                                       error_session);
            QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

            QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_RST_RESP(qlibContext, resetResp_test.response1));
            QLIB_ASSERT_RET(0 == memcmp(deviceConf->resetResp.response1, resetResp_test.response1, sizeof(_512BIT)),
                            QLIB_STATUS__COMMAND_FAIL);
            QLIB_ASSERT_RET(0 == memcmp(deviceConf->resetResp.response2, resetResp_test.response2, sizeof(_512BIT)),
                            QLIB_STATUS__COMMAND_FAIL);
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set Section Configuration Registers (SCRn) for sections that their policy was not configured before */
    /*-----------------------------------------------------------------------------------------------------*/
    if (sectionTable != NULL)
    {
        for (sectionIndex = 0; sectionIndex < QLIB_NUM_OF_SECTIONS; sectionIndex++)
        {
            if ((sectionTable[sectionIndex].size != 0) && (QLIB_SEC_ConfigSectionPolicyAfterGMT_L(qlibContext, sectionIndex)))
            {
                QLIB_STATUS_RET_CHECK(QLIB_SEC_ConfigInitialSectionPolicy_L(qlibContext,
                                                                            (U32)sectionIndex,
                                                                            &sectionTable[sectionIndex].policy,
                                                                            fullAccessKeys[sectionIndex]));
            }
        }
    }

    ret = QLIB_STATUS__OK;

    goto exit;

error_session:
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

exit:
    return ret;
}

QLIB_STATUS_T QLIB_SEC_GetSectionConfiguration(QLIB_CONTEXT_T* qlibContext,
                                               U32             sectionID,
                                               U32*            baseAddr,
                                               U32*            size,
                                               QLIB_POLICY_T*  policy,
                                               U64*            digest,
                                               U32*            crc,
                                               U32*            version)
{
    SCRn_T  SCRn;
    SSPRn_T SSPRn = 0;
    GMT_T   GMT;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if section is enabled                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_GMT(qlibContext, GMT));

    if (0 == QLIB_REG_GMT_GET_ENABLE(GMT, sectionID))
    {
        // section is disabled
        if (NULL != baseAddr)
        {
            *baseAddr = 0;
        }
        if (NULL != size)
        {
            *size = 0;
        }
        if (NULL != policy)
        {
            memset(policy, 0, sizeof(QLIB_POLICY_T));
        }

        return QLIB_STATUS__OK;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read the SCR                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_SCRn(qlibContext, sectionID, SCRn));

    if (NULL != version)
    {
        *version = QLIB_REG_SCRn_GET_VER(SCRn);
    }

    if (NULL != crc)
    {
        *crc = QLIB_REG_SCRn_GET_CHECKSUM(SCRn);
    }

    if (NULL != digest)
    {
        *digest = QLIB_REG_SCRn_GET_DIGEST(SCRn);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Retrieve the section configurations (policy)                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    if (NULL != policy)
    {
        SSPRn = QLIB_REG_SCRn_GET_SSPRn(SCRn);

        memset(policy, 0, sizeof(QLIB_POLICY_T));
        policy->digestIntegrity        = READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__INTG_PROT_CFG);
        policy->checksumIntegrity      = READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__INTG_PROT_AC);
        policy->writeProt              = READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__WP_EN);
        policy->rollbackProt           = READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__ROLLBACK_EN);
        policy->plainAccessReadEnable  = READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__PA_RD_EN);
        policy->plainAccessWriteEnable = READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__PA_WR_EN);
        policy->authPlainAccess        = READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__AUTH_PA);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Retrieve the section base address and size                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if (NULL != baseAddr)
    {
        *baseAddr = QLIB_REG_SMRn__BASE_IN_TAG_TO_BYTES(QLIB_REG_GMT_GET_BASE(GMT, sectionID));
    }
    if (NULL != size)
    {
        *size = QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(QLIB_REG_GMT_GET_LEN(GMT, sectionID));
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_ConfigSection(QLIB_CONTEXT_T*      qlibContext,
                                     U32                  sectionID,
                                     const QLIB_POLICY_T* policy,
                                     const U64*           digest,
                                     const U32*           crc,
                                     const U32*           newVersion,
                                     QLIB_SWAP_T          swap)
{
    SCRn_T  SCRn;
    SCRn_T  SCRn_test;
    SSPRn_T SSPRn = 0;
    U32     ver   = 0;
    GMT_T   GMT;
    BOOL    needInitPA = FALSE;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__DEVICE_SESSION_ERR);
    QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionID), QLIB_STATUS__DEVICE_PRIVILEGE_ERR);
    if (NULL != policy)
    {
        QLIB_ASSERT_RET((0 == policy->checksumIntegrity) ? (NULL == crc) : (NULL != crc), QLIB_STATUS__INVALID_PARAMETER);
        QLIB_ASSERT_RET((0 == policy->digestIntegrity) ? (NULL == digest) : (NULL != digest), QLIB_STATUS__INVALID_PARAMETER);
#ifndef QLIB_SEC_ONLY
        // make sure all plain sections are accessible with plain access
        QLIB_ASSERT_RET((policy->plainAccessReadEnable == 0 && policy->plainAccessWriteEnable == 0) ||
                            (QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(qlibContext->sectionsState[sectionID].sizeTag) <=
                                 _QLIB_MAX_LEGACY_OFFSET(qlibContext) &&
                             sectionID < _QLIB_MAX_LEGACY_SECTION_ID(qlibContext)),
                        QLIB_STATUS__INVALID_PARAMETER);
#endif
        if (policy->checksumIntegrity == 1 && policy->rollbackProt == 1 && sectionID == Q2_BOOT_SECTION_FALLBACK)
        {
            GMC_T    GMC;
            DEVCFG_T DEVCFG = 0;
            QLIB_STATUS_RET_CHECK(QLIB_SEC__get_GMC(qlibContext, GMC));
            DEVCFG = QLIB_REG_GMC_GET_DEVCFG(GMC);
            QLIB_ASSERT_RET(READ_VAR_FIELD(DEVCFG, QLIB_REG_DEVCFG__FB_EN) == 0, QLIB_STATUS__INVALID_PARAMETER);
        }

        // Make sure that rollback Protected section have at least 2 blocks
        if (policy->rollbackProt)
        {
            QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_GMT_UNSIGNED(qlibContext, GMT));
            // GMT-LEN value of 0 is one block, any other value is at least 2 blocks
            QLIB_ASSERT_RET(0 < QLIB_REG_GMT_GET_LEN(GMT, sectionID), QLIB_STATUS__INVALID_PARAMETER);
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Build the new SCR value                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_SCRn_UNSIGNED(qlibContext, sectionID, SCRn));

    ver = QLIB_REG_SCRn_GET_VER(SCRn);
    if (NULL != newVersion)
    {
        QLIB_REG_SCRn_SET_VER(SCRn, *newVersion);
    }
    else if (0xFFFFFFFF == ver)
    {
        QLIB_REG_SCRn_SET_VER(SCRn, 0);
    }

    if (NULL != crc)
    {
        QLIB_REG_SCRn_SET_CHECKSUM(SCRn, *crc);
    }

    if (NULL != digest)
    {
        QLIB_REG_SCRn_SET_DIGEST(SCRn, *digest);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the section configurations (policy)                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (NULL != policy)
    {
        SSPRn = 0;
        SET_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__INTG_PROT_CFG, policy->digestIntegrity);
        SET_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__INTG_PROT_AC, policy->checksumIntegrity);
        SET_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__WP_EN, policy->writeProt);
        SET_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__ROLLBACK_EN, policy->rollbackProt);
        SET_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__PA_RD_EN, policy->plainAccessReadEnable);
        SET_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__PA_WR_EN, policy->plainAccessWriteEnable);
        SET_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__AUTH_PA, policy->authPlainAccess);
        QLIB_REG_SCRn_SET_SSPRn(SCRn, SSPRn);
    }
    else
    {
        SSPRn = QLIB_REG_SCRn_GET_SSPRn(SCRn);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Mark if session need to be init PA after set SCR                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__PA_RD_EN) || READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__PA_WR_EN)) &&
        !READ_VAR_FIELD(SSPRn, QLIB_REG_SSPRn__AUTH_PA) && qlibContext->sectionsState[sectionID].plainEnabled)
    {
        needInitPA = TRUE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform (SCRn) write                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    if (swap == QLIB_SWAP_NO)
    {
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__set_SCRn(qlibContext, sectionID, SCRn, needInitPA));
    }
    else
    {
        QLIB_STATUS_RET_CHECK(
            QLIB_CMD_PROC__set_SCRn_swap(qlibContext, sectionID, SCRn, swap == QLIB_SWAP_AND_RESET, needInitPA));
    }

    qlibContext->sectionsState[sectionID].plainEnabled = (needInitPA == TRUE ? 1 : 0);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Mark session as closed as SET_CSR/SET_SCR_SWAP closes the session in flash                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_MarkSessionClose_L(qlibContext));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Confirm operation success                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_SCRn_UNSIGNED(qlibContext, sectionID, SCRn_test));
    QLIB_ASSERT_RET(0 == memcmp(SCRn, SCRn_test, sizeof(SCRn_T)), QLIB_STATUS__COMMAND_FAIL);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_OpenSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_SESSION_ACCESS_T sessionAccess)
{
    BOOL       configOnly = FALSE;
    QLIB_KID_T kid        = QLIB_KID__INVALID;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(FALSE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Define the key description                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_SESSION_ACCESS_FULL == sessionAccess)
    {
        kid = QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__FULL_ACCESS_SECTION, sectionID);
    }
    else if (QLIB_SESSION_ACCESS_CONFIG_ONLY == sessionAccess)
    {
        kid        = QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__FULL_ACCESS_SECTION, sectionID);
        configOnly = TRUE;
    }
    else if (QLIB_SESSION_ACCESS_RESTRICTED == sessionAccess)
    {
        kid = QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__RESTRICTED_ACCESS_SECTION, sectionID);
    }
    else
    {
        return QLIB_STATUS__INVALID_PARAMETER;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Open session                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, kid, NULL, configOnly));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_CloseSession(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionID) ||
                        QLIB_KEY_MNGR_IS_SECTION_RESTRICTED_ACCESS(qlibContext, sectionID),
                    QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Close session                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_AuthPlainAccess_Grant(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    QLIB_STATUS_RET_CHECK(QLIB_SEC_GrantRevokePA_L(qlibContext, sectionID, TRUE));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_AuthPlainAccess_Revoke(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    QLIB_STATUS_RET_CHECK(QLIB_SEC_GrantRevokePA_L(qlibContext, sectionID, FALSE));

    return QLIB_STATUS__OK;
}

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
QLIB_STATUS_T QLIB_SEC_Read(QLIB_CONTEXT_T* qlibContext, U8* buf, U32 sectionID, U32 offset, U32 size, BOOL auth)
{
    U32           offsetInPage = 0;
    QLIB_STATUS_T ret          = QLIB_STATUS__OK;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != buf, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__DEVICE_SESSION_ERR);
    QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionID) ||
                        QLIB_KEY_MNGR_IS_SECTION_RESTRICTED_ACCESS(qlibContext, sectionID),
                    QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Mark multi-transaction started                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/

    qlibContext->multiTransactionCmd = TRUE;
#ifdef QLIB_SPI_OPTIMIZATION_ENABLED
    PLAT_SPI_MultiTransactionStart();
#endif //QLIB_SPI_OPTIMIZATION_ENABLED
    offsetInPage = (offset % QLIB_SEC_READ_PAGE_SIZE_BYTE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if we can use aligned access optimization while flash is busy                                 */
    /*-----------------------------------------------------------------------------------------------------*/
#ifndef QLIB_SUPPORT_XIP
    if ((offsetInPage == 0) && ((size % QLIB_SEC_READ_PAGE_SIZE_BYTE) == 0) && ADDRESS_ALIGNED32(buf) &&
        (qlibContext->busInterface.busMode != QLIB_BUS_MODE_4_4_4))
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Access is aligned                                                                               */
        /*-------------------------------------------------------------------------------------------------*/
        if (auth == TRUE)
        {
            ret = QLIB_CMD_PROC__SARD_Multi(qlibContext, offset, (U32*)(UPTR)buf, size);
        }
        else
        {
            ret = QLIB_CMD_PROC__SRD_Multi(qlibContext, offset, (U32*)(UPTR)buf, size);
        }
    }
    else
#endif // QLIB_SUPPORT_XIP
    {
        U32              dataPage[QLIB_SEC_READ_PAGE_SIZE_BYTE / sizeof(U32)];
        QLIB_READ_FUNC_T readFunc = (TRUE == auth) ? QLIB_CMD_PROC__SARD : QLIB_CMD_PROC__SRD;
        U32              iterSize = MIN(size, (QLIB_SEC_READ_PAGE_SIZE_BYTE - offsetInPage));

        offset = offset - offsetInPage;

        while (0 != size)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* One page Read                                                                               */
            /*---------------------------------------------------------------------------------------------*/
            if (QLIB_SEC_READ_PAGE_SIZE_BYTE == iterSize && ADDRESS_ALIGNED32(buf))
            {
                QLIB_STATUS_RET_CHECK_GOTO(readFunc(qlibContext, offset, (U32*)(UPTR)buf), ret, finish);
            }
            else
            {
                QLIB_STATUS_RET_CHECK_GOTO(readFunc(qlibContext, offset, dataPage), ret, finish);
                memcpy(buf, (U8*)(dataPage) + offsetInPage, iterSize);
            }

            /*---------------------------------------------------------------------------------------------*/
            /* Prepare pointers for next iteration                                                         */
            /*---------------------------------------------------------------------------------------------*/
            size         = size - iterSize;
            buf          = buf + iterSize;
            offset       = offset + QLIB_SEC_READ_PAGE_SIZE_BYTE;
            offsetInPage = 0;
            iterSize     = MIN(size, QLIB_SEC_READ_PAGE_SIZE_BYTE);
        }
    }

finish:
    /*-----------------------------------------------------------------------------------------------------*/
    /* Mark multi-transaction ended                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    qlibContext->multiTransactionCmd = FALSE;

#ifdef QLIB_SPI_OPTIMIZATION_ENABLED
    PLAT_SPI_MultiTransactionStop();
#endif //QLIB_SPI_OPTIMIZATION_ENABLED

    return ret;
}

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
QLIB_STATUS_T QLIB_SEC_Write(QLIB_CONTEXT_T* qlibContext, const U8* buf, U32 sectionID, U32 offset, U32 size)
{
    U32           iterSize     = 0;
    U32           offsetInPage = 0;
    U32           pageBuf[QLIB_SEC_WRITE_PAGE_SIZE_BYTE / sizeof(U32)];
    QLIB_STATUS_T ret = QLIB_STATUS__OK;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(NULL != buf, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__DEVICE_SESSION_ERR);
    QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionID), QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Mark multi-transaction command                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    qlibContext->multiTransactionCmd = TRUE;

#ifdef QLIB_SPI_OPTIMIZATION_ENABLED
    PLAT_SPI_MultiTransactionStart();
#endif //QLIB_SPI_OPTIMIZATION_ENABLED

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set variables                                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    offsetInPage = (offset % QLIB_SEC_WRITE_PAGE_SIZE_BYTE);
    offset       = offset - offsetInPage;
    iterSize     = MIN(size, (QLIB_SEC_WRITE_PAGE_SIZE_BYTE - offsetInPage));

    while (0 != size)
    {
        if (QLIB_SEC_WRITE_PAGE_SIZE_BYTE != iterSize)
        {
            memset(pageBuf, 0xFF, QLIB_SEC_WRITE_PAGE_SIZE_BYTE);
        }

        memcpy((U8*)pageBuf + offsetInPage, buf, iterSize);

        /*-------------------------------------------------------------------------------------------------*/
        /* One page write                                                                                  */
        /* Address must be aligned to 32B (5 LS-bits are *ignored*)                                        */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__SAWR(qlibContext, offset, pageBuf), ret, finish);

        /*-------------------------------------------------------------------------------------------------*/
        /* Prepare pointers for next iteration                                                             */
        /*-------------------------------------------------------------------------------------------------*/
        size         = size - iterSize;
        buf          = buf + iterSize;
        offset       = offset + QLIB_SEC_WRITE_PAGE_SIZE_BYTE;
        offsetInPage = 0;
        iterSize     = MIN(size, QLIB_SEC_WRITE_PAGE_SIZE_BYTE);
    }
finish:
    /*-----------------------------------------------------------------------------------------------------*/
    /* Multi-transaction ended                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (qlibContext->multiTransactionCmd == TRUE)
    {
        qlibContext->multiTransactionCmd = FALSE;

#ifdef QLIB_SPI_OPTIMIZATION_ENABLED
        PLAT_SPI_MultiTransactionStop();
#endif //QLIB_SPI_OPTIMIZATION_ENABLED
    }
    return ret;
}

QLIB_STATUS_T QLIB_SEC_Erase(QLIB_CONTEXT_T* qlibContext, U32 sectionID, U32 offset, U32 size)
{
    U32          eraseSize = 0;
    QLIB_ERASE_T eraseType = QLIB_ERASE_FIRST;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(0 == (offset % FLASH_SECTOR_SIZE), QLIB_STATUS__INVALID_DATA_ALIGNMENT);
    QLIB_ASSERT_RET(0 == (size % FLASH_SECTOR_SIZE), QLIB_STATUS__INVALID_DATA_SIZE);
    QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__DEVICE_SESSION_ERR);
    QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionID), QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Start erasing with optimal command                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    while (0 < size)
    {
        if (_64KB_ <= size && 0 == (offset % _64KB_))
        {
            eraseSize = _64KB_;
            eraseType = QLIB_ERASE_BLOCK_64K;
        }
        else if (_32KB_ <= size && 0 == (offset % _32KB_))
        {
            eraseSize = _32KB_;
            eraseType = QLIB_ERASE_BLOCK_32K;
        }
        else
        {
            eraseSize = _4KB_;
            eraseType = QLIB_ERASE_SECTOR_4K;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Perform the erase                                                                               */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__SERASE(qlibContext, eraseType, offset));

        /*-------------------------------------------------------------------------------------------------*/
        /* Prepare pointers for next iteration                                                             */
        /*-------------------------------------------------------------------------------------------------*/
        offset = offset + eraseSize;
        size   = size - eraseSize;
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_EraseSection(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL secure)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if plain or secure section erase                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    if (secure == TRUE)
    {
        QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__DEVICE_SESSION_ERR);
        QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionID), QLIB_STATUS__DEVICE_PRIVILEGE_ERR);
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__SERASE(qlibContext, QLIB_ERASE_SECTION, 0));
    }
    else
    {
        {
            QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__ERASE_SECT_PA(qlibContext, sectionID));
        }
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_ConfigAccess(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL readEnable, BOOL writeEnable)
{
    ACLR_T aclr      = 0;
    U32    readMask  = 0;
    U32    writeMask = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(sectionID < QLIB_NUM_OF_SECTIONS, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read ACLR                                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_ACLR(qlibContext, &aclr));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Update section configuration                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    writeMask = READ_VAR_FIELD(aclr, QLIB_REG_ACLR_WR_LOCK);
    readMask  = READ_VAR_FIELD(aclr, QLIB_REG_ACLR_RD_LOCK);
    WRITE_VAR_BIT(writeMask, sectionID, writeEnable == FALSE);
    WRITE_VAR_BIT(readMask, sectionID, readEnable == FALSE);
    SET_VAR_FIELD(aclr, QLIB_REG_ACLR_WR_LOCK, writeMask);
    SET_VAR_FIELD(aclr, QLIB_REG_ACLR_RD_LOCK, readMask);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Write ACLR                                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__set_ACLR(qlibContext, (aclr & ~(QLIB_REG_ACLR_RESERVED_MASK))));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_LoadKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, const KEY_T key, BOOL fullAccess)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_KEY_MNGR__IS_KEY_VALID(key), QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);

    QLIB_KEYMNGR_SetKey(&qlibContext->keyMngr, key, sectionID, fullAccess);
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_RemoveKey(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL fullAccess)
{
    QLIB_KID_TYPE_T kid_type = fullAccess == TRUE ? QLIB_KID__FULL_ACCESS_SECTION : QLIB_KID__RESTRICTED_ACCESS_SECTION;
    QLIB_KID_T      kid      = QLIB_KEY_MNGR__KID_WITH_SECTION(kid_type, sectionID);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Prevent key removal if session with this key is currently open                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->keyMngr.kid != kid, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    QLIB_KEYMNGR_RemoveKey(&qlibContext->keyMngr, sectionID, fullAccess);
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_CheckIntegrity(QLIB_CONTEXT_T* qlibContext, U32 sectionID, QLIB_INTEGRITY_T integrityType)
{
    QLIB_POLICY_T policy;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Error checking                                                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_NUM_OF_SECTIONS > sectionID, QLIB_STATUS__INVALID_PARAMETER);
    QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__DEVICE_SESSION_ERR);

    QLIB_STATUS_RET_CHECK(QLIB_SEC_GetSectionConfiguration(qlibContext, sectionID, NULL, NULL, &policy, NULL, NULL, NULL));

    if (QLIB_INTEGRITY_DIGEST == integrityType)
    {
        // Perform CALC_SIG command for secure integrity check
        U64    digest = 0;
        SCRn_T SCRn;

        QLIB_ASSERT_RET(policy.digestIntegrity, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__CALC_SIG(qlibContext,
                                                      QLIB_SIGNED_DATA_TYPE_SECTION_DIGEST,
                                                      sectionID,
                                                      (U32*)&digest,
                                                      sizeof(digest),
                                                      NULL));
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__CALC_SIG(qlibContext,
                                                      QLIB_SIGNED_DATA_TYPE_SECTION_CONFIG,
                                                      sectionID,
                                                      (U32*)SCRn,
                                                      sizeof(SCRn),
                                                      NULL));

        if (0 != memcmp(&digest, QLIB_REG_SCRn_GET_DIGEST_PTR(SCRn), sizeof(digest)))
        {
            return QLIB_STATUS__SECURITY_ERR;
        }
    }
    else if (QLIB_INTEGRITY_CRC == integrityType)
    {
        // Perform VER_INTG for faster (but less secure) check
        QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionID) ||
                            QLIB_KEY_MNGR_IS_SECTION_RESTRICTED_ACCESS(qlibContext, sectionID),
                        QLIB_STATUS__DEVICE_PRIVILEGE_ERR);
        QLIB_ASSERT_RET(policy.checksumIntegrity, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__Check_Integrity(qlibContext, sectionID));
    }
    else
    {
        return QLIB_STATUS__INVALID_PARAMETER;
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_CalcCDI(QLIB_CONTEXT_T* qlibContext, _256BIT nextCdi, _256BIT prevCdi, U32 sectionId)
{
    _512BIT       hashData;
    U8*           hashDataP = (U8*)hashData;
    QLIB_POLICY_T policy;
    U64           digest = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    if (sectionId == 0)
    {
        QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, 0), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
        return QLIB_CMD_PROC__CALC_CDI(qlibContext, 0, nextCdi);
    }
    else
    {
        QLIB_ASSERT_RET(NULL != prevCdi, QLIB_STATUS__INVALID_PARAMETER);
        QLIB_STATUS_RET_CHECK(QLIB_SEC_GetSectionConfiguration(qlibContext, sectionId, NULL, NULL, &policy, &digest, NULL, NULL));

        if ((policy.digestIntegrity) && (policy.writeProt || policy.rollbackProt))
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Using stored digest                                                                         */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_ASSERT_RET(digest != 0, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
        }
        else
        {
            /*---------------------------------------------------------------------------------------------*/
            /* recalculating digest                                                                        */
            /*---------------------------------------------------------------------------------------------*/
            QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__CALC_SIG(qlibContext,
                                                          QLIB_SIGNED_DATA_TYPE_SECTION_DIGEST,
                                                          sectionId,
                                                          (U32*)&digest,
                                                          sizeof(digest),
                                                          NULL));
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* data consists of the following                                                                  */
        /* 256 bit  => prevCdi                                                                             */
        /* 64  bit  => digest                                                                              */
        /* 64  bit  => zero bits                                                                           */
        /* 48  bit  => zero bits                                                                           */
        /* 8   bit  => index                                                                               */
        /*-------------------------------------------------------------------------------------------------*/
        memcpy(hashDataP, prevCdi, BITS_TO_BYTES(256));
        hashDataP += BITS_TO_BYTES(256);

        memcpy(hashDataP, &digest, BITS_TO_BYTES(64));
        hashDataP += BITS_TO_BYTES(64);

        memset(hashDataP, 0, BITS_TO_BYTES(112));
        hashDataP += BITS_TO_BYTES(112);

        memcpy(hashDataP, &sectionId, BITS_TO_BYTES(8));

        PLAT_HASH(nextCdi, hashData, BITS_TO_BYTES(256 + 64 + 112 + 8));
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_Watchdog_ConfigSet(QLIB_CONTEXT_T* qlibContext, const QLIB_WATCHDOG_CONF_T* watchdogCFG)
{
    AWDTCFG_T AWDTCFG      = 0;
    AWDTCFG_T AWDTCFG_test = 0;
    U32       AWDTCFG_KID  = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read AWDTCFG register and check the state                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_AWDTCNFG(qlibContext, &AWDTCFG));

    QLIB_ASSERT_RET(0 == READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__LOCK), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    qlibContext->watchdogIsSecure = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__AUTH_WDT));

    AWDTCFG_KID = READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__KID);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Configure the new parameters                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__AWDT_EN, BOOLEAN_TO_INT(watchdogCFG->enable));
    SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__LFOSC_EN, BOOLEAN_TO_INT(watchdogCFG->lfOscEn));
    SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__SRST_EN, BOOLEAN_TO_INT(watchdogCFG->swResetEn));
    SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__AUTH_WDT, BOOLEAN_TO_INT(watchdogCFG->authenticated));
    SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__KID, watchdogCFG->sectionID);
    SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__TH, watchdogCFG->threshold);
    SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__LOCK, BOOLEAN_TO_INT(watchdogCFG->lock));

    if (0 != watchdogCFG->oscRateHz)
    {
        SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__OSC_RATE_KHZ, watchdogCFG->oscRateHz >> 10);
        SET_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__OSC_RATE_FRAC, (watchdogCFG->oscRateHz >> 6));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Write the new configurations to the flash                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    AWDTCFG = AWDTCFG & (~QLIB_REG_AWDTCFG_RESERVED_MASK);

    if (TRUE == qlibContext->watchdogIsSecure)
    {
        QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__DEVICE_SESSION_ERR);
        QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, AWDTCFG_KID), QLIB_STATUS__DEVICE_PRIVILEGE_ERR);
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__set_AWDT(qlibContext, AWDTCFG));
    }
    else
    {
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__set_AWDT_PA(qlibContext, AWDTCFG));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read AWDTCFG register and verify the change                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_AWDTCNFG(qlibContext, &AWDTCFG_test));
    QLIB_ASSERT_RET(AWDTCFG == AWDTCFG_test, QLIB_STATUS__COMMAND_FAIL);
    qlibContext->watchdogIsSecure  = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__AUTH_WDT));
    qlibContext->watchdogSectionId = READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__KID);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_Watchdog_ConfigGet(QLIB_CONTEXT_T* qlibContext, QLIB_WATCHDOG_CONF_T* watchdogCFG)
{
    AWDTCFG_T AWDTCFG = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read AWDTCFG register and check the state                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_AWDTCNFG(qlibContext, &AWDTCFG));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Fill the user struct                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    watchdogCFG->enable        = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__AWDT_EN));
    watchdogCFG->lfOscEn       = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__LFOSC_EN));
    watchdogCFG->swResetEn     = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__SRST_EN));
    watchdogCFG->authenticated = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__AUTH_WDT));
    watchdogCFG->sectionID     = READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__KID);
    watchdogCFG->threshold     = (QLIB_AWDT_TH_T)READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__TH);
    watchdogCFG->lock          = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__LOCK));

    watchdogCFG->oscRateHz = ((U32)READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__OSC_RATE_KHZ) << 10) +
                             ((U32)READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__OSC_RATE_FRAC) << 6);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_Watchdog_Touch(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    if (TRUE == qlibContext->watchdogIsSecure)
    {
        QLIB_ASSERT_RET(TRUE == QLIB_KEY_MNGR__SESSION_IS_OPEN((qlibContext)), QLIB_STATUS__DEVICE_SESSION_ERR);
        QLIB_ASSERT_RET(QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS((qlibContext), qlibContext->watchdogSectionId) ||
                            QLIB_KEY_MNGR_IS_SECTION_RESTRICTED_ACCESS((qlibContext), qlibContext->watchdogSectionId),
                        QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__AWDT_TOUCH(qlibContext));
    }
    else
    {
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__AWDT_TOUCH_PA(qlibContext));
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_Watchdog_Trigger(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    return QLIB_CMD_PROC__AWDT_expire(qlibContext);
}

QLIB_STATUS_T QLIB_SEC_Watchdog_Get(QLIB_CONTEXT_T* qlibContext, U32* secondsPassed, U32* ticsResidue, BOOL* expired)
{
    AWDTSR_T AWDTSR = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_AWDTSR(qlibContext, &AWDTSR));

    if (NULL != secondsPassed)
    {
        *secondsPassed = READ_VAR_FIELD(AWDTSR, QLIB_REG_AWDTSR__AWDT_VAL);
    }

    if (NULL != ticsResidue)
    {
        *ticsResidue = READ_VAR_FIELD(AWDTSR, QLIB_REG_AWDTSR__AWDT_RES);
    }

    if (NULL != expired)
    {
        *expired = (1 == READ_VAR_FIELD(AWDTSR, QLIB_REG_AWDTSR__AWDT_EXP_S)) ? TRUE : FALSE;
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_GetWID(QLIB_CONTEXT_T* qlibContext, QLIB_WID_T id)
{
    QLIB_ASSERT_RET(id != NULL, QLIB_STATUS__INVALID_PARAMETER);
    memcpy(id, qlibContext->wid, sizeof(QLIB_WID_T));
    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_GetHWVersion(QLIB_CONTEXT_T* qlibContext, QLIB_SEC_HW_VER_T* hwVer)
{
    HW_VER_T hwVerReg;
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_HW_VER(qlibContext, &hwVerReg));
    hwVer->flashVersion    = READ_VAR_FIELD(hwVerReg, QLIB_REG_HW_VER__FLASH_VER);
    hwVer->flashSize       = READ_VAR_FIELD(hwVerReg, QLIB_REG_HW_VER__FLASH_SIZE);
    hwVer->securityVersion = READ_VAR_FIELD(hwVerReg, QLIB_REG_HW_VER__QSF_VER);
    hwVer->revision        = READ_VAR_FIELD(hwVerReg, QLIB_REG_HW_VER__REVISION);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_GetId(QLIB_CONTEXT_T* qlibContext, QLIB_SEC_ID_T* id_p)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    QLIB_STATUS_RET_CHECK(QLIB_SEC_GetWID(qlibContext, id_p->wid));
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_SUID(qlibContext, id_p->suid));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_GetStatus(QLIB_CONTEXT_T* qlibContext)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* GET_SSR command is ignored if power is down                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform SSR read with error check                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    return qlibContext->isSuspended == TRUE ? QLIB_CMD_PROC__get_SSR_UNSIGNED(qlibContext, NULL, SSR_MASK__ALL_ERRORS)
                                            : QLIB_SEC__get_SSR(qlibContext, NULL, SSR_MASK__ALL_ERRORS);
}

QLIB_STATUS_T QLIB_SEC_SetInterface(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat)
{
    QLIB_BUS_MODE_T secureFormat = QLIB_BUS_MODE_INVALID;
    QLIB_BUS_MODE_T format       = QLIB_BUS_FORMAT_GET_MODE(busFormat);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Match the format with the supported formats for secure commands                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    switch (format)
    {
        case QLIB_BUS_MODE_1_1_1:
            secureFormat = QLIB_BUS_MODE_1_1_1;
            break;
#ifdef QLIB_SUPPORT_DUAL_SPI
        case QLIB_BUS_MODE_1_1_2:
        case QLIB_BUS_MODE_1_2_2:
            secureFormat = QLIB_BUS_MODE_1_1_2;
            break;
#endif
        case QLIB_BUS_MODE_1_1_4:
        case QLIB_BUS_MODE_1_4_4:
            secureFormat = QLIB_BUS_MODE_1_1_4;
            break;
#ifdef QLIB_SUPPORT_QPI
        case QLIB_BUS_MODE_4_4_4:
            secureFormat = QLIB_BUS_MODE_4_4_4;
            break;
#endif
        default:
            return QLIB_STATUS__NOT_SUPPORTED;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Update the globals                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    qlibContext->busInterface.secureCmdsFormat = secureFormat;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Config OPs                                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_configOPS(qlibContext));

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_SyncAfterFlashReset(QLIB_CONTEXT_T* qlibContext)
{
    QLIB_STATUS_T  status = QLIB_STATUS__TEST_FAIL;
    QLIB_REG_SSR_T ssr;
    U32            sectionID;

    /*-----------------------------------------------------------------------------------------------------*/
    /* refresh the out-dated information                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_MarkSessionClose_L(qlibContext));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Plain sessions got closed after reset                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    for (sectionID = 0; sectionID < QLIB_NUM_OF_SECTIONS; sectionID++)
    {
        qlibContext->sectionsState[sectionID].plainEnabled = 0;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Wait while secure module is not-ready                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    do
    {
        status = QLIB_SEC__get_SSR(qlibContext, &ssr, SSR_MASK__ALL_ERRORS);
    } while (QLIB_STATUS__OK != status || READ_VAR_FIELD(ssr.asUint, QLIB_REG_SSR__BUSY));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Cache reset status                                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    qlibContext->resetStatus.powerOnReset      = READ_VAR_FIELD(ssr.asUint, QLIB_REG_SSR__POR);
    qlibContext->resetStatus.fallbackRemapping = READ_VAR_FIELD(ssr.asUint, QLIB_REG_SSR__FB_REMAP);
    qlibContext->resetStatus.watchdogReset     = READ_VAR_FIELD(ssr.asUint, QLIB_REG_SSR__AWDT_EXP);

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_EnablePlainAccess(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    GMT_T          gmt;
    QLIB_REG_SSR_T ssr;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Secure command is ignored if power is down or suspended                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(qlibContext->isPoweredDown == FALSE, QLIB_STATUS__COMMAND_IGNORED);
    QLIB_ASSERT_RET(qlibContext->isSuspended == FALSE, QLIB_STATUS__COMMAND_IGNORED);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get SSR, to check chip state                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_SSR(qlibContext, &ssr, SSR_MASK__ALL_ERRORS));

    /*-----------------------------------------------------------------------------------------------------*/
    /* We open plain session only when the flash is in working mode                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET((READ_VAR_FIELD(ssr.asUint, QLIB_REG_SSR__STATE) & QLIB_REG_SSR__STATE_WORKING_MASK) ==
                        QLIB_REG_SSR__STATE_WORKING,
                    QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get GMT, to find which sections enabled                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_GMT_UNSIGNED(qlibContext, gmt));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if section enabled                                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_REG_GMT_GET_ENABLE(gmt, sectionID) == 1, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable plain access                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_EnablePlainAccess_L(qlibContext, sectionID));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Re-open secure session                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext))
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, qlibContext->keyMngr.kid, NULL, FALSE));
    }

    return QLIB_STATUS__OK;
}

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                            LOCAL FUNCTIONS                                              */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
* @brief       This function enables plain access to the section
*
* @param       qlibContext
* @param       sectionID
*
* @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_EnablePlainAccess_L(QLIB_CONTEXT_T* qlibContext, U32 sectionID)
{
    SCRn_T scr;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Get SCR                                                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_SCRn_UNSIGNED(qlibContext, sectionID, scr))

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check plain access is enabled to this section                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(READ_VAR_FIELD(QLIB_REG_SCRn_GET_SSPRn(scr), QLIB_REG_SSPRn__PA_RD_EN) ||
                        READ_VAR_FIELD(QLIB_REG_SCRn_GET_SSPRn(scr), QLIB_REG_SSPRn__PA_WR_EN),
                    QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check that Authentication is not enabled on this section                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(0 == READ_VAR_FIELD(QLIB_REG_SCRn_GET_SSPRn(scr), QLIB_REG_SSPRn__AUTH_PA),
                    QLIB_STATUS__DEVICE_PRIVILEGE_ERR);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Enable plain access                                                                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    return QLIB_CMD_PROC__init_section_PA(qlibContext, sectionID);
}

QLIB_STATUS_T QLIB_SEC_KeyProvisioning_L(QLIB_CONTEXT_T* qlibContext, const KEY_T Kd, QLIB_KID_T new_kid, const KEY_T new_key)
{
    QLIB_KID_T    prov_kid = QLIB_KID__INVALID;
    KEY_T         prov_key;
    QLIB_STATUS_T ret = QLIB_STATUS__COMMAND_FAIL;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Check if the key is already provisioned                                                             */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_KID__DEVICE_MASTER == new_kid)
    {
        QLIB_ALLOW_TO_FAIL__START();
        ret = QLIB_SEC_OpenSessionInternal_L(qlibContext, new_kid, new_key, TRUE);
        (void)QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE);
        QLIB_ALLOW_TO_FAIL__END();

        if (QLIB_STATUS__OK == ret)
        {
            // Exit if key already there
            return QLIB_STATUS__OK;
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Calculate provisioning key descriptor                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    switch (QLIB_KEY_MNGR__GET_KEY_TYPE(new_kid))
    {
        case QLIB_KID__RESTRICTED_ACCESS_SECTION:
        case QLIB_KID__FULL_ACCESS_SECTION:
            prov_kid = QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__SECTION_PROVISIONING, QLIB_KEY_MNGR__GET_KEY_SECTION(new_kid));
            break;

        case QLIB_KID__DEVICE_SECRET:
        case QLIB_KID__DEVICE_MASTER:
            prov_kid =
                QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__DEVICE_KEY_PROVISIONING, QLIB_KEY_MNGR__GET_KEY_SECTION(new_kid));
            break;

        default:
            return QLIB_STATUS__PARAMETER_OUT_OF_RANGE;
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Calculate provisioning key                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CRYPTO_GetProvisionKey(prov_kid, Kd, TRUE, FALSE, prov_key);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Open session with provisioning key                                                                  */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, prov_kid, prov_key, FALSE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set the Key                                                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__set_KEY(qlibContext, new_kid, new_key), ret, close_session);

close_session:
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

    return ret;
}

QLIB_STATUS_T QLIB_SEC_setAllKeys_L(QLIB_CONTEXT_T*   qlibContext,
                                    const KEY_T       deviceMasterKey,
                                    const KEY_T       deviceSecretKey,
                                    const KEY_ARRAY_T restrictedKeys,
                                    const KEY_ARRAY_T fullAccessKeys)
{
    QLIB_KID_T kid          = QLIB_KID__INVALID;
    U8         sectionIndex = 0;
    KEY_T      key_buff;

    /*-----------------------------------------------------------------------------------------------------*/
    /* This function cannot work without device key                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_KEY_MNGR__IS_KEY_VALID(deviceMasterKey), QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set device Master Key (use the default K_d)                                                         */
    /*-----------------------------------------------------------------------------------------------------*/
    memset(key_buff, 0xFF, sizeof(KEY_T)); ///< deviceMasterKey initial value
    QLIB_STATUS_RET_CHECK(QLIB_SEC_KeyProvisioning_L(qlibContext, key_buff, QLIB_KID__DEVICE_MASTER, deviceMasterKey));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set device Secret Key                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_KEY_MNGR__IS_KEY_VALID(deviceSecretKey))
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC_KeyProvisioning_L(qlibContext, deviceMasterKey, QLIB_KID__DEVICE_SECRET, deviceSecretKey));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set sections Keys                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((restrictedKeys != NULL) || (fullAccessKeys != NULL))
    {
        for (sectionIndex = 0; sectionIndex < QLIB_NUM_OF_SECTIONS; sectionIndex++)
        {
            if (restrictedKeys != NULL)
            {
                if (QLIB_KEY_MNGR__IS_KEY_VALID(restrictedKeys[sectionIndex]))
                {
                    kid = QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__RESTRICTED_ACCESS_SECTION, sectionIndex);
                    QLIB_STATUS_RET_CHECK(
                        QLIB_SEC_KeyProvisioning_L(qlibContext, deviceMasterKey, kid, (restrictedKeys[sectionIndex])));
                }
            }
            if (fullAccessKeys != NULL)
            {
                if (QLIB_KEY_MNGR__IS_KEY_VALID(fullAccessKeys[sectionIndex]))
                {
                    kid = QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__FULL_ACCESS_SECTION, sectionIndex);
                    QLIB_STATUS_RET_CHECK(
                        QLIB_SEC_KeyProvisioning_L(qlibContext, deviceMasterKey, kid, (fullAccessKeys[sectionIndex])));
                }
            }
        }
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SEC_configGMC_L(QLIB_CONTEXT_T*             qlibContext,
                                   const KEY_T                 deviceMasterKey,
                                   const QLIB_WATCHDOG_CONF_T* watchdogDefault,
                                   const QLIB_DEVICE_CONF_T*   deviceConf)
{
    GMC_T         GMC;
    GMC_T         GMC_test;
    AWDTCFG_T     AWDT   = 0;
    DEVCFG_T      DEVCFG = 0;
    U32           ver    = 0;
    QLIB_STATUS_T ret    = QLIB_STATUS__COMMAND_FAIL;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read the previews value of Global Configuration Register (GMC)                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_GMC(qlibContext, GMC));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Prepare the Global Configuration Register (GMC)                                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    // Increase the version value
    ver = QLIB_REG_GMC_GET_VER(GMC) + 1;
    QLIB_REG_GMC_SET_VER(GMC, ver);

    /*-----------------------------------------------------------------------------------------------------*/
    /* build AWDTCFG defaults                                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    AWDT = QLIB_REG_GMC_GET_AWDT_DFLT(GMC);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set watchdog configuration                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    if (watchdogDefault != NULL)
    {
        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__AWDT_EN, BOOLEAN_TO_INT(watchdogDefault->enable));
        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__LFOSC_EN, BOOLEAN_TO_INT(watchdogDefault->lfOscEn));
        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__SRST_EN, BOOLEAN_TO_INT(watchdogDefault->swResetEn));
        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__AUTH_WDT, BOOLEAN_TO_INT(watchdogDefault->authenticated));
        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__KID, watchdogDefault->sectionID);
        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__TH, watchdogDefault->threshold);

#ifdef Q2_AWDT_IS_CALIBRATED
        // Use the existing calibration value
#else
        if (0 != watchdogDefault->oscRateHz)
        {
            // Set the WD LF-oscillator calibration value from user
            SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__OSC_RATE_KHZ, watchdogDefault->oscRateHz >> 10);
            SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__OSC_RATE_FRAC, watchdogDefault->oscRateHz >> 6);
        }
        else
        {
            // Set the WD LF-oscillator calibration value to default
            SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__OSC_RATE_KHZ, QLIB_AWDTCFG__OSC_RATE_KHZ_DEFAULT);
            SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__OSC_RATE_FRAC, 0);
        }
#endif // Q2_AWDT_IS_CALIBRATED

        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__LOCK, BOOLEAN_TO_INT(watchdogDefault->lock));
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Set pin-mux configuration                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    if (deviceConf != NULL)
    {
        BOOL quadEnable = FALSE;

        /*-------------------------------------------------------------------------------------------------*/
        /* Set IO2/IO3 pin-mux                                                                             */
        /*-------------------------------------------------------------------------------------------------*/
        switch (deviceConf->pinMux.io23Mux)
        {
            /*---------------------------------------------------------------------------------------------*/
            /* Legacy mode: QE=0, RSTI_OVRD=0, RSTO=0                                                      */
            /*---------------------------------------------------------------------------------------------*/
            case QLIB_IO23_MODE__LEGACY_WP_HOLD:
                quadEnable = FALSE;
                SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RSTI_OVRD, 0);
                SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RSTO_EN, 0);
                break;
            /*---------------------------------------------------------------------------------------------*/
            /* RESET mode, QE=0, RSTI_OVRD=1, RSTI=1, RSTO=1                                               */
            /*---------------------------------------------------------------------------------------------*/
            case QLIB_IO23_MODE__RESET_IN_OUT:
                quadEnable = FALSE;
                SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RSTI_OVRD, 1);
                SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RSTI_EN, 1);
                SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RSTO_EN, 1);
                break;
            /*---------------------------------------------------------------------------------------------*/
            /* QUAD mode (QE=1, RSTI_OVRD=0, RSTO=0                                                        */
            /*---------------------------------------------------------------------------------------------*/
            case QLIB_IO23_MODE__QUAD:
                quadEnable = TRUE;
                SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RSTI_OVRD, 0);
                SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RSTO_EN, 0);
                break;
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Set dedicated reset_in pin-mux                                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        SET_VAR_FIELD(AWDT, QLIB_REG_AWDTCFG__RST_IN_EN, BOOLEAN_TO_INT(deviceConf->pinMux.dedicatedResetInEn));

        /*-------------------------------------------------------------------------------------------------*/
        /* Set QE value                                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_STD_SetQuadEnable(qlibContext, quadEnable));

        /*-------------------------------------------------------------------------------------------------*/
        /* Set hold/reset value to 0, we always control the RESET via RSTI_OVRD                            */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_STATUS_RET_CHECK(QLIB_STD_SetResetInEnable(qlibContext, FALSE));

    }

    QLIB_REG_GMC_SET_AWDT_DFLT(GMC, AWDT & (~QLIB_REG_AWDTCFG_RESERVED_MASK));

    /*-----------------------------------------------------------------------------------------------------*/
    /* build DEVCFG register                                                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    if (deviceConf != NULL)
    {
        QLIB_RESET_RESPONSE_T disabledResetResp = {0};

        DEVCFG = QLIB_REG_GMC_GET_DEVCFG(GMC);
#ifndef QLIB_SEC_ONLY
        /*-------------------------------------------------------------------------------------------------*/
        /* Update standard address size                                                                    */
        /*-------------------------------------------------------------------------------------------------*/
        qlibContext->addrSize = QLIB_STD_ADDR_LEN_TO_ADDRESS_OFFSET_SIZE(deviceConf->stdAddrSize.addrLen);
        QLIB_ASSERT_RET(LOG2(QLIB_MAX_STD_ADDR_SIZE) >= qlibContext->addrSize, QLIB_STATUS__INVALID_PARAMETER);
        QLIB_ASSERT_RET(LOG2(QLIB_MIN_STD_ADDR_SIZE) <= qlibContext->addrSize, QLIB_STATUS__INVALID_PARAMETER);
        SET_VAR_FIELD(DEVCFG, QLIB_REG_DEVCFG__SECT_SEL, qlibContext->addrSize - LOG2(QLIB_MIN_STD_ADDR_SIZE));
#endif
        SET_VAR_FIELD(DEVCFG,
                      QLIB_REG_DEVCFG__RST_RESP_EN,
                      (0 == memcmp(&deviceConf->resetResp, &disabledResetResp, sizeof(QLIB_RESET_RESPONSE_T))) ? 0 : 1);
        SET_VAR_FIELD(DEVCFG, QLIB_REG_DEVCFG__FB_EN, BOOLEAN_TO_INT(deviceConf->safeFB));
        SET_VAR_FIELD(DEVCFG, QLIB_REG_DEVCFG__CK_SPECUL, BOOLEAN_TO_INT(deviceConf->speculCK));
        SET_VAR_FIELD(DEVCFG, QLIB_REG_DEVCFG__FORMAT_EN, BOOLEAN_TO_INT(deviceConf->nonSecureFormatEn));
        SET_VAR_FIELD(DEVCFG, QLIB_REG_DEVCFG__STM_EN, 0);
        QLIB_REG_GMC_SET_DEVCFG(GMC, DEVCFG & ~QLIB_REG_DEVCFG_RESERVED_MASK);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Write the new Global Configuration Register (GMC)                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, QLIB_KID__DEVICE_MASTER, deviceMasterKey, FALSE));
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__set_GMC(qlibContext, GMC), ret, error_session);
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Confirm operation success                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_GMC(qlibContext, GMC_test));
    QLIB_ASSERT_RET(0 == memcmp(GMC, GMC_test, sizeof(GMT_T)), QLIB_STATUS__COMMAND_FAIL);

    ret = QLIB_STATUS__OK;

    goto exit;


error_session:
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

exit:
    return ret;
}

QLIB_STATUS_T QLIB_SEC_configGMT_L(QLIB_CONTEXT_T*                   qlibContext,
                                   const KEY_T                       deviceMasterKey,
                                   const QLIB_SECTION_CONFIG_TABLE_T sectionTable)
{
    GMT_T         GMT;
    GMT_T         GMT_test;
    U16           sectionIndex = 0;
    U32           ver          = 0;
    QLIB_STATUS_T ret          = QLIB_STATUS__COMMAND_FAIL;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Read the previews value of Global Mapping Table (GMT)                                               */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_GMT_UNSIGNED(qlibContext, GMT));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Increase the version value                                                                          */
    /*-----------------------------------------------------------------------------------------------------*/
    ver = QLIB_REG_GMT_GET_VER(GMT) + 1;
    QLIB_REG_GMT_SET_VER(GMT, ver);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Prepare the Global Mapping Table (GMT)                                                              */
    /*-----------------------------------------------------------------------------------------------------*/
    for (sectionIndex = 0; sectionIndex < QLIB_NUM_OF_SECTIONS; sectionIndex++)
    {
        U32 sectionBase    = sectionTable[sectionIndex].baseAddr;
        U32 sectionLen     = sectionTable[sectionIndex].size;
        U32 sectionBaseTag = QLIB_REG_SMRn__BASE_IN_BYTES_TO_TAG(sectionBase);
        U32 sectionLenTag  = QLIB_REG_SMRn__LEN_IN_BYTES_TO_TAG(sectionLen);

        /*-------------------------------------------------------------------------------------------------*/
        /* Error checking                                                                                  */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_ASSERT_RET(MAX_FIELD_VAL(QLIB_REG_SMRn__BASE) > sectionBaseTag, QLIB_STATUS__INVALID_PARAMETER);
        QLIB_ASSERT_RET(MAX_FIELD_VAL(QLIB_REG_SMRn__LEN) > sectionLenTag, QLIB_STATUS__INVALID_PARAMETER);
        QLIB_ASSERT_RET((sectionBase + sectionLen) <= QLIB_SEC_FLASH_SIZE, QLIB_STATUS__PARAMETER_OUT_OF_RANGE);
        if (sectionLen != 0)
        {
            QLIB_ASSERT_RET(ALIGNED_TO(sectionBase, _256KB_), QLIB_STATUS__INVALID_PARAMETER);
            QLIB_ASSERT_RET(ALIGNED_TO(sectionLen, _256KB_), QLIB_STATUS__INVALID_PARAMETER);
            QLIB_ASSERT_RET(QLIB_REG_SMRn__BASE_IN_TAG_TO_BYTES(sectionBaseTag) == sectionBase, QLIB_STATUS__INVALID_PARAMETER);
            QLIB_ASSERT_RET(QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(sectionLenTag) == sectionLen, QLIB_STATUS__INVALID_PARAMETER);
        }

        /*-------------------------------------------------------------------------------------------------*/
        /* Set the section mapping                                                                         */
        /*-------------------------------------------------------------------------------------------------*/
        QLIB_REG_GMT_SET_BASE(GMT, sectionIndex, sectionBaseTag);
        QLIB_REG_GMT_SET_LEN(GMT, sectionIndex, sectionLenTag);
        QLIB_REG_GMT_SET_ENABLE(GMT, sectionIndex, sectionLen != 0);
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Write the new Global Mapping Table (GMT)                                                            */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, QLIB_KID__DEVICE_MASTER, deviceMasterKey, FALSE));
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_CMD_PROC__set_GMT(qlibContext, GMT), ret, error_session);
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Confirm operation success                                                                           */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_GMT_UNSIGNED(qlibContext, GMT_test));
    QLIB_ASSERT_RET(0 == memcmp(GMT, GMT_test, sizeof(GMT_T)), QLIB_STATUS__COMMAND_FAIL);

    ret = QLIB_STATUS__OK;

    goto exit;

error_session:
    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));

exit:
    return ret;
}

QLIB_STATUS_T QLIB_SEC_OpenSessionInternal_L(QLIB_CONTEXT_T* qlibContext,
                                             QLIB_KID_T      kid,
                                             const KEY_T     keyBuf,
                                             BOOL            ignoreScrValidity)
{
    QLIB_STATUS_T ret;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Refresh the monotonic counter                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__synch_MC(qlibContext));

    /*-----------------------------------------------------------------------------------------------------*/
    /* Open session                                                                                        */
    /*-----------------------------------------------------------------------------------------------------*/
    ret = QLIB_CMD_PROC__Session_Open(qlibContext, kid, keyBuf, TRUE, ignoreScrValidity);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Open session to a section also enables plain access to this section                                 */
    /*-----------------------------------------------------------------------------------------------------*/
    if ((QLIB_KID__FULL_ACCESS_SECTION == QLIB_KEY_MNGR__GET_KEY_TYPE(kid) ||
         QLIB_KID__RESTRICTED_ACCESS_SECTION == QLIB_KEY_MNGR__GET_KEY_TYPE(kid)) &&
        (QLIB_STATUS__OK == ret || QLIB_STATUS__DEVICE_INTEGRITY_ERR == ret))
    {
        QLIB_POLICY_T policy;
        QLIB_ASSERT_RET(QLIB_KEY_MNGR__GET_KEY_SECTION(kid) < QLIB_NUM_OF_SECTIONS, QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
        QLIB_STATUS_RET_CHECK(QLIB_SEC_GetSectionConfiguration(qlibContext,
                                                               QLIB_KEY_MNGR__GET_KEY_SECTION(kid),
                                                               NULL,
                                                               NULL,
                                                               &policy,
                                                               NULL,
                                                               NULL,
                                                               NULL));
        if (policy.plainAccessReadEnable || policy.plainAccessWriteEnable)
        {
            qlibContext->sectionsState[QLIB_KEY_MNGR__GET_KEY_SECTION(kid)].plainEnabled = 1;
        }
    }

    /*-----------------------------------------------------------------------------------------------------*/
    /* Reseed prng for every session                                                                       */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_CRYPTO_Reseed(&qlibContext->prng);

    return ret;
}

/************************************************************************************************************
 * @brief This function updates the context about a closed session
 *
 * @param qlibContext   QLIB context
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_MarkSessionClose_L(QLIB_CONTEXT_T* qlibContext)
{
    memset(&(qlibContext->keyMngr.sessionKey), 0xFF, sizeof(_128BIT));
    memset(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[0].hashBuf), 0xFF, sizeof(_128BIT));
    memset(QLIB_HASH_BUF_GET__KEY(qlibContext->keyMngr.cmdContexArr[1].hashBuf), 0xFF, sizeof(_128BIT));
    qlibContext->keyMngr.kid = QLIB_KID__INVALID;
    qlibContext->mcInSync    = FALSE;

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief This function closes the session
 *
 * @param qlibContext   QLIB context
 * @param revokePA      Set to TRUE in order to revoke plain-access privileges to restricted or full access section
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_CloseSessionInternal_L(QLIB_CONTEXT_T* qlibContext, BOOL revokePA)
{
    QLIB_ASSERT_RET(QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext), QLIB_STATUS__SYSTEM_IN_INCORRECT_STATE);
    QLIB_ASSERT_RET((revokePA == FALSE) ||
                        (QLIB_KEY_MNGR__GET_KEY_TYPE(qlibContext->keyMngr.kid) == QLIB_KID__FULL_ACCESS_SECTION) ||
                        (QLIB_KEY_MNGR__GET_KEY_TYPE(qlibContext->keyMngr.kid) == QLIB_KID__RESTRICTED_ACCESS_SECTION),
                    QLIB_STATUS__INVALID_PARAMETER);

    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__Session_Close(qlibContext, qlibContext->keyMngr.kid, revokePA));
    QLIB_STATUS_RET_CHECK(QLIB_SEC_MarkSessionClose_L(qlibContext));

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief The function reads Winbond ID
 *
 * @param qlibContext  QLIB context
 * @param id           Returned Winbond ID
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_GetWID_L(QLIB_CONTEXT_T* qlibContext, QLIB_WID_T id)
{
    return QLIB_CMD_PROC__get_WID_UNSIGNED(qlibContext, id);
}

/************************************************************************************************************
 * @brief       This function reads the standard address size from device
 *
 * @param       qlibContext   QLIB state object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_GetStdAddrSize_L(QLIB_CONTEXT_T* qlibContext)
{
    GMC_T    GMC;
    DEVCFG_T DEVCFG = 0;

    // Get GMC
    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_GMC(qlibContext, GMC));
    // Get DEVCFG
    DEVCFG = QLIB_REG_GMC_GET_DEVCFG(GMC);
    // Get SECT_SEL
    qlibContext->addrSize = READ_VAR_FIELD(DEVCFG, QLIB_REG_DEVCFG__SECT_SEL) + LOG2(QLIB_MIN_STD_ADDR_SIZE);

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This function reads the Global Mapping Table and saves the sections sizes in context
 *
 * @param       qlibContext   QLIB state object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_GetSectionsSize_L(QLIB_CONTEXT_T* qlibContext)
{
    GMT_T gmt;
    U32   sectionID;

    QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__get_GMT_UNSIGNED(qlibContext, gmt));

    for (sectionID = 0; sectionID < QLIB_NUM_OF_SECTIONS; sectionID++)
    {
        qlibContext->sectionsState[sectionID].enabled = QLIB_REG_GMT_GET_ENABLE(gmt, sectionID);
        qlibContext->sectionsState[sectionID].sizeTag = QLIB_REG_GMT_GET_LEN(gmt, sectionID);
    }

    return QLIB_STATUS__OK;
}

/************************************************************************************************************
 * @brief       This function reads the watchdog configuration from the device
 *
 * @param       qlibContext   QLIB state object
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_GetWatchdogConfig_L(QLIB_CONTEXT_T* qlibContext)
{
    AWDTCFG_T AWDTCFG;

    QLIB_STATUS_RET_CHECK(QLIB_SEC__get_AWDTCNFG(qlibContext, &AWDTCFG));
    qlibContext->watchdogIsSecure  = INT_TO_BOOLEAN(READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__AUTH_WDT));
    qlibContext->watchdogSectionId = READ_VAR_FIELD(AWDTCFG, QLIB_REG_AWDTCFG__KID);

    return QLIB_STATUS__OK;
}


/************************************************************************************************************
 * @brief       This function performs initial section policy configuration
 *
 * @param[in,out]   qlibContext         QLIB state object
 * @param[in]       sectionIndex        Section configuration table
 * @param[in]       policy              Section policy to configure
 * @param[in]       fullAccessKey       Full-access key of the section
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SEC_ConfigInitialSectionPolicy_L(QLIB_CONTEXT_T*      qlibContext,
                                                    U32                  sectionIndex,
                                                    const QLIB_POLICY_T* policy,
                                                    const KEY_T          fullAccessKey)
{
    QLIB_STATUS_T ret        = QLIB_STATUS__OK;
    QLIB_KID_T    kid        = QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__FULL_ACCESS_SECTION, sectionIndex);
    QLIB_POLICY_T initPolicy = *policy;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Must have fullAccessKey in order to write SCRn                                                      */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_ASSERT_RET(QLIB_KEY_MNGR__IS_KEY_VALID(fullAccessKey), QLIB_STATUS__INVALID_PARAMETER);

    /*-----------------------------------------------------------------------------------------------------*/
    /* Ignore checksum and digest integrity on initial section configuration                               */
    /*-----------------------------------------------------------------------------------------------------*/
    initPolicy.checksumIntegrity = 0;
    initPolicy.digestIntegrity   = 0;

    /*-----------------------------------------------------------------------------------------------------*/
    /* Perform (SCRn) write                                                                                */
    /*-----------------------------------------------------------------------------------------------------*/
    QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSessionInternal_L(qlibContext, kid, fullAccessKey, TRUE));
    QLIB_STATUS_RET_CHECK_GOTO(QLIB_SEC_ConfigSection(qlibContext, sectionIndex, &initPolicy, NULL, NULL, NULL, QLIB_SWAP_NO),
                               ret,
                               error_session);
    // After QLIB_SEC_ConfigSection the session is closed automatically

    return QLIB_STATUS__OK;

error_session:
    QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE);
    return ret;
}

/************************************************************************************************************
 * @brief       This function checks if the section policy should be configured after GMT configuration or before
 *
 * @param[in,out]   qlibContext         QLIB state object
 * @param[in]       sectionIndex        Section configuration table
 *
 * @return      TRUE if section policy should be configured only after GMT. FALSE to configure before GMT
************************************************************************************************************/
BOOL QLIB_SEC_ConfigSectionPolicyAfterGMT_L(QLIB_CONTEXT_T* qlibContext, U32 sectionIndex)
{
    /*-----------------------------------------------------------------------------------------------------*/
    /* If the section was not configured yet - it needs to first be set in GMT.                            */
    /* If section rollback protection is enabled, the minimal section size is 2 blocks. Therefore, in case */
    /* the section is currently configured with size of 1 block, first section size should be set and only */
    /* then, the policy.                                                                                   */
    /*-----------------------------------------------------------------------------------------------------*/
    return ((qlibContext->sectionsState[sectionIndex].enabled == 0) || (qlibContext->sectionsState[sectionIndex].sizeTag == 0));
}

/************************************************************************************************************
 * @brief       This function Opens a session to a section if it is not already open.
 *              If another session is open, the function closes it first and optionally returns its ID.
 *
 * @param[in,out]   qlibContext         QLIB state object
 * @param[in]       sectionID           Section to open
 * @param[in]       grant               TRUE to grant authenticated plain access. FALSE to revoke it.
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_GrantRevokePA_L(QLIB_CONTEXT_T* qlibContext, U32 sectionID, BOOL grant)
{
    U32             sectionOpened = QLIB_NUM_OF_SECTIONS; //invalid value
    QLIB_KID_TYPE_T keyType       = QLIB_KID__INVALID;

    /*-----------------------------------------------------------------------------------------------------*/
    /* If session is open on section other then sectionID, we close it                                     */
    /*-----------------------------------------------------------------------------------------------------*/
    if (QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContext))
    {
        keyType = QLIB_KEY_MNGR__GET_KEY_TYPE(qlibContext->keyMngr.kid);
        if (QLIB_KID__FULL_ACCESS_SECTION == keyType || QLIB_KID__RESTRICTED_ACCESS_SECTION == keyType)
        {
            sectionOpened = QLIB_KEY_MNGR__GET_KEY_SECTION(qlibContext->keyMngr.kid);

            if (sectionOpened == sectionID)
            {
                if (grant == FALSE)
                {
                    QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, TRUE));
                }
                return QLIB_STATUS__OK;
            }
            if (grant == TRUE)
                QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSession(qlibContext, sectionOpened));
        }
    }

    if (grant == TRUE)
    {
        /*-------------------------------------------------------------------------------------------------*/
        /* Here no section is open, so we open the session to the section                                  */
        /*-------------------------------------------------------------------------------------------------*/
        if (NULL != QLIB_KEY_MNGR__GET_SECTION_KEY_RESTRICTED(qlibContext, sectionID))
        {
            QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSession(qlibContext, sectionID, QLIB_SESSION_ACCESS_RESTRICTED));
        }
        else if (NULL != QLIB_KEY_MNGR__GET_SECTION_KEY_FULL_ACCESS(qlibContext, sectionID))
        {
            QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSession(qlibContext, sectionID, QLIB_SESSION_ACCESS_FULL));
        }
        else
        {
            return QLIB_STATUS__DEVICE_PRIVILEGE_ERR;
        }
    }

    if (grant == TRUE)
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC_CloseSessionInternal_L(qlibContext, FALSE));
    }
    else
    {
        // revoke PA
        QLIB_STATUS_RET_CHECK(QLIB_CMD_PROC__init_section_PA(qlibContext, sectionID));
        QLIB_STATUS_RET_CHECK(QLIB_SEC_MarkSessionClose_L(qlibContext));
        qlibContext->sectionsState[sectionID].plainEnabled = 0;
    }
    /*-----------------------------------------------------------------------------------------------------*/
    /* If session X was open before it must be reopened                                                    */
    /*-----------------------------------------------------------------------------------------------------*/
    if (sectionOpened != QLIB_NUM_OF_SECTIONS)
    {
        QLIB_STATUS_RET_CHECK(QLIB_SEC_OpenSession(qlibContext,
                                                   sectionOpened,
                                                   keyType == QLIB_KID__FULL_ACCESS_SECTION ? QLIB_SESSION_ACCESS_FULL
                                                                                            : QLIB_SESSION_ACCESS_RESTRICTED));
    }

    return QLIB_STATUS__OK;
}

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * @brief       This function verifies that all plain access sections are accessible according to the configured address size
 *
 * @param[in,out]   qlibContext         QLIB state object
 * @param[in]       addrSizeConf        Address size configuration
 *
 * @return      QLIB_STATUS__OK on success or QLIB_STATUS__[ERROR] otherwise
************************************************************************************************************/
static QLIB_STATUS_T QLIB_SEC_VerifyAddressSizeConfig_L(QLIB_CONTEXT_T* qlibContext, const QLIB_STD_ADDR_SIZE_T* addrSizeConf)
{
    U8            sectionIndex = 0;
    QLIB_POLICY_T policy;

    for (sectionIndex = 0; sectionIndex < QLIB_NUM_OF_SECTIONS; sectionIndex++)
    {
        if (qlibContext->sectionsState[sectionIndex].enabled == 1)
        {
            U8 maxStdSecId;
            QLIB_STATUS_RET_CHECK(
                QLIB_SEC_GetSectionConfiguration(qlibContext, sectionIndex, NULL, NULL, &policy, NULL, NULL, NULL));

            if (policy.plainAccessReadEnable == 1 || policy.plainAccessWriteEnable == 1)
            {
                maxStdSecId =
                    _QLIB_MAX_LEGACY_SECTION_ID_BY_ADDR_SIZE(QLIB_STD_ADDR_LEN_TO_ADDRESS_OFFSET_SIZE(addrSizeConf->addrLen));
                QLIB_ASSERT_RET(sectionIndex < maxStdSecId, QLIB_STATUS__INVALID_PARAMETER);
                QLIB_ASSERT_RET(QLIB_REG_SMRn__LEN_IN_TAG_TO_BYTES(qlibContext->sectionsState[sectionIndex].sizeTag) <=
                                    _QLIB_MAX_LEGACY_OFFSET_BY_ADDR_SIZE(
                                        QLIB_STD_ADDR_LEN_TO_ADDRESS_OFFSET_SIZE(addrSizeConf->addrLen)),
                                QLIB_STATUS__INVALID_PARAMETER);
            }
        }
    }
    return QLIB_STATUS__OK;
}
#endif
