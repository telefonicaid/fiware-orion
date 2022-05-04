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
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM_*

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/rest/OrionLdRestService.h"                     // ORIONLD_SERVICE_OPTION_DATASET_SUPPORT
#include "orionld/kjTree/kjArrayAdd.h"                           // kjArrayAdd
#include "orionld/kjTree/kjTimestampAdd.h"                       // kjTimestampAdd
#include "orionld/kjTree/kjTreeLog.h"                            // kjTreeLog
#include "orionld/context/orionldSubAttributeExpand.h"           // orionldSubAttributeExpand
#include "orionld/dbModel/dbModelFromApiSubAttribute.h"          // dbModelFromApiSubAttribute
#include "orionld/dbModel/dbModelFromApiAttribute.h"             // Own interface



// -----------------------------------------------------------------------------
//
// dbModelFromApiAttributeDatasetArray -
//
bool dbModelFromApiAttributeDatasetArray(KjNode* attrArrayP, KjNode* dbAttrsP, KjNode* attrAddedV, KjNode* attrRemovedV, bool* ignoreP)
{
  bool     defaultFound  = false;
  KjNode*  datasetArrayP = NULL;
  char*    attrNameEq    = attrArrayP->name;

  //
  // Allocate (if needed) the datasets object "orionldState.datasets"
  //
  if (orionldState.datasets == NULL)
  {
    orionldState.datasets = kjObject(orionldState.kjsonP, "@datasets");
    if (orionldState.datasets == NULL)
    {
      orionldError(OrionldInternalError, "Out of memory", "allocating dataset object for an entity", 500);
      return false;
    }
  }
  else
    datasetArrayP = kjLookup(orionldState.datasets, attrNameEq);

  if (datasetArrayP == NULL)
  {
    datasetArrayP = kjArray(orionldState.kjsonP, attrNameEq);

    if (datasetArrayP == NULL)
    {
      orionldError(OrionldInternalError, "Out of memory", "allocating dataset array for an attribute", 500);
      return false;
    }

    kjChildAdd(orionldState.datasets, datasetArrayP);
    LM_TMP(("DS: Created a dataset array for the attribute '%s'", attrNameEq));
  }


  KjNode*  attrP = attrArrayP->value.firstChildP;
  KjNode*  next;
  while (attrP != NULL)
  {
    next = attrP->next;

    kjTreeLog(attrP, "attr instance");
    if (next != NULL)
      kjTreeLog(next, "next attr instance");

    KjNode* datasetIdNodeP = kjLookup(attrP, "datasetId");

    if (datasetIdNodeP == NULL)  // Default instance
    {
      if (defaultFound == true)
      {
        orionldError(OrionldBadRequestData, "More than one attribute instances without datasetId", attrP->name, 400);
        return false;
      }

      LM_TMP(("DS: Got a default instance of attribute '%s'", attrNameEq));
      if (dbModelFromApiAttribute(attrP, dbAttrsP, attrAddedV, attrRemovedV, ignoreP) == false)
        return false;

      defaultFound = true;
    }
    else
    {
      LM_TMP(("DS: Got a dataset instance (%s) of attribute '%s' - adding it to dataset array for '%s'", datasetIdNodeP->value.s, attrNameEq, datasetArrayP->name));

      kjChildRemove(attrArrayP, attrP);  // Remove the attribute instance from the attribute ...
      kjChildAdd(datasetArrayP, attrP);  // ... And insert it under @datasets::attrName

      //
      // The DB Model for datasetId instances is just as the API - only need to add the timestamps
      // If any of the timestamps are already present in the incoing payload ...
      // Should they be removed here or are they already removed by pCheckAttribute?
      // For now, I remove them here - then we'll see ...
      //
      LM_TMP(("DS: Adding timestamps for attr '%s'", attrP->name));
      KjNode* createdAtP  = kjLookup(attrP, "createdAt");
      KjNode* modifiedAtP = kjLookup(attrP, "modifiedAt");

      if (createdAtP  != NULL) kjChildRemove(attrP, createdAtP);
      if (modifiedAtP != NULL) kjChildRemove(attrP, modifiedAtP);

      createdAtP  = kjFloat(orionldState.kjsonP, "createdAt",  orionldState.requestTime);
      modifiedAtP = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);

      LM_TMP(("DS: Adding timestamps for attr '%s' (of type '%s')", attrP->name, kjValueType(attrP->type)));
      kjTreeLog(attrP, "DS: attrP");
      LM_TMP(("DS: attrP->lastChild at %p", attrP->lastChild));
      kjChildAdd(attrP, createdAtP);
      LM_TMP(("DS: Adding timestamps for attr '%s'", attrP->name));
      kjChildAdd(attrP, modifiedAtP);
      LM_TMP(("DS: Added timestamps for attr '%s'", attrP->name));
    }

    attrP = next;
    LM_TMP(("And the loop continues? (attrP==%p)", attrP));
  }

  LM_TMP(("Done"));
  // No datasets?
  if (datasetArrayP->value.firstChildP == NULL)
  {
    LM_TMP(("DS: No datasets for attribute '%s'", datasetArrayP->name));
    kjChildRemove(orionldState.datasets, datasetArrayP);
  }
  LM_TMP(("Done"));

  //
  // The array of the attribute now needs to be an object - just the default attribute is left in there -
  // all other instances (with datasetId) have been moved to datasetArrayP (that's inside orionldState.datasets).
  // Unless it is empty of course ...
  //
  if (attrArrayP->value.firstChildP != NULL)
  {
    attrArrayP->type  = attrArrayP->value.firstChildP->type;
    LM_TMP(("Done"));
    attrArrayP->value = attrArrayP->value.firstChildP->value;
  }

  LM_TMP(("Done"));
  return true;
}



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
bool dbModelFromApiAttribute(KjNode* attrP, KjNode* dbAttrsP, KjNode* attrAddedV, KjNode* attrRemovedV, bool* ignoreP)
{
  KjNode* mdP         = NULL;
  char*   attrEqName  = kaStrdup(&orionldState.kalloc, attrP->name);
  char*   attrDotName = kaStrdup(&orionldState.kalloc, attrP->name);

  LM_TMP(("DS: dbModelFromApiAttribute(%s)", attrEqName));

  // attrP->name is cloned as we can't destroy the context (expansion of attr name comes from there)
  dotForEq(attrEqName);      // Orion DB-Model states that dots are replaced for equal signs in attribute names
  attrP->name = attrEqName;  // Cause, that's the name of the attribute in the database

  if (attrP->type == KjNull)
  {
    KjNode* attrNameNodeP = kjString(orionldState.kjsonP, NULL, attrDotName);
    kjChildAdd(attrRemovedV, attrNameNodeP);
    return true;
  }

  if (attrP->type == KjArray)
  {
    LM_TMP(("DS: Attribute '%s' is an array", attrP->name));
    if ((orionldState.serviceP->options & ORIONLD_SERVICE_OPTION_DATASET_SUPPORT) == 0)
    {
      LM_W(("Datasets present - not yet implemented"));
      orionldError(OrionldOperationNotSupported, "Not Implemented", "Datasets not implemented for this type of request", 501);
      return false;
    }

    return dbModelFromApiAttributeDatasetArray(attrP, dbAttrsP, attrAddedV, attrRemovedV, ignoreP);
  }
  else
    LM_TMP(("DS: Attribute '%s' is NOT an array", attrP->name));

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

    LM_TMP(("DS: Morphing attribute '%s' into an array", attrEqName));
    // Now morph attrP into an Array:
    attrP->type = KjArray;

    // And make newAttrP its only child
    attrP->value.firstChildP = newAttrP;
    attrP->lastChild         = newAttrP;

    LM_TMP(("DS: Must remove the attr '%s' from its entity and put under $datasets!", attrP->name));
    kjTreeLog(attrP, "DS: Morphed attr into array with itself as only member");
    *ignoreP = true;
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
        if (dbAttrP == NULL)  // Attribute does not already exist
          kjTimestampAdd(attrP, "creDate");

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
      *ignoreP = true;
      return true;  // Just ignore it
    }

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
