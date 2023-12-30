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

#include "orionld/types/QNode.h"                               // QNode
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/mongoc/mongocIndexString.h"                  // mongocIndexString
#include "orionld/q/qNodeType.h"                               // qNodeType
#include "orionld/q/qPresent.h"                                // qPresent
#include "orionld/q/qTreeToBson.h"                             // Own interface



// ----------------------------------------------------------------------------
//
// qTreeToBson -
//
bool qTreeToBson(QNode* treeP, bson_t* bsonP, char** titleP, char** detailsP)
{
  if (treeP->type == QNodeOr)
  {
    // { "$or": [ { }, { }, ... { } ] }
    bson_t orArrayBson;

    bson_init(&orArrayBson);
    bson_append_array_begin(bsonP, "$or", 3, &orArrayBson);

    int ix = 0;
    for (QNode* qNodeP = treeP->value.children; qNodeP != NULL; qNodeP = qNodeP->next)
    {
      bson_t orItemBson;
      char   buf[16];
      int    bufLen = mongocIndexString(ix, buf);

      bson_append_document_begin(&orArrayBson, buf, bufLen, &orItemBson);
      if (qTreeToBson(qNodeP, &orItemBson, titleP, detailsP) == false)
        return false;
      bson_append_document_end(&orArrayBson, &orItemBson);
      ++ix;
    }
    bson_append_array_end(bsonP, &orArrayBson);
    bson_destroy(&orArrayBson);
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
      bson_append_int32(&gtBson, op, opLen, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)
      bson_append_double(&gtBson, op, opLen, rightP->value.f);
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
      bson_append_int32(&ltBson, op, opLen, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)
      bson_append_double(&ltBson, op, opLen, rightP->value.f);
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
      bson_append_int32(bsonP, leftP->value.v, -1, rightP->value.i);
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
      bson_append_document_begin(bsonP, leftP->value.v, -1, &limitBson);

      if (lowerLimitNodeP->type == QNodeIntegerValue)
      {
        bson_append_int32(&limitBson, "$gte", 4, lowerLimitNodeP->value.i);
        bson_append_int32(&limitBson, "$lte", 4, upperLimitNodeP->value.i);
      }
      else if (lowerLimitNodeP->type == QNodeFloatValue)
      {
        bson_append_double(&limitBson, "$gte", 4, lowerLimitNodeP->value.f);
        bson_append_double(&limitBson, "$lte", 4, upperLimitNodeP->value.f);
      }
      else if (lowerLimitNodeP->type == QNodeStringValue)
      {
        bson_append_utf8(&limitBson, "$gte", 4, lowerLimitNodeP->value.s, -1);
        bson_append_utf8(&limitBson, "$lte", 4, upperLimitNodeP->value.s, -1);
      }
      else
      {
        bson_append_document_end(bsonP, &limitBson);
        bson_destroy(&limitBson);
        orionldError(OrionldOperationNotSupported, "Not Implemented - value type for Equal-Range", qNodeType(lowerLimitNodeP->type), 501);
        return false;
      }

      bson_append_document_end(bsonP, &limitBson);
      bson_destroy(&limitBson);
    }
    else if (rightP->type == QNodeComma)
    {
      // { "https://schema=org/xxx/P1.md.P100" : { "$in": [ 1, 2, 3 ] } }
      bson_t inBson;
      bson_t commaArrayBson;

      bson_append_document_begin(bsonP, leftP->value.v, -1, &inBson);

      bson_append_array_begin(&inBson, "$in", -1, &commaArrayBson);

      int ix = 0;
      for (QNode* valueNodeP = rightP->value.children; valueNodeP != NULL; valueNodeP = valueNodeP->next)
      {
        char buf[16];
        int  bufLen = mongocIndexString(ix, buf);

        if (valueNodeP->type == QNodeIntegerValue)
          bson_append_int32(&commaArrayBson, buf, bufLen, valueNodeP->value.i);
        else if (valueNodeP->type == QNodeFloatValue)
          bson_append_double(&commaArrayBson, buf, bufLen, valueNodeP->value.f);
        else if (valueNodeP->type == QNodeStringValue)
          bson_append_utf8(&commaArrayBson, buf, bufLen, valueNodeP->value.s, -1);
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
    QNode* leftP  = treeP->value.children;
    QNode* rightP = leftP->next;
    bson_t neBson;

    bson_init(&neBson);

    // From the mongo manual:
    //   $ne selects the documents where the value of the field is not equal to the specified value.
    //   This includes documents that do not contain the field.
    //
    // So, we'll have to add an "$exists" as well :(
    //
    bson_t existsBson;
    bson_init(&existsBson);
    bson_append_bool(&existsBson, "$exists", 7, true);
    bson_append_document(bsonP, leftP->value.v, -1, &existsBson);
    bson_destroy(&existsBson);

    if      (rightP->type == QNodeStringValue)   bson_append_utf8(&neBson,   "$ne", 3, rightP->value.s, -1);
    else if (rightP->type == QNodeIntegerValue)  bson_append_int32(&neBson,  "$ne", 3, rightP->value.i);
    else if (rightP->type == QNodeFloatValue)    bson_append_double(&neBson, "$ne", 3, rightP->value.f);
    else if (rightP->type == QNodeTrueValue)     bson_append_bool(&neBson,   "$ne", 3, true);
    else if (rightP->type == QNodeFalseValue)    bson_append_bool(&neBson,   "$ne", 3, false);
    else if (rightP->type == QNodeRange)
    {
      //
      // q="P1 != 1..4" => P1 < 1 OR P1 > 4
      //
      // { $or [ {P1: {$lt: 1}}, {P1: {$gt: 4}} ]}
      //
      QNode* lowerLimitNodeP = rightP->value.children;
      QNode* upperLimitNodeP = lowerLimitNodeP->next;
      bson_t ltBson;
      bson_t gtBson;
      bson_t ltComparisonBson;
      bson_t gtComparisonBson;
      bson_t orBson;

      bson_init(&ltBson);
      bson_init(&gtBson);

      if (lowerLimitNodeP->type == QNodeIntegerValue)
      {
        bson_append_int32(&ltBson, "$lt", 3, lowerLimitNodeP->value.i);
        bson_append_int32(&gtBson, "$gt", 3, upperLimitNodeP->value.i);
      }
      else if (lowerLimitNodeP->type == QNodeFloatValue)
      {
        bson_append_double(&ltBson, "$lt", 3, lowerLimitNodeP->value.f);
        bson_append_double(&gtBson, "$gt", 3, upperLimitNodeP->value.f);
      }
      else if (lowerLimitNodeP->type == QNodeStringValue)
      {
        bson_append_utf8(&ltBson, "$lt", 3, lowerLimitNodeP->value.s, -1);
        bson_append_utf8(&gtBson, "$gt", 3, upperLimitNodeP->value.s, -1);
      }
      else
      {
        bson_destroy(&ltBson);
        bson_destroy(&gtBson);
        orionldError(OrionldOperationNotSupported, "Not Implemented - value type for Not-Equal-Range", qNodeType(lowerLimitNodeP->type), 501);
        return false;
      }

      bson_init(&ltComparisonBson);
      bson_init(&gtComparisonBson);
      bson_init(&orBson);

      bson_append_document(&ltComparisonBson, leftP->value.v, -1, &ltBson);
      bson_append_document(&gtComparisonBson, leftP->value.v, -1, &gtBson);
      bson_append_document(&orBson, "0", 1, &ltComparisonBson);
      bson_append_document(&orBson, "1", 1, &gtComparisonBson);
      bson_append_array(bsonP, "$or", 3, &orBson);

      bson_destroy(&ltBson);
      bson_destroy(&gtBson);
      bson_destroy(&ltComparisonBson);
      bson_destroy(&gtComparisonBson);
      bson_destroy(&orBson);
    }
    else if (rightP->type == QNodeComma)
    {
      // { "https://schema=org/xxx/P1.md.P100" : { "$not": { "$in": [ 1, 2, 3 ] } } }
      bson_t notBson;
      bson_t inBson;
      bson_t commaArrayBson;

      bson_init(&notBson);
      bson_init(&inBson);
      bson_init(&commaArrayBson);

      bson_append_array_begin(&inBson, "$in", -1, &commaArrayBson);

      int ix = 0;
      for (QNode* valueNodeP = rightP->value.children; valueNodeP != NULL; valueNodeP = valueNodeP->next)
      {
        char buf[16];
        int  bufLen = mongocIndexString(ix, buf);

        if      (valueNodeP->type == QNodeIntegerValue)  bson_append_int32(&commaArrayBson,  buf, bufLen, valueNodeP->value.i);
        else if (valueNodeP->type == QNodeFloatValue)    bson_append_double(&commaArrayBson, buf, bufLen, valueNodeP->value.f);
        else if (valueNodeP->type == QNodeStringValue)   bson_append_utf8(&commaArrayBson,   buf, bufLen, valueNodeP->value.s, -1);

        ++ix;
      }
      bson_append_array_end(&inBson, &commaArrayBson);
      bson_append_document(&notBson, "$not", 4, &inBson);
      bson_append_document(bsonP, leftP->value.v, -1, &notBson);

      bson_destroy(&notBson);
      bson_destroy(&inBson);
      bson_destroy(&commaArrayBson);
    }
    else
    {
      bson_destroy(&neBson);
      orionldError(OrionldOperationNotSupported, "Not Implemented - Q-Node type for Not-Equal", qNodeType(rightP->type), 501);
      *titleP   = (char*) "ngsi-ld query language: Not Implemented - Q-Node type for Not-Equal";
      *detailsP = (char*) qNodeType(rightP->type);
      return false;
    }

    if ((rightP->type != QNodeRange) && (rightP->type != QNodeComma))
      bson_append_document(bsonP, leftP->value.v, -1, &neBson);
    bson_destroy(&neBson);
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
