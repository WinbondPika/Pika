/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2021 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_client.c
* @brief      client sample code
*
* @example    qlib_sample_client.c
*
* @page       This sample code shows how client application over w77q can be implemented
*
* @include    samples/remote/qlib_sample_client.c
*
************************************************************************************************************/

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           INCLUDES                                                      */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
#include "qlib_sample_client.h"

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           FUNCTIONS                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

QLIB_STATUS_T QLIB_SAMPLE_CLIENT_ExecuteTm(U8*                        message,
                                           U32                        len,
                                           U16                        messageType,
                                           QLIB_SAMPLE_CLIENT_INFO_T* client,
                                           U32*                       replySize)
{
    QLIB_STATUS_T          status;
    U32                    address = 0;
    QLIB_SAMPLE_HDR_STD_T* stdCmd;
    QLIB_SAMPLE_HDR_SEC_T* secCmd;
    U8*                    replyBuffer = (U8*)client->replyBuffer;

    // We begin from empty frame, each time we add data to reply buffer we sum it to replySize.
    *replySize = 0;

    //| TYPE | LEN | .....<buffer of LEN size>
    switch (messageType)
    {
        case QLIB_SAMPLE_NET_TYPE_TM_SEC:

            // Request   ===> | TYPE | LEN | QLIB_SAMPLE_HDR_SEC_T value | writeData buffer |
            // Response  <=== | TYPE | LEN | QLIB_STATUS_T value | QLIB_REG_SSR_T value | readData buffer |

            secCmd = (QLIB_SAMPLE_HDR_SEC_T*)message;

            ASSERT(QLIB_SAMPLE_MTU >=
                   (secCmd->readDataSize + sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_STATUS_T) + sizeof(QLIB_REG_SSR_T)));

            status =
                QLIB_TM_Secure(client->qlib,
                               secCmd->ctag,
                               // such casting is ok since "message" is aligned and QLIB_SAMPLE_HDR_SEC_T is packed with U32
                               secCmd->writeDataSize > 0 ? (U32*)(message + sizeof(QLIB_SAMPLE_HDR_SEC_T)) : NULL,
                               secCmd->writeDataSize,
                               secCmd->readDataSize > 0 ? (U32*)(replyBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T) +
                                                                 sizeof(QLIB_STATUS_T) + sizeof(QLIB_REG_SSR_T))
                                                        : NULL,
                               secCmd->readDataSize,
                               (QLIB_REG_SSR_T*)(secCmd->ssrValue == 0
                                                     ? NULL
                                                     : (replyBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_STATUS_T))));

            *replySize += sizeof(QLIB_REG_SSR_T) + secCmd->readDataSize;
            break;

        case QLIB_SAMPLE_NET_TYPE_TM_STD:

            stdCmd = (QLIB_SAMPLE_HDR_STD_T*)message;
            // Request  ===> | TYPE | LEN | QLIB_SAMPLE_HDR_STD_T value | writeData buffer |
            // Response <=== | TYPE | LEN | QLIB_STATUS_T value | QLIB_REG_SSR_T value | readData buffer |

            ASSERT(QLIB_SAMPLE_MTU >=
                   (stdCmd->readDataSize + sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_STATUS_T) + sizeof(QLIB_REG_SSR_T)));

            memcpy(&address, &stdCmd->address, sizeof(U32));

            status =
                QLIB_TM_Standard(client->qlib,
                                 QLIB_BUS_FORMAT(stdCmd->format, stdCmd->dtr, FALSE),
                                 stdCmd->needWriteEnable,
                                 stdCmd->waitWhileBusy,
                                 stdCmd->cmd & 0xFF,
                                 address == MAX_U32 ? NULL : &address,
                                 stdCmd->writeDataSize > 0 ? (U8*)(message + sizeof(QLIB_SAMPLE_HDR_STD_T)) : NULL,
                                 stdCmd->writeDataSize,
                                 stdCmd->dummyCycles,
                                 stdCmd->readDataSize > 0 ? (replyBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_STATUS_T) +
                                                             sizeof(QLIB_REG_SSR_T))
                                                          : NULL,
                                 stdCmd->readDataSize,
                                 (QLIB_REG_SSR_T*)(stdCmd->ssrValue == 0
                                                       ? NULL
                                                       : (replyBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T) + sizeof(QLIB_STATUS_T))));
            *replySize += sizeof(QLIB_REG_SSR_T) + stdCmd->readDataSize;

            break;

        case QLIB_SAMPLE_NET_TYPE_TM_CONNECT:

            // Request   ===> | TYPE | LEN |
            // Response  <=== | TYPE | LEN | QLIB_STATUS_T value |
            status = QLIB_TM_Connect(client->qlib);
            if (status == QLIB_STATUS__OK)
            {
                client->connectedToServer = TRUE;
            }
            break;

        case QLIB_SAMPLE_NET_TYPE_TM_DISCONNECT:

            // Request   ===> | TYPE | LEN |
            // Response  <=== | TYPE | LEN | QLIB_STATUS_T value |
            status = QLIB_TM_Disconnect(client->qlib);
            if (status == QLIB_STATUS__OK)
            {
                client->connectedToServer = FALSE;
            }
            break;

        default:
            ASSERT(0);
            break;
    }

    //              | QLIB_STATUS_T value |
    memcpy(replyBuffer + sizeof(QLIB_SAMPLE_HDR_NET_T), &status, sizeof(QLIB_STATUS_T));
    *replySize += sizeof(QLIB_STATUS_T);

    // | TYPE | LEN |
    QLIB_SAMPLE_HDR_NET_T* netHdr = (QLIB_SAMPLE_HDR_NET_T*)replyBuffer;
    netHdr->messageType           = messageType;
    netHdr->messageLen            = (U16)*replySize;
    *replySize += sizeof(QLIB_SAMPLE_HDR_NET_T);
    return QLIB_STATUS__OK;
}

