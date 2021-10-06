/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2021 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_server.h
* @brief      server sample code
*
* @example    qlib_sample_server.h
*
* @page       This sample code shows server application over w77q sample definitions
*
* @include    samples/remote/qlib_sample_server.h
*
************************************************************************************************************/

#ifndef _QLIB_SAMPLE_SERVER__H_
#define _QLIB_SAMPLE_SERVER__H_

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INCLUDES                                                      */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_sample_net.h"
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           DEFINITIONS                                                   */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

#define QLIB_SERVER_SAMPLE_CLIENT_DATA (QLIB_SAMPLE_MTU - sizeof(QLIB_SAMPLE_HDR_NET_T))

#if defined(__linux__)

#define QLIB_SAMPLE_THREAD pthread_t
#define QLIB_SAMPLE_MUTEX  pthread_mutex_t
#define QLIB_SERVER_SAMPLE_WAIT_FOR_RESPONSE(iotInfo)                                     \
    {                                                                                     \
        struct timespec ts;                                                               \
        clock_gettime(CLOCK_REALTIME, &ts);                                               \
        ts.tv_sec += iotInfo->respTimeoutSeconds;                                         \
        pthread_cond_timedwait(&iotInfo->responseOccurred, &iotInfo->responseMutex, &ts); \
    }
#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_INIT(iotInfo)      \
    {                                                        \
        pthread_mutex_init(&iotInfo->responseMutex, NULL);   \
        pthread_cond_init(&iotInfo->responseOccurred, NULL); \
    }
#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_FREE(iotInfo)   \
    {                                                     \
        pthread_mutex_destroy(&iotInfo->responseMutex);   \
        pthread_cond_destroy(&iotInfo->responseOccurred); \
    }
#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_LOCK(iotInfo) \
    {                                                   \
        pthread_mutex_lock(&iotInfo->responseMutex);    \
    }
#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_UNLOCK(iotInfo) \
    {                                                     \
        pthread_mutex_unlock(&iotInfo->responseMutex);    \
    }
#define QLIB_SAMPLE_SERVER_RESPONSE_READY_SIGNAL(iotInfo) \
    {                                                     \
        pthread_cond_signal(&iotInfo->responseOccurred);  \
    }

// time for supervising and stats
#define QLIB_SAMPLE_SERVER_IOT_SET_RX_TIME(iotInfo)      \
    {                                                    \
        clock_gettime(CLOCK_REALTIME, &(iotInfo)->rxTs); \
    }
#define QLIB_SAMPLE_SERVER_IOT_SET_TX_TIME(iotInfo)      \
    {                                                    \
        clock_gettime(CLOCK_REALTIME, &(iotInfo)->txTs); \
    }

#elif defined(WIN32)

#define QLIB_SAMPLE_THREAD HANDLE
#define QLIB_SAMPLE_MUTEX  U32
#define QLIB_SERVER_SAMPLE_WAIT_FOR_RESPONSE(iotInfo) \
    {                                                 \
        while ((iotInfo)->waitingForIot == TRUE)      \
        {                                             \
        }                                             \
    }

#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_INIT(iotInfo)
#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_FREE(iotInfo)
#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_LOCK(iotInfo)
#define QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_UNLOCK(iotInfo)
#define QLIB_SAMPLE_SERVER_RESPONSE_READY_SIGNAL(iotInfo)

// time for supervising and stats
#define QLIB_SAMPLE_SERVER_IOT_SET_RX_TIME(iotInfo)
#define QLIB_SAMPLE_SERVER_IOT_SET_TX_TIME(iotInfo)

#else
#WARNING add definitions for your platform
#endif

#define IOT_CONN_STATE_TO_STR(state) \
    state == QLIB_SERVER_IOT_CONNECTED ? "connected" : state == QLIB_SERVER_IOT_REGISTERED ? "registered" : "unknown state"
typedef enum
{

    QLIB_SERVER_IOT_CONNECTED, // in this state we wait for REGISTER ( Discovery ) frame from IOT
    QLIB_SERVER_IOT_REGISTERED

} QLIB_SERVER_IOT_CONNECTION_STATE_E;

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


