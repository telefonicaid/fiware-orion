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

extern void         arrayToKjTree(KjNode* containerP, mongo::BSONArray* bsonArrayP, char** titleP, char** detailsP);
extern const char*  mongoTypeName(mongo::BSONType type);



void objectToKjTree(KjNode* containerP, mongo::BSONObj* bsonObjP, char** titleP, char** detailsP)
{
  for (mongo::BSONObj::iterator iter = bsonObjP->begin(); iter.more();)
  {
    mongo::BSONElement be       = iter.next();
    mongo::BSONType    type     = be.type();
    KjNode*            nodeP    = NULL;
    const char*        nodeName = be.fieldName();

    if (type == mongo::String)
      nodeP = kjString(orionldState.kjsonP, nodeName, be.String().c_str());
    else if (type == mongo::Bool)
      nodeP = kjBoolean(orionldState.kjsonP, nodeName, be.Bool());
    else if (type == mongo::NumberDouble)
      nodeP = kjFloat(orionldState.kjsonP, nodeName, be.Number());
    else if (type == mongo::jstNULL)
      nodeP = kjNull(orionldState.kjsonP, nodeName);
    else if (type == mongo::Object)
    {
      mongo::BSONObj bo = be.embeddedObject();
      nodeP = kjObject(orionldState.kjsonP, nodeName);
      objectToKjTree(nodeP, &bo, titleP, detailsP);
    }
    else if (type == mongo::Array)
    {
      mongo::BSONArray ba = (mongo::BSONArray) be.embeddedObject();
      nodeP = kjArray(orionldState.kjsonP, nodeName);
      arrayToKjTree(nodeP, &ba, titleP, detailsP);
    }
    else
    {
      LM_E(("Unsupported mongo type %d for field '%s'", type, nodeName));
      continue;
    }

    kjChildAdd(containerP, nodeP);
  }
}


void arrayToKjTree(KjNode* containerP, mongo::BSONArray* bsonArrayP, char** titleP, char** detailsP)
{
  for (mongo::BSONObj::iterator iter = bsonArrayP->begin(); iter.more();)
  {
    mongo::BSONElement be       = iter.next();
    mongo::BSONType    type     = be.type();
    KjNode*            nodeP    = NULL;
    char*              nodeName = NULL;

    if (type == mongo::String)
      nodeP = kjString(orionldState.kjsonP, nodeName, be.String().c_str());
    else if (type == mongo::Bool)
      nodeP = kjBoolean(orionldState.kjsonP, nodeName, be.Bool());
    else if (type == mongo::NumberDouble)
      nodeP = kjFloat(orionldState.kjsonP, nodeName, be.Number());
    else if (type == mongo::jstNULL)
      nodeP = kjNull(orionldState.kjsonP, nodeName);
    else if (type == mongo::Object)
    {
      mongo::BSONObj bo = be.embeddedObject();
      nodeP = kjObject(orionldState.kjsonP, nodeName);
      objectToKjTree(nodeP, &bo, titleP, detailsP);
    }
    else if (type == mongo::Array)
    {
      mongo::BSONArray ba = (mongo::BSONArray) be.embeddedObject();
      nodeP = kjArray(orionldState.kjsonP, nodeName);
      arrayToKjTree(nodeP, &ba, titleP, detailsP);
    }
    else
    {
      LM_E(("Unsupported mongo type %d for field '%s'", type, nodeName));
      continue;
    }

    kjChildAdd(containerP, nodeP);
  }
}


KjNode* mongoCppLegacyDataToKjTree(const void* dataP, bool isArray, char** titleP, char** detailsP)
{
  KjNode* rootP;

  if (isArray == false)
  {
    rootP = kjObject(orionldState.kjsonP, NULL);
    objectToKjTree(rootP, (mongo::BSONObj*) dataP, titleP, detailsP);
  }
  else
  {
    rootP = kjArray(orionldState.kjsonP, NULL);
    arrayToKjTree(rootP, (mongo::BSONArray*) dataP, titleP, detailsP);
  }

  return rootP;
}



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
