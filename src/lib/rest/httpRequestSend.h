#ifndef SRC_LIB_REST_HTTPREQUESTSEND_H_
#define SRC_LIB_REST_HTTPREQUESTSEND_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: developer
*/
#include <string>
#include <vector>

#include "ConnectionInfo.h"

#define URI_BUF          (256)
#define TAM_BUF          (8 * 1024)            // 8 KB  (for HTTP responses and pre-payload bytes in request, which will be very small)



/***************************************************************************
*
* httpRequestInit -
*/
extern void httpRequestInit(long defaultTimeoutInMilliseconds);



/***************************************************************************
*
* httpRequestConnect -
*/
extern int httpRequestConnect(const std::string& host, unsigned short port);



/* ****************************************************************************
*
* httpRequestSend - 
*/
extern int httpRequestSend
(
  const std::string&                         from,
  const std::string&                         ip,
  unsigned short                             port,
  const std::string&                         protocol,
  const std::string&                         verb,
  const std::string&                         tenant,
  const std::string&                         servicePath,
  const std::string&                         xauthToken,
  const std::string&                         resource,
  const std::string&                         content_type,
  const std::string&                         content,
  const std::string&                         fiwareCorrelation,
  const std::string&                         ngisv2AttrFormat,
  std::string*                               outP,
  long long*                                 statusCodeP,
  const std::map<std::string, std::string>&  extraHeaders,
  const std::string&                         acceptFormat          = "",
  long                                       timeoutInMilliseconds = -1
);



/* ****************************************** **********************************
*
* httpRequestSendWithCurl -
*/
extern int httpRequestSendWithCurl
(
  CURL*                                      curl,
  const std::string&                         from,
  const std::string&                         ip,
  unsigned short                             port,
  const std::string&                         protocol,
  const std::string&                         verb,
  const std::string&                         tenant,
  const std::string&                         servicePath,
  const std::string&                         xauthToken,
  const std::string&                         resource,
  const std::string&                         content_type,
  const std::string&                         content,
  const std::string&                         fiwareCorrelation,
  const std::string&                         ngisv2AttrFormat,
  std::string*                               outP,
  long long*                                 statusCodeP,
  const std::map<std::string, std::string>&  extraHeaders,
  const std::string&                         acceptFormat          = "",
  long                                       timeoutInMilliseconds = -1
);



#endif  // SRC_LIB_REST_HTTPREQUESTSEND_H_