typedef struct
{
    // iot properties
    QLIB_ID_T          id;
    U32                qlibVer;
    QLIB_SAMPLE_SOCKET clientSocket;
    char               iotName[QLIB_SAMPLE_IOT_NAME_SIZE];

    // standard key for usage with sections
    KEY_T key;

    // request and response from IOT mechanism
    QLIB_SAMPLE_THREAD rxThread;
    QLIB_SAMPLE_MUTEX  responseMutex;
    U32                lastRx;
    BOOL               waitingForIot;
    QLIB_STATUS_T      tmStatus;
    QLIB_REG_SSR_T     ssr;
    U8                 readData[QLIB_SERVER_SAMPLE_CLIENT_DATA];
    U32                readDataSize;

    // server related info
    U32                                serverIndex;
    void*                              serverPtr;
    QLIB_SERVER_IOT_CONNECTION_STATE_E state;
    U32                                mustDie; // once error on sending detected, this field will be marked and wait for kill

#if defined(__linux__)

    struct sockaddr address;
    socklen_t       addr_len;

    struct timespec rxTs;
    struct timespec txTs;

    pthread_cond_t responseOccurred;
#elif defined(WIN32)

    DWORD rxThreadId;

#else
#error NOT SUPPORTED PLATFORM! STOP
#endif

} QLIB_SAMPLE_IOT_INFO_T;

#define QLIB_SAMPLE_SERVER_IOTS_SUPPORTED 100

typedef struct
{
    QLIB_CONTEXT_T* IoTs[QLIB_SAMPLE_SERVER_IOTS_SUPPORTED];
    char            port[20];


} QLIB_SAMPLE_SERVER_T;

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           FUNCTIONS                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine implements reception of non-tm frames.
 *
 * @param[in]       iotInfo             iot information
 * @param[in]       qlibContext         qlib context associated with a iot
 * @param[in]       message             arriving frame
 * @param[in]       len                 length of arriving frame
 * @param[in]       type                type of arriving frame
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_SERVER_RxManagement(QLIB_SAMPLE_IOT_INFO_T* iotInfo,
                                              QLIB_CONTEXT_T*         qlibContext,
                                              char*                   message,
                                              U32                     len,
                                              U16                     type);

/************************************************************************************************************
 * @brief       This routine inits the server structure.
 *
 * @param[in]       server              server structure
 *
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_SERVER_Init(QLIB_SAMPLE_SERVER_T* server);

/************************************************************************************************************
 * @brief       This routine implements the reception of arriving frames in server
 *
 * @param[in]       qlibContext         qlib context of the iot that receives frame
 * @param[in]       message             arriving frame
 * @param[in]       len                 length of arriving frame
 *
************************************************************************************************************/
void QLIB_SAMPLE_SERVER_Rx(QLIB_CONTEXT_T* qlibContext, char* message, U32 len);

/************************************************************************************************************
 * @brief       This routine sends data to iot.
 *
 * @param[in]       iotInfo             iot information
 * @param[in]       data                data buffer for to be sent
 * @param[in]       len                 length of data buffer in bytes
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SERVER_SAMPLE_sendData(QLIB_SAMPLE_IOT_INFO_T* iotInfo, char* data, U32 len);

/************************************************************************************************************
 * @brief       This routine finds free index in server data structure for just connected iot
 *
 * @param[in]       server              server structure
 *
 * @return      index in array, QLIB_SAMPLE_SERVER_IOTS_SUPPORTED if full already
************************************************************************************************************/
U32 QLIB_SAMPLE_SERVER_getIndex(QLIB_SAMPLE_SERVER_T* qlib_server);

/************************************************************************************************************
 * @brief       This routine inits the iot structure.
 *
 * @param[in]       info                iot information
 * @param[in]       iotIndex            index in server array
 * @param[in]       server              pointer to server structure
 * @param[in]       qlib                qlib context associated with iot
 *
************************************************************************************************************/
void QLIB_SAMPLE_SERVER_IotInit(QLIB_SAMPLE_IOT_INFO_T* info, U32 iotIndex, void* server, QLIB_CONTEXT_T* qlib);

#endif // _QLIB_SAMPLE_SERVER__H_
