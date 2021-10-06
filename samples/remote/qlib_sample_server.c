/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2021 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_server.c
* @brief      server sample code
*
* @example    qlib_sample_server.c
*
* @page       This sample code shows how server application over w77q can be implemented
*
* @include    samples/remote/qlib_sample_server.c
*
************************************************************************************************************/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INCLUDES                                                      */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include <stdio.h>
#include "qlib_sample_server.h"

#ifndef SERVER_DBG
#define SERVER_DBG(server, format, ...) printf(format, ##__VA_ARGS__)
#endif

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                       TM LAYER (qlib_tm.h) SERVER IMPLEMENTATION                                        */
/*                                                                                                         */
/* Implementation consists of two parts:                                                                   */
/* 1) Sending. Implementing qlib_tm.h over network aka sending the frames, waiting for response,           */
/*    returning data back to user.                                                                         */
/* 2) Receiving. Implementing mechanism to receive the iot's frame, identify that this is the response to  */
/*     previously issued request and stop waiting thread from waiting for response further                 */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------------------------------------*/
/*                       Part 1 - SENDING REQUEST TO IOT                                                   */
/*---------------------------------------------------------------------------------------------------------*/

// This function is called by QlibInitLib function per every connected IoT and it initializes
// per-IoT transaction layer of the server. For example might do here memory allocations, send welcome
// frame etc
QLIB_STATUS_T QLIB_TM_Init(QLIB_CONTEXT_T* qlibContext)
{
    char                    sendbuf[sizeof(QLIB_SAMPLE_HDR_NET_T)];
    QLIB_SAMPLE_HDR_NET_T   header;
    QLIB_SAMPLE_IOT_INFO_T* iotInfo = (QLIB_SAMPLE_IOT_INFO_T*)QLIB_GetUserData(qlibContext);
    U32                     i       = 0;

    // When IoT gets this frame it must reply with a SYNCH OBJ which is processed by RX thread
    // and if server and client are synchronized - iot will go to REGISTERED state.
    header.messageType = QLIB_SAMPLE_NET_TYPE_IOT_DISCOVERY;
    header.messageLen  = 0;
    memcpy(sendbuf, &header, sizeof(QLIB_SAMPLE_HDR_NET_T));
    QLIB_STATUS_RET_CHECK(QLIB_SERVER_SAMPLE_sendData(iotInfo, sendbuf, sizeof(QLIB_SAMPLE_HDR_NET_T)));

    QLIB_SAMPLE_DELAY_SEC(3);

    // during 3 seconds we want to get response frame and if so, register iot
    QLIB_ASSERT_RET(iotInfo->state == QLIB_SERVER_IOT_REGISTERED, QLIB_STATUS__CONNECTIVITY_ERR);


    return QLIB_STATUS__OK;
}

