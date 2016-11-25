#ifndef SRC_LIB_CONVENIENCE_UPDATECONTEXTELEMENTRESPONSE_H_
#define SRC_LIB_CONVENIENCE_UPDATECONTEXTELEMENTRESPONSE_H_

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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "convenience/ContextAttributeResponseVector.h"
#include "ngsi/StatusCode.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* Forward declaration
*/
struct UpdateContextResponse;



/* ****************************************************************************
*
* UpdateContextElementResponse - 
*
* FIXME P5: AppendContextElementResponse and UpdateContextElementResponse are
*           identical. They should 'merge' into ONE struct.
*           This problem origins from an ?error? in the ngsi10 binding doc by NEC
*           See: https://github.com/telefonicaid/fiware-orion/issues/106
*/
typedef struct UpdateContextElementResponse
{
  ContextAttributeResponseVector   contextAttributeResponseVector;  // Optional, but mandatory if success
  StatusCode                       errorCode;                       // Optional, but mandatory if failure

  UpdateContextElementResponse();

  std::string  render(ApiVersion   apiVersion,
                      bool         asJsonObject,
                      RequestType  requestType);
  void         present(const std::string&  indent);
  void         release();
  std::string  check(ApiVersion          apiVersion,
                     bool                asJsonObject,
                     RequestType         requestType,
                     const std::string&  predetectedError);
  void         fill(UpdateContextResponse* ucrsP);
} UpdateContextElementResponse;

#endif  // SRC_LIB_CONVENIENCE_UPDATECONTEXTELEMENTRESPONSE_H_
