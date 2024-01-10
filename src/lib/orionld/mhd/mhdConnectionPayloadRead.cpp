/*
*
* Copyright 2018 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "kalloc/kaStrdup.h"                                   // kaStrdup
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/mhd/mhdConnectionPayloadRead.h"              // Own interface



// -----------------------------------------------------------------------------
//
// mhdConnectionPayloadRead -
//
MHD_Result mhdConnectionPayloadRead
(
  size_t*          upload_data_size,
  const char*      upload_data
)
{
  size_t  dataLen = *upload_data_size;

  //
  // If the HTTP header says the request is bigger than inReqPayloadMaxSize,
  // just silently "eat" the entire message.
  //
  // The problem occurs when the broker is lied to and there aren't orionldState.in.contentLength
  // bytes to read.
  // When this happens, MHD blocks until it times out (MHD_OPTION_CONNECTION_TIMEOUT defaults to 5 seconds),
  // and the broker isn't able to respond. MHD just closes the connection.
  // Question asked in mhd mailing list.
  //
  // See github issue:
  //   https://github.com/telefonicaid/fiware-orion/issues/2761
  //
  if ((unsigned long long) orionldState.in.contentLength > inReqPayloadMaxSize)
  {
    //
    // Errors can't be returned yet, postpone ...
    //
    *upload_data_size = 0;
    return MHD_YES;
  }

  //
  // First call with payload - use the pre-allocated  "orionldState.preallocReqBuf" if possible,
  // otherwise allocate a bigger buffer
  //
  // FIXME P1: This could be done in "Part I" instead, saving an "if" for each "Part II" call
  //           Once we *really* look to scratch some efficiency, this change should be made.
  //
  if (orionldState.in.payloadSize == 0)  // First call with payload
  {
    if (orionldState.in.contentLength >= (int) sizeof(orionldState.preallocReqBuf))
    {
      orionldState.in.payload = (char*) malloc(orionldState.in.contentLength + 1);
      if (orionldState.in.payload == NULL)
      {
        LM_E(("Out of memory!!!"));
        return MHD_NO;
      }
    }
    else
      orionldState.in.payload = orionldState.preallocReqBuf;
  }

  // Copy the chunk
  memcpy(&orionldState.in.payload[orionldState.in.payloadSize], upload_data, dataLen);

  // Add to the size of the accumulated read buffer
  orionldState.in.payloadSize += dataLen;

  // Zero-terminate the payload
  orionldState.in.payload[orionldState.in.payloadSize] = 0;

  // Acknowledge the data and return
  *upload_data_size = 0;

  return MHD_YES;
}
