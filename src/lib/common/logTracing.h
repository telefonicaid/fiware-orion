#ifndef SRC_LIB_COMMON_LOGTRACING_H_
#define SRC_LIB_COMMON_LOGTRACING_H_

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

#include <string.h>



/* ****************************************************************************
*
* logInfoNotification - rc as int
*/
extern void logInfoNotification
(
  const char*  subId,
  const char*  verb,
  const char*  url,
  int          rc
);



/* ****************************************************************************
*
* logInfoNotification - rc as string
*/
extern void logInfoNotification
(
  const char*  subId,
  const char*  verb,
  const char*  url,
  const char*  rc
);



/* ****************************************************************************
*
* logInfoRequestWithoutPayload -
*/
extern void logInfoRequestWithoutPayload
(
  const char*  verb,
  const char*  url,
  int          rc
);



/* ****************************************************************************
*
* logInfoRequestWithPayload -
*/
extern void logInfoRequestWithPayload
(
  const char*  verb,
  const char*  url,
  const char*  payload,
  int          rc
);



/* ****************************************************************************
*
* logInfoFwdStart
*/
extern void logInfoFwdStart(const char*  verb, const char* url);



/* ****************************************************************************
*
* logInfoFwdRequest - rc as int
*/
extern void logInfoFwdRequest
(
  const char*  regId,
  const char*  verb,
  const char*  url,
  const char*  requestPayload,
  const char*  responsePayload,
  int          rc
);



/* ****************************************************************************
*
* logInfoFwdRequest - rc as string
*/
extern void logInfoFwdRequest
(
  const char*  regId,
  const char*  verb,
  const char*  url,
  const char*  requestPayload,
  const char*  responsePayload,
  const char*  rc
);



#endif // SRC_LIB_COMMON_LOGTRACING_H_

