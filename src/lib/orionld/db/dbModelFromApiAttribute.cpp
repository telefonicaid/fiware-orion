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
#include "kbase/kMacros.h"                                       // K_VEC_SIZE
#include "kalloc/kaStrdup.h"                                     // kaStrdup
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjBuilder.h"                                     // kjString, kjObject, kjChildAdd, ...
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/kjTree/kjArrayAdd.h"                           // kjArrayAdd
#include "orionld/kjTree/kjTimestampAdd.h"                       // kjTimestampAdd
#include "orionld/context/orionldSubAttributeExpand.h"           // orionldSubAttributeExpand
#include "orionld/db/dbModelFromApiSubAttribute.h"               // dbModelFromApiSubAttribute
#include "orionld/db/dbModelFromApiAttribute.h"                  // Own interface



// -----------------------------------------------------------------------------
//
// dbModelFromApiAttribute -
//
// FOREACH attribute:
//   * First an object "md" is created, and all fields of the attribute, except the special ones are moved inside "md".
//     Special fields:
//     - type
//     - value/object/languageMap (must be renamed to "value" - that's part of the database model)
//     - observedAt
//     - datasetId
//     - unitCode
//
//   * Actually, all children of 'attrP' are moved to 'md'
//   * And then, all the special fields are moved back to 'attrP'
//   * ".added"   is added
//   * ".removed" is added
//   * "modDate"  is added
//   * "creDate"  is added iff the attribute did not previously exist
//
bool dbModelFromApiAttribute(KjNode* attrP, KjNode* dbAttrsP, KjNode* attrAddedV, KjNode* attrRemovedV)
{
  KjNode* mdP         = NULL;
  char*   attrEqName  = kaStrdup(&orionldState.kalloc, attrP->name);
  char*   attrDotName = kaStrdup(&orionldState.kalloc, attrP->name);

  // attrP->name is cloned as we can't destroy the context (expansion of attr name comes from there)
  dotForEq(attrEqName);      // Orion DB-Model states that dots are replaced for equal signs in attribute names
  attrP->name = attrEqName;  // Cause, that's the name of the attribute in the database

  if (attrP->type == KjNull)
  {
    KjNode* attrNameNodeP = kjString(orionldState.kjsonP, NULL, attrDotName);
    kjChildAdd(attrRemovedV, attrNameNodeP);
    return true;
  }

  // Move everything into "md", leaving attrP EMPTY
  mdP = kjObject(orionldState.kjsonP, "md");
  mdP->value.firstChildP   = attrP->value.firstChildP;
  mdP->lastChild           = attrP->lastChild;
  attrP->value.firstChildP = NULL;
  attrP->lastChild         = NULL;

  // Move special fields back to "attrP"
  const char* specialV[] = { "type", "value", "object", "languageMap", "datasetId" };  // observedAt+unitCode are mds (db-model)
  for (unsigned int ix = 0; ix < K_VEC_SIZE(specialV); ix++)
  {
    KjNode* nodeP = kjLookup(mdP, specialV[ix]);
    if (nodeP != NULL)
    {
      if ((ix == 2) || (ix == 3))         // "object", "languageMap", change name to "value" (Orion's DB model)
        nodeP->name = (char*) "value";

      kjChildRemove(mdP, nodeP);
      kjChildAdd(attrP, nodeP);
    }
  }

  // Also, the attribute is being modified - need to update the "modifiedAt" (called modDate in Orion's DB-Model)
  kjTimestampAdd(attrP, "modDate");

  KjNode*  mdAddedP     = NULL;
  KjNode*  mdRemovedP   = NULL;
  KjNode*  mdNamesP     = NULL;
  KjNode*  dbAttrP      = kjLookup(dbAttrsP, attrEqName);
  KjNode*  dbMdP        = NULL;

  if (dbAttrP == NULL)  // Attribute does not already exist
  {
    if (attrP->type == KjNull)
    {
      // Apparently this is OK
#if 1
      return true;  // Just ignore it
#else
      LM_W(("Attempt to DELETE an attribute that doesn't exist (%s)", attrEqName));
      orionldError(OrionldResourceNotFound, "Cannot delete an attribute that does not exist", attrEqName, 404);
      return false;
#endif
    }

    kjTimestampAdd(attrP, "creDate");

    // The "attrNames" array stores the sub-attribute names in their original names, with dots - attrDotName
    kjChildAdd(attrAddedV, kjString(orionldState.kjsonP, NULL, attrDotName));

    // All attributes have a field 'mdNames' - it's an array
    mdNamesP = kjArray(orionldState.kjsonP, "mdNames");
    kjChildAdd(attrP, mdNamesP);
  }
  else
  {
    // The attribute is not new, but, perhaps it didn't have any sub-attrs before but now it does?
    // If so, never mind .added, .removed etc, what we need is to include the entire "md"
    //
    dbMdP    = kjLookup(dbAttrP, "md");
    mdNamesP = kjLookup(dbAttrP, "mdNames");

    if (mdNamesP == NULL)
    {
      //
      // No 'mdNames' field in DB Attribute?
      // That's a DB Error.
      // Never mind, let's just silently fix it
      //
      mdNamesP = kjArray(orionldState.kjsonP, ".names");
    }
    else
    {
      kjChildRemove(dbAttrP, mdNamesP);
      mdNamesP->name = (char*) ".names";  // We hijack the 'mdNames' array for the compilation of the Patch Tree
    }

    kjChildAdd(attrP, mdNamesP);

    // if (dbMdP != NULL)  // .added/.removed should not be needed if dbMdP == NULL ...
    {
      mdAddedP   = kjArrayAdd(attrP, ".added");
      mdRemovedP = kjArrayAdd(attrP, ".removed");
    }
  }

  if (attrP->type == KjNull)
    return true;

  // Sub-Attributes
  KjNode* subP = mdP->value.firstChildP;
  KjNode* next;

  while (subP != NULL)
  {
    bool ignore = false;

    next = subP->next;

    subP->name = orionldSubAttributeExpand(orionldState.contextP, subP->name, true, NULL);
    if (dbModelFromApiSubAttribute(subP, dbMdP, mdAddedP, mdRemovedP, &ignore) == false)
      return false;

    if (ignore == true)
      kjChildRemove(mdP, subP);

    subP = next;
  }

  if (mdP->value.firstChildP != NULL)
    kjChildAdd(attrP, mdP);

  return true;
}
