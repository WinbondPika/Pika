/************************************************************************************************************
* @internal
* @remark     Winbond Electronics Corporation - Confidential
* @copyright  Copyright (c) 2021 by Winbond Electronics Corporation . All rights reserved
* @endinternal
*
* @file       qlib_sample_client.h
* @brief      client sample code
*
* @example    qlib_sample_client.h
*
* @page       This sample code shows client application over w77q sample definitions
*
* @include    samples/remote/qlib_sample_client.h
*
************************************************************************************************************/

#ifndef _QLIB_SAMPLE_CLIENT__H_
#define _QLIB_SAMPLE_CLIENT__H_

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

typedef enum
{
    QLIB_CLIENT_STATE_LOOKING_FOR_SERVER, // in this state we try to open socket to qserver.tech
    QLIB_CLIENT_STATE_SERVER_FOUND,       // in this state we send registration/discovery frame to the server


} QLIB_CLIENT_STATE_E;

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           TYPES                                                         */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/


typedef struct QLIB_SAMPLE_CLIENT_INFO_T
{
    U32                 replyBuffer[QLIB_SAMPLE_MTU_INTS];
    U32                 rxBuffer[QLIB_SAMPLE_MTU / sizeof(U32)];
    QLIB_CONTEXT_T*     qlib;
    QLIB_SAMPLE_SOCKET  sock;
    char*               port;
    QLIB_CLIENT_STATE_E clientState;
    BOOL                connectedToServer;
    char                iotName[QLIB_SAMPLE_IOT_NAME_SIZE];
    char                serverDomainName[QLIB_SAMPLE_IOT_NAME_SIZE];

} QLIB_SAMPLE_CLIENT_INFO_T;

/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/
/*                                           FUNCTIONS                                                     */
/*---------------------------------------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------------*/

/************************************************************************************************************
 * @brief       This routine implements reception of management frames. In other words here handling of non-tm
 *              frames is done
 *
 * @param[in]       message             frame arrived
 * @param[in]       len                 length of frame
 * @param[in]       client              client information object
 * @param[out]      replySize           reply size if the answer to arrived frame
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_CLIENT_Rx_Management(U8*                        message,
                                               U32                        len,
                                               U16                        messageType,
                                               QLIB_SAMPLE_CLIENT_INFO_T* client,
                                               U32*                       replySize);

/************************************************************************************************************
 * @brief       This routine sends data to server.
 *
 * @param[in]       data                data buffer for to be send
 * @param[in]       len                 length of data buffer in bytes
 * @param[in]       client              client information
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_CLIENT_tx(const U8* data, U32 len, QLIB_SAMPLE_CLIENT_INFO_T* client);

/************************************************************************************************************
 * @brief       This routine implements arriving frames handling
 *
 * @param[in]       message             arriving frame
 * @param[in]       len                 length of frame
 * @param[in]       client              client information
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_CLIENT_Rx(U8* message, U32 len, QLIB_SAMPLE_CLIENT_INFO_T* client);

/************************************************************************************************************
 * @brief       This routine inits the client structure.
 *
 * @param[in]       info                client information
 * @param[in]       qlib                qlib context of the client
 * @param[in]       name                name of the client
 * @param[in]       host                hostname of the server
 * @param[in]       port                port to connect to
 * @param[in]       initialState        state to begin with
 *
 * @return      0 if no error occurred, QLIB_STATUS__(ERROR) otherwise
************************************************************************************************************/
QLIB_STATUS_T QLIB_SAMPLE_CLIENT_Init(QLIB_SAMPLE_CLIENT_INFO_T* info,
                                      QLIB_CONTEXT_T*            qlib,
                                      char*                      name,
                                      char*                      host,
                                      char*                      port,
                                      QLIB_CLIENT_STATE_E        initialState
);

/************************************************************************************************************
 * @brief       This routine frees the client structure.
 *
 * @param[in]       info                client information
 *
************************************************************************************************************/
void QLIB_SAMPLE_CLIENT_Finit(QLIB_SAMPLE_CLIENT_INFO_T* info);

#endif // _QLIB_SAMPLE_CLIENT__H_
