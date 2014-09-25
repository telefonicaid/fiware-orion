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
* fermin at tid dot es
*
* Author: Fermin Galan Marquez
*/

#include <string>
#include <map>

#include "utility/EntityTypesResponse.h"
#include "utility/EntityTypesAttributesResponse.h"

/* ****************************************************************************
*
* mongoEntityTypes -
*/
extern HttpStatusCode mongoEntityTypes
(
  EntityTypesResponse*                  responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams
);

/* ****************************************************************************
*
* mongoAttributesForEntityType -
*/
extern HttpStatusCode mongoAttributesForEntityType
(
  std::string                           entityType,
  EntityTypesAttributesResponse*        responseP,
  const std::string&                    tenant,
  const std::vector<std::string>&       servicePathV,
  std::map<std::string, std::string>&   uriParams
);

#endif

