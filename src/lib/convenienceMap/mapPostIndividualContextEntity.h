#ifndef SRC_LIB_CONVENIENCEMAP_MAPPOSTINDIVIDUALCONTEXTENTITY_H_
#define SRC_LIB_CONVENIENCEMAP_MAPPOSTINDIVIDUALCONTEXTENTITY_H_

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
#include <string>

#include "mongoBackend/MongoGlobal.h"
#include "convenience/AppendContextElementRequest.h"
#include "convenience/AppendContextElementResponse.h"
#include "rest/HttpStatusCode.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* mapPostIndividualContextEntity - 
*/
extern HttpStatusCode mapPostIndividualContextEntity
(
  const std::string&             entityId,
  const std::string&             entityType,
  AppendContextElementRequest*   request,
  AppendContextElementResponse*  response,
  ConnectionInfo*                ciP
);

#endif  // SRC_LIB_CONVENIENCEMAP_MAPPOSTINDIVIDUALCONTEXTENTITY_H_
