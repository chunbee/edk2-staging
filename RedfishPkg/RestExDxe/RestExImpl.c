/** @file
  RestExDxe support functions implementation.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RestExImpl.h"

/**
  Notify the callback function when an event is triggered.

  @param[in]  Context         The opaque parameter to the function.

**/
VOID
EFIAPI
HttpIoNotifyDpc (
  IN VOID                *Context
  )
{
  *((BOOLEAN *) Context) = TRUE;
}

/**
  Request HttpIoNotifyDpc as a DPC at TPL_CALLBACK.

  @param[in]  Event                 The event signaled.
  @param[in]  Context               The opaque parameter to the function.

**/
VOID
EFIAPI
HttpIoNotify (
  IN EFI_EVENT              Event,
  IN VOID                   *Context
  )
{
  //
  // Request HttpIoNotifyDpc as a DPC at TPL_CALLBACK
  //
  QueueDpc (TPL_CALLBACK, HttpIoNotifyDpc, Context);
}

/**
  Destroy the HTTP_IO and release the resources.

  @param[in]  HttpIo          The HTTP_IO which wraps the HTTP service to be destroyed.

**/
VOID
HttpIoDestroyIo (
  IN HTTP_IO                *HttpIo
  )
{
  EFI_HTTP_PROTOCOL         *Http;
  EFI_EVENT                 Event;

  if (HttpIo == NULL) {
    return;
  }

  Event = HttpIo->ReqToken.Event;
  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Event = HttpIo->RspToken.Event;
  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Event = HttpIo->TimeoutEvent;
  if (Event != NULL) {
    gBS->CloseEvent (Event);
  }

  Http = HttpIo->Http;
  if (Http != NULL) {
    Http->Configure (Http, NULL);
    gBS->CloseProtocol (
           HttpIo->Handle,
           &gEfiHttpProtocolGuid,
           HttpIo->Image,
           HttpIo->Controller
           );
  }

  NetLibDestroyServiceChild (
    HttpIo->Controller,
    HttpIo->Image,
    &gEfiHttpServiceBindingProtocolGuid,
    HttpIo->Handle
    );
}

