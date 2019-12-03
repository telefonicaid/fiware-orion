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
#include <bson/bson.h>                                         // BSON

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "orionld/common/QNode.h"                              // QNode
#include "orionld/common/qTreeToBson.h"                        // Own interface



// ----------------------------------------------------------------------------
//
// qTreeToBson -
//
bool qTreeToBson(QNode* treeP, bson_t* bsonP, char** titleP, char** detailsP)
{
  if (treeP->type == QNodeOr)
  {
    // { "%or": [ { }, { }, ... { } ] }
    bson_t orArrayBson;

    bson_append_array_begin(bsonP, "$or", 3, &orArrayBson);

    for (QNode* qNodeP = treeP->value.children; qNodeP != NULL; qNodeP = qNodeP->next)
    {
      bson_t orItemBson;

      bson_append_document_begin(&orArrayBson, "", 0, &orItemBson);
      if (qTreeToBson(qNodeP, &orItemBson, titleP, detailsP) == false)
        return false;
      bson_append_document_end(&orArrayBson, &orItemBson);
    }
    bson_append_array_end(bsonP, &orArrayBson);
  }
  else if (treeP->type == QNodeAnd)
  {
    // { { }, { }, ... { } }

    for (QNode* qNodeP = treeP->value.children; qNodeP != NULL; qNodeP = qNodeP->next)
    {
      if (qTreeToBson(qNodeP, bsonP, titleP, detailsP) == false)
        return false;
    }
  }
  else if ((treeP->type == QNodeGT) || (treeP->type == QNodeGE))
  {
    QNode*       leftP  = treeP->value.children;
    QNode*       rightP = leftP->next;
    const char*  op     = (treeP->type == QNodeGT)? "$gt" : "$gte";
    int          opLen  = (treeP->type == QNodeGT)?     3 : 4;
    bson_t       gtBson;

    bson_init(&gtBson);

    bson_append_document_begin(bsonP, leftP->value.v, -1, &gtBson);
    if (rightP->type == QNodeStringValue)
      bson_append_utf8(&gtBson, op, opLen, rightP->value.s, -1);
    else if (rightP->type == QNodeIntegerValue)
      bson_append_int64(&gtBson, op, opLen, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)
      bson_append_int64(&gtBson, op, opLen, rightP->value.f);
    else
    {
      *titleP   = (char*) "ngsi-ld query language: invalid token after GT";
      *detailsP = (char*) qNodeType(rightP->type);
      return false;
    }

    bson_append_document_end(bsonP, &gtBson);
  }
  else if ((treeP->type == QNodeLT) || (treeP->type == QNodeLE))
  {
    QNode*       leftP  = treeP->value.children;
    QNode*       rightP = leftP->next;
    const char*  op     = (treeP->type == QNodeLT)? "$lt" : "$lte";
    int          opLen  = (treeP->type == QNodeLT)?     3 : 4;
    bson_t       ltBson;

    bson_init(&ltBson);

    bson_append_document_begin(bsonP, leftP->value.v, -1, &ltBson);
    if (rightP->type == QNodeStringValue)
      bson_append_utf8(&ltBson, op, opLen, rightP->value.s, -1);
    else if (rightP->type == QNodeIntegerValue)
      bson_append_int64(&ltBson, op, opLen, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)
      bson_append_int64(&ltBson, op, opLen, rightP->value.f);
    else
    {
      *titleP   = (char*) "ngsi-ld query language: invalid token after LT";
      *detailsP = (char*) qNodeType(rightP->type);
      return false;
    }

    bson_append_document_end(bsonP, &ltBson);
  }
  else if (treeP->type == QNodeEQ)
  {
    QNode* leftP  = treeP->value.children;
    QNode* rightP = leftP->next;

    if (rightP->type == QNodeStringValue)
      bson_append_utf8(bsonP, leftP->value.v, -1, rightP->value.s, -1);
    else if (rightP->type == QNodeIntegerValue)
      bson_append_int64(bsonP, leftP->value.v, -1, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)
      bson_append_double(bsonP, leftP->value.v, -1, rightP->value.f);
    else if (rightP->type == QNodeTrueValue)
      bson_append_bool(bsonP, leftP->value.v, -1, true);
    else if (rightP->type == QNodeFalseValue)
      bson_append_bool(bsonP, leftP->value.v, -1, false);
    else if (rightP->type == QNodeRange)
    {
      QNode* lowerLimitNodeP = rightP->value.children;
      QNode* upperLimitNodeP = lowerLimitNodeP->next;
      bson_t limitBson;

      bson_init(&limitBson);

      if (lowerLimitNodeP->type == QNodeIntegerValue)
      {
        bson_append_document_begin(bsonP, leftP->value.v, -1, &limitBson);
        bson_append_int64(&limitBson, "$gte", 4, lowerLimitNodeP->value.i);
        bson_append_int64(&limitBson, "$lte", 4, upperLimitNodeP->value.i);
        bson_append_document_end(bsonP, &limitBson);
      }
      else if (lowerLimitNodeP->type == QNodeFloatValue)
      {
        bson_append_document_begin(bsonP, leftP->value.v, -1, &limitBson);
        bson_append_double(&limitBson, "$gte", 4, lowerLimitNodeP->value.f);
        bson_append_double(&limitBson, "$lte", 4, upperLimitNodeP->value.f);
        bson_append_document_end(bsonP, &limitBson);
      }
      else if (lowerLimitNodeP->type == QNodeStringValue)
      {
        bson_append_document_begin(bsonP, leftP->value.v, -1, &limitBson);
        bson_append_utf8(&limitBson, "$gte", 4, lowerLimitNodeP->value.s, -1);
        bson_append_utf8(&limitBson, "$lte", 4, upperLimitNodeP->value.s, -1);
        bson_append_document_end(bsonP, &limitBson);
      }
    }
    else if (rightP->type == QNodeComma)
    {
      // { "https://schema=org/xxx/P1.md.P100" : { "$in": [ 1, 2, 3 ] } }
      bson_t inBson;
      bson_t commaArrayBson;

      bson_append_document_begin(bsonP, leftP->value.v, -1, &inBson);

      bson_append_array_begin(&inBson, "$in", -1, &commaArrayBson);

      for (QNode* valueNodeP = rightP->value.children; valueNodeP != NULL; valueNodeP = valueNodeP->next)
      {
        if (valueNodeP->type == QNodeIntegerValue)
          bson_append_int32(&commaArrayBson, "0", 1, valueNodeP->value.i);
        else if (valueNodeP->type == QNodeFloatValue)
          bson_append_double(&commaArrayBson, "0", 1, valueNodeP->value.f);
        else if (valueNodeP->type == QNodeStringValue)
          bson_append_utf8(&commaArrayBson, "0", 1, valueNodeP->value.s, -1);
      }
      bson_append_array_end(&inBson, &commaArrayBson);
      bson_append_document_end(bsonP, &inBson);
    }
    else
    {
      *titleP   = (char*) "ngsi-ld query language: invalid token after EQ";
      *detailsP = (char*) qNodeType(rightP->type);
      return false;
    }
  }
  else if (treeP->type == QNodeNE)
  {
    //
    // Instead of implementing all $ne, $nin, and reversed checks for ranges, etc,
    // we simply change the operator from NE to EQ and add a "$not" before the expression.
    // Should work.
    // Not sure about efficiency though ...
    //
    // Is:
    //   { "P1": { "$ne": 12 } }
    // faster than:
    //   { "$not": { "P1": 12 } } ?
    //
    bson_t notBson;

    treeP->type = QNodeEQ;  // QNodeNE => QNodeEQ

    bson_init(&notBson);
    bson_append_document_begin(bsonP, "$not", 4, &notBson);  // Preceding the expression with a negation

    bool b = qTreeToBson(treeP, &notBson, titleP, detailsP);
    bson_append_document_end(bsonP, &notBson);
    if (b == false)
      return false;
  }
  else if (treeP->type == QNodeExists)
  {
    QNode* leftP  = treeP->value.children;
    bson_t existsBson;

    bson_init(&existsBson);
    bson_append_document_begin(bsonP, leftP->value.v, -1, &existsBson);
    bson_append_bool(&existsBson, "$exists", 7, true);
    bson_append_document_end(bsonP, &existsBson);
  }
  else if (treeP->type == QNodeNotExists)
  {
    QNode* leftP  = treeP->value.children;
    bson_t notExistsBson;

    bson_init(&notExistsBson);
    bson_append_document_begin(bsonP, leftP->value.v, -1, &notExistsBson);
    bson_append_bool(&notExistsBson, "$exists", 7, false);
    bson_append_array_end(bsonP, &notExistsBson);
  }
  else if (treeP->type == QNodeMatch)
  {
    QNode* leftP  = treeP->value.children;
    QNode* rightP = leftP->next;
    bson_t matchBson;

    bson_init(&matchBson);
    bson_append_document_begin(bsonP, leftP->value.v, -1, &matchBson);
    bson_append_utf8(&matchBson, "$regex", 6, rightP->value.re, -1);
    bson_append_array_end(bsonP, &matchBson);
  }
  else if (treeP->type == QNodeNoMatch)
  {
    QNode* leftP  = treeP->value.children;
    QNode* rightP = leftP->next;
    bson_t notBson;
    bson_t noMatchBson;

    bson_init(&notBson);
    bson_init(&noMatchBson);

    bson_append_document_begin(bsonP, leftP->value.v, -1, &notBson);
    bson_append_document_begin(&notBson, "$not", 4, &noMatchBson);
    bson_append_utf8(&noMatchBson, "$regex", 6, rightP->value.re, -1);
    bson_append_array_end(&notBson, &noMatchBson);
    bson_append_array_end(bsonP, &notBson);
  }
  else
  {
    printf("\n");
    printf("***********************************************\n");
    printf("Not Implemented: %s\n", qNodeType(treeP->type));
    exit(1);
  }

  return true;
}
