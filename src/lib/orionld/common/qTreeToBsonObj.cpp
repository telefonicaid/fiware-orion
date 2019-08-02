/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "mongo/client/dbclient.h"                             // mongo::BSONObj

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/QNode.h"                              // QNode
#include "orionld/common/qTreeToBsonObj.h"                     // Own interface



// ----------------------------------------------------------------------------
//
// qTreeToBsonObj - 
//
bool qTreeToBsonObj(QNode* treeP, mongo::BSONObjBuilder* topBsonObjP, char** titleP, char** detailsP)
{
  LM_TMP(("Q: Got a tree of type %s", qNodeType(treeP->type)));

  if (treeP->type == QNodeOr)
  {
    // { "%or": [ { }, { }, ... { } ] }
    mongo::BSONArrayBuilder arrBuilder;

    for (QNode* qNodeP = treeP->value.children; qNodeP != NULL; qNodeP = qNodeP->next)
    {
      mongo::BSONObjBuilder arrItemObject;
      if (qTreeToBsonObj(qNodeP, &arrItemObject, titleP, detailsP) == false)
        return false;

      arrBuilder.append(arrItemObject.obj());
    }
    topBsonObjP->append("$or", arrBuilder.arr());
  }
  else if (treeP->type == QNodeAnd)
  {
    for (QNode* qNodeP = treeP->value.children; qNodeP != NULL; qNodeP = qNodeP->next)
    {
      if (qTreeToBsonObj(qNodeP, topBsonObjP, titleP, detailsP) == false)
        return false;
    }
  }
  else if (treeP->type == QNodeEQ)
  {
    QNode*                 leftP  = treeP->value.children;
    QNode*                 rightP = leftP->next;

    LM_TMP(("Q: Creating EQ-Object"));
    if (rightP->type == QNodeIntegerValue)
      topBsonObjP->append(leftP->value.v, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)
      topBsonObjP->append(leftP->value.v, rightP->value.f);
    else if (rightP->type == QNodeStringValue)
      topBsonObjP->append(leftP->value.v, rightP->value.s);

    // LM_TMP(("Q: Added EQ-Object to top-level object: %s", topBsonObjP->obj().toString().c_str()));
  }
  else if ((treeP->type == QNodeGT) || (treeP->type == QNodeGE) || (treeP->type == QNodeLT) || (treeP->type == QNodeLE))
  {
    QNode*                 leftP  = treeP->value.children;
    QNode*                 rightP = leftP->next;
    char*                  op;
    mongo::BSONObjBuilder  gtObj;
    mongo::BSONObjBuilder  compObj;

    if      (treeP->type == QNodeGT)  op = (char*) "$gt";
    else if (treeP->type == QNodeGE)  op = (char*) "$gte";
    else if (treeP->type == QNodeLT)  op = (char*) "$lt";
    else if (treeP->type == QNodeLE)  op = (char*) "$lte";

    LM_TMP(("Q: Creating GT-Object"));
    if      (rightP->type == QNodeIntegerValue)  gtObj.append(op, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)    gtObj.append(op, rightP->value.f);
    else if (rightP->type == QNodeStringValue)   gtObj.append(op, rightP->value.s);

    LM_TMP(("Q: Adding GT-Object to top-level object: var: '%s'", leftP->value.v));
    topBsonObjP->append(leftP->value.v, gtObj.obj());
    LM_TMP(("Q: Added GT-Object to top-level object"));
  }
  else
  {
    // ERROR
  }

  LM_TMP(("Q: Done"));
  return true;
}
