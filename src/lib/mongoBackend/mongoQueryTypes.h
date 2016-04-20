#ifndef MONGO_QUERY_TYPES_H
#define MONGO_QUERY_TYPES_H

/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermin Galan Marquez
*/

#include <string>
#include <map>

#include "orionTypes/EntityTypeVectorResponse.h"
#include "orionTypes/EntityTypeResponse.h"

#include "mongoBackend/MongoGlobal.h"
#include "mongoBackend/dbConstants.h"


/* Some string tokens used for aggregation commands */
const std::string C_ID_ENTITY      = std::string("_id.") + "type";
const std::string C_ID_SERVICEPATH = std::string("_id.") + "servicePath";
const std::string CS_ID_ENTITY     = std::string("$_id.") + "type";
const std::string C_ID_NAME        = std::string("_id.") + "name";
const std::string C_ID_TYPE        = std::string("_id.") + "type";
const std::string S_ATTRNAMES      = std::string("$") + ENT_ATTRNAMES;


/* ****************************************************************************
*
* mongoEntityTypes -
*/
extern HttpStatusCode mongoEntityTypes
(
  EntityTypeVectorResponse*            responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,
  const std::string&                   apiVersion,
  unsigned int*                        totalTypes
);

/* ****************************************************************************
*
* mongoAttributesForEntityType -
*/
extern HttpStatusCode mongoAttributesForEntityType
(
  const std::string&                   entityType,
  EntityTypeResponse*                  responseP,
  const std::string&                   tenant,
  const std::vector<std::string>&      servicePathV,
  std::map<std::string, std::string>&  uriParams,
  const std::string&                   apiVersion = "v1"
);

#endif
