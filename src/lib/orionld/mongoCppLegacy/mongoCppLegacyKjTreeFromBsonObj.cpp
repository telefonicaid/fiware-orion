/*
*
* Copyright 2019 FIWARE Foundation e.V.
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
#include <string>                                                    // std::string

#include "mongo/client/dbclient.h"                                   // mongo::BSONObj

extern "C"
{
#include "kalloc/kaStrdup.h"                                         // kaStrdup
#include "kjson/KjNode.h"                                            // KjNode
#include "kjson/kjParse.h"                                           // kjParse
#include "kjson/kjBuilder.h"                                         // kjBuilder
}

#include "logMsg/logMsg.h"                                           // LM_*
#include "logMsg/traceLevels.h"                                      // Lmt*

#include "orionld/common/orionldState.h"                             // orionldState, orionldStateDelayedFreeEnqueue
#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeFromBsonObj.h"  // Own interface



// -----------------------------------------------------------------------------
//
// mongoCppLegacyKjTreeFromBsonObj -
//
KjNode* mongoCppLegacyKjTreeFromBsonObj(const void* dataP, char** titleP, char** detailsP)
{
  mongo::BSONObj* bsonObjP = (mongo::BSONObj*) dataP;
  KjNode*         treeP    = NULL;

  std::string jsonString = bsonObjP->jsonString();

  if (jsonString == "")
  {
    *titleP   = (char*) "Internal Error";
    *detailsP = (char*) "Error creating JSON from BSONObj";
  }
  else
  {
    orionldState.jsonBuf = strdup(jsonString.c_str());
    if (orionldPhase != OrionldPhaseStartup)
      orionldStateDelayedFreeEnqueue(orionldState.jsonBuf);  // automatic free if not in startup phase

    treeP = kjParse(orionldState.kjsonP, orionldState.jsonBuf);
    if (treeP == NULL)
    {
      *titleP   = (char*) "Internal Error";
      *detailsP = (char*) "Error parsing JSON output from bson_as_json";
    }
  }

  return treeP;
}
