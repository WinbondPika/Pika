/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2021 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_net.h
* @brief      network definitions
*
* @example    qlib_sample_net.h
*
* @page       Contains network related definitions needed for sample code of server/client
*             application over w77q
*
* @include    samples/remote/qlib_sample_net.h
*
************************************************************************************************************/

#ifndef _QLIB_SAMPLE_NET__H_
#define _QLIB_SAMPLE_NET__H_

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INCLUDES                                                      */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           DEFINITIONS                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define QLIB_SAMPLE_IOT_PORT_DEF "50001"

// Management frames
#define QLIB_SAMPLE_NET_TYPE_IOT_DISCOVERY 0x30
#define QLIB_SAMPLE_NET_TYPE_MESSAGE       0x31

// Frame types dedicated to TM layer implementation
#define QLIB_SAMPLE_NET_TYPE_TM_STD        0x32
#define QLIB_SAMPLE_NET_TYPE_TM_SEC        0x33
#define QLIB_SAMPLE_NET_TYPE_TM_CONNECT    0x34
#define QLIB_SAMPLE_NET_TYPE_TM_DISCONNECT 0x35

// every platform will define how it sends packets and hao socket is defined
#if defined(__linux__)
#define QLIB_SAMPLE_SOCKET         int
#define QLIB_SAMPLE_MUTEX          pthread_mutex_t
#define QLIB_SAMPLE_DELAY_SEC(sec) sleep(sec)

#elif defined(WIN32)
#define QLIB_SAMPLE_SOCKET         SOCKET
#define QLIB_SAMPLE_MUTEX          U32
#define QLIB_SAMPLE_DELAY_SEC(sec) Sleep(sec * 1000)

#elif defined(__MCUXPRESSO)
#define QLIB_SAMPLE_SOCKET         int
#define QLIB_SAMPLE_MUTEX          int
#define QLIB_SAMPLE_DELAY_SEC(sec) SDK_DelayAtLeastUs(sec * 1000000, SDK_DEVICE_MAXIMUM_CPU_CLOCK_FREQUENCY)
#else
#WARNING add definitions for your platform
#endif

#define QLIB_SAMPLE_IOT_NAME_SIZE 50

// FLASH_PAGE_SIZE is a maximum data SENT ( in standard transaction )
#define QLIB_SAMPLE_MAX_DATA_SIZE 128

// MTU between server client for transmission without data fragmentation
#define QLIB_SAMPLE_MTU                                                                                  \
    (sizeof(QLIB_SAMPLE_HDR_NET_T) + MAX(sizeof(QLIB_SAMPLE_HDR_SEC_T), sizeof(QLIB_SAMPLE_HDR_STD_T)) + \
     QLIB_SAMPLE_MAX_DATA_SIZE)

#define QLIB_SAMPLE_MTU_INTS (QLIB_SAMPLE_MTU / sizeof(U32) + 1)

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------------------------------------*/
/* Start of packed definitions                                                                             */
/*---------------------------------------------------------------------------------------------------------*/
PACKED_START

// Network header to all packets
typedef struct
{
    U16 messageType;
    U16 messageLen;
} PACKED QLIB_SAMPLE_HDR_NET_T;

// Header of QLIB_TM_Secure function. Used to pack QLIB_TM_Secure parameters for sending to client.
typedef struct
{
    U32 ctag;
    U32 writeDataSize;
    U32 readDataSize;
    U32 ssrValue;
} PACKED QLIB_SAMPLE_HDR_SEC_T;

// Header of QLIB_TM_Standard function. Used to pack QLIB_TM_Standard parameters for sending to client.
typedef struct
{
    QLIB_BUS_MODE_T format;
    BOOL            dtr;
    BOOL            needWriteEnable;
    BOOL            waitWhileBusy;
    U32             cmd;
    U32             address;
    U32             dummyCycles;
    U32             writeDataSize;
    U32             readDataSize;
    U32             ssrValue;
} PACKED QLIB_SAMPLE_HDR_STD_T;

/*---------------------------------------------------------------------------------------------------------*/
/* End of packed definitions                                                                               */
/*---------------------------------------------------------------------------------------------------------*/
PACKED_END

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INTERFACE                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine sends data via socket. Should be given specific OS implementation.
 *
 * @param[in]       data                data buffer for to be send
 * @param[in]       len                 length of data buffer in bytes
 * @param[in]       sock                socket to be sent via
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
I32 QLIB_SAMPLE_NET_socketSend(const char* data, U32 len, QLIB_SAMPLE_SOCKET sock);

#endif // _QLIB_SAMPLE_NET__H_