/**
  Create a HTTP_IO to access the HTTP service. It will create and configure
  a HTTP child handle.

  @param[in]  Image          The handle of the driver image.
  @param[in]  Controller     The handle of the controller.
  @param[in]  IpVersion      IP_VERSION_4 or IP_VERSION_6.
  @param[in]  ConfigData     The HTTP_IO configuration data , NULL means not to configure the HTTP child.
  @param[out] HttpIo         The HTTP_IO.

  @retval EFI_SUCCESS            The HTTP_IO is created and configured.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_UNSUPPORTED        One or more of the control options are not
                                 supported in the implementation.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval Others                 Failed to create the HTTP_IO or configure it.

**/
EFI_STATUS
HttpIoCreateIo (
  IN EFI_HANDLE             Image,
  IN EFI_HANDLE             Controller,
  IN UINT8                  IpVersion,
  IN HTTP_IO_CONFIG_DATA    *ConfigData, OPTIONAL
  OUT HTTP_IO               *HttpIo
  )
{
  EFI_STATUS                Status;
  EFI_HTTP_CONFIG_DATA      HttpConfigData;
  EFI_HTTPv4_ACCESS_POINT   Http4AccessPoint;
  EFI_HTTPv6_ACCESS_POINT   Http6AccessPoint;
  EFI_HTTP_PROTOCOL         *Http;
  EFI_EVENT                 Event;

  if ((Image == NULL) || (Controller == NULL) || (HttpIo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (IpVersion != IP_VERSION_4 && IpVersion != IP_VERSION_6) {
    return EFI_UNSUPPORTED;
  }

  ZeroMem (HttpIo, sizeof (HTTP_IO));
  ZeroMem (&HttpConfigData, sizeof (EFI_HTTP_CONFIG_DATA));

  //
  // Create the HTTP child instance and get the HTTP protocol.
  //
  Status = NetLibCreateServiceChild (
             Controller,
             Image,
             &gEfiHttpServiceBindingProtocolGuid,
             &HttpIo->Handle
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->OpenProtocol (
                  HttpIo->Handle,
                  &gEfiHttpProtocolGuid,
                  (VOID **) &Http,
                  Image,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) || (Http == NULL)) {
    goto ON_ERROR;
  }

  //
  // Init the configuration data and configure the HTTP child.
  //
  HttpIo->Image       = Image;
  HttpIo->Controller  = Controller;
  HttpIo->IpVersion   = IpVersion;
  HttpIo->Http        = Http;

  if (ConfigData != NULL) {
    HttpConfigData.HttpVersion        = ConfigData->Config4.HttpVersion;
    HttpConfigData.TimeOutMillisec    = ConfigData->Config4.RequestTimeOut;
    if (HttpIo->IpVersion == IP_VERSION_4) {
      HttpConfigData.LocalAddressIsIPv6 = FALSE;

      Http4AccessPoint.UseDefaultAddress = ConfigData->Config4.UseDefaultAddress;
      Http4AccessPoint.LocalPort         = ConfigData->Config4.LocalPort;
      IP4_COPY_ADDRESS (&Http4AccessPoint.LocalAddress, &ConfigData->Config4.LocalIp);
      IP4_COPY_ADDRESS (&Http4AccessPoint.LocalSubnet, &ConfigData->Config4.SubnetMask);
      HttpConfigData.AccessPoint.IPv4Node = &Http4AccessPoint;
    } else {
      HttpConfigData.LocalAddressIsIPv6 = TRUE;
      Http6AccessPoint.LocalPort        = ConfigData->Config6.LocalPort;
      IP6_COPY_ADDRESS (&Http6AccessPoint.LocalAddress, &ConfigData->Config6.LocalIp);
      HttpConfigData.AccessPoint.IPv6Node = &Http6AccessPoint;
    }

    Status = Http->Configure (Http, &HttpConfigData);
    if (EFI_ERROR (Status)) {
      goto ON_ERROR;
    }
  }

  //
  // Create events for variuos asynchronous operations.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpIoNotify,
                  &HttpIo->IsTxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  HttpIo->ReqToken.Event = Event;
  HttpIo->ReqToken.Message = &HttpIo->ReqMessage;

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  HttpIoNotify,
                  &HttpIo->IsRxDone,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }
  HttpIo->RspToken.Event = Event;
  HttpIo->RspToken.Message = &HttpIo->RspMessage;

  return EFI_SUCCESS;

ON_ERROR:
  HttpIoDestroyIo (HttpIo);

  return Status;
}

/**
  Synchronously send a HTTP REQUEST message to the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   Request          A pointer to storage such data as URL and HTTP method.
  @param[in]   HeaderCount      Number of HTTP header structures in Headers list.
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[in]   BodyLength       Length in bytes of the HTTP body.
  @param[in]   Body             Body associated with the HTTP request.

  @retval EFI_SUCCESS            The HTTP request is trasmitted.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoSendRequest (
  IN  HTTP_IO                *HttpIo,
  IN  EFI_HTTP_REQUEST_DATA  *Request,
  IN  UINTN                  HeaderCount,
  IN  EFI_HTTP_HEADER        *Headers,
  IN  UINTN                  BodyLength,
  IN  VOID                   *Body
  )
{
  EFI_STATUS                 Status;
  EFI_HTTP_PROTOCOL          *Http;

  if (HttpIo == NULL || HttpIo->Http == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  HttpIo->ReqToken.Status  = EFI_NOT_READY;
  HttpIo->ReqToken.Message->Data.Request = Request;
  HttpIo->ReqToken.Message->HeaderCount  = HeaderCount;
  HttpIo->ReqToken.Message->Headers      = Headers;
  HttpIo->ReqToken.Message->BodyLength   = BodyLength;
  HttpIo->ReqToken.Message->Body         = Body;

  //
  // Queue the request token to HTTP instances.
  //
  Http = HttpIo->Http;
  HttpIo->IsTxDone = FALSE;
  Status = Http->Request (
                   Http,
                   &HttpIo->ReqToken
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Poll the network until transmit finish.
  //
  while (!HttpIo->IsTxDone) {
    Http->Poll (Http);
  }

  return HttpIo->ReqToken.Status;
}

/**
  Synchronously receive a HTTP RESPONSE message from the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   RecvMsgHeader    TRUE to receive a new HTTP response (from message header).
                                FALSE to continue receive the previous response message.
  @param[out]  ResponseData     Point to a wrapper of the received response data.

  @retval EFI_SUCCESS            The HTTP response is received.
  @retval EFI_INVALID_PARAMETER  One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_DEVICE_ERROR       An unexpected network or system error occurred.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoRecvResponse (
  IN      HTTP_IO                  *HttpIo,
  IN      BOOLEAN                  RecvMsgHeader,
     OUT  HTTP_IO_RESPONSE_DATA    *ResponseData
  )
{
  EFI_STATUS                 Status;
  EFI_HTTP_PROTOCOL          *Http;

  if (HttpIo == NULL || HttpIo->Http == NULL || ResponseData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create new TimeoutEvent for response
  //
  if (HttpIo->TimeoutEvent != NULL) {
    gBS->CloseEvent (HttpIo->TimeoutEvent);
    HttpIo->TimeoutEvent = NULL;
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &HttpIo->TimeoutEvent
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Start the timer, and wait Timeout seconds to receive the header packet.
  //
  Status = gBS->SetTimer (HttpIo->TimeoutEvent, TimerRelative, HttpIo->Timeout * TICKS_PER_MS);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Queue the response token to HTTP instances.
  //
  HttpIo->RspToken.Status  = EFI_NOT_READY;
  if (RecvMsgHeader) {
    HttpIo->RspToken.Message->Data.Response = &ResponseData->Response;
  } else {
    HttpIo->RspToken.Message->Data.Response = NULL;
  }
  HttpIo->RspToken.Message->HeaderCount   = 0;
  HttpIo->RspToken.Message->Headers       = NULL;
  HttpIo->RspToken.Message->BodyLength    = ResponseData->BodyLength;
  HttpIo->RspToken.Message->Body          = ResponseData->Body;

  Http = HttpIo->Http;
  HttpIo->IsRxDone = FALSE;
  Status = Http->Response (
                   Http,
                   &HttpIo->RspToken
                   );

  if (EFI_ERROR (Status)) {
    gBS->SetTimer (HttpIo->TimeoutEvent, TimerCancel, 0);
    return Status;
  }

  //
  // Poll the network until receive finish.
  //
  while (!HttpIo->IsRxDone && ((HttpIo->TimeoutEvent == NULL) || EFI_ERROR (gBS->CheckEvent (HttpIo->TimeoutEvent)))) {
    Http->Poll (Http);
  }

  gBS->SetTimer (HttpIo->TimeoutEvent, TimerCancel, 0);

  if (!HttpIo->IsRxDone) {
    //
    // Timeout occurs, cancel the response token.
    //
    Http->Cancel (Http, &HttpIo->RspToken);

    Status = EFI_TIMEOUT;

    return Status;
  } else {
    HttpIo->IsRxDone = FALSE;
  }

  //
  // Store the received data into the wrapper.
  //
  ResponseData->Status = HttpIo->RspToken.Status;
  ResponseData->HeaderCount = HttpIo->RspToken.Message->HeaderCount;
  ResponseData->Headers     = HttpIo->RspToken.Message->Headers;
  ResponseData->BodyLength  = HttpIo->RspToken.Message->BodyLength;

  return Status;
}

/**
  Get the value of the content length if there is a "Content-Length" header.

  @param[in]    HeaderCount        Number of HTTP header structures in Headers.
  @param[in]    Headers            Array containing list of HTTP headers.
  @param[out]   ContentLength      Pointer to save the value of the content length.

  @retval EFI_SUCCESS              Successfully get the content length.
  @retval EFI_NOT_FOUND            No "Content-Length" header in the Headers.

**/
EFI_STATUS
HttpIoGetContentLength (
  IN     UINTN                HeaderCount,
  IN     EFI_HTTP_HEADER      *Headers,
     OUT UINTN                *ContentLength
  )
{
  EFI_HTTP_HEADER       *Header;

  Header = HttpFindHeader (HeaderCount, Headers, HTTP_HEADER_CONTENT_LENGTH);
  if (Header == NULL) {
    return EFI_NOT_FOUND;
  }

  return AsciiStrDecimalToUintnS (Header->FieldValue, (CHAR8 **) NULL, ContentLength);
}

/**
  Synchronously receive a HTTP RESPONSE message from the server.

  @param[in]   HttpIo           The HttpIo wrapping the HTTP service.
  @param[in]   HeaderCount      Number of headers in Headers.
  @param[in]   Headers          Array containing list of HTTP headers.
  @param[out]  ChunkListHead    A pointer to receivce list head of chunked data.
                                Caller has to release memory of ChunkListHead
                                and all list entries.
  @param[out]  ContentLength    Total content length

  @retval EFI_SUCCESS            The HTTP chunked transfer is received.
  @retval EFI_NOT_FOUND          No chunked transfer coding header found.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
  @retval EFI_INVALID_PARAMETER  Improper parameters.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
HttpIoGetChunkedTransferContent (
  IN     HTTP_IO              *HttpIo,
  IN     UINTN                HeaderCount,
  IN     EFI_HTTP_HEADER      *Headers,
  OUT    LIST_ENTRY           **ChunkListHead,
  OUT    UINTN                *ContentLength
  )
{
  EFI_HTTP_HEADER       *Header;
  CHAR8                 ChunkSizeAscii[256];
  EFI_STATUS            Status;
  UINTN                 Index;
  HTTP_IO_RESPONSE_DATA ResponseData;
  UINTN                 TotalLength;
  LIST_ENTRY            *HttpChunks;
  REST_EX_HTTP_CHUNKS   *ThisChunk;
  LIST_ENTRY            *ThisListEntry;

  if (ChunkListHead == NULL || ContentLength == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *ContentLength = 0;
  Header = HttpFindHeader (HeaderCount, Headers, HTTP_HEADER_TRANSFER_ENCODING);
  if (Header == NULL) {
    return EFI_NOT_FOUND;
  }
  if (AsciiStrCmp (Header->FieldValue, HTTP_HEADER_TRANSFER_ENCODING_HEADER_VALUE) == 0) {
    //
    // Loop to get all chunks.
    //
    TotalLength = 0;
    HttpChunks = (LIST_ENTRY *)AllocateZeroPool (sizeof (LIST_ENTRY));
    if (HttpChunks == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto DeleterChunks;
    }
    InitializeListHead (HttpChunks);
    DEBUG ((DEBUG_INFO, "%a: Chunked transfer\n", __FUNCTION__));
    while (TRUE) {
      ZeroMem((VOID *)&ResponseData, sizeof(HTTP_IO_RESPONSE_DATA));
      ResponseData.BodyLength = CHUNKED_TRANSFER_CODING_DATA_LENGTH;
      ResponseData.Body = ChunkSizeAscii;
      Status = HttpIoRecvResponse (
                 HttpIo,
                 FALSE,
                 &ResponseData
                 );
      if (EFI_ERROR (Status)) {
        return Status;
      }
      //
      // Decoding Chunked Transfer Coding.
      // Only decode chunk-size and last chunk.
      //

      //
      // Break if this is last chunk.
      //
      if (ChunkSizeAscii [0] == CHUNKED_TRNASFER_CODING_LAST_CHUNK) {
        Status = EFI_SUCCESS;
        DEBUG ((DEBUG_INFO, "%a: Last chunk\n", __FUNCTION__));
        ThisChunk = (REST_EX_HTTP_CHUNKS *)AllocateZeroPool (sizeof (REST_EX_HTTP_CHUNKS));
        if (ThisChunk == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto DeleterChunks;
        }
        InitializeListHead (&ThisChunk->NextChunk);
        ThisChunk->Length = ResponseData.BodyLength - 1 - 2; // Minus sizeof '0' and CRLF.
        ThisChunk->Data = (CHAR8 *)AllocatePool (ThisChunk->Length);
        if (ThisChunk->Data == NULL) {
          FreePool ((UINT8 *)ThisChunk);
          Status = EFI_OUT_OF_RESOURCES;
          goto DeleterChunks;
        }
        CopyMem ((UINT8 *)ThisChunk->Data, (UINT8 *)ResponseData.Body + 1, ThisChunk->Length);
        TotalLength += ThisChunk->Length;
        InsertTailList (HttpChunks, &ThisChunk->NextChunk);
        break;
      }

      //
      // Get the chunk length
      //
      Index = 0;
      while ((ChunkSizeAscii [Index] != CHUNKED_TRNASFER_CODING_EXTENSION_SAPERATOR) &&
             (ChunkSizeAscii [Index] != (CHAR8)CHUNKED_TRNASFER_CODING_CR) &&
             (Index != CHUNKED_TRANSFER_CODING_DATA_LENGTH)) {
        Index ++;
      };
      if (Index == CHUNKED_TRANSFER_CODING_DATA_LENGTH) {
        return EFI_NOT_FOUND;
      }
      ChunkSizeAscii[Index] = 0;
      AsciiStrHexToUintnS (ChunkSizeAscii, NULL, ContentLength);
      DEBUG ((DEBUG_INFO, "%a: Length of this chunk %d\n", __FUNCTION__, *ContentLength));
      //
      // Receive the data;
      //
      ThisChunk = (REST_EX_HTTP_CHUNKS *)AllocateZeroPool (sizeof (REST_EX_HTTP_CHUNKS));
      if (ThisChunk == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto DeleterChunks;
      }
      ResponseData.BodyLength = *ContentLength;
      ResponseData.Body = (CHAR8 *)AllocatePool (*ContentLength);
      if (ResponseData.Body == NULL) {
        FreePool (ThisChunk);
        Status = EFI_OUT_OF_RESOURCES;
        goto DeleterChunks;
      }
      InitializeListHead (&ThisChunk->NextChunk);
      ThisChunk->Length = *ContentLength;
      ThisChunk->Data = ResponseData.Body;
      InsertTailList (HttpChunks, &ThisChunk->NextChunk);
      Status = HttpIoRecvResponse (
                 HttpIo,
                 FALSE,
                 &ResponseData
                 );
      if (EFI_ERROR (Status)) {
        goto DeleterChunks;
      }
      //
      // Read CRLF
      //
      ZeroMem((VOID *)&ResponseData, sizeof(HTTP_IO_RESPONSE_DATA));
      ResponseData.BodyLength = 2;
      ResponseData.Body = ChunkSizeAscii;
      Status = HttpIoRecvResponse (
                 HttpIo,
                 FALSE,
                 &ResponseData
                 );
      if (EFI_ERROR (Status)) {
        goto DeleterChunks;
      }

      TotalLength += *ContentLength;
    };
    *ContentLength = TotalLength;
    *ChunkListHead = HttpChunks;
    DEBUG ((DEBUG_INFO, "%a: Total of lengh of chunks :%d\n", __FUNCTION__, TotalLength));
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
DeleterChunks:;
  while (!IsListEmpty (HttpChunks)) {
    ThisListEntry = GetFirstNode (HttpChunks);
    RemoveEntryList (ThisListEntry);
  };
  return Status;
}