// Goal of QLIB_TM_Standard is to encode functions's parameters and send them to the IoT.
// Received on IoT frames will parsed back into parameters and QLIB_TM_Standard is executed on spi (qlib_tm.c)
// The status ( and output buffer ) are sent back to server, see corresponding sample qlib_sample_client.c
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
    I8                      sendBuffer[QLIB_SAMPLE_MTU];
    QLIB_SAMPLE_IOT_INFO_T* iotInfo              = (QLIB_SAMPLE_IOT_INFO_T*)QLIB_GetUserData(qlibContext);
    QLIB_SAMPLE_HDR_NET_T*  header               = (QLIB_SAMPLE_HDR_NET_T*)(sendBuffer);
    QLIB_SAMPLE_HDR_STD_T*  stdCmd               = (QLIB_SAMPLE_HDR_STD_T*)(sendBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T));
    U32                     readDataSizeOffset   = 0;
    U32                     writeDataSizeOffset  = 0;
    U32                     readDataSizeCurrent  = 0;
    U32                     writeDataSizeCurrent = 0;

    // one of parameters always 0, single transaction can not read and write
    ASSERT(!(writeDataSize && readDataSize));

    if (iotInfo->waitingForIot == TRUE)
    {
        SERVER_DBG(IOT_2_SERVER(iotInfo), "QLIB_TM_Standard: Tm is waiting for previous transaction\n");
        return QLIB_STATUS__DEVICE_BUSY;
    }

    // Locking sending code to prevent additional sending from possibly additional app thread
    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_LOCK(iotInfo);

    while (1)
    {
        iotInfo->waitingForIot = TRUE;

        readDataSizeCurrent  = MIN(readDataSize - readDataSizeOffset, QLIB_SAMPLE_MAX_DATA_SIZE);
        writeDataSizeCurrent = MIN(writeDataSize - writeDataSizeOffset, QLIB_SAMPLE_MAX_DATA_SIZE);

        memset(&iotInfo->ssr, 0, sizeof(QLIB_REG_SSR_T));

        // TM is available - we serialize parameters to sendBuffer
        header->messageType = QLIB_SAMPLE_NET_TYPE_TM_STD;
        header->messageLen  = (U16)(sizeof(QLIB_SAMPLE_HDR_STD_T) + writeDataSizeCurrent);

        stdCmd->format          = QLIB_BUS_FORMAT_GET_MODE(busFormat);
        stdCmd->dtr             = QLIB_BUS_FORMAT_GET_DTR(busFormat);
        stdCmd->needWriteEnable = needWriteEnable;
        stdCmd->waitWhileBusy   = waitWhileBusy;
        stdCmd->cmd             = cmd;
        stdCmd->address         = (address == NULL) ? MAX_U32 : (*address + writeDataSizeOffset + readDataSizeOffset);
        stdCmd->dummyCycles     = dummyCycles;
        stdCmd->writeDataSize   = writeDataSizeCurrent;
        stdCmd->readDataSize    = readDataSizeCurrent;
        stdCmd->ssrValue        = (U32)(ssr == NULL ? 0 : ssr);

        memcpy(sendBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_SAMPLE_HDR_STD_T),
               writeData + writeDataSizeOffset,
               writeDataSizeCurrent);


        // Function to send data
        QLIB_SERVER_SAMPLE_sendData(iotInfo,
                                    sendBuffer,
                                    sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_SAMPLE_HDR_STD_T) + writeDataSizeCurrent);

        // Os - specific mechanism to wait for response, triggered by QLIB_SAMPLE_SERVER_RESPONSE_READY_SIGNAL call when response received
        QLIB_SERVER_SAMPLE_WAIT_FOR_RESPONSE(iotInfo);

        // In case we got timeout on waiting, we return QLIB_STATUS__COMMUNICATION_ERR
        if (iotInfo->waitingForIot == TRUE)
        {
            // timeout happened, do not wait iot anymore
            SERVER_DBG(IOT_2_SERVER(iotInfo), "Timeout on iot request detected, tired to wait\n");
            iotInfo->waitingForIot = FALSE;
            iotInfo->tmStatus      = QLIB_STATUS__COMMUNICATION_ERR;
            break;
        }

        SERVER_DBG(IOT_2_SERVER(iotInfo),
                   "got %d bytes response to std cmd=0x%x, status=%s, ssr=0x%x\n",
                   iotInfo->readDataSize,
                   cmd,
                   STATUS_TO_STR(iotInfo->tmStatus),
                   ssr == NULL ? 0xffffffff : ssr->asUint);

        // If transaction was successful we update local variables with data received
        if (QLIB_STATUS__OK == iotInfo->tmStatus)
        {
            if (NULL != ssr)
            {
                memcpy(ssr, &iotInfo->ssr, sizeof(QLIB_REG_SSR_T));
            }
            if (NULL != readData)
            {
                memcpy(readData + readDataSizeOffset, iotInfo->readData, iotInfo->readDataSize);
            }
        }
        else
        {
            break;
        }

        readDataSizeOffset += readDataSizeCurrent;
        writeDataSizeOffset += writeDataSizeCurrent;

        if (readDataSizeOffset == readDataSize && writeDataSizeOffset == writeDataSize)
        {
            break; // that transaction is final
        }
    }

    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_UNLOCK(iotInfo);

    // Note TM_standard returns status of network transaction, NOT ssr of w77q.
    // When you have wrong key, for instance, transaction will succeed but
    // ssr will get QLIB_STATUS__DEVICE_AUTHENTICATION_ERR
    return iotInfo->tmStatus;
}

