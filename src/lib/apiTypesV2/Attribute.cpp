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
#include "common/errorMessages.h"
#include "common/RenderFormat.h"
#include "common/string.h"
#include "common/JsonHelper.h"
#include "logMsg/logMsg.h"
#include "ngsi10/QueryContextResponse.h"
#include "apiTypesV2/Attribute.h"
#include "rest/OrionError.h"




/* ****************************************************************************
*
* Attribute::toJson -
*/
std::string Attribute::toJson
(
  bool                             acceptedTextPlain,   // in parameter (pass-through)
  bool                             acceptedJson,        // in parameter (pass-through)
  MimeType                         outFormatSelection,  // in parameter (pass-through)
  MimeType*                        outMimeTypeP,        // out parameter (pass-through)
  HttpStatusCode*                  scP,                 // out parameter (pass-through)
  bool                             keyValues,           // in parameter
  const std::vector<std::string>&  metadataFilter,      // in parameter
  RequestType                      requestType          // in parameter
)
{
  if (contextAttributeP == NULL) {
    LM_E(("Runtime Error (NULL contextAttributeP)"));
    return "";
  }

  RenderFormat  renderFormat = (keyValues == true)? NGSI_V2_KEYVALUES : NGSI_V2_NORMALIZED;

  std::string out;

  if (requestType == EntityAttributeValueRequest)
  {
    out = contextAttributeP->toJsonAsValue(V2,
                                           acceptedTextPlain,
                                           acceptedJson,
                                           outFormatSelection,
                                           outMimeTypeP,
                                           scP);
  }
  else
  {
    if (renderFormat == NGSI_V2_KEYVALUES)
    {
      JsonObjectHelper jh;
      jh.addRaw(contextAttributeP->name, contextAttributeP->toJsonValue());
      out = jh.str();
    }
    else  // NGSI_V2_NORMALIZED
    {
      out = contextAttributeP->toJson(metadataFilter);
    }
  }

  return out;
}




/* ****************************************************************************
*
* Attribute::fill -
*
* CAUTION
*   The Query should be for an indvidual entity
*
*/
void Attribute::fill(const QueryContextResponse& qcrs, const std::string& attrName, OrionError* oeP)
{
  if (qcrs.errorCode.code == SccContextElementNotFound)
  {
    oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ENTITY, ERROR_NOT_FOUND);
  }
  else if (qcrs.errorCode.code != SccOk)
  {
    //
    // any other error distinct from Not Found
    //
    oeP->fill(qcrs.errorCode.code, qcrs.errorCode.details, qcrs.errorCode.reasonPhrase);
  }
  else if (qcrs.contextElementResponseVector.size() > 1)  // qcrs.errorCode.code == SccOk
  {
    //
    // If there are more than one entity, we return an error
    //
    oeP->fill(SccConflict, ERROR_DESC_TOO_MANY_ENTITIES, ERROR_TOO_MANY);
  }
  else
  {
    contextAttributeP = NULL;
    // Look for the attribute by name

    ContextElementResponse* cerP = qcrs.contextElementResponseVector[0];

    for (std::size_t i = 0; i < cerP->entity.attributeVector.size(); ++i)
    {
      if (cerP->entity.attributeVector[i]->name == attrName)
      {
        contextAttributeP = cerP->entity.attributeVector[i];
        break;
      }
    }

    if (contextAttributeP == NULL)
    {
      oeP->fill(SccContextElementNotFound, ERROR_DESC_NOT_FOUND_ATTRIBUTE, ERROR_NOT_FOUND);
    }
  }
}
