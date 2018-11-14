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
#include "kbase/kStringSplit.h"                                // kStringSplit
#include "kjson/kjBuilder.h"                                   // kjArray, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "serviceRoutines/postQueryContext.h"                  // V1 service routine that does the whole work ...
#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "mongoBackend/mongoQueryContext.h"                    // mongoQueryContext
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

  n = uriExpansion(contextP, shortName, &expandedName, &expandedType, detailsP);
  if (n == -1)
  {
    LM_E(("uriExpansion error: %s", *detailsP));
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
  char*        attrs          = (ciP->uriParam["attrs"].empty())?       NULL : (char*) ciP->uriParam["attrs"].c_str();
  EntityId*    entityIdP;
  char         typeExpanded[256];
  char*        details;
  char*        idVector[32];
  char*        typeVector[32];
  int          idVecItems   = (int) sizeof(idVector) / sizeof(idVector[0]);
  int          typeVecItems = (int) sizeof(typeVector) / sizeof(typeVector[0]);

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

  if (*type == 0)  // No type given - match all types
  {
    type          = (char*) ".*";
    isTypePattern = true;
    typeVecItems  = 0;  // Just to avoid entering the "if (typeVecItems == 1)"
  }
  else
    typeVecItems = kStringSplit(type, ',', (char**) typeVector, typeVecItems);

  idVecItems   = kStringSplit(id, ',', (char**) idVector, idVecItems);

  //
  // ID-list and Type-list at the same time is not supported
  //
  if ((idVecItems > 1) && (typeVecItems > 1))
  {
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "URI params /id/ and /type/ are both lists", "Not Permitted", OrionldDetailsString);
    return false;
  }

  if (typeVecItems == 1)  // type needs to be modified according to @context
  {
    if (uriExpand(ciP->contextP, type, typeExpanded, sizeof(typeExpanded), &details) == false)
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error during URI expansion of entity type", details, OrionldDetailsString);
      return false;
    }

    type          = typeExpanded;
    isTypePattern = false;  // Just in case ...
  }


  if (idVecItems > 1)  // A list of Entity IDs
  {
    for (int ix = 0; ix < idVecItems; ix++)
    {
      entityIdP = new EntityId(idVector[ix], type, "false", isTypePattern);
      parseData.qcr.res.entityIdVector.push_back(entityIdP);
    }
  }
  else if (typeVecItems > 1)  // A list of Entity Types
  {
    for (int ix = 0; ix < typeVecItems; ix++)
    {
      if (uriExpand(ciP->contextP, typeVector[ix], typeExpanded, sizeof(typeExpanded), &details) == false)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error during URI expansion of entity type", details, OrionldDetailsString);
        return false;
      }

      entityIdP = new EntityId(idString, typeExpanded, isIdPattern, false);
      parseData.qcr.res.entityIdVector.push_back(entityIdP);
    }
  }
  else  // Definitely no lists in EntityId id/type
  {
    entityIdP = new EntityId(idString, type, isIdPattern, isTypePattern);
    parseData.qcr.res.entityIdVector.push_back(entityIdP);
  }


#if 0
  LM_TMP(("KZ: ------------- Created EntityId --------------"));
  LM_TMP(("KZ: entityId.id:            '%s'", entityIdP->id.c_str()));
  LM_TMP(("KZ: entityId.isPattern:     '%s'", entityIdP->isPattern.c_str()));
  LM_TMP(("KZ: entityId.type:          '%s'", entityIdP->type.c_str()));
  LM_TMP(("KZ: entityId.isTypePattern: '%s'", entityIdP->isTypePattern? "true" : "false"));
