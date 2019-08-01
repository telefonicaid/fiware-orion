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
* Author: Larysse Savanna
*/
extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjString, kjBoolean, ...
#include "kjson/kjParse.h"                                     // kjParse
}

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/httpHeaderAdd.h"                                // httpHeaderAdd, httpHeaderLinkAdd
#include "apiTypesV2/Registration.h"                           // Registration
#include "orionld/context/orionldContextLookup.h"              // orionldContextLookup
#include "orionld/context/orionldAliasLookup.h"                // orionldAliasLookup
#include "orionld/context/orionldCoreContext.h"                // orionldDefaultContext
#include "orionld/common/numberToDate.h"                       // numberToDate
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate, OrionldInternalError
#include "orionld/common/OrionldConnection.h"                  // orionldState
#include "orionld/kjTree/kjTreeFromRegistration.h"             // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeFromRegistration -
//
KjNode* kjTreeFromRegistration(ConnectionInfo* ciP, ngsiv2::Registration* registrationP)
{
  KjNode*       topP = kjObject(orionldState.kjsonP, NULL);
  KjNode*       objectP;
  KjNode*       objectP2;
  KjNode*       arrayP;
  KjNode*       arrayP2;
  KjNode*       nodeP;
  unsigned int  size;
  unsigned int  infoSize;
  unsigned int  i;
  unsigned int  j;

  // id
  nodeP = kjString(orionldState.kjsonP, "id", registrationP->id.c_str());
  kjChildAdd(topP, nodeP);

  // type
  nodeP = kjString(orionldState.kjsonP, "type", "ContextSource Registration");
  kjChildAdd(topP, nodeP);
  
  // name
  nodeP = kjString(orionldState.kjsonP, "name", registrationP->name.c_str());
  kjChildAdd(topP, nodeP);

  // description
  if (registrationP->description != "")
  {
    nodeP = kjString(orionldState.kjsonP, "description", registrationP->description.c_str());
    kjChildAdd(topP, nodeP);
  }

  // information
  // FIXME: Change do accept more than one member in the future

  infoSize = 1;
  arrayP = kjArray(orionldState.kjsonP, "information");

  for (i = 0; i < infoSize; i++)
  {
    objectP = kjObject(orionldState.kjsonP, NULL);

    // entities
    size = registrationP->dataProvided.entities.size();
    if (size != 0)
    {
      arrayP2 = kjArray(orionldState.kjsonP, "entities");

      for (j = 0; j < size; j++)
      {
        ngsiv2::EntID* eP = &registrationP->dataProvided.entities[j];

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

        char* alias = orionldAliasLookup(orionldState.contextP, eP->type.c_str());

        nodeP = kjString(orionldState.kjsonP, "type", alias);
        kjChildAdd(objectP2, nodeP);
        kjChildAdd(arrayP2, objectP2);
      }

      kjChildAdd(objectP, arrayP2);  // adding entities array to information object
    }

    // properties (attributes)
    size = registrationP->dataProvided.attributes.size();
    if (size != 0)
    {
      arrayP2 = kjArray(orionldState.kjsonP, "attrs");

      for (j = 0; j < size; j++)
      {
        nodeP = kjString(orionldState.kjsonP, NULL, registrationP->dataProvided.attributes[j].c_str());

        kjChildAdd(arrayP2, nodeP);
      }
      kjChildAdd(objectP, arrayP2);
    }
    kjChildAdd(arrayP, objectP);
  }
  kjChildAdd(topP, arrayP);

  // observationalInterval (can be empty)
  if(registrationP->observationInterval.start != "")
  {
    objectP = kjObject(orionldState.kjsonP, "observationInterval");
    nodeP = kjString(orionldState.kjsonP, "start", registrationP->observationInterval.start.c_str());
    kjChildAdd(objectP, nodeP);

    if(registrationP->observationInterval.end != "")
    {
      nodeP = kjString(orionldState.kjsonP, "end", registrationP->observationInterval.end.c_str());
      kjChildAdd(objectP, nodeP);
    }
    kjChildAdd(topP, objectP);
  }
  
  // managementInterval : type = TimeInterval (can be empty)
  if(registrationP->managementInterval.start != "")
  {
    objectP = kjObject(orionldState.kjsonP, "managementInterval");
    nodeP = kjString(orionldState.kjsonP, "start", registrationP->managementInterval.start.c_str());
    kjChildAdd(objectP, nodeP);

    if(registrationP->managementInterval.end != "")
    {
      nodeP = kjString(orionldState.kjsonP, "end", registrationP->managementInterval.end.c_str());
      kjChildAdd(objectP, nodeP);
    }
  }
  kjChildAdd(topP, objectP);

  // location

  // observationSpace

  // operationSpace

  // expires
  char*            details;
  char             date[64];

  if (numberToDate((time_t) registrationP->expires, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'expires'"));
    orionldErrorResponseCreate(ciP, OrionldInternalError, "unable to create a stringified date", details, OrionldDetailsEntity);
    return NULL;
  }
  nodeP = kjString(orionldState.kjsonP, "expiress", date);
  kjChildAdd(topP, nodeP);

  // endpoint
  nodeP = kjString(orionldState.kjsonP, "endpoint", registrationP->endpoint.c_str());
  kjChildAdd(topP, nodeP);
  

  return topP;
}
