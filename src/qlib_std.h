/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_std.h
* @brief      This file contains QLIB standard flash interface
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_STD_H__
#define __QLIB_STD_H__

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
/*                                               DEFINITIONS                                               */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define QLIB_STD_GET_BUS_MODE(context) \
    ((QLIB_BUS_MODE_4_4_4 == (context)->busInterface.busMode) ? QLIB_BUS_MODE_4_4_4 : QLIB_BUS_MODE_1_1_1)

#define QLIB_STD_ENTER_EXIT_QPI(context, enter)                                                    \
    QLIB_STD_execute_std_cmd_L(context,                                                            \
                               enter == TRUE ? QLIB_BUS_MODE_1_1_1 : QLIB_BUS_MODE_4_4_4,          \
                               FALSE,                                                              \
                               FALSE,                                                              \
                               FALSE,                                                              \
                               enter == TRUE ? SPI_FLASH_CMD__ENTER_QPI : SPI_FLASH_CMD__EXIT_QPI, \
                               NULL,                                                               \
                               NULL,                                                               \
                               0,                                                                  \
                               0,                                                                  \
                               NULL,                                                               \
                               0,                                                                  \
                               NULL)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                 MACROS                                                  */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#define _QLIB_MAKE_LOGICAL_ADDRESS(secId, offset, addrSize) (((secId) << (addrSize)) | (offset))
#ifndef QLIB_SEC_ONLY
#define _QLIB_MAX_LEGACY_OFFSET_BY_ADDR_SIZE(addrSize) ((U32)0x1 << (addrSize))
#define _QLIB_MAX_LEGACY_OFFSET(context)               _QLIB_MAX_LEGACY_OFFSET_BY_ADDR_SIZE(QLIB_STD_ADDR_SIZE(context))
#define _QLIB_MAX_LEGACY_SECTION_ID_BY_ADDR_SIZE(addrSize) (22 == (addrSize) ? 4UL : QLIB_NUM_OF_SECTIONS)
#define _QLIB_MAX_LEGACY_SECTION_ID(context) _QLIB_MAX_LEGACY_SECTION_ID_BY_ADDR_SIZE(context->addrSize)
#define _QLIB_OFFSET_FROM_LOGICAL_ADDRESS(logicalAddr, addrSize) \
    ((_QLIB_MAX_LEGACY_OFFSET_BY_ADDR_SIZE(addrSize) - 1) & (logicalAddr))
#define _QLIB_SECTION_FROM_LOGICAL_ADDRESS(logicalAddr, addrSize) ((logicalAddr) >> (addrSize))
#endif // QLIB_SEC_ONLY

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE FUNCTIONS                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * @brief       This routine initializes the context of the STD module
 *              This function should be called once at the beginning of every host (local or remote)
 *
 * @param       qlibContext       internal context object
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_InitLib(QLIB_CONTEXT_T* qlibContext);
#endif // QLIB_SEC_ONLY

