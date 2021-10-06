/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_key_mngr.h
* @brief      This file contains QLIB Key Manager interface
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_KEY_MNGR_H__
#define __QLIB_KEY_MNGR_H__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define QLIB_KEY_MNGR__GET_KEY_TYPE(kid)              ((QLIB_KID_TYPE_T)(((kid) < 0x30) ? ((kid)&0xF0) : (kid)))
#define QLIB_KEY_MNGR__GET_KEY_SECTION(kid)           ((kid)&0xF)
#define QLIB_KEY_MNGR__KID_WITH_SECTION(kid, section) ((QLIB_KID_T)((kid) | (section)))
#define QLIB_KEY_MNGR__INVALIDATE_KEY(key)            memset((key), 0, sizeof(KEY_T))
#define QLIB_KEY_MNGR__SESSION_IS_OPEN(qlibContex)    ((qlibContex)->keyMngr.kid != QLIB_KID__INVALID)
#define QLIB_KEY_MNGR__IS_KEY_VALID(key) \
    (((key) != NULL) && (((key)[0] != 0) || ((key)[1] != 0) || ((key)[2] != 0) || ((key)[3] != 0)))
#define QLIB_KEY_MNGR__GET_SECTION_KEY_FULL_ACCESS(qlibContext, section) ((qlibContext)->keyMngr.fullAccessKeys[section])
#define QLIB_KEY_MNGR__GET_SECTION_KEY_RESTRICTED(qlibContext, section)  ((qlibContext)->keyMngr.restrictedKeys[section])

#define QLIB_KEY_ID_IS_PROVISIONING(kid)                                     \
    ((QLIB_KEY_MNGR__GET_KEY_TYPE(kid) == QLIB_KID__SECTION_PROVISIONING) || \
     (QLIB_KEY_MNGR__GET_KEY_TYPE(kid) == QLIB_KID__DEVICE_KEY_PROVISIONING))

#define QLIB_KEYMNGR_IS_SECTION_FULL_ACCESS(qlibContext, sectionId) \
    ((qlibContext)->keyMngr.kid == QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__FULL_ACCESS_SECTION, sectionId))

#define QLIB_KEY_MNGR_IS_SECTION_RESTRICTED_ACCESS(qlibContext, sectionId) \
    ((qlibContext)->keyMngr.kid == QLIB_KEY_MNGR__KID_WITH_SECTION(QLIB_KID__RESTRICTED_ACCESS_SECTION, sectionId))

/*---------------------------------------------------------------------------------------------------------*/
/* Stores new key to Key Manager                                                                           */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_KEYMNGR_SetKey(keyMngr, key, sectionID, fullAccess)                                                  \
    {                                                                                                             \
        CONST_KEY_P_T* __keys = (((fullAccess) == TRUE) ? (keyMngr)->fullAccessKeys : (keyMngr)->restrictedKeys); \
        __keys[(sectionID)]   = (key);                                                                            \
    }

/*---------------------------------------------------------------------------------------------------------*/
/* Retrieves key from Key Manager                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_SEC_KEYMNGR_GetKey(keyMngr, key, sectionID, fullAccess)                                                         \
    {                                                                                                                        \
        *(key) = (((fullAccess) == TRUE) ? (keyMngr)->fullAccessKeys[(sectionID)] : (keyMngr)->restrictedKeys[(sectionID)]); \
    }

/*---------------------------------------------------------------------------------------------------------*/
/* Removes key from Key Manager                                                                            */
/*---------------------------------------------------------------------------------------------------------*/
#define QLIB_KEYMNGR_RemoveKey(keyMngr, sectionID, fullAccess) QLIB_KEYMNGR_SetKey((keyMngr), NULL, (sectionID), (fullAccess))

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief           This routine performs Key Manager init
 *
 * @param[in,out]   keyMngr   Key manager context
 *
 * @return
 * QLIB_STATUS__OK = 0                      - no error occurred\n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T QLIB_KEYMNGR_Init(QLIB_KEY_MNGR_T* keyMngr);

#ifdef __cplusplus
}
#endif

#endif // __QLIB_KEY_MNGR_H__