// Goal of QLIB_TM_Secure is to encode functions's parameters and send them to the IoT.
// Received on IoT frames will parsed back into parameters and QLIB_TM_Secure is executed on spi (qlib_tm.c)
// The status ( and output buffer ) are sent back to server, see corresponding sample qlib_sample_client.c
QLIB_STATUS_T QLIB_TM_Secure(QLIB_CONTEXT_T* qlibContext,
                             U32             ctag,
                             const U32*      writeData,
                             U32             writeDataSize,
                             U32*            readData,
                             U32             readDataSize,
                             QLIB_REG_SSR_T* ssr)
{
    I8                      sendBuffer[QLIB_SAMPLE_MTU];
    QLIB_SAMPLE_IOT_INFO_T* iotInfo = (QLIB_SAMPLE_IOT_INFO_T*)QLIB_GetUserData(qlibContext);
    QLIB_SAMPLE_HDR_NET_T*  header  = (QLIB_SAMPLE_HDR_NET_T*)sendBuffer;
    QLIB_SAMPLE_HDR_SEC_T*  secCmd  = (QLIB_SAMPLE_HDR_SEC_T*)(sendBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T));

    ASSERT(sizeof(QLIB_SAMPLE_HDR_SEC_T) + writeDataSize <= QLIB_SAMPLE_MTU);

    if (iotInfo->waitingForIot == TRUE)
    {
        SERVER_DBG(IOT_2_SERVER(iotInfo), "QLIB_TM_Secure: Tm is waiting for previous transaction\n");
        return QLIB_STATUS__DEVICE_BUSY;
    }

    // TM is available - we serialize parameters to sendBuffer
    header->messageType = QLIB_SAMPLE_NET_TYPE_TM_SEC;
    header->messageLen  = (U16)(sizeof(QLIB_SAMPLE_HDR_SEC_T) + writeDataSize);

    secCmd->ctag          = ctag;
    secCmd->readDataSize  = readDataSize;
    secCmd->writeDataSize = writeDataSize;
    secCmd->ssrValue      = (U32)(ssr == NULL ? 0 : ssr);

    memcpy(sendBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_SAMPLE_HDR_SEC_T), writeData, writeDataSize);
    // Locking sending code to prevent additional sending from possibly additional app thread
    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_LOCK(iotInfo);
    iotInfo->waitingForIot = TRUE;

    // Function to send data
    QLIB_SERVER_SAMPLE_sendData(iotInfo,
                                sendBuffer,
                                sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_SAMPLE_HDR_SEC_T) + writeDataSize);

    // Os - specific mechanism to wait for response, triggered by QLIB_SAMPLE_SERVER_RESPONSE_READY_SIGNAL call when response received
    QLIB_SERVER_SAMPLE_WAIT_FOR_RESPONSE(iotInfo);

    // In case we got timeout on waiting, we return QLIB_STATUS__COMMUNICATION_ERR
    if (iotInfo->waitingForIot == TRUE)
    {
        // timeout happened, do not wait iot anymore
        SERVER_DBG(IOT_2_SERVER(iotInfo), "Timeout on iot request detected, tired to wait\n");
        iotInfo->waitingForIot = FALSE;
        iotInfo->tmStatus      = QLIB_STATUS__COMMUNICATION_ERR;
    }

    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_UNLOCK(iotInfo);

    // If transaction was successful we update local variables with data received
    if (QLIB_STATUS__OK == iotInfo->tmStatus)
    {
        if (NULL != ssr)
        {
            memcpy(ssr, &iotInfo->ssr, sizeof(QLIB_REG_SSR_T));
        }
        if (NULL != readData)
        {
            memcpy(readData, iotInfo->readData, iotInfo->readDataSize);
        }
    }

    SERVER_DBG(IOT_2_SERVER(iotInfo),
               "got %d bytes response to sec ctag=0x%x, status=%s, ssr=0x%x\n",
               iotInfo->readDataSize,
               ctag,
               STATUS_TO_STR(iotInfo->tmStatus),
               ssr == NULL ? 0xffffffff : ssr->asUint);

    // Note TM_secure returns status of network transaction, NOT ssr of w77q.
    // When you have wrong key, for instance, transaction will succeed but
    // ssr will get QLIB_STATUS__DEVICE_AUTHENTICATION_ERR
    return iotInfo->tmStatus;
}

