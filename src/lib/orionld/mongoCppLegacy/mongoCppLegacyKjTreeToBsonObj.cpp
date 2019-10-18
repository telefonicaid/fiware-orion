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
#include "mongo/client/dbclient.h"                                 // MongoDB C++ Client Legacy Driver

extern "C"
{
#include "kjson/KjNode.h"                                          // KjNode
}

#include "logMsg/logMsg.h"                                         // LM_*
#include "logMsg/traceLevels.h"                                    // Lmt*
#include "orionld/mongoCppLegacy/mongoCppLegacyKjTreeToBsonObj.h"  // Own interface



// -----------------------------------------------------------------------------
//
// Forward declarations
//
static void kjTreeToBsonArray(KjNode* nodeP, mongo::BSONArrayBuilder* arrayBuilderP);



// -----------------------------------------------------------------------------
//
// kjTreeToBsonObject -
//
static void kjTreeToBsonObject(KjNode* nodeP, mongo::BSONObjBuilder* objBuilderP)
{
  if (nodeP->type == KjArray)
  {
    mongo::BSONArrayBuilder bab;

    for (KjNode* arrayItemP = nodeP->value.firstChildP; arrayItemP != NULL; arrayItemP = arrayItemP->next)
      kjTreeToBsonArray(arrayItemP, &bab);

    objBuilderP->append(nodeP->name, bab.arr());
  }
  else if (nodeP->type == KjObject)
  {
    mongo::BSONObjBuilder bob;

    for (KjNode* objItemP = nodeP->value.firstChildP; objItemP != NULL; objItemP = objItemP->next)
      kjTreeToBsonObject(objItemP, &bob);

    objBuilderP->append(nodeP->name, bob.obj());
  }
  else if (nodeP->type == KjString)
    objBuilderP->append(nodeP->name, nodeP->value.s);
  else if (nodeP->type == KjFloat)
    objBuilderP->append(nodeP->name, nodeP->value.f);
  else if (nodeP->type == KjInt)
    objBuilderP->append(nodeP->name, (int) nodeP->value.i);  // long long => int to avoid "NumberLong(i)" in mongo ...
  else if (nodeP->type == KjBoolean)
    objBuilderP->appendBool(nodeP->name, nodeP->value.b);
}



// -----------------------------------------------------------------------------
//
// kjTreeToBsonArray -
//
static void kjTreeToBsonArray(KjNode* nodeP, mongo::BSONArrayBuilder* arrayBuilderP)
{
  if (nodeP->type == KjArray)
  {
    mongo::BSONArrayBuilder bab;

    for (KjNode* arrayItemP = nodeP->value.firstChildP; arrayItemP != NULL; arrayItemP = arrayItemP->next)
      kjTreeToBsonArray(arrayItemP, &bab);

    arrayBuilderP->append(bab.arr());
  }
  else if (nodeP->type == KjObject)
  {
    mongo::BSONObjBuilder bob;

    for (KjNode* objItemP = nodeP->value.firstChildP; objItemP != NULL; objItemP = objItemP->next)
      kjTreeToBsonObject(objItemP, &bob);

    arrayBuilderP->append(bob.obj());
  }
  else if (nodeP->type == KjString)
    arrayBuilderP->append(nodeP->value.s);
  else if (nodeP->type == KjFloat)
    arrayBuilderP->append(nodeP->value.f);
  else if (nodeP->type == KjInt)
    arrayBuilderP->append((int) nodeP->value.i);  // long long => int to avoid "NumberLong(i)" in mongo ...
  else if (nodeP->type == KjBoolean)
    arrayBuilderP->appendBool(nodeP->value.b);
}



// -----------------------------------------------------------------------------
//
// mongoCppLegacyKjTreeToBsonObj -
//
void mongoCppLegacyKjTreeToBsonObj(KjNode* nodeP, void* dbDataP)
{
  mongo::BSONObj* bsonObjP = (mongo::BSONObj*) dbDataP;

  if (nodeP->type == KjObject)
  {
    LM_TMP(("BOB: %s is an OBJECT", nodeP->name));
    mongo::BSONObjBuilder bob;

    for (KjNode* objItemP = nodeP->value.firstChildP; objItemP != NULL; objItemP = objItemP->next)
    {
      LM_TMP(("BOB: Adding child '%s'", objItemP->name));
      kjTreeToBsonObject(objItemP, &bob);
    }

    LM_TMP(("BOB: bsonObjP at %p", bsonObjP));
    *bsonObjP = bob.obj();
    LM_TMP(("BOB: Still here ...", nodeP->name));
  }
  else if (nodeP->type == KjArray)
  {
    LM_TMP(("BOB: %s is an ARRAY", nodeP->name));
    mongo::BSONArrayBuilder bab;

    for (KjNode* arrayItemP = nodeP->value.firstChildP; arrayItemP != NULL; arrayItemP = arrayItemP->next)
      kjTreeToBsonArray(arrayItemP, &bab);

    *bsonObjP = bab.arr();
  }
  else
  {
    mongo::BSONObjBuilder bob;

    LM_TMP(("BOB: %s is neither a KjObject nor a KjArray - creating mongo::BSONObjBuilder and calling kjTreeToBsonObject", nodeP->name));
    kjTreeToBsonObject(nodeP, &bob);
    LM_TMP(("BOB: converting the mongo::BSONObjBuilder into a mongo::BSONObj"));
    *bsonObjP = bob.obj();
  }
}
