/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Fermín Galán
*/

#include "common/logTracing.h"

#include <string.h>

#include "logMsg/logMsg.h"

/* ****************************************************************************
*
* logInfoNotification - rc as int
*/
void logInfoHttpNotification
(
  const char*  subId,
  const char*  endpoint,
  const char*  verb,
  const char*  resource,
  int          rc
)
{
  char buffer[STRING_SIZE_FOR_INT];
  snprintf(buffer, sizeof(buffer), "%d", rc);
  logInfoHttpNotification(subId, endpoint, verb, resource, buffer);
}



/* ****************************************************************************
*
* logInfoHttpNotification - rc as string
*/
void logInfoHttpNotification
(
  const char*  subId,
  const char*  endpoint,
  const char*  verb,
  const char*  resource,
  const char*  rc
)
{
  LM_I(("Notif delivered (subId: %s): %s %s%s, response code: %s", subId, verb, endpoint, resource, rc));
}



/* ****************************************************************************
*
* logInfoMqttNotification
*/
void logInfoMqttNotification
(
  const char*  subId,
  const char*  endpoint,
  const char*  resource
)
{
  LM_I(("MQTT Notif delivered (subId: %s): broker: %s, topic: %s", subId, endpoint, resource));
}



/* ****************************************************************************
*
* logInfoRequestWithoutPayload -
*/
void logInfoRequestWithoutPayload
(
  const char*  verb,
  const char*  url,
  int          rc
)
{
  LM_I(("Request received: %s %s, response code: %d", verb, url, rc));
}


/* ****************************************************************************
*
* truncatePayload -
*
* NOTE: this function allocated dynamic memory, be careful with memory leaks!
*/
static char* truncatePayload(const char* payload)
{
  // +5 due to "(...)"
  // +1 due to '\0'
  unsigned int truncatedPayloadLengh = logInfoPayloadMaxSize + 5 + 1;

  char* truncatedPayload = (char*) malloc(logInfoPayloadMaxSize + 5 + 1);
  strncpy(truncatedPayload, payload, logInfoPayloadMaxSize);
  strncpy(truncatedPayload + logInfoPayloadMaxSize, "(...)", 5);
  truncatedPayload[truncatedPayloadLengh - 1] = '\0';

  return truncatedPayload;
}



/* ****************************************************************************
*
* logInfoRequestWithPayload -
*/
void logInfoRequestWithPayload
(
  const char*  verb,
  const char*  url,
  const char*  payload,
  int          rc
)
{
  bool cleanAfterUse = false;
  char* effectivePayload;

  if (strlen(payload) > logInfoPayloadMaxSize)
  {
    effectivePayload = truncatePayload(payload);
    cleanAfterUse = true;
  }
  else
  {
    effectivePayload = (char*) payload;
  }

  LM_I(("Request received: %s %s, request payload (%d bytes): %s, response code: %d", verb, url, strlen(payload), effectivePayload, rc));

  if (cleanAfterUse)
  {
    free(effectivePayload);
  }
}



/* ****************************************************************************
*
* logInfoFwdStart
*/
void logInfoFwdStart(const char* verb, const char*  url)
{
  //
  // WARNING:
  //  This log line is used during functional tests, thus crucial. Please DO NOT modify nor delete it
  //
  LM_I(("Starting forwarding for %s %s", verb, url));
}



/* ****************************************************************************
*
* logInfoFwdRequest - rc as int
*/
void logInfoFwdRequest
(
  const char*  regId,
  const char*  verb,
  const char*  url,
  const char*  requestPayload,
  const char*  responsePayload,
  int          rc
)
{
  char buffer[STRING_SIZE_FOR_INT];
  snprintf(buffer, sizeof(buffer), "%d", rc);
  logInfoFwdRequest(regId, verb, url, requestPayload, responsePayload, buffer);
}



/* ****************************************************************************
*
* logInfoFwdRequest - rc as string
*/
void logInfoFwdRequest
(
  const char*  regId,
  const char*  verb,
  const char*  url,
  const char*  requestPayload,
  const char*  responsePayload,
  const char*  rc
)
{
  bool cleanAfterUseReq = false;
  bool cleanAfterUseRes = false;
  char* effectivePayloadReq;
  char* effectivePayloadRes;

  if (strlen(requestPayload) > logInfoPayloadMaxSize)
  {
    effectivePayloadReq = truncatePayload(requestPayload);
    cleanAfterUseReq = true;
  }
  else
  {
    effectivePayloadReq = (char*) requestPayload;
  }

  if (strlen(responsePayload) > logInfoPayloadMaxSize)
  {
    effectivePayloadRes = truncatePayload(responsePayload);
    cleanAfterUseRes = true;
  }
  else
  {
    effectivePayloadRes = (char*) responsePayload;
  }

  LM_I(("Request forwarded (regId: %s): %s %s, request payload (%d bytes): %s, response payload (%d bytes): %s, response code: %s",
    regId, verb, url, strlen(requestPayload), effectivePayloadReq, strlen(responsePayload), effectivePayloadRes, rc));

  if (cleanAfterUseReq)
  {
    free(effectivePayloadReq);
  }

  if (cleanAfterUseRes)
  {
    free(effectivePayloadRes);
  }

}



