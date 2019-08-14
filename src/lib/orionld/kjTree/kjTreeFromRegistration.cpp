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
  char          date[128];
  char*         details;
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
  if (registrationP->descriptionProvided == true)
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

    // properties
    size = registrationP->dataProvided.propertyV.size();
    if (size != 0)
    {
      arrayP2 = kjArray(orionldState.kjsonP, "properties");

      for (j = 0; j < size; j++)
      {
        nodeP = kjString(orionldState.kjsonP, NULL, registrationP->dataProvided.propertyV[j].c_str());
        kjChildAdd(arrayP2, nodeP);
      }
      kjChildAdd(objectP, arrayP2);
    }

    // relationships
    size = registrationP->dataProvided.relationshipV.size();
    if(size != 0)
    {
      arrayP2 = kjArray(orionldState.kjsonP, "relationships");

      for (j = 0; j < size; j++)
      {
        nodeP = kjString(orionldState.kjsonP, NULL, registrationP->dataProvided.relationshipV[j].c_str());
        kjChildAdd(arrayP2, nodeP);
      }
      kjChildAdd(objectP, arrayP2);
    }
    kjChildAdd(arrayP, objectP);
  }
  kjChildAdd(topP, arrayP);
  
  // observationalInterval
  // start
  if(numberToDate((time_t) registrationP->observationInterval.start, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'observationalInterval start'"));
    orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified observationalInterval date", details, OrionldDetailsEntity);
    return NULL;
  }
  objectP = kjObject(orionldState.kjsonP, "observationInterval");
  nodeP = kjString(orionldState.kjsonP, "start", date);
  kjChildAdd(objectP, nodeP);

  // end
  if(numberToDate((time_t) registrationP->observationInterval.end, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'observationalInterval end'"));
    orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified observationalInterval date", details, OrionldDetailsEntity);
    return NULL;
  }
  nodeP = kjString(orionldState.kjsonP, "end", date);
  kjChildAdd(objectP, nodeP);
  kjChildAdd(topP, objectP);

  
  // managementInterval
  // start
  if(numberToDate((time_t) registrationP->managementInterval.start, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'managementInterval start'"));
    orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified managementInterval date", details, OrionldDetailsEntity);
    return NULL;
  }
  objectP = kjObject(orionldState.kjsonP, "managementInterval");
  nodeP = kjString(orionldState.kjsonP, "start", date);
  kjChildAdd(objectP, nodeP);

  // end
  if(numberToDate((time_t) registrationP->managementInterval.end, date, sizeof(date), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'managementInterval end'"));
    orionldErrorResponseCreate(OrionldInternalError, "Unable to create a stringified managementInterval date", details, OrionldDetailsEntity);
    return NULL;
  }
  nodeP = kjString(orionldState.kjsonP, "end", date);
  kjChildAdd(objectP, nodeP);
  
  kjChildAdd(topP, objectP);
  
  // location

  // observationSpace

  // operationSpace

  // expires
  char             dateExp[64];

  if (numberToDate((time_t) registrationP->expires, dateExp, sizeof(dateExp), &details) == false)
  {
    LM_E(("Error creating a stringified date for 'expires'"));
    orionldErrorResponseCreate(OrionldInternalError, "unable to create a stringified date", details, OrionldDetailsEntity);
    return NULL;
  }
  nodeP = kjString(orionldState.kjsonP, "expires", dateExp);
  kjChildAdd(topP, nodeP);

  // endpoint
  nodeP = kjString(orionldState.kjsonP, "endpoint", registrationP->provider.http.url.c_str());
  kjChildAdd(topP, nodeP);
  
  return topP;
}
