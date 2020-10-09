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
void logInfoNotification
(
  const char*  subId,
  const char*  verb,
  const char*  url,
  int          rc
)
{
  char buffer[STRING_SIZE_FOR_INT];
  snprintf(buffer, sizeof(buffer), "%d", rc);
  logInfoNotification(subId, verb, url, buffer);
}



/* ****************************************************************************
*
* logInfoNotification - rc as string
*/
void logInfoNotification
(
  const char*  subId,
  const char*  verb,
  const char*  url,
  const char*  rc
)
{
  LM_I(("Notif delivered (subId: %s): %s %s, response code: %s", subId, verb, url, rc));
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
  LM_I(("Request received: %s %s, request payload (%d bytes): %s, response code: %d", verb, url, strlen(payload), payload, rc));
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
  LM_I(("Request forwarded (regId: %s): %s %s, request payload (%d bytes): %s, response payload (%d bytes): %s, response code: %d",
    regId, verb, url, strlen(requestPayload), requestPayload, strlen(responsePayload), responsePayload, rc));
}



