/*
*
* Copyright 2018 FIWARE Foundation e.V.
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
* Author: Larysse Savanna
*/
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjString, kjBoolean, ...
#include "kjson/kjParse.h"                                       // kjParse
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                  // httpHeaderAdd
#include "apiTypesV2/Registration.h"                             // Registration
#include "orionld/common/numberToDate.h"                         // numberToDate
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate, OrionldInternalError
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/context/OrionldContext.h"                      // OrionldContext
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContext
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/context/orionldContextCacheLookup.h"           // orionldContextCacheLookup
#include "orionld/kjTree/kjTreeFromRegistration.h"               // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromRegistration -
//
KjNode* kjTreeFromRegistration(ConnectionInfo* ciP, ngsiv2::Registration* registrationP)
{
  KjNode*          topP = kjObject(orionldState.kjsonP, NULL);
  KjNode*          objectP;
  KjNode*          objectP2;
  KjNode*          arrayP;
  KjNode*          arrayP2;
  KjNode*          nodeP;
  char             date[128];
  char*            details;
  unsigned int     infoSize;
  OrionldContext*  contextP = orionldContextCacheLookup(registrationP->ldContext.c_str());

  if (contextP == NULL)
    contextP = orionldCoreContextP;

  // id
  nodeP = kjString(orionldState.kjsonP, "id", registrationP->id.c_str());
  kjChildAdd(topP, nodeP);

  // type - must always be ContextSourceRegistration
  nodeP = kjString(orionldState.kjsonP, "type", "ContextSourceRegistration");
  kjChildAdd(topP, nodeP);

  // name
  if (registrationP->name != "")
  {
    nodeP = kjString(orionldState.kjsonP, "name", registrationP->name.c_str());
    kjChildAdd(topP, nodeP);
  }

  // description
  if (registrationP->descriptionProvided == true)
  {
    nodeP = kjString(orionldState.kjsonP, "description", registrationP->description.c_str());
    kjChildAdd(topP, nodeP);
  }


  //
  // sysAttrs - createdAt and modifiedAt
  //
  if (ciP->uriParamOptions["sysAttrs"] == true)
  {
    if (registrationP->createdAt != -1)
    {
      KjNode* createdAtNodeP = kjInteger(orionldState.kjsonP, "createdAt", registrationP->createdAt);
      kjChildAdd(topP, createdAtNodeP);
    }

    if (registrationP->modifiedAt != -1)
    {
      KjNode* modifiedAtNodeP = kjInteger(orionldState.kjsonP, "modifiedAt", registrationP->modifiedAt);
      kjChildAdd(topP, modifiedAtNodeP);
    }
  }


  //
  // expires
  //
  if (registrationP->expires != -1)
  {
    char dateExp[64];

    if (numberToDate((time_t) registrationP->expires, dateExp, sizeof(dateExp), &details) == false)
    {
      LM_E(("Error creating a stringified date for 'expires'"));
      orionldErrorResponseCreate(OrionldInternalError, "unable to create a stringified date", details);
      return NULL;
    }
    nodeP = kjString(orionldState.kjsonP, "expires", dateExp);
    kjChildAdd(topP, nodeP);
  }


  //
  // endpoint
  //
  nodeP = kjString(orionldState.kjsonP, "endpoint", registrationP->provider.http.url.c_str());
  kjChildAdd(topP, nodeP);


  // information
  // FIXME: Change to accept more than one member in the future

  infoSize = 1;
  arrayP = kjArray(orionldState.kjsonP, "information");

  for (unsigned int iIx = 0; iIx < infoSize; iIx++)
  {
    unsigned int  size;

    objectP = kjObject(orionldState.kjsonP, NULL);

    // information::entities
    size = registrationP->dataProvided.entities.size();
    if (size != 0)
    {
      arrayP2 = kjArray(orionldState.kjsonP, "entities");

      for (unsigned int eIx = 0; eIx < size; eIx++)
      {
        ngsiv2::EntID* eP = &registrationP->dataProvided.entities[eIx];
        objectP2 = kjObject(orionldState.kjsonP, NULL);

        if (eP->id != "")
        {
          nodeP = kjString(orionldState.kjsonP, "id", eP->id.c_str());
          kjChildAdd(objectP2, nodeP);
        }

        if (eP->idPattern != "")
        {
          nodeP = kjString(orionldState.kjsonP, "idPattern", eP->idPattern.c_str());
          kjChildAdd(objectP2, nodeP);
        }

        char* alias = orionldContextItemAliasLookup(contextP, eP->type.c_str(), NULL, NULL);

        nodeP = kjString(orionldState.kjsonP, "type", alias);
        kjChildAdd(objectP2, nodeP);
        kjChildAdd(arrayP2, objectP2);
      }

      kjChildAdd(objectP, arrayP2);  // adding entities array to information object
    }

    // information::properties
    size = registrationP->dataProvided.propertyV.size();
    if (size != 0)
    {
      arrayP2 = kjArray(orionldState.kjsonP, "properties");

      for (unsigned int pIx = 0; pIx < size; pIx++)
      {
        char* alias = orionldContextItemAliasLookup(contextP, registrationP->dataProvided.propertyV[pIx].c_str(), NULL, NULL);
        nodeP = kjString(orionldState.kjsonP, NULL, alias);
        kjChildAdd(arrayP2, nodeP);
      }
      kjChildAdd(objectP, arrayP2);
    }

    // information::relationships
    size = registrationP->dataProvided.relationshipV.size();
    if (size != 0)
    {
      arrayP2 = kjArray(orionldState.kjsonP, "relationships");

      for (unsigned int rIx = 0; rIx < size; rIx++)
      {
        char* alias = orionldContextItemAliasLookup(contextP, registrationP->dataProvided.relationshipV[rIx].c_str(), NULL, NULL);
        nodeP = kjString(orionldState.kjsonP, NULL, alias);
        kjChildAdd(arrayP2, nodeP);
      }
      kjChildAdd(objectP, arrayP2);
    }
    kjChildAdd(arrayP, objectP);
  }
  kjChildAdd(topP, arrayP);


  //
  // observationalInterval
  //
  if (registrationP->observationInterval.start > 0)
  {
    // start
    if (numberToDate((time_t) registrationP->observationInterval.start, date, sizeof(date), &details) == false)
    {
      LM_E(("Error creating a stringified date for 'observationalInterval start'"));
      orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified observationalInterval date", details);
      return NULL;
    }
    objectP = kjObject(orionldState.kjsonP, "observationInterval");
    nodeP = kjString(orionldState.kjsonP, "start", date);
    kjChildAdd(objectP, nodeP);

    if (registrationP->observationInterval.end > 0)
    {
      // end
      if (numberToDate((time_t) registrationP->observationInterval.end, date, sizeof(date), &details) == false)
      {
        LM_E(("Error creating a stringified date for 'observationalInterval end'"));
        orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified observationalInterval date", details);
        return NULL;
      }
      nodeP = kjString(orionldState.kjsonP, "end", date);
      kjChildAdd(objectP, nodeP);
    }

    kjChildAdd(topP, objectP);
  }


  //
  // managementInterval
  //
  if (registrationP->managementInterval.start > 0)
  {
    // start
    if (numberToDate((time_t) registrationP->managementInterval.start, date, sizeof(date), &details) == false)
    {
      LM_E(("Error creating a stringified date for 'managementInterval start'"));
      orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified managementInterval date", details);
      return NULL;
    }
    objectP = kjObject(orionldState.kjsonP, "managementInterval");
    nodeP   = kjString(orionldState.kjsonP, "start", date);
    kjChildAdd(objectP, nodeP);

    if (registrationP->managementInterval.end > 0)
    {
      // end
      if (numberToDate((time_t) registrationP->managementInterval.end, date, sizeof(date), &details) == false)
      {
        LM_E(("Error creating a stringified date for 'managementInterval end'"));
        orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified managementInterval date", details);
        return NULL;
      }
      nodeP = kjString(orionldState.kjsonP, "end", date);
      kjChildAdd(objectP, nodeP);
    }

    kjChildAdd(topP, objectP);
  }


  //
  // location
  //
  if (registrationP->location.geoType != NULL)
  {
    objectP = kjObject(orionldState.kjsonP, "location");
    nodeP   = kjString(orionldState.kjsonP, "type", registrationP->location.geoType);
    kjChildAdd(objectP, nodeP);
    kjChildAdd(objectP, registrationP->location.coordsNodeP);
    kjChildAdd(topP, objectP);
  }


  //
  // observationSpace
  //
  if (registrationP->observationSpace.geoType != NULL)
  {
    objectP = kjObject(orionldState.kjsonP, "observationSpace");
    nodeP   = kjString(orionldState.kjsonP, "type", registrationP->observationSpace.geoType);
    kjChildAdd(objectP, nodeP);
    kjChildAdd(objectP, registrationP->observationSpace.coordsNodeP);
    kjChildAdd(topP, objectP);
  }


  //
  // operationSpace
  //
  if (registrationP->operationSpace.geoType != NULL)
  {
    objectP = kjObject(orionldState.kjsonP, "operationSpace");
    nodeP   = kjString(orionldState.kjsonP, "type", registrationP->operationSpace.geoType);
    kjChildAdd(objectP, nodeP);
    kjChildAdd(objectP, registrationP->operationSpace.coordsNodeP);
    kjChildAdd(topP, objectP);
  }


  //
  // Properties
  //
  if (registrationP->properties != NULL)
  {
    //
    // Loop over the properties to replace the expanded 'name' with its alias from the context
    //
    for (KjNode* nodeP = registrationP->properties->value.firstChildP; nodeP != NULL; nodeP = nodeP->next)
    {
      char* alias = orionldContextItemAliasLookup(orionldState.contextP, nodeP->name, NULL, NULL);

      if (alias != NULL)
        nodeP->name = alias;
    }

    //
    // Incorporate the children of registrationP->properties into topP, at the end
    //
    topP->lastChild->next = registrationP->properties->value.firstChildP;
    topP->lastChild       = registrationP->properties->lastChild;
  }

  return topP;
}
