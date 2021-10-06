/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2019 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_platform.h
* @brief      This file includes platform specific definitions
*
* ### project qlib
*
************************************************************************************************************/
#ifndef __QLIB_PLATFORM_H__
#define __QLIB_PLATFORM_H__

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                          QLIB CONFIGURATIONS                                            */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * define QLIB_MAX_SPI_INPUT_SIZE to SPI input buffer limitation. If defined PLAT_SPI_WriteReadTransaction
 * will be invoked in chunks of that limit.
************************************************************************************************************/
//example for 1024 bytes limit
//#define QLIB_MAX_SPI_INPUT_SIZE 1024


/************************************************************************************************************
 * Enable async HASH implementation if available
************************************************************************************************************/
//#define QLIB_HASH_OPTIMIZATION_ENABLED
//#define QLIB_SPI_OPTIMIZATION_ENABLED

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                         QLIB DEFINE OVERRIDES                                           */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief   This macro declare variable to store interrupt state
 * @param   ints     variable name that is declared to store the interrupt state
************************************************************************************************************/
#ifndef INTERRUPTS_VAR_DECLARE
#define INTERRUPTS_VAR_DECLARE(ints)
#endif

/************************************************************************************************************
 * @brief   This macro preserves interrupt state and disable interrupts - Used in the beginning of an atomic flow
 * @param   ints   the variable declared in @ref INTERRUPTS_VAR_DECLARE which will store the current enabled interrupts
************************************************************************************************************/
#ifndef INTERRUPTS_SAVE_DISABLE
#define INTERRUPTS_SAVE_DISABLE(ints)
#endif

/************************************************************************************************************
 * @brief   This macro restores the saved interrupt state from @ref INTERRUPTS_SAVE_DISABLE. Used at the end of an atomic flow
 * @param   ints   the variable declared in @ref INTERRUPTS_VAR_DECLARE which used in @ref INTERRUPTS_SAVE_DISABLE and stored the enabled interrupts
************************************************************************************************************/
#ifndef INTERRUPTS_RESTORE
#define INTERRUPTS_RESTORE(ints)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                                INCLUDES                                                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                       PLATFORM SPECIFIC FUNCTIONS                                       */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This function resets the core CPU
************************************************************************************************************/
void CORE_RESET(void);

/************************************************************************************************************
 * @brief       This function initialize the platform specific modules used by Qlib
 *
 * @param[in]   spiFreq   SPI bus clock frequency
 *
 * @return      none
************************************************************************************************************/
void PLAT_Init(U32 spiFreq);

/************************************************************************************************************
 * @brief       This function is an implementation of the hash function supported by the W77Q defined in the spec.\n
 * For performance reasons, it is recommended to use HW implementation of this function.\n
 * Test vectors can be found in the spec TBD
 *
 * @param[out]  output     digest
 * @param[in]   data       Input data
 * @param[in]   dataSize   Input data size in bytes
 *
 * @return      none
************************************************************************************************************/
void PLAT_HASH(U32* output, const U32* data, U32 dataSize);

#ifdef QLIB_HASH_OPTIMIZATION_ENABLED

/************************************************************************************************************
 * @brief The function starts asynchronous HASH calculation
 *
 * @param[out]  output     Pointer to HASH output
 * @param[in]   data       Input data
 * @param[in]   dataSize   Input data size in bytes
************************************************************************************************************/
void PLAT_HASH_Async(U32* output, const U32* data, U32 dataSize);

/************************************************************************************************************
 * @brief The function waits for previous asynchronous HASH calculation to finish.
************************************************************************************************************/
void PLAT_HASH_Async_WaitWhileBusy(void);

#endif //QLIB_HASH_OPTIMIZATION_ENABLED

/************************************************************************************************************
 * @brief       This function returns non-repeating 'nonce' number.
 * A 'nonce' is a 64bit number that is used in session establishment.\n
 * To prevent replay attacks such nonce must be 'non-repeating' - appear different each function execution.\n
 * Typically implemented as a HW TRNG.
 *
 * @return      64 bit random number
************************************************************************************************************/
U64 PLAT_GetNONCE(void);

/************************************************************************************************************
 * @brief       This routine performs SPI write-read transaction.
 * This function should be linked to RAM memory.\n
 * In order to verify that the function works properly, please use the following wave form examples:\n\n
 * PLAT_SPI_WriteReadTransaction(format=1_1_1, dtr=0, cmd=0x90, addr=0, addrSize=3, dataOut=NULL, dataOutSize=0, dummy=0, dataIn=Ptr, dataInSize=2)\n
 * ![wave form example 1](spi_wave_form_1.png)\n\n
 * PLAT_SPI_WriteReadTransaction(format=1_1_1, dtr=0, cmd=0x0B, addr=0, addrSize=3, dataOut=NULL, dataOutSize=0, dummy=8, dataIn=Ptr, dataInSize=32)\n
 * ![wave form example 2](spi_wave_form_2.png)
 * The system integrator needs to allocate sufficient space in the SPI controller on the MCU side to drive the command, address, and dataOut for the SPI transaction.
 * Sufficient space should be allocated in the SPI controller to sample dataIn.
 * The maximal sizes for the fields are specified in the routine's interface below @p dataOutSize and @p dataInSize parameters.
 *
 * @param[in,out]   userData        User data which is set using @ref QLIB_SetUserData
 * @param[in]       format          SPI format
 * @param[in]       dtr             DTR - Data is received and transmitted in both falling and rising edge of the CLK
 * @param[in]       cmd             SPI command
 * @param[in]       address         Command address
 * @param[in]       addressSize     Size of the address in bytes
 * @param[in]       dataOut         pointer to a buffer which holds the data to transmit
 * @param[in]       dataOutSize     transmit data size in bytes. Maximal dataOutSize is 40 bytes for secure commands (if configuring reset response (optional) the maximal dataOutSize is 72 bytes).
 * @param[in]       dummyCycles     Dummy cycles between write and read phases
 * @param[out]      dataIn          pointer to a buffer which holds the data received
 * @param[in]       dataInSize      data received size in bytes. Maximal dataInSize is 44 bytes for secure commands.
 *
 * @return
 * QLIB_STATUS__OK = 0                      - no error occurred\n
 * QLIB_STATUS__(ERROR)                     - Other error
************************************************************************************************************/
QLIB_STATUS_T PLAT_SPI_WriteReadTransaction(const void*     userData,
                                            QLIB_BUS_MODE_T format,
                                            BOOL            dtr,
                                            U8              cmd,
                                            U32             address,
                                            U32             addressSize,
                                            const U8*       dataOut,
                                            U32             dataOutSize,
                                            U32             dummyCycles,
                                            U8*             dataIn,
                                            U32             dataInSize) __RAM_SECTION;

#ifdef QLIB_SPI_OPTIMIZATION_ENABLED

/************************************************************************************************************
 * @brief       This routine performs SPI multi transaction start.
 * When using the same SPI command, `PLAT_SPI_MultiTransactionStart` saves the command in dedicated SPI cache.\n
 * Next time the command will be called it will be taken directly from the SPI cache thus saving calculation time
************************************************************************************************************/
void PLAT_SPI_MultiTransactionStart(void);

/************************************************************************************************************
 * @brief       This routine performs SPI multi transaction stop.
 * This function stops the usage of SPI cache saved while calling `PLAT_SPI_MultiTransactionStart`
************************************************************************************************************/
void PLAT_SPI_MultiTransactionStop(void);

#endif //QLIB_SPI_OPTIMIZATION_ENABLED

#ifdef __cplusplus
}
#endif

#endif // __QLIB_PLATFORM_H__
