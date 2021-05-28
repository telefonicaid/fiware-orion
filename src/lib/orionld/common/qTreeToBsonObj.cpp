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
#include <regex.h>                                             // regcomp

#include "mongo/client/dbclient.h"                             // mongo::BSONObj

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/QNode.h"                              // QNode
#include "orionld/common/qTreeToBsonObj.h"                     // Own interface


//
// FIXME: This entire function depends on Mongo C++ Legacy Client and must be moved to
//        src/lib/orionld/mongoCppLegacy/mongoCppLegacyQtreeToDbObject.h/cpp.
//
//        To be DB-agnostic, its signature could be like this:
//
//          void* mongoCppLegacyQtreeToDbObject(QNode* treeP, char** titleP, char** detailsP)
//          {
//            mongo::BSONObjBuilder* objBuilderP = kaAlloc();
//            ...
//            return (void*) objBuilderP;
//          }
//


// ----------------------------------------------------------------------------
//
// qTreeToBsonObj -
//
bool qTreeToBsonObj(QNode* treeP, mongo::BSONObjBuilder* topBsonObjP, char** titleP, char** detailsP)
{
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
  else if ((treeP->type == QNodeNotExists) || (treeP->type == QNodeExists))
  {
    QNode*                 rightP = treeP->value.children;
    mongo::BSONObjBuilder  nexObj;
    bool                   value = (treeP->type == QNodeNotExists)? false : true;

    nexObj.append("$exists", value);
    topBsonObjP->append(rightP->value.v, nexObj.obj());
  }
  else if (treeP->type == QNodeMatch)
  {
    QNode*                 leftP  = treeP->value.children;
    QNode*                 rightP = leftP->next;
    mongo::BSONObjBuilder  matchObj;

    matchObj.append("$regex", rightP->value.s);
    topBsonObjP->append(leftP->value.v, matchObj.obj());
  }
  else if (treeP->type == QNodeEQ)
  {
    QNode* leftP  = treeP->value.children;
    QNode* rightP = leftP->next;

    if (rightP->type == QNodeIntegerValue)
      topBsonObjP->append(leftP->value.v, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)
      topBsonObjP->append(leftP->value.v, rightP->value.f);
    else if (rightP->type == QNodeStringValue)
      topBsonObjP->append(leftP->value.v, rightP->value.s);
    else if (rightP->type == QNodeTrueValue)
      topBsonObjP->append(leftP->value.v, true);
    else if (rightP->type == QNodeFalseValue)
      topBsonObjP->append(leftP->value.v, false);
    else if (rightP->type == QNodeRange)
    {
      QNode*                 lowerLimitNodeP = rightP->value.children;
      QNode*                 upperLimitNodeP = lowerLimitNodeP->next;
      mongo::BSONObjBuilder  limitObj;

      if (lowerLimitNodeP->type == QNodeIntegerValue)
      {
        limitObj.append("$gte", lowerLimitNodeP->value.i);
        limitObj.append("$lte", upperLimitNodeP->value.i);
      }
      else if (lowerLimitNodeP->type == QNodeFloatValue)
      {
        limitObj.append("$gte", lowerLimitNodeP->value.f);
        limitObj.append("$lte", upperLimitNodeP->value.f);
      }
      else if (lowerLimitNodeP->type == QNodeStringValue)
      {
        limitObj.append("$gte", lowerLimitNodeP->value.s);
        limitObj.append("$lte", upperLimitNodeP->value.s);
      }

      topBsonObjP->append(leftP->value.v, limitObj.obj());
    }
    else if (rightP->type == QNodeComma)
    {
      mongo::BSONObjBuilder    inObj;
      mongo::BSONArrayBuilder  listArr;

      for (QNode* listItemP = rightP->value.children; listItemP != NULL; listItemP = listItemP->next)
      {
        if (listItemP->type == QNodeIntegerValue)
          listArr.append(listItemP->value.i);
        else if (listItemP->type == QNodeFloatValue)
          listArr.append(listItemP->value.f);
        else if (listItemP->type == QNodeStringValue)
          listArr.append(listItemP->value.s);
      }
      inObj.append("$in", listArr.arr());
      topBsonObjP->append(leftP->value.v, inObj.obj());
    }
  }
  else if (treeP->type == QNodeNE)
  {
    //
    // Extract from the Mongo Manual:
    //   $ne selects the documents where the value of the field is not equal to the specified value.
    //   This includes documents that do not contain the field.
    //
    // This means we will needan $exists also :(
    //
    // Orion does this:
    //   varName: { $exists: true, $nin: [ 11.0 ] }
    //
    // I want to do this:
    //   varName: { $exists: true, $ne: VALUE }
    //
    QNode*                 leftP  = treeP->value.children;
    QNode*                 rightP = leftP->next;
    mongo::BSONObjBuilder  neObj;
    mongo::BSONObjBuilder  existsObj;

    existsObj.append("$exists", true);
    topBsonObjP->append(leftP->value.v, existsObj.obj());

    if      (rightP->type == QNodeIntegerValue) neObj.append("$ne", rightP->value.i);
    else if (rightP->type == QNodeFloatValue)   neObj.append("$ne", rightP->value.f);
    else if (rightP->type == QNodeStringValue)  neObj.append("$ne", rightP->value.s);
    else if (rightP->type == QNodeTrueValue)    neObj.append("$ne", true);
    else if (rightP->type == QNodeFalseValue)   neObj.append("$ne", false);

    if ((rightP->type != QNodeRange) && (rightP->type != QNodeComma))
      topBsonObjP->append(leftP->value.v, neObj.obj());

    if (rightP->type == QNodeRange)
    {
      //
      // A1=12..24:
      // { "A1": { "$exists": true } }, { "$or": [ { "A1": { "$lt" 12 } }, { "A1": { "$gt", 24 } } ] }
      //
      mongo::BSONArrayBuilder  orVec;
      mongo::BSONObjBuilder    lowerLimitObj;
      mongo::BSONObjBuilder    a1LowerLimitObj;
      mongo::BSONObjBuilder    upperLimitObj;
      mongo::BSONObjBuilder    a1UpperLimitObj;
      QNode*                   lowerLimitNodeP = rightP->value.children;
      QNode*                   upperLimitNodeP = lowerLimitNodeP->next;

      if (lowerLimitNodeP->type == QNodeIntegerValue)
      {
        lowerLimitObj.append("$lt", lowerLimitNodeP->value.i);
        upperLimitObj.append("$gt", upperLimitNodeP->value.i);
      }
      else if (lowerLimitNodeP->type == QNodeFloatValue)
      {
        lowerLimitObj.append("$lt", lowerLimitNodeP->value.f);
        upperLimitObj.append("$gt", upperLimitNodeP->value.f);
      }
      else if (lowerLimitNodeP->type == QNodeStringValue)
      {
        lowerLimitObj.append("$lt", lowerLimitNodeP->value.s);
        upperLimitObj.append("$gt", upperLimitNodeP->value.s);
      }

      a1LowerLimitObj.append(leftP->value.v, lowerLimitObj.obj());
      a1UpperLimitObj.append(leftP->value.v, upperLimitObj.obj());
      orVec.append(a1LowerLimitObj.obj());
      orVec.append(a1UpperLimitObj.obj());

      topBsonObjP->append("$or", orVec.arr());
    }
    else if (rightP->type == QNodeComma)
    {
      mongo::BSONArrayBuilder  ninVec;
      mongo::BSONObjBuilder    ninObj;

      for (QNode* ninValueP = rightP->value.children; ninValueP != NULL; ninValueP = ninValueP->next)
      {
        //
        // FIXME: I could skip this if for each loop.
        //        We already know that all "comma values" are of the same type - checked by qParse.
        //
        // So, this would be faster in execution:
        //   if (rightP->value.children->type == QNodeIntegerValue)
        //   {
        //     for (Qnode* ninValueP = rightP->value.children; ninValueP != NULL; ninValueP = ninValueP->next)
        //     {
        //       ninVec.append(ninValueP->value.i);
        //     }
        //   }
        //   else if (ninValueP->type == QNodeFloatValue) ...
        //
        if (ninValueP->type == QNodeIntegerValue)
          ninVec.append(ninValueP->value.i);
        else if (ninValueP->type == QNodeFloatValue)
          ninVec.append(ninValueP->value.f);
        else if (ninValueP->type == QNodeStringValue)
          ninVec.append(ninValueP->value.s);
      }

      ninObj.append("$nin", ninVec.arr());
      topBsonObjP->append(leftP->value.v, ninObj.obj());
    }
    else if (treeP->type == QNodeMatch)
    {
      QNode*                 leftP  = treeP->value.children;
      QNode*                 rightP = leftP->next;
      mongo::BSONObjBuilder  notObj;
      mongo::BSONObjBuilder  matchObj;

      matchObj.append("$regex", rightP->value.s);
      notObj.append("$not", matchObj.obj());
      topBsonObjP->append(leftP->value.v, notObj.obj());
    }
  }
  else if ((treeP->type == QNodeGT) || (treeP->type == QNodeGE) || (treeP->type == QNodeLT) || (treeP->type == QNodeLE))
  {
    QNode*                 leftP  = treeP->value.children;
    QNode*                 rightP = leftP->next;
    char*                  op;
    mongo::BSONObjBuilder  gtObj;

    if      (treeP->type == QNodeGT)  op = (char*) "$gt";
    else if (treeP->type == QNodeGE)  op = (char*) "$gte";
    else if (treeP->type == QNodeLT)  op = (char*) "$lt";
    else                              op = (char*) "$lte";

    if      (rightP->type == QNodeIntegerValue)  gtObj.append(op, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)    gtObj.append(op, rightP->value.f);
    else if (rightP->type == QNodeStringValue)   gtObj.append(op, rightP->value.s);
    else if (rightP->type == QNodeTrueValue)     gtObj.append(op, true);
    else if (rightP->type == QNodeFalseValue)    gtObj.append(op, false);

    topBsonObjP->append(leftP->value.v, gtObj.obj());
  }
  else
  {
    // ERROR
  }

  return true;
}
