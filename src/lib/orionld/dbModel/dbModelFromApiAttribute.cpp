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
#include "kbase/kMacros.h"                                        // K_VEC_SIZE
#include "kalloc/kaStrdup.h"                                      // kaStrdup
#include "kjson/KjNode.h"                                         // KjNode
#include "kjson/kjLookup.h"                                       // kjLookup
#include "kjson/kjBuilder.h"                                      // kjString, kjObject, kjChildAdd, ...
#include "kjson/kjClone.h"                                        // kjClone
}

#include "logMsg/logMsg.h"                                        // LM_*

#include "orionld/types/OrionLdRestService.h"                     // ORIONLD_SERVICE_OPTION_DATASET_SUPPORT
#include "orionld/common/orionldState.h"                          // orionldState
#include "orionld/common/orionldError.h"                          // orionldError
#include "orionld/common/dotForEq.h"                              // dotForEq
#include "orionld/kjTree/kjArrayAdd.h"                            // kjArrayAdd
#include "orionld/kjTree/kjTimestampAdd.h"                        // kjTimestampAdd
#include "orionld/kjTree/kjTreeLog.h"                             // kjTreeLog
#include "orionld/context/orionldSubAttributeExpand.h"            // orionldSubAttributeExpand
#include "orionld/dbModel/dbModelFromApiSubAttribute.h"           // dbModelFromApiSubAttribute
#include "orionld/dbModel/dbModelFromApiAttributeDatasetArray.h"  // dbModelFromApiAttributeDatasetArray
#include "orionld/dbModel/dbModelFromApiAttribute.h"              // Own interface



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
//   * All children of 'attrP' are moved to 'md'
//   * And then, all the special fields are moved back to 'attrP'
//   * + ".added"   is added
//   * + ".removed" is added
//   * + "modDate"  is added
//   * + "creDate"  is added OR "stolen" from dbAttrP
//
bool dbModelFromApiAttribute(KjNode* attrP, KjNode* dbAttrsP, KjNode* attrAddedV, KjNode* attrRemovedV, bool* ignoreP, bool stealCreDate)
{
  KjNode* mdP         = NULL;
  char*   attrEqName  = kaStrdup(&orionldState.kalloc, attrP->name);
  char*   attrDotName = kaStrdup(&orionldState.kalloc, attrP->name);

  // attrP->name is cloned as we can't destroy the context (expansion of attr name comes from there)
  dotForEq(attrEqName);      // Orion DB-Model states that dots are replaced for equal signs in attribute names
  attrP->name = attrEqName;  // Cause, that's the name of the attribute in the database

  if (attrP->type == KjNull)
  {
    // Check: is null supported by the current service?
    if (attrRemovedV != NULL)
    {
      KjNode* attrNameNodeP = kjString(orionldState.kjsonP, NULL, attrDotName);
      kjChildAdd(attrRemovedV, attrNameNodeP);
    }

    return true;
  }

  if (attrP->type == KjArray)
  {
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_DATASET_SUPPORT) == 0)
    {
      LM_W(("Datasets present - not yet implemented"));
      orionldError(OrionldOperationNotSupported, "Not Implemented", "Datasets not implemented for this type of request", 501);
      return false;
    }

    return dbModelFromApiAttributeDatasetArray(attrP, dbAttrsP, attrAddedV, attrRemovedV, ignoreP);
  }

  // Can also be datasetId without the attribute to be an array - just one instance BUT with a datasetId !
  KjNode* datasetIdP = kjLookup(attrP, "datasetId");
  if (datasetIdP != NULL)
  {
    //
    // WARNING !!!
    // Is it safe to remove the attribute from its container???
    // - Probably not
    //
    // Let's do it in a different way, by "cloning" attrP (copy it's type and value) and after that modift attrP to be an array
    // This way, "attrP" stays in its place in the KjNode-Tree, with it's next pointer, etc all correct.
    // Only, after this, "attrP" is an array, and it contains a clone of its former self
    //
    KjNode* newAttrP    = kjClone(orionldState.kjsonP, attrP);

    // Now morph attrP into an Array:
    attrP->type = KjArray;

    // And make newAttrP its only child
    attrP->value.firstChildP = newAttrP;
    attrP->lastChild         = newAttrP;

    if (ignoreP != NULL)
      *ignoreP = true;  // Ignoring datasetId attrs for TRoE

    return dbModelFromApiAttributeDatasetArray(attrP, dbAttrsP, attrAddedV, attrRemovedV, ignoreP);
  }

  KjNode*  dbAttrP = (dbAttrsP != NULL)? kjLookup(dbAttrsP, attrEqName) : NULL;

  // Move everything into "md", leaving attrP EMPTY
  mdP = kjObject(orionldState.kjsonP, "md");
  mdP->value.firstChildP   = attrP->value.firstChildP;
  mdP->lastChild           = attrP->lastChild;
  attrP->value.firstChildP = NULL;
  attrP->lastChild         = NULL;

  //
  // DB Model (NGSIv1) has the order:
  // - type
  // - creDate
  // - modeDate
  // - value
  // - mdNames
  //

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

      // After "type", add creDate, modDate
      if (ix == 0)
      {
        if (dbAttrP == NULL)  // Attribute does not already exist - needs a creDate
          kjTimestampAdd(attrP, "creDate");
        else if (stealCreDate == true)  // FIXME: opMode == REPLACE
        {
          KjNode* creDateP = kjLookup(dbAttrP, "creDate");
          if (creDateP != NULL)
            kjChildRemove(dbAttrP, creDateP);
          else
            creDateP = kjFloat(orionldState.kjsonP, "creDate", orionldState.requestTime);

          kjChildAdd(attrP, creDateP);
        }

        // Also, the attribute is being modified - need to update the "modifiedAt" (called modDate in Orion's DB-Model)
        kjTimestampAdd(attrP, "modDate");
      }
    }
  }

  KjNode*  mdAddedP     = NULL;
  KjNode*  mdRemovedP   = NULL;
  KjNode*  mdNamesP     = NULL;
  KjNode*  dbMdP        = NULL;

  if (dbAttrP == NULL)  // Attribute does not already exist
  {
    if (attrP->type == KjNull)
    {
      // Apparently it's OK to try to delete an attribute that does not exist
      if (ignoreP != NULL)
        *ignoreP = true;

      return true;  // Just ignore it
    }

    // The "attrNames" array stores the sub-attribute names in their original names, with dots - attrDotName
    if (attrAddedV != NULL)
      kjChildAdd(attrAddedV, kjString(orionldState.kjsonP, NULL, attrDotName));

    // All attributes have a field 'mdNames' - it's an array
    mdNamesP = kjArray(orionldState.kjsonP, "mdNames");
    kjChildAdd(attrP, mdNamesP);
  }
  else
  {
    // The attribute is not new, but, perhaps it didn't have any sub-attrs before but now it does?
    // If so, never mind '.added', '.removed' etc.
    // What we need is to include the entire "md"
    //
    dbMdP    = kjLookup(dbAttrP, "md");
    mdNamesP = kjLookup(dbAttrP, "mdNames");

    if (stealCreDate == true)  // FIXME: opMode == REPLACE
    {
      // The Attribute is being REPLACED - "mdNames" need to start from scratch
      // Only those sub-attrs in 'attrP' are to be part of 'mdNames'
      mdNamesP = kjArray(orionldState.kjsonP, "mdNames");
      kjChildAdd(attrP, mdNamesP);
    }
    else if (mdNamesP == NULL)
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

    // if (dbMdP != NULL)  // .added/.removed should not be needed if dbMdP == NULL ...
    {
      mdAddedP   = kjArrayAdd(attrP, ".added");
      mdRemovedP = kjArrayAdd(attrP, ".removed");
    }

    //
    // Very Special case: If key-values is set, and an uri param 'observedAt' is present, and we're doing a patchEntity2, then:
    // modify the observedAt sub-attr accordingly
    //
    if ((orionldState.uriParamOptions.keyValues == true) && (orionldState.uriParams.observedAt != NULL) && (dbMdP != NULL))
    {
      KjNode* observedAtP = kjLookup(dbMdP, "observedAt");
      if (observedAtP != NULL)
      {
        KjNode* mdObservedAt = kjFloat(orionldState.kjsonP, "observedAt", orionldState.uriParams.observedAtAsDouble);
        kjChildAdd(mdP, mdObservedAt);
      }
    }

    kjChildAdd(attrP, mdNamesP);
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

    if (strcmp(subP->name, "creDate") == 0)
    {
      kjChildRemove(mdP, subP);
      kjChildAdd(attrP, subP);
      subP = next;
      continue;
    }

    subP->name = orionldSubAttributeExpand(orionldState.contextP, subP->name, true, NULL);

    // Add the name of the sub-attribute to the "mdNames" array
    KjNode* mdNameP = kjString(orionldState.kjsonP, NULL, subP->name);
    kjChildAdd(mdNamesP, mdNameP);

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
