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
* Author: Orion dev team
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
  bool         keyValues  = ciP->uriParamOptions["keyValues"];
  std::string  renderMode = (keyValues == true)? "keyValues" : "normalized";

  if (pcontextAttribute)
  {
    std::string out;

    if (requestType == EntityAttributeValueRequest)
    {
      out = pcontextAttribute->toJsonAsValue(ciP);
    }
    else
    {
      out = "{";
      out += pcontextAttribute->toJson(true, false, renderMode, requestType);  // param 1 'true' as it is the last and only element
      out += "}";
    }


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
// The Query should be for an indvidual entity
//

void Attribute::fill(QueryContextResponse* qcrsP, std::string attrName)
{

  if (qcrsP->errorCode.code == SccContextElementNotFound)
  {
    errorCode.fill("NotFound",  "The requested entity has not been found. Check type and id");
  }
  else if (qcrsP->errorCode.code != SccOk)
  {
    //
    // any other error distinct from Not Found
    //
    errorCode.fill(qcrsP->errorCode);
  }
  else if (qcrsP->contextElementResponseVector.size() > 1) // qcrsP->errorCode.code == SccOk
  {
    //
    // If there are more than one entity, we return an error
    //
    errorCode.fill("TooManyResults", "There is more than one entity with that id. Refine your query.");
  }
  else
  {
    pcontextAttribute = NULL;
    // Look for the attribute by name
    for(std::size_t i = 0; i < qcrsP->contextElementResponseVector[0]->contextElement.contextAttributeVector.size();
          ++i)
    {
      if (qcrsP->contextElementResponseVector[0]->contextElement.contextAttributeVector[i]->name == attrName)
      {
        pcontextAttribute = qcrsP->contextElementResponseVector[0]->contextElement.contextAttributeVector[i];
        break;
      }
    }
    if (pcontextAttribute == NULL) {
        errorCode.fill("NotFound",  "The entity does not have such an attribute");
    }
  }
}
