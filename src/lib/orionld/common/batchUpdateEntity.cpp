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
#include <string.h>                                            // strncpy

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjArray, ...
#include "kjson/kjClone.h"                                     // kjClone
}

#include "logMsg/logMsg.h"                                     // LM_*

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/dotForEq.h"                           // dotForEq
#include "orionld/dbModel/dbModelFromApiAttribute.h"           // dbModelFromApiAttribute
#include "orionld/common/batchUpdateEntity.h"                  // Own interface



// ----------------------------------------------------------------------------
//
// batchUpdateEntity -
//
// Take the Original DB Entity and add/remove from it (not sure if we need to clone it - it might noe be used after this)
// Loop over inEntityP, all the attributes
//   * Transform API Attribute into DB-Attribute (dbModelToDbAttribute)
//   * if attr in "Original DB Entity"
//       remove it from "Original DB Entity"
//     else
//       add attr name to "attrNames"
//   * Add DB-Attribute to "Original DB Entity"
//   * Return "Original DB Entity"
//   * The creDate is already OK
//   * The modDate needs to be updated
//
// Slightly different if BATCH UPDATE - already existing attributes stay as is by default
//
KjNode* batchUpdateEntity(KjNode* inEntityP, KjNode* originalDbEntityP, char* entityId, char* entityType, bool ignore)
{
  KjNode* dbFinalEntityP = kjClone(orionldState.kjsonP, originalDbEntityP);

  //
  // "attr" member of the DB Entity - might not be present (stupid DB Model !!!)
  //
  KjNode* dbAttrsP     = kjLookup(dbFinalEntityP, "attrs");
  KjNode* dbAttrNamesP = kjLookup(dbFinalEntityP, "attrNames");

  // No "attrs" in the entity in the database? - normal, if the entity has no attributes
  if (dbAttrsP == NULL)
  {
    dbAttrsP = kjObject(orionldState.kjsonP, "attrs");
    kjChildAdd(dbFinalEntityP, dbAttrsP);
  }

  // No "attrNames" in the entity in the database? - that should not happen
  if (dbAttrNamesP == NULL)
  {
    dbAttrNamesP = kjArray(orionldState.kjsonP, "attrNames");
    kjChildAdd(dbFinalEntityP, dbAttrsP);
  }

  KjNode* apiAttrP = inEntityP->value.firstChildP;
  KjNode* next;
  while (apiAttrP != NULL)
  {
    next = apiAttrP->next;

    if (strcmp(apiAttrP->name, "id")   == 0) { apiAttrP = next; continue; }
    if (strcmp(apiAttrP->name, "type") == 0) { apiAttrP = next; continue; }

    char eqAttrName[512];
    strncpy(eqAttrName, apiAttrP->name, sizeof(eqAttrName) - 1);
    dotForEq(eqAttrName);
    KjNode* dbAttrP = kjLookup(dbAttrsP, eqAttrName);

    if (dbAttrP != NULL)   // The attribute already existed - we remove it before the new version of the attribute is added
    {
      if (ignore == true)  // ... Or we ignore it if BATCH Update and ?options=noOverwrite
      {
        kjChildRemove(inEntityP, apiAttrP);  // Must remove the ignored attribute from the incoming entity - for TRoE and Alterations
        apiAttrP = next;
        continue;
      }

      kjChildRemove(dbAttrsP, dbAttrP);
    }
    else
    {
      // The attribute is to be ADDED, so it must be added to "attrNames" (with dots, not eq)
      KjNode* attrNameNodeP = kjString(orionldState.kjsonP, NULL, apiAttrP->name);
      kjChildAdd(dbAttrNamesP, attrNameNodeP);
    }

    //
    // Transforming the attribute into DB Model and adding it to "attrs"
    // ".added" and ".removed" ...   need to look into this !!!
    //
    KjNode* attrAddedV   = kjArray(orionldState.kjsonP, ".added");
    KjNode* attrRemovedV = kjArray(orionldState.kjsonP, ".removed");

    dbAttrP = kjClone(orionldState.kjsonP, apiAttrP);  // Copy the API attribute and then transform it into DB Model

    dbModelFromApiAttribute(dbAttrP, dbAttrsP, attrAddedV, attrRemovedV, NULL);
    kjChildAdd(dbAttrsP, dbAttrP);

    apiAttrP = next;
  }

  //
  // Update the entity's modDate (or create it if it for some obscure reason is missing!)
  //
  KjNode* modDateNodeP = kjLookup(dbFinalEntityP, "modDate");
  if (modDateNodeP != NULL)
    modDateNodeP->value.f = orionldState.requestTime;
  else
  {
    modDateNodeP = kjFloat(orionldState.kjsonP, "modDate", orionldState.requestTime);
    kjChildAdd(dbFinalEntityP, modDateNodeP);
  }

  // Inserting the ".troe" field AFTER the DB Entity has been created
  if (troe)
  {
    KjNode* troeNodeP = kjString(orionldState.kjsonP, ".troe", "Update");
    kjChildAdd(inEntityP, troeNodeP);
  }

  return dbFinalEntityP;
}
