#ifndef CONNECTION_INFO_H
#define CONNECTION_INFO_H

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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <stdint.h>
#include <string>
#include <vector>

#include "logMsg/logMsg.h"

#include "common/Format.h"
#include "rest/HttpStatusCode.h"
#include "rest/mhd.h"
#include "rest/Verb.h"
#include "rest/HttpHeaders.h"
#include "parse/ComplexValueNode.h"


/* ****************************************************************************
*
* ConnectionInfo - 
*/
class ConnectionInfo
{
public:
  ConnectionInfo(std::string _url, std::string _method, std::string _version, MHD_Connection* _connection = NULL) : url(_url), method(_method), version(_version)
  {
    connection            = _connection;
    payload               = NULL;
    payloadSize           = 0;
    inFormat              = XML;
    outFormat             = XML;
    httpStatusCode        = SccOk;
    fractioned            = false;
    callNo                = 1;
    requestEntityTooLarge = false;
    inComplexValue        = false;
    complexValueContainer = NULL;
    current               = NULL;

    memset(payloadWord, 0, sizeof(payloadWord));

    if      (_method == "POST")    verb = POST;
    else if (_method == "PUT")     verb = PUT;
    else if (_method == "GET")     verb = GET;
    else if (_method == "DELETE")  verb = DELETE;
    else                           verb = NOVERB;
  }

  MHD_Connection*           connection;
  Verb                      verb;
  Format                    inFormat;
  Format                    outFormat;
  std::string               url;
  std::string               method;
  std::string               version;
  std::string               charset;
  HttpHeaders               httpHeaders;
  char*                     payload;
  int                       payloadSize;
  char                      payloadWord[64];
  std::string               answer;
  bool                      fractioned;
  int                       callNo;
  bool                      requestEntityTooLarge;
  bool                      inComplexValue;
  orion::ComplexValueNode*  complexValueContainer;
  orion::ComplexValueNode*  current;

  ::std::vector<orion::ComplexValueNode*> complexValueNode;

  // Outgoing
  HttpStatusCode            httpStatusCode;
  std::vector<std::string>  httpHeader;
  std::vector<std::string>  httpHeaderValue;
};

#endif
