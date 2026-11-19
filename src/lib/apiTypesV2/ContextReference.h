#ifndef SRC_LIB_APITYPESV2_CONTEXTREFERENCE_H_
#define SRC_LIB_APITYPESV2_CONTEXTREFERENCE_H_

/*
*
* Copyright 2026 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Orion dev team
*/
#include <string>
#include <vector>

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONArray.h"



namespace ngsiv2
{
/* ****************************************************************************
*
* ContextReference -
*
* Reference to an entity (by id and type) which information is included in
* the expression context of custom notifications. Both id and type may
* include macros (e.g. ${refId}) to be resolved at notification time.
*/
struct ContextReference
{
  std::string  id;
  std::string  type;

  std::string  toJson();
};



/* ****************************************************************************
*
* contextReferencesFill -
*
* Fill a vector of ContextReference from the CSUB_CONTEXT_REFERENCES field of the
* BSON object passed as argument (if the field doesn't exist, the vector is left empty)
*/
extern void contextReferencesFill(const orion::BSONObj& bo, std::vector<ContextReference>* crV);



/* ****************************************************************************
*
* contextReferencesToBson -
*/
extern orion::BSONArray contextReferencesToBson(const std::vector<ContextReference>& crV);
}

#endif  // SRC_LIB_APITYPESV2_CONTEXTREFERENCE_H_