// Goal of QLIB_TM_Connect is to encode functions's parameters and send them to the IoT.
// Received on IoT frames will parsed back into parameters and QLIB_TM_Connect is executed locally (qlib_tm.c)
// The status is sent back to server, see corresponding sample qlib_sample_client.c
QLIB_STATUS_T QLIB_TM_Connect(QLIB_CONTEXT_T* qlibContext)
{
    I8                      sendBuffer[QLIB_SAMPLE_MTU];
    QLIB_SAMPLE_IOT_INFO_T* iotInfo = (QLIB_SAMPLE_IOT_INFO_T*)QLIB_GetUserData(qlibContext);
    QLIB_SAMPLE_HDR_NET_T*  header  = (QLIB_SAMPLE_HDR_NET_T*)sendBuffer;

    if (iotInfo->waitingForIot == TRUE)
    {
        SERVER_DBG(IOT_2_SERVER(iotInfo), "Tm is waiting for previous transaction\n");
        return QLIB_STATUS__DEVICE_BUSY;
    }

    // TM is available - we serialize parameters to sendBuffer
    header->messageType = QLIB_SAMPLE_NET_TYPE_TM_CONNECT;
    header->messageLen  = 0;
    // Locking sending code to prevent additional sending from possibly additional app thread
    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_LOCK(iotInfo);
    iotInfo->waitingForIot = TRUE;

    // Function to send data
    QLIB_SERVER_SAMPLE_sendData(iotInfo, (char*)sendBuffer, sizeof(QLIB_SAMPLE_HDR_NET_T));

    // Os - specific mechanism to wait for response,
    // triggered by QLIB_SAMPLE_SERVER_RESPONSE_READY_SIGNAL call when response received
    QLIB_SERVER_SAMPLE_WAIT_FOR_RESPONSE(iotInfo);

    // In case we got timeout on waiting, we return QLIB_STATUS__COMMUNICATION_ERR
    if (iotInfo->waitingForIot == TRUE)
    {
        SERVER_DBG(IOT_2_SERVER(iotInfo), "Timeout on iot request detected, tired to wait\n");
        iotInfo->waitingForIot = FALSE;
        iotInfo->tmStatus      = QLIB_STATUS__COMMUNICATION_ERR;
    }

    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_UNLOCK(iotInfo);

    SERVER_DBG(IOT_2_SERVER(iotInfo), "got response to connect status=%s\n", STATUS_TO_STR(iotInfo->tmStatus));
    return iotInfo->tmStatus;
}

// Goal of QLIB_TM_Disconnect is to encode functions's parameters and send them to the IoT.
// Received on IoT frames will parsed back into parameters and QLIB_TM_Disconnect is executed locally (qlib_tm.c)
// The status is sent back to server, see corresponding sample qlib_sample_client.c
QLIB_STATUS_T QLIB_TM_Disconnect(QLIB_CONTEXT_T* qlibContext)
{
    I8                      sendBuffer[QLIB_SAMPLE_MTU];
    QLIB_SAMPLE_HDR_NET_T*  header  = (QLIB_SAMPLE_HDR_NET_T*)sendBuffer;
    QLIB_SAMPLE_IOT_INFO_T* iotInfo = (QLIB_SAMPLE_IOT_INFO_T*)QLIB_GetUserData((QLIB_CONTEXT_T*)qlibContext);

    if (iotInfo->waitingForIot == TRUE)
    {
        SERVER_DBG(IOT_2_SERVER(iotInfo), "Tm is waiting for previous transaction\n");
        return QLIB_STATUS__DEVICE_BUSY;
    }

    // TM is available - we serialize parameters to sendBuffer
    header->messageType = QLIB_SAMPLE_NET_TYPE_TM_DISCONNECT;
    header->messageLen  = 0;


    // Locking sending code to prevent additional sending from possibly additional app thread
    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_LOCK(iotInfo);
    iotInfo->waitingForIot = TRUE;

    // Function to send data
    QLIB_SERVER_SAMPLE_sendData(iotInfo, sendBuffer, sizeof(QLIB_SAMPLE_HDR_NET_T));

    // Os - specific mechanism to wait for response, triggered by QLIB_SAMPLE_SERVER_RESPONSE_READY_SIGNAL call when response received
    QLIB_SERVER_SAMPLE_WAIT_FOR_RESPONSE(iotInfo);

    // In case we got timeout on waiting, we return QLIB_STATUS__COMMUNICATION_ERR
    if (iotInfo->waitingForIot == TRUE)
    {
        SERVER_DBG(IOT_2_SERVER(iotInfo), "Timeout on iot request detected, tired to wait\n");
        iotInfo->waitingForIot = FALSE;
        iotInfo->tmStatus      = QLIB_STATUS__COMMUNICATION_ERR;
    }

    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_UNLOCK(iotInfo);

    SERVER_DBG(IOT_2_SERVER(iotInfo), "got response to disconnect status=%s\n", STATUS_TO_STR(iotInfo->tmStatus));
    return iotInfo->tmStatus;
}

