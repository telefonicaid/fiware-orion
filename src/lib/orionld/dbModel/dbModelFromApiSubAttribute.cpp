/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
extern "C"
{
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjChildAdd, ...
}
#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/common/dateTime.h"                             // dateTimeFromString
#include "orionld/kjTree/kjTimestampAdd.h"                       // kjTimestampAdd
#include "orionld/dbModel/dbModelFromApiSubAttribute.h"          // Own interface



// -----------------------------------------------------------------------------
//
// dbModelFromApiSubAttribute -
//
// PARAMETERS
//   - saP            pointer to the sub-attribute KjNode tree
//   - dbMdP          array of names od alreadty existing metadata (from DB)
//   - mdAddedV       array of names of new metadata (sub-attributes)
//   - mdRemovedV     array of names of sub-attributes that are to be removed (RHS == null)
//
bool dbModelFromApiSubAttribute(KjNode* saP, KjNode* dbMdP, KjNode* mdAddedV, KjNode* mdRemovedV, bool* ignoreP)
{
  char* saEqName  = kaStrdup(&orionldState.kalloc, saP->name);  // Can't destroy the context - need to copy before calling dotForEq
  char* saDotName = kaStrdup(&orionldState.kalloc, saP->name);  // Can't destroy the context - need to copy before calling dotForEq

  dotForEq(saEqName);      // Orion DB-Model states that dots are replaced for equal signs in sub-attribute names
  saP->name = saEqName;

  KjNode* dbSubAttributeP = (dbMdP == NULL)? NULL : kjLookup(dbMdP, saEqName);

  if (saP->type == KjNull)
  {
    if (dbSubAttributeP == NULL)
    {
      // Apparently it's OK to try to delete an attribute that does not exist (RFC 7396))
      *ignoreP = true;
      return true;  // Just ignore it
    }

    if (mdRemovedV != NULL)
      kjChildAdd(mdRemovedV, kjString(orionldState.kjsonP, NULL, saDotName));

    return true;
  }

  if (strcmp(saDotName, "value") == 0)
    return true;
  else if ((strcmp(saDotName, "object") == 0) || (strcmp(saDotName, "languageMap") == 0))
    saDotName = (char*) "value";  // Orion-LD's database model states that all attributes have a "value"
  else if (strcmp(saDotName, "observedAt") == 0)
  {
    //
    // observedAt is stored as a JSON object, as any other sub-attribute (my mistake, bad idea but too late now)
    // Also, the string representation of the ISO8601 is turned into a floating point representation
    // So, those two problems are fixed here:
    //
    if (saP->type == KjString)
    {
      char   errorString[256];
      double timestamp = dateTimeFromString(saP->value.s, errorString, sizeof(errorString));

      if (timestamp < 0)
      {
        orionldError(OrionldBadRequestData, "Invalid ISO8601 timestamp", errorString, 400);
        LM_W(("Bad Request (Invalid ISO8601 timestamp: %s)", saP->value.s));
        return false;
      }

      KjNode* valueP = kjFloat(orionldState.kjsonP, "value", timestamp);

      saP->type = KjObject;
      saP->value.firstChildP = valueP;
      saP->lastChild         = valueP;
    }

    // Also, these two special sub-attrs have no creDate/modDate but they're still sub-attrs, need to be in "mdNames"
    if ((dbSubAttributeP == NULL) && (mdAddedV != NULL))
      kjChildAdd(mdAddedV, kjString(orionldState.kjsonP, NULL, saDotName));
  }
  else if (strcmp(saDotName, "unitCode") == 0)
  {
    //
    // unitCode is stored as a JSON object, as any other sub-attribute (my mistake, bad idea but too late now)
    // So, that problem is fixed here:
    //
    if (saP->type == KjString)
    {
      KjNode* valueP = kjString(orionldState.kjsonP, "value", saP->value.s);

      saP->type = KjObject;
      saP->value.firstChildP = valueP;
      saP->lastChild         = valueP;
    }

    // Also, these two special sub-attrs have no creDate/modDate but they're still sub-attrs, need to be in "mdNames"
    if ((dbSubAttributeP == NULL) && (mdAddedV != NULL))
      kjChildAdd(mdAddedV, kjString(orionldState.kjsonP, NULL, saDotName));
  }
  else if (strcmp(saDotName, "creDate") == 0) {}
  else if (strcmp(saDotName, "modDate") == 0) {}
  else if (strcmp(saDotName, "createdAt") == 0)
  {
    // This is not a sub-attribute, it's the timestamp of the attribute, and it must be called "creDate"
    saP->name = (char*) "creDate";
  }
  else if (strcmp(saDotName, "modifiedAt") == 0)
  {
    // This is not a sub-attribute, it's the timestamp of the attribute, and it must be called "modDate"
    saP->name = (char*) "modDate";
  }
  else
  {
    if (saP->type != KjObject)
    {
      // Not an object ... weird, but let's leave it as it is
      LM_W(("The sub-attribute '%s' is not a JSON Object", saP->name));
      return true;
    }

    // If object or languageMap exist, change name to value
    KjNode* valueP = kjLookup(saP, "object");

    if (valueP == NULL)
      valueP = kjLookup(saP, "languageMap");

    if (valueP != NULL)
      valueP->name = (char*) "value";

    //
    // If the sub-attr didn't exist, it needs a "createdAt"
    // If it did exist, then steal if from DB, if there. If not, create it
    //
    if (dbSubAttributeP == NULL)
    {
      kjTimestampAdd(saP, "createdAt");

      // The "mdNames" array stores the sub-attribute names in their original names, with dots - saDotName
      if (mdAddedV != NULL)
        kjChildAdd(mdAddedV, kjString(orionldState.kjsonP, NULL, saDotName));
    }
    else
    {
      KjNode* createdAtP = kjLookup(dbSubAttributeP, "createdAt");

      if (createdAtP != NULL)
      {
        kjChildRemove(dbSubAttributeP, createdAtP);
        kjChildAdd(saP, createdAtP);
      }
      else
        kjTimestampAdd(saP, "createdAt");
    }

    // All sub-attrs get a fresh "modifiedAt"
    kjTimestampAdd(saP, "modifiedAt");
  }

  return true;
}
