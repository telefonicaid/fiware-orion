/*
*
* Copyright 2021 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/CHECK.h"                                // X_CHECK
#include "orionld/payloadCheck/pcheckAttribute.h"                // Own interface



bool pcheckSubAttribute(KjNode* saP, char** detailP)  // FIXME: Implement and move to its own module
{
  return true;
}



// -----------------------------------------------------------------------------
//
// pcheckAttribute -
//
bool pcheckAttribute(KjNode* aP, char* type, bool typeMandatory, char** detailP)
{
  KjNode* typeP = NULL;

  //
  // We need the value of 'type', so, we'll get that first (unless already given)
  //
  if (type == NULL)
  {
    typeP = kjLookup(aP, "type");

    if (typeP != NULL)
    {
      if (typeP->type != KjString)
      {
        *detailP = (char*) "Attribute type must be a string";
        orionldErrorResponseCreate(OrionldBadRequestData, "Attribute type must be a string", aP->name);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }

      type = typeP->value.s;
    }
    else if (typeMandatory == true)
    {
      *detailP = (char*) "attribute type missing";
      orionldErrorResponseCreate(OrionldBadRequestData, "Attribute type missing", aP->name);
      orionldState.httpStatusCode = SccBadRequest;
      return false;
    }
  }

  KjNode* valueP      = NULL;
  KjNode* objectP     = NULL;
  KjNode* observedAtP = NULL;
  KjNode* datasetIdP  = NULL;

  for (KjNode* memberP = aP->value.firstChildP; memberP != NULL; memberP = memberP->next)
  {
    if (memberP == typeP)
      continue;  // 'type' already dealt with
    else if (strcmp(memberP->name, "value") == 0)
    {
      DUPLICATE_CHECK(valueP, "value", memberP);

      // Relationships cannot have a 'value' member
      if ((type != NULL) && (strcmp(type, "Relationship") == 0))
      {
        *detailP = (char*) "Relationships cannot have a 'value' member";
        orionldErrorResponseCreate(OrionldBadRequestData, "Relationships cannot have a 'value' member", aP->name);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(memberP->name, "object") == 0)
    {
      // Only Relationships can have an 'object' member
      if ((type != NULL) && (strcmp(type, "Relationship") != 0))
      {
        *detailP = (char*) "Only Relationships can have an 'object' member";
        orionldErrorResponseCreate(OrionldBadRequestData, "Only Relationships can have an 'object' member", aP->name);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }

      DUPLICATE_CHECK(objectP, "object", memberP);
      STRING_CHECK(memberP, "object");
      URI_CHECK(memberP->value.s, "object", true);
    }
    else if (strcmp(memberP->name, "observedAt") == 0)
    {
      double dateTime;
      DUPLICATE_CHECK(observedAtP, "observedAt", memberP);
      STRING_CHECK(memberP, "observedAt");
      DATETIME_CHECK(memberP->value.s, dateTime, "observedAt");
    }
    else if (strcmp(memberP->name, "datasetId") == 0)
    {
      DUPLICATE_CHECK(datasetIdP, "datasetId", memberP);
      STRING_CHECK(memberP, "datasetId");
      URI_CHECK(memberP->value.s, "datasetId", true);
    }
    else  // Seems to be a sub-attribute
    {
      if (pcheckSubAttribute(memberP, detailP) == false)  // pcheckSubAttribute calls orionldErrorResponseCreate
        return false;
    }
  }

  return true;
}
