/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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

#include "common/tag.h"
#include "apiTypesV2/Attribute.h"
#include "ngsi10/QueryContextResponse.h"


/* ****************************************************************************
*
* Attribute::render -
*/
std::string Attribute::render(ConnectionInfo* ciP, RequestType requestType, bool comma)
{

  LM_I(("pcontextAttribute %p", pcontextAttribute));

  if (pcontextAttribute)
  {
      std::string out = "{";
      out += pcontextAttribute->toJson(true /* is the last and only element*/);
      out += "}";
      if (comma)
      {
        out += ",";
      }

      return out;
  }

  return errorCode.toJson(true);

}




/* ****************************************************************************
*
* Attribute::fill -
*/

//
// Caution
// The Query should be for an indvidual entity and for a specific attribute
//

void Attribute::fill(QueryContextResponse* qcrsP)
{


  if (qcrsP->errorCode.code == SccContextElementNotFound)
  {
    //
    // The entity does not exist OR the attribute does not exist for that entity
    //
    errorCode.error = "NotFound";
    errorCode.description = "The entity does not have such an attribute";
  }
  else if (qcrsP->errorCode.code != SccOk)
  {
    //
    // Any error except not found - use the error for the response
    //
    errorCode.fill(qcrsP->errorCode);
  }
  else if (qcrsP->contextElementResponseVector.size()>1)
  {
    //
    // If there are more than one entity, we return an error
    //
    errorCode.fill("Many entities with that ID", "/v2/entities?id="+qcrsP->contextElementResponseVector[0]->contextElement.entityId.id);
  }
  else
  {
    // At least, one attribute was found
    pcontextAttribute = qcrsP->contextElementResponseVector[0]->contextElement.contextAttributeVector[0];
  }
}
