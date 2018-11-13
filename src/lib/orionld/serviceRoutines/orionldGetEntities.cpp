/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
extern "C"
{
#include "kjson/kjBuilder.h"                                   // kjArray, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "serviceRoutines/postQueryContext.h"                  // V1 service routine that does the whole work ...
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "orionld/kjTree/kjTreeFromQueryContextResponse.h"     // kjTreeFromQueryContextResponse
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultUrl
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/serviceRoutines/orionldGetEntities.h"        // Own Interface


//
// FIXME: URI Expansion from 'orionldPostEntities.cpp' to its own module!
//
extern int uriExpansion(OrionldContext* contextP, const char* name, char** expandedNameP, char** expandedTypeP, char** detailsPP);



// -----------------------------------------------------------------------------
//
// uriExpand - FIXME: move to src/lib/orionld/uriExpand/uriExpand.*
//
bool uriExpand(OrionldContext* contextP, char* shortName, char* longName, int longNameLen, char** detailsP)
{
  char* expandedName;
  char* expandedType;
  int   n;

  if (contextP != NULL)
    LM_TMP(("KZ; Calling uriExpansion for '%s' in context '%s'", shortName, contextP->url));
  else
    LM_TMP(("KZ; Calling uriExpansion for '%s' in NULL context", shortName));

  n = uriExpansion(contextP, shortName, &expandedName, &expandedType, detailsP);
  LM_TMP(("KZ; uriExpansion for '%s': %d", shortName, n));
  if (n == -1)
  {
    LM_E(("KZ: uriExpansion error: %s", *detailsP));
    return false;
  }
  else if (n == -2)  // expansion NOT found in the context - use default URL
  {
    //
    // FIXME:
    //   What if it is not a shortname in URI param?
    //   Check for http:// in typeName?
    //
    snprintf(longName, longNameLen, "%s%s", orionldDefaultUrl, shortName);
  }
  else  // expansion found
  {
    snprintf(longName, longNameLen, "%s", expandedName);
  }

  LM_TMP(("KZ: uriExpand expanded '%s' to '%s'", shortName, longName));
  return true;
}



// ----------------------------------------------------------------------------
//
// orionldGetEntities -
//
// URI params supported by APIv2 getEntities():
// - id
// - idPattern
// - type
// - typePattern - Not possible - ignored (need an exact type name to lookup alias)
// - q
// - mq          - Not interesting for ngsi-ld
// - geometry    - Not interesting for ngsi-ld
// - coords      - Not interesting for ngsi-ld
// - georel      - Not interesting for ngsi-ld
//
bool orionldGetEntities(ConnectionInfo* ciP)
{
  ParseData    parseData;
  char*        id             = (ciP->uriParam["id"].empty())?          NULL : (char*) ciP->uriParam["id"].c_str();
  char*        type           = (ciP->uriParam["type"].empty())?        (char*) "" : (char*) ciP->uriParam["type"].c_str();
  char*        idPattern      = (ciP->uriParam["idPattern"].empty())?   NULL : (char*) ciP->uriParam["idPattern"].c_str();
  char*        idString       = (id != NULL)? id      : idPattern;
  const char*  isIdPattern    = (id != NULL)? "false" : "true";
  bool         isTypePattern  = (*type != 0)? false : true;
  EntityId*    entityIdP;
  char         typeExpanded[256];

  LM_T(LmtServiceRoutine, ("In orionldGetEntities"));

  if ((idPattern != NULL) && (id != NULL))
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Incompatible parameters", "id, idPattern", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (idString == NULL)
  {
    idString    = (char*) ".*";
    isIdPattern = (char*) "true";
  }

  //
  // type and typePattern need to be modified according to the @context
  //
  if (*type != 0)
  {
    char* details;

    if (uriExpand(ciP->contextP, type, typeExpanded, sizeof(typeExpanded), &details) == false)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error during URI expansion", details, OrionldDetailsString);
      return false;
    }
    type = typeExpanded;
    LM_TMP(("KZ: type '%s' expanded to '%s'", type, typeExpanded));
  }
  else  // No type given - match all types
  {
    type          = (char*) ".*";
    isTypePattern = true;
  }


  LM_TMP(("KZ: ------------- Creating EntityId --------------"));

  entityIdP = new EntityId(idString, type, isIdPattern, isTypePattern);

  parseData.qcr.res.entityIdVector.push_back(entityIdP);

  LM_TMP(("KZ: entityId.id:            '%s'", entityIdP->id.c_str()));
  LM_TMP(("KZ: entityId.isPattern:     '%s'", entityIdP->isPattern.c_str()));
  LM_TMP(("KZ: entityId.type:          '%s'", entityIdP->type.c_str()));
  LM_TMP(("KZ: entityId.isTypePattern: '%s'", entityIdP->isTypePattern? "true" : "false"));


  // Call standard op postQueryContext
  std::vector<std::string>  compV;    // Not used but part of signature for postQueryContext

  LM_TMP(("Calling postQueryContext"));
  std::string answer = postQueryContext(ciP, 0, compV, &parseData);
  LM_TMP(("KZ: postQueryContext gave %d results", parseData.qcrs.res.contextElementResponseVector.size()));

  // Transform QueryContextResponse to KJ-Tree
  ciP->httpStatusCode = SccOk;
  ciP->responseTree = kjTreeFromQueryContextResponse(ciP, false, &parseData.qcrs.res);

  return true;
}