#endif

  if (attrs != NULL)
  {
    char  longName[256];
    char* details;
    char* shortName;
    char* shortNameVector[32];
    int   vecItems = (int) sizeof(shortNameVector) / sizeof(shortNameVector[0]);;

    vecItems = kStringSplit(attrs, ',', (char**) shortNameVector, vecItems);

    for (int ix = 0; ix < vecItems; ix++)
    {
      shortName = shortNameVector[ix];

      if (uriExpand(ciP->contextP, shortName, longName, sizeof(longName), &details) == true)
        parseData.qcr.res.attributeList.push_back(longName);
      else
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error during URI expansion of attribute", shortName, OrionldDetailsString);
        return false;
      }
    }
  }

  // Call standard op postQueryContext
  std::vector<std::string>  compV;    // Not used but part of signature for postQueryContext

  LM_TMP(("Calling postQueryContext"));
  std::string answer   = postQueryContext(ciP, 0, compV, &parseData);
  int         entities = parseData.qcrs.res.contextElementResponseVector.size();

  LM_TMP(("KZ: postQueryContext gave %d results", entities));

  if (attrs != NULL)  // FIXME: Move all this to a separate function
  {
    LM_TMP(("KZ: 'attrs' URI option was used, so we now need to query the '@context' from all these entities", entities));
    //
    // Unfortunately, we need to do a second query now, to get the attribute "@context" of all the matching entities
    //
    // The special attribute "@context" cannot be included in the first query as the list of attributes in that query is the filter
    // for the entities to match. Pretty much all entities have an attribute called "@context", so if we include "@context" in the first
    // query, all entities would match, and that is not what we want.
    //
    QueryContextResponse  qResForContextAttr;  // Response buffer where all "@context" attributes will be found

    if (entities > 0)
    {
      QueryContextRequest       qReq;
      std::vector<std::string>  servicePathV;

      // Add each entity in the result of the first query to the request of the second query
      for (int ix = 0; ix < entities; ix++)
      {
        EntityId* entityP = &parseData.qcrs.res.contextElementResponseVector[ix]->contextElement.entityId;

        qReq.entityIdVector.push_back(entityP);
      }

      // Only interested in the attribute "@context"
      qReq.attributeList.push_back("@context");

      // mongoQueryContext requires a ServicePath, even though ngsi-ld doesn't support service paths
      servicePathV.push_back("/#");

      HttpStatusCode sCode = mongoQueryContext(&qReq, &qResForContextAttr, ciP->tenant, servicePathV, ciP->uriParam, ciP->uriParamOptions, NULL, ciP->apiVersion);

      if (sCode != SccOk)
      {
        orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Error querying for @context attributes", NULL, OrionldDetailsString);
        return false;
      }
    }

    // Now we need to add the "@context" member to each entity in parseData.qcrs.res
    QueryContextResponse* responseP = &parseData.qcrs.res;

    for (unsigned int ix = 0; ix < responseP->contextElementResponseVector.size(); ix++)
    {
      EntityId* eIdP = &responseP->contextElementResponseVector[ix]->contextElement.entityId;

      // Find eIdP in qResForContextAttr
      for (unsigned int cx = 0; cx < qResForContextAttr.contextElementResponseVector.size(); cx++)
      {
        if (qResForContextAttr.contextElementResponseVector[cx]->contextElement.entityId.id == eIdP->id)
        {
          ContextElement* ceForContextAttrP = &qResForContextAttr.contextElementResponseVector[cx]->contextElement;

          // Found the entity, now get its "@context"
          ContextAttribute* atContextP = ceForContextAttrP->contextAttributeVector.lookup("@context");

          if (atContextP != NULL)
          {
            ContextAttribute* aP = atContextP->clone();

            // Add the @context attribute to its corresponding entity in responseP
            responseP->contextElementResponseVector[ix]->contextElement.contextAttributeVector.push_back(aP);
          }
        }
      }
    }
  }

  //
  // Transform QueryContextResponse to KJ-Tree
  //
  ciP->httpStatusCode = SccOk;
  LM_TMP(("Transform QueryContextResponse to KJ-Tree"));
  ciP->responseTree   = kjTreeFromQueryContextResponse(ciP, false, &parseData.qcrs.res);

  return true;
}