/*---------------------------------------------------------------------------------------------------------*/
/*                       Part 2 - RECEIVING THE RESPONSE FROM IOT                                          */
/*---------------------------------------------------------------------------------------------------------*/

// this is for rx that come as a response to a tm api
void QLIB_SAMPLE_SERVER_RxTmResponse(QLIB_SAMPLE_IOT_INFO_T* iotInfo, char* message, U32 len, U16 type)
{
    switch (type)
    {
        case QLIB_SAMPLE_NET_TYPE_TM_CONNECT:    // response to the QLIB_TM_Connect
        case QLIB_SAMPLE_NET_TYPE_TM_DISCONNECT: // response to the QLIB_TM_Disconnect

            // Client's format of such response frame
            // ----------------------------------
            //| TYPE | LEN | QLIB_STATUS_T value |
            // ----------------------------------
            memcpy(&iotInfo->tmStatus, message, sizeof(QLIB_STATUS_T));
            SERVER_DBG(IOT_2_SERVER(iotInfo),
                       "Got %s response from %s with status 0x%x\n",
                       type == QLIB_SAMPLE_NET_TYPE_TM_CONNECT ? "connect" : "disconnect",
                       iotInfo->iotName,
                       iotInfo->tmStatus);
            break;

        case QLIB_SAMPLE_NET_TYPE_TM_STD: // response to QLIB_TM_Standard
        case QLIB_SAMPLE_NET_TYPE_TM_SEC: // response to QLIB_TM_Secure

            // Client's format of such response frame
            // ---------------------------------------------------------------------------
            //| TYPE | LEN | QLIB_STATUS_T value | QLIB_REG_SSR_T value | readData buffer |
            // ---------------------------------------------------------------------------
            memcpy(&iotInfo->tmStatus, message, sizeof(QLIB_STATUS_T));
            message = message + sizeof(QLIB_STATUS_T);
            len     = len - sizeof(QLIB_STATUS_T);

            memcpy(&iotInfo->ssr, message, sizeof(QLIB_REG_SSR_T));
            message = message + sizeof(QLIB_REG_SSR_T);
            len     = len - sizeof(QLIB_REG_SSR_T);

            iotInfo->readDataSize = len;
            memcpy(iotInfo->readData, message, len);

            break;

        default:

            ASSERT(0); //should NEVER happen, since only  known type is handled here
            break;
    }
}