/************************************************************************************************************
 * @brief       This routine exits power down mode, check if the flash is run in SPI or QPI mode,
 *              find the fastest bus format (lines count), and make sure flash is not busy and there is
 *              no connectivity issues.
 *
 * @param       qlibContext   qlib context
 * @param[out]  busMode       current optimal bus mode
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_AutoSense(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_MODE_T* busMode);

/************************************************************************************************************
 * @brief       This routine configures Flash interface configuration. If upgrade from single -> quad
 *              detected, quad hw is enabled. If detected quad -> single, hw quad is NOT disabled since to
 *              save transaction.
 *
 * @param       qlibContext   qlib context object
 * @param[in]   busFormat     SPI bus format
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_SetInterface(QLIB_CONTEXT_T* qlibContext, QLIB_BUS_FORMAT_T busFormat);

/************************************************************************************************************
 * @brief           This function sets non-volatile QE (QuadEnable) bit value
 *
 * @param[out]      qlibContext     qlib context object
 * @param[in]       enable          QE value
 *
 * @return          0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_SetQuadEnable(QLIB_CONTEXT_T* qlibContext, BOOL enable);

/************************************************************************************************************
 * @brief           This function sets SR3.HOLD/RST bit value
 *
 * @param[out]      qlibContext     qlib context object
 * @param[in]       enable          HOLD/RST value
 *
 * @return          0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_SetResetInEnable(QLIB_CONTEXT_T* qlibContext, BOOL enable);

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * @brief       This routine performs legacy Flash read command
 *
 * @param       qlibContext   qlib context object
 * @param[out]  output        Output buffer for read data
 * @param[in]   logicalAddr   logical flash address
 * @param[in]   size          Number of bytes to read from Flash
 *
 * @return      0 in no error occurred, or QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_Read(QLIB_CONTEXT_T* qlibContext, U8* output, U32 logicalAddr, U32 size);

/************************************************************************************************************
 * @brief       This routine performs STD Flash write command
 *
 * @param       qlibContext   qlib context object
 * @param[in]   input         Data for writing
 * @param[in]   logicalAddr   logical flash address
 * @param[in]   size          Number of bytes to write
 *
 * @return      0 in no error occurred, or QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_Write(QLIB_CONTEXT_T* qlibContext, const U8* input, U32 logicalAddr, U32 size);

/************************************************************************************************************
 * @brief       This routine performs blocking / non-blocking Flash erase (sector/block/chip)
 *
 * @param       qlibContext   qlib context object
 * @param[in]   eraseType     type of erase (sector/block/chip)
 * @param[in]   logicalAddr   logical flash address
 * @param[in]   blocking      if true, this function is blocking till the erase is finish
 *
 * @return      0 on success, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_PerformErase(QLIB_CONTEXT_T* qlibContext, QLIB_ERASE_T eraseType, U32 logicalAddr, BOOL blocking);

/************************************************************************************************************
 * @brief       This routine performs STD Flash erase command (sector/block)
 *
 * @param       qlibContext   qlib context object
 * @param[in]   logicalAddr   logical flash address
 * @param[in]   size          number of Bytes to erase
 *
 * @return      0 in no error occurred, or QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_Erase(QLIB_CONTEXT_T* qlibContext, U32 logicalAddr, U32 size);

/************************************************************************************************************
 * @brief       This routine suspends the on-going erase operation
 *
 * @param       qlibContext   qlib context object
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_EraseSuspend(QLIB_CONTEXT_T* qlibContext) __RAM_SECTION;

/************************************************************************************************************
 * @brief       This routine resumes the suspended erase operation
 *
 * @param       qlibContext   qlib context object
 * @param       blocking      if TRUE, this function will block execution till the suspended operation end
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_EraseResume(QLIB_CONTEXT_T* qlibContext, BOOL blocking) __RAM_SECTION;

/************************************************************************************************************
 * @brief       This routine changes Flash power state
 *
 * @param       qlibContext   internal context object
 * @param       power         the require new power state
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_Power(QLIB_CONTEXT_T* qlibContext, QLIB_POWER_T power) __RAM_SECTION;
#endif // QLIB_SEC_ONLY

/************************************************************************************************************
 * @brief       This routine resets the Flash, forceReset must be true if the reset is called before
 *              the module has initiated
 *
 * @param       qlibContext   qlib context object
 * @param       forceReset    if TRUE, the reset operation force the reset, this may cause a
 *                            a corrupted data.
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_ResetFlash(QLIB_CONTEXT_T* qlibContext, BOOL forceReset) __RAM_SECTION;

#ifndef QLIB_SEC_ONLY
/************************************************************************************************************
 * @brief       This routine return the Hardware info of the device (manufacturer, device, memory type, capacity)
 *
 * @param       qlibContext   qlib context object
 * @param[out]  hwVer        (OUT) pointer to struct contains the HW version
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_GetHwVersion(QLIB_CONTEXT_T* qlibContext, QLIB_STD_HW_VER_T* hwVer);

/************************************************************************************************************
 * @brief       This routine return the Unique IDs of the flash
 *
 * @param       qlibContext   qlib context object
 * @param[out]  id_p          (OUT) pointer to struct contains the Unique IDs of flash
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_GetId(QLIB_CONTEXT_T* qlibContext, QLIB_STD_ID_T* id_p);

#endif // QLIB_SEC_ONLY



/************************************************************************************************************
* @brief       This routine fetches the flash memory type
*
* @param       qlibContext   qlib context object
* @param[in]   memType       (OUT) The flash memory type
*
* @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_STD_GetMemoryType(QLIB_CONTEXT_T* qlibContext, U8* memType);
#ifdef __cplusplus
}
#endif

#endif // __QLIB_STD_H__
