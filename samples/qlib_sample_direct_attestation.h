/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2020 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_direct_attestation.h
* @brief      This file contains QLIB direct attestation sample definitions
*
* ### project qlib_sample
*
************************************************************************************************************/

#ifndef _QLIB_SAMPLE_DIRECT_ATTESTATION__H_
#define _QLIB_SAMPLE_DIRECT_ATTESTATION__H_

/************************************************************************************************************
 * @brief       This function retrieves value for direct attestation flow on w77q
 *
 * @param[in]   qlibContext             qlib context
 * @param[in]   sectionId               section id
 * @param[out]  digest                  digest value of the section that is used for attestation
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_DirectAttestationDigestGet(QLIB_CONTEXT_T* qlibContext, U32 sectionId, U64* digest);

/************************************************************************************************************
 * @brief       This function demonstrates direct attestation flow on w77q
 *              The sample demonstrates the "Generating Direct Root Of Trust (Direct Attestation)" secure flow
 *              described in W77Q AS v0.93 revA section from January 17,2021 (section 8.6.4)
 *
 * @param[in]   digestExp               Expected digest value of the section that is used for attestation
 *
 * @return      QLIB_STATUS__OK if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_DirectAttestationFlow(U64 digestExp);

#endif // _QLIB_SAMPLE_DIRECT_ATTESTATION__H_
