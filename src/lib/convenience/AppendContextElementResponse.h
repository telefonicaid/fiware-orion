#ifndef SRC_LIB_CONVENIENCE_APPENDCONTEXTELEMENTRESPONSE_H_
#define SRC_LIB_CONVENIENCE_APPENDCONTEXTELEMENTRESPONSE_H_

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
#include "ngsi/EntityId.h"
#include "ngsi/StatusCode.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* Forward declaration
*/
struct UpdateContextResponse;



/* ****************************************************************************
*
* AppendContextElementResponse - 
*
* FIXME P5: AppendContextElementResponse and UpdateContextElementResponse are
*           identical. They should 'merge' into ONE struct.
*           This problem origins from an ?error? in the ngsi10 binding doc by NEC
*           See: https://github.com/telefonicaid/fiware-orion/issues/106
*
* NOTE
* The field 'entity' is:
*   o MANDATORY for "POST /v1/contextEntities"
*   o FORBIDDEN for "POST /v1/contextEntities/{entityId::id}"
*   o FORBIDDEN for "POST /v1/contextEntities/type/{entityId::type}/id/{entityId::id}"
*
* So, for its response (AppendContextElementResponse), the field 'entity' will be 
* rendered if the response is for "POST /v1/contextEntities", but NOT if the
* response is for the other two requests.
* 
*/
typedef struct AppendContextElementResponse
{
  EntityId                         entity;                          // See NOTE in type header above
  ContextAttributeResponseVector   contextAttributeResponseVector;  // Optional, but mandatory if success
  StatusCode                       errorCode;                       // Optional, but mandatory if failure

  AppendContextElementResponse();

  std::string  render(ApiVersion          apiVersion,
                      bool                asJsonObject,
                      RequestType         requestType);
  void         present(void);
  void         release(void);
  std::string  check(ApiVersion          apiVersion,
                     bool                asJsonObject,
                     RequestType         requestType,
                     const std::string&  predetectedError);
  void         fill(UpdateContextResponse* ucrsP, const std::string& entityId = "", const std::string& entityType = "");
} AppendContextElementResponse;

#endif  // SRC_LIB_CONVENIENCE_APPENDCONTEXTELEMENTRESPONSE_H_
