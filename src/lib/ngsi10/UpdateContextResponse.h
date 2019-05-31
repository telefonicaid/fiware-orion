#ifndef SRC_LIB_NGSI10_UPDATECONTEXTRESPONSE_H_
#define SRC_LIB_NGSI10_UPDATECONTEXTRESPONSE_H_

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

#include "ngsi/ContextElementResponseVector.h"
#include "ngsi/StatusCode.h"
#include "ngsi10/UpdateContextRequest.h"

#include "rest/OrionError.h"



/* ****************************************************************************
*
* UpdateContextResponse - 
*/
typedef struct UpdateContextResponse
{
  ContextElementResponseVector  contextElementResponseVector;  // Optional
  StatusCode                    errorCode;                     // Optional

  OrionError                    oe;                            // Used by NGSIv2

  UpdateContextResponse();
  UpdateContextResponse(StatusCode& _errorCode);
  ~UpdateContextResponse();

  std::string   toJsonV1(bool asJsonObject);
  std::string   check(ApiVersion apiVersion, bool asJsonObject, const std::string& predetectedError);
  void          release(void);
  void          fill(UpdateContextResponse* upcrsP);
  void          fill(UpdateContextRequest* upcrP, HttpStatusCode sc);   // Needed by NGSIv2 forwarding logic
  void          notFoundPush(Entity* eP, ContextAttribute* aP, StatusCode* scP);
  void          foundPush(Entity* eP, ContextAttribute* aP);
  void          merge(UpdateContextResponse* upcrsP);
} UpdateContextResponse;

#endif  // SRC_LIB_NGSI10_UPDATECONTEXTRESPONSE_H_
