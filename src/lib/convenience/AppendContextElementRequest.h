#ifndef SRC_LIB_CONVENIENCE_APPENDCONTEXTELEMENTREQUEST_H_
#define SRC_LIB_CONVENIENCE_APPENDCONTEXTELEMENTREQUEST_H_

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

#include "ngsi/EntityId.h"
#include "ngsi/ContextAttributeVector.h"
#include "ngsi/MetadataVector.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* AppendContextElementRequest - 
*
* NOTE
* The field 'entity' is MANDATORY for "POST /v1/contextEntities".
* For other requests, data in the URL (path and parameters) must coincide
* with the data in the payload.
* If not, an error is raised.
*
*/
typedef struct AppendContextElementRequest
{
  EntityId                   entity;                     // See NOTE in type header above
  ContextAttributeVector     contextAttributeVector;     // Optional

  AppendContextElementRequest();

  std::string  toJsonV1(bool asJsonObject, RequestType requestType);
  void         release();
  std::string  check(ApiVersion          apiVersion,
                     bool                asJsonObject,
                     RequestType         requestType,
                     const std::string&  predetectedError);
} AppendContextElementRequest;

#endif  // SRC_LIB_CONVENIENCE_APPENDCONTEXTELEMENTREQUEST_H_