// This function will receive frame from server, identify the request, execute
// function over spi layer and send the answer back.
QLIB_STATUS_T QLIB_SAMPLE_CLIENT_Rx(U8* message, U32 len, QLIB_SAMPLE_CLIENT_INFO_T* client)
{
    U32 replySize   = 0;
    U16 messageType = ((QLIB_SAMPLE_HDR_NET_T*)message)->messageType;
    U8* replyBuffer = (U8*)client->replyBuffer;

    //| TYPE | LEN | .....<buffer of LEN size>
    message = message + sizeof(QLIB_SAMPLE_HDR_NET_T);
    len     = len - sizeof(QLIB_SAMPLE_HDR_NET_T);

    switch (messageType)
    {
        case QLIB_SAMPLE_NET_TYPE_TM_SEC:
        case QLIB_SAMPLE_NET_TYPE_TM_STD:
        case QLIB_SAMPLE_NET_TYPE_TM_CONNECT:
        case QLIB_SAMPLE_NET_TYPE_TM_DISCONNECT:

            QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_CLIENT_ExecuteTm(message, len, messageType, client, &replySize));
            break;

        default:
            QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_CLIENT_Rx_Management(message, len, messageType, client, &replySize));
            break;
    }

    // Sending the reply to the server back this received frame requires that
    if (replySize > 0)
    {
        QLIB_STATUS_RET_CHECK(QLIB_SAMPLE_CLIENT_tx(replyBuffer, replySize, client));
    }

    return QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_CLIENT_tx(const U8* data, U32 len, QLIB_SAMPLE_CLIENT_INFO_T* client)
{
    return QLIB_SAMPLE_NET_socketSend((const char*)data, len, client->sock) ? QLIB_STATUS__COMMUNICATION_ERR : QLIB_STATUS__OK;
}

QLIB_STATUS_T QLIB_SAMPLE_CLIENT_Init(QLIB_SAMPLE_CLIENT_INFO_T* info,
                                      QLIB_CONTEXT_T*            qlib,
                                      char*                      name,
                                      char*                      host,
                                      char*                      port,
                                      QLIB_CLIENT_STATE_E        initialState
)
{
    info->qlib              = qlib;
    info->clientState       = initialState;
    info->port              = port;
    info->connectedToServer = FALSE;
    memcpy(info->iotName, name, strlen(name) + 1);
    memcpy(info->serverDomainName, host, strlen(host) + 1);


    return QLIB_STATUS__OK;
}

void QLIB_SAMPLE_CLIENT_Finit(QLIB_SAMPLE_CLIENT_INFO_T* info)
{

    info->clientState = QLIB_CLIENT_STATE_LOOKING_FOR_SERVER;
    if (info->connectedToServer == TRUE)
    {
        QLIB_TM_Disconnect(info->qlib);
        info->connectedToServer = FALSE;
    }
}