void QLIB_SAMPLE_SERVER_Rx(QLIB_CONTEXT_T* qlibContext, char* message, U32 len)
{
    QLIB_SAMPLE_HDR_NET_T*  header  = (QLIB_SAMPLE_HDR_NET_T*)message;
    QLIB_SAMPLE_IOT_INFO_T* iotInfo = (QLIB_SAMPLE_IOT_INFO_T*)QLIB_GetUserData(qlibContext);
    QLIB_STATUS_T           ret;


    QLIB_SAMPLE_SERVER_IOT_SET_RX_TIME(iotInfo);

    //  Network header
    // ----------------
    //| TYPE | LEN | .....<buffer of LEN size>
    // ----------------
    //   16b   16b
    message = message + sizeof(QLIB_SAMPLE_HDR_NET_T);
    len     = len - sizeof(QLIB_SAMPLE_HDR_NET_T);

    iotInfo->lastRx = header->messageType;

    switch (iotInfo->lastRx)
    {
        case QLIB_SAMPLE_NET_TYPE_TM_CONNECT:    // this is response to the TM_connect
        case QLIB_SAMPLE_NET_TYPE_TM_DISCONNECT: // This is response to the Tm_Disconnect
        case QLIB_SAMPLE_NET_TYPE_TM_STD:        // this response to TM_Standard
        case QLIB_SAMPLE_NET_TYPE_TM_SEC:        //this is response to TM_Secure

            QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_LOCK(iotInfo);

            if (iotInfo->waitingForIot != TRUE)
            {
                SERVER_DBG(IOT_2_SERVER(iotInfo), "response comes too long and server ceased to wait for client\n");
            }
            else
            {
                QLIB_SAMPLE_SERVER_RxTmResponse(iotInfo, message, len, header->messageType);

                iotInfo->waitingForIot = FALSE;
                QLIB_SAMPLE_SERVER_RESPONSE_READY_SIGNAL(iotInfo);
            }

            QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_UNLOCK(iotInfo);
            break;

        default:
            ret = QLIB_SAMPLE_SERVER_RxManagement(iotInfo, qlibContext, message, len, header->messageType);
            if (ret != QLIB_STATUS__OK)
            {
                SERVER_DBG(IOT_2_SERVER(iotInfo), "management frame of type %d finished with status %d\n", iotInfo->lastRx, ret);
            }
    }
}

QLIB_STATUS_T QLIB_SERVER_SAMPLE_sendData(QLIB_SAMPLE_IOT_INFO_T* iotInfo, char* data, U32 len)
{
    if (0 != QLIB_SAMPLE_NET_socketSend(data, len, iotInfo->clientSocket))
    {
        iotInfo->mustDie = 1;
        SERVER_DBG(IOT_2_SERVER(iotInfo), "Failed to send %d bytes to %s. Mark mustDie\n", len, iotInfo->iotName);
        return QLIB_STATUS__COMMAND_FAIL; // eventually status not needed ?
    }

    QLIB_SAMPLE_SERVER_IOT_SET_TX_TIME(iotInfo);


    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_SERVER_Init(QLIB_SAMPLE_SERVER_T* server)
{
    for (int i = 0; i < QLIB_SAMPLE_SERVER_IOTS_SUPPORTED; i++)
    {
        server->IoTs[i] = NULL;
    }
    return QLIB_STATUS__OK;
}

U32 QLIB_SAMPLE_SERVER_getIndex(QLIB_SAMPLE_SERVER_T* qlib_server)
{
    for (int i = 0; i < QLIB_SAMPLE_SERVER_IOTS_SUPPORTED; i++)
    {
        if (qlib_server->IoTs[i] == NULL)
        {
            return i;
        }
    }
    return QLIB_SAMPLE_SERVER_IOTS_SUPPORTED;
}

void QLIB_SAMPLE_SERVER_IotInit(QLIB_SAMPLE_IOT_INFO_T* info, U32 iotIndex, void* server, QLIB_CONTEXT_T* qlib)
{
    U32 i = 0;

    memset(info, 0, sizeof(QLIB_SAMPLE_IOT_INFO_T));

    // load demo keys for all sections
    memset(info->key, 0x55, sizeof(info->key));
    for (i = 0; i < QLIB_NUM_OF_SECTIONS; ++i)
    {
        (void)QLIB_LoadKey(qlib, i, info->key, TRUE);
    }

    info->serverIndex = iotIndex;
    info->serverPtr   = server;
    info->mustDie     = 0;

    QLIB_SAMPLE_SERVER_RESPONSE_MUTEX_INIT(info);

    // setting dummy value so that guarding threads will no throw fresh iot away because of lack of traffic
    QLIB_SAMPLE_SERVER_IOT_SET_RX_TIME(info);
    QLIB_SAMPLE_SERVER_IOT_SET_TX_TIME(info);

}
