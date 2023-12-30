/*
*
* Copyright 2023 FIWARE Foundation e.V.
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
#include "kjson/kjBuilder.h"                                     // kjObject
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kjson/kjClone.h"                                       // kjClone
}

#include "logMsg/logMsg.h"                                       // LM*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "orionld/types/OrionldAttributeType.h"                  // OrionldAttributeType
#include "orionld/types/OrionLdRestService.h"                    // OrionLdRestService
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldError.h"                         // orionldError
#include "orionld/common/responseFix.h"                          // responseFix
#include "orionld/common/dotForEq.h"                             // dotForEq
#include "orionld/mongoc/mongocEntityLookup.h"                   // mongocEntityLookup
#include "orionld/mongoc/mongocAttributeReplace.h"               // mongocAttributeReplace
#include "orionld/payloadCheck/pCheckAttribute.h"                // pCheckAttribute
#include "orionld/dbModel/dbModelToApiEntity.h"                  // dbModelToApiEntity2
#include "orionld/dbModel/dbModelFromApiAttribute.h"             // dbModelFromApiAttribute
#include "orionld/dbModel/dbModelAttributeCreatedAtLookup.h"     // dbModelAttributeCreatedAtLookup
#include "orionld/dbModel/dbModelAttributeCreatedAtSet.h"        // dbModelAttributeCreatedAtSet
#include "orionld/dbModel/dbModelAttributeLookup.h"              // dbModelAttributeLookup
#include "orionld/dbModel/dbModelEntityTypeLookup.h"             // dbModelEntityTypeLookup
#include "orionld/distOp/distOpRequests.h"                       // distOpRequests
#include "orionld/distOp/distOpResponses.h"                      // distOpResponses
#include "orionld/distOp/distOpListRelease.h"                    // distOpListRelease
#include "orionld/distOp/distOpFailure.h"                        // distOpFailure
#include "orionld/distOp/distOpSuccess.h"                        // distOpSuccess
#include "orionld/notifications/alteration.h"                    // alteration
#include "orionld/notifications/previousValuePopulate.h"         // previousValuePopulate
#include "orionld/notifications/sysAttrsStrip.h"                 // sysAttrsStrip
#include "orionld/serviceRoutines/orionldPutAttribute.h"         // Own Interface



// ----------------------------------------------------------------------------
//
// orionldPutAttribute -
//
bool orionldPutAttribute(void)
{
  char*   entityId     = orionldState.wildcard[0];
  char*   attrName     = orionldState.wildcard[1];
  char*   attrLongName = orionldState.in.pathAttrExpanded;
  KjNode* responseBody = kjObject(orionldState.kjsonP, NULL);

  // 01. GET the entity+attribute from the DB (dbAttrP)
  // 02. Check the payload body (with dbAttrP as input)
  //     - Error if something wrong in the check
  // 03. if (dbAttrP == NULL)
  //   - 404 if forwarding is OFF
  //   - distOpFailure if forwarding is ON
  // 04. distOpRequests
  // 05. if (attribute still there)
  //       - clone for TRoE (if necessary)
  //       - clone for Alterations (if necessary)
  //       - dbModelFromAttribute()
  //       - mongocAttributeReplace()
  //       - alterations
  // 06. distOpResponses
  // 07. responseFix()
  //
  char*   detail                = NULL;
  char*   entityType            = NULL;
  KjNode* dbEntityP             = mongocEntityLookup(entityId, NULL, NULL, NULL, &detail);
  KjNode* dbAttrP               = NULL;
  KjNode* apiAttributeP         = NULL;
  double  createdAt             = 0;
  char*   attrLongNameEq        = kaStrdup(&orionldState.kalloc, attrLongName);
  KjNode* apiAttributeAsEntityP = NULL;
  KjNode* dbEntityCopy          = NULL;
  KjNode* oldAttrP              = NULL;
  KjNode* apiAttributeClone     = NULL;
  bool    entityNotFoundLocally = false;
  bool    attrNotFoundLocally   = false;

  OrionldAlteration* alterationP                = NULL;
  KjNode*            finalApiEntityWithSysAttrs = NULL;
  KjNode*            finalApiEntity             = NULL;
  KjNode*            createdAtP                 = NULL;
  KjNode*            modifiedAtP                 = NULL;

  dotForEq(attrLongNameEq);

  if (dbEntityP == NULL)
  {
    if (orionldState.distributed == false)
    {
      orionldError(OrionldResourceNotFound, "Entity Not Found", entityId, 404);
      return false;
    }
    else
      entityNotFoundLocally = true;
  }
  else
  {
    // Extract the DB attribute from dbEntityP
    dbAttrP = dbModelAttributeLookup(dbEntityP, attrLongNameEq);
    if (dbAttrP == NULL)
    {
      if (orionldState.distributed == false)
      {
        orionldError(OrionldResourceNotFound, "Attribute Not Found", attrLongName, 404);
        return false;
      }
      else
        attrNotFoundLocally = true;
    }
    else
    {
      // GET Attribute creation date from database
      createdAt = dbModelAttributeCreatedAtLookup(dbAttrP);
      if (createdAt == -1)
      {
        orionldError(OrionldInternalError, "Database Error (attribute::createdAt field not present in database)", entityId, 500);
        return false;
      }

      // GET Entity Type from the DB
      entityType = dbModelEntityTypeLookup(dbEntityP, entityId);
    }
  }

  if (pCheckAttribute(entityId, orionldState.requestTree, true, NoAttributeType, true, NULL) == false)
    return false;  // pcheckAttribute() calls orionldError

  previousValuePopulate(NULL, dbAttrP, orionldState.in.pathAttrExpanded);

  // Distributed requests?
  DistOp* distOpList   = NULL;
  KjNode* entityObject = kjObject(orionldState.kjsonP, NULL);
  bool    localData    = true;
  if (orionldState.distributed == true)
  {
    KjNode* attrClone = kjClone(orionldState.kjsonP, orionldState.requestTree);

    attrClone->name = attrLongName;
    kjChildAdd(entityObject, attrClone);

    if ((entityType == NULL) && (orionldState.in.typeList.items == 1))
      entityType = orionldState.in.typeList.array[0];

    distOpList = distOpRequests(entityId, entityType, DoReplaceAttr, entityObject);
    if (entityObject->value.firstChildP == NULL)
      localData = false;
  }


  //
  // Local treatment?
  // Only if something is still left after the DistOps
  //
  if (localData == false)
  {
    // Make troe do NOTHING, as there was no local update
    orionldState.requestTree = NULL;
    goto response;
  }

  if (entityNotFoundLocally == true)
    distOpFailure(responseBody, NULL, "Entity Not Found", entityId, 404, attrName);
  else if (attrNotFoundLocally == true)
    distOpFailure(responseBody, NULL, "Attribute Not Found", entityId, 404, attrName);

  // Save requestTree before it is destroyed by dbModelFromApiAttribute (needed for notifications and TRoE
  apiAttributeP = kjClone(orionldState.kjsonP, orionldState.requestTree);

  // Convert to DB Model (dbModelFromApiAttribute adds creDat/modDate - creDate needs a modification)
  if (dbModelFromApiAttribute(orionldState.requestTree, NULL, NULL, NULL, NULL, true) == false)
    goto response;

  //
  // The attribute name needs to be in DB format (replace dots for '=')
  //
  orionldState.requestTree->name = attrLongNameEq;

  // Set creDate (mongocAttributeReplace sets modDate)
  dbModelAttributeCreatedAtSet(orionldState.requestTree, createdAt);
  kjTreeLog(orionldState.requestTree, "orionldState.requestTree", LmtSR);

  // Write to mongo
  if (mongocAttributeReplace(entityId, orionldState.requestTree, &detail) == false)
  {
    LM_E(("mongocAttributeReplace failed: %s", detail));
    if (distOpList == NULL)
    {
      orionldError(OrionldInternalError, "Database Error", detail, 500);
      return false;
    }
    else
      distOpFailure(responseBody, NULL, "Database Error", detail, 500, attrName);
  }

  // Alterations
  //   For this we need:
  //   o Entity ID
  //   o Entity Type
  //   o Resulting and complete API entity
  //   o Incoming API Attribute "fragment"
  //   o DB Entity as it was before the modification
  //
  apiAttributeAsEntityP = kjObject(orionldState.kjsonP, NULL);
  kjChildAdd(apiAttributeAsEntityP, apiAttributeP);

  apiAttributeP->name = orionldState.in.pathAttrExpanded;

  //
  // Resulting and complete API entity:
  // o Clone the dbEntity
  // o transform it into an API Entity
  // o Remove the attribute that was replaced (old copy)
  // o Insert a copy of the attribute that was replaced (new copy)
  //
  dbEntityCopy = kjClone(orionldState.kjsonP, dbEntityP);

  finalApiEntityWithSysAttrs = dbModelToApiEntity2(dbEntityCopy, true, RF_NORMALIZED, NULL, false, &orionldState.pd);

  if (finalApiEntityWithSysAttrs == NULL)
  {
    LM_E(("dbModelToApiEntity unable to convert DB Entity '%s' to API Entity (%s: %s)", entityId, orionldState.pd.title, orionldState.pd.detail));
    goto response;
  }

  oldAttrP = kjLookup(finalApiEntityWithSysAttrs, attrLongName);
  if (oldAttrP == NULL)
  {
    LM_E(("Unable to find the attribute '%s' in the entity '%s'", attrLongName, entityId));
    goto response;
  }

  apiAttributeClone = kjClone(orionldState.kjsonP, apiAttributeP);
  if (apiAttributeClone == NULL)
  {
    LM_E(("Unable to clone the attribute '%s' in the entity '%s'", attrLongName, entityId));
    goto response;
  }
  kjChildRemove(finalApiEntityWithSysAttrs, oldAttrP);
  kjChildAdd(finalApiEntityWithSysAttrs, apiAttributeClone);

  // Now get createdAt/modifiedAt from oldAttrP
  createdAtP  = kjLookup(oldAttrP, "createdAt");
  modifiedAtP = kjLookup(oldAttrP, "modifiedAt");

  if (createdAtP != NULL)
  {
    kjChildRemove(oldAttrP, createdAtP);
    kjChildAdd(apiAttributeClone, createdAtP);
  }

  if (modifiedAtP != NULL)
  {
    kjChildRemove(oldAttrP, modifiedAtP);
    kjChildAdd(apiAttributeClone, modifiedAtP);
  }


  finalApiEntity = kjClone(orionldState.kjsonP, finalApiEntityWithSysAttrs);  // Check for NULL !
  sysAttrsStrip(finalApiEntity);


  alterationP = alteration(entityId, entityType, finalApiEntity, apiAttributeAsEntityP, dbEntityP);
  alterationP->finalApiEntityWithSysAttrsP = finalApiEntityWithSysAttrs;

 response:
  if (distOpList != NULL)
  {
    distOpResponses(distOpList, responseBody);
    distOpListRelease(distOpList);
  }

  responseFix(responseBody, DoReplaceAttr, 204, entityId);

  if (troe == true)
    orionldState.requestTree = apiAttributeP;

  return true;
}
