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
extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjBuilder.h"                                     // kjObject, kjChildAdd, ...
#include "kjson/kjLookup.h"                                      // kjLookup
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo

#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/types/OrionldProblemDetails.h"                 // OrionldProblemDetails
#include "orionld/rest/OrionLdRestService.h"                     // OrionLdRestService
#include "orionld/db/dbConfiguration.h"                          // dbEntityTypeGet
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/context/orionldContextItemAliasLookup.h"       // orionldContextItemAliasLookup
#include "orionld/kjTree/kjStringValueLookupInArray.h"           // kjStringValueLookupInArray
#include "orionld/serviceRoutines/orionldGetEntityType.h"        // Own Interface



// -----------------------------------------------------------------------------
//
// kjAttrIdLookup - match a value for 'id' field in object array
//
static KjNode* kjAttrIdLookup(KjNode* arrayP, const char* attrName)
{
  for (KjNode* attrP = arrayP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    KjNode* idNodeP = kjLookup(attrP, "id");
    if ((idNodeP == NULL) || (idNodeP->type != KjString))
      continue;

    if (strcmp(idNodeP->value.s, attrName) == 0)
      return attrP;
  }

  return NULL;
}



// ----------------------------------------------------------------------------
//
// outAttrCreate -
//
static void outAttrCreate(KjNode* attributeDetailsNodeP, const char* attrName, const char* attrType)
{
  char*   attrNameAlias       = orionldContextItemAliasLookup(orionldState.contextP, attrName, NULL, NULL);
  char*   attrTypeAlias       = orionldContextItemAliasLookup(orionldState.contextP, attrType, NULL, NULL);

  KjNode* attrObjectP         = kjObject(orionldState.kjsonP, NULL);
  KjNode* idNodeP             = kjString(orionldState.kjsonP, "id", attrName);
  KjNode* typeNodeP           = kjString(orionldState.kjsonP, "type", "Attribute");
  KjNode* attributeNameNodeP  = kjString(orionldState.kjsonP, "attributeName", attrNameAlias);
  KjNode* attributeTypesNodeP = kjArray(orionldState.kjsonP,  "attributeTypes");

  KjNode* attrTypeAliasNodeP  = kjString(orionldState.kjsonP, NULL, attrTypeAlias);

  kjChildAdd(attributeTypesNodeP, attrTypeAliasNodeP);

  kjChildAdd(attrObjectP, idNodeP);
  kjChildAdd(attrObjectP, typeNodeP);
  kjChildAdd(attrObjectP, attributeNameNodeP);
  kjChildAdd(attrObjectP, attributeTypesNodeP);

  kjChildAdd(attributeDetailsNodeP, attrObjectP);
}



// ----------------------------------------------------------------------------
//
// outAttrInfoAdd -
//
static void outAttrInfoAdd(KjNode* outAttrP, const char* attrType)
{
  KjNode* attributeTypesNodeP = kjLookup(outAttrP, "attributeTypes");

  if (attributeTypesNodeP == NULL)
    return;

  //
  // Is attrType already present in attributeTypesNodeP?
  // If not, add it
  //
  if (kjStringValueLookupInArray(attributeTypesNodeP, attrType) == NULL)
  {
    KjNode* attrTypeNodeP = kjString(orionldState.kjsonP, NULL, attrType);
    kjChildAdd(attributeTypesNodeP, attrTypeNodeP);
  }
}



// ----------------------------------------------------------------------------
//
// orionldGetEntityType -
//
// FIXME: Only local types are taken into account, for now
//
bool orionldGetEntityType(ConnectionInfo* ciP)
{
  OrionldProblemDetails  pd;
  int                    entities;
  char*                  typeExpanded = orionldContextItemExpand(orionldState.contextP, orionldState.wildcard[0], true, NULL);
  char*                  typeAlias    = orionldContextItemAliasLookup(orionldState.contextP, typeExpanded, NULL, NULL);
  KjNode*                entityTypeV;

  entityTypeV = dbEntityTypeGet(&pd, typeExpanded, &entities);

  if (entityTypeV == NULL)
  {
    // dbEntityTypeGet has filled in 'pd'
    return false;
  }
  else if (entities == 0)
  {
    LM_E(("dbEntityTypeGet: no entities found"));
    orionldErrorResponseCreate(OrionldResourceNotFound, "Entity Type Not Found", typeExpanded);
    orionldState.httpStatusCode = 404;
    return false;
  }

  //
  // The output  from dbEntityTypeGet is as follows:
  // [
  //   {
  //     "https://uri.etsi.org/ngsi-ld/default-context/P1": "Property",
  //     "https://uri.etsi.org/ngsi-ld/default-context/R1": "Relationship"
  //   },
  //   {
  //     "https://uri.etsi.org/ngsi-ld/default-context/P2": "Property",
  //     "https://uri.etsi.org/ngsi-ld/default-context/R2": "Relationship"
  //   },
  //   {
  //     "https://uri.etsi.org/ngsi-ld/default-context/P1": "Property",
  //     "https://uri.etsi.org/ngsi-ld/default-context/P2": "Property",
  //     "https://uri.etsi.org/ngsi-ld/default-context/R1": "Relationship",
  //     "https://uri.etsi.org/ngsi-ld/default-context/R2": "Relationship"
  //   }
  // ]
  //
  // So, now we need to loop over the objects (attribute lists) and populate the output object with it
  //
  // The output object looks like this:
  // {
  //   "id": <FQN of the type>,
  //   "type": "EntityTypeInformation",
  //   "typeName": <shortname>,           // Mandatory! (will try to change that - only if shortName found)
  //   "entityCount": 27,                 // Number of entities with this type
  //   "attributeDetails": [              // Array of Attribute - See GET /attributes?details=true
  //     {
  //       "id": "",
  //       "type": "",
  //       "attributeName": "",
  //       "attributeTypes": []
  //     },
  //     {
  //     },
  //     ...
  //   ]
  // }
  //
  orionldState.responseTree = kjObject(orionldState.kjsonP, NULL);

  KjNode* idNodeP               = kjString(orionldState.kjsonP, "id", typeExpanded);
  KjNode* typeNodeP             = kjString(orionldState.kjsonP, "type", "EntityTypeInformation");
  KjNode* typeNameNodeP         = kjString(orionldState.kjsonP, "typeName", typeAlias);
  KjNode* entityCountNodeP      = kjInteger(orionldState.kjsonP, "entityCount", entities);
  KjNode* attributeDetailsNodeP = kjArray(orionldState.kjsonP, "attributeDetails");

  kjChildAdd(orionldState.responseTree, idNodeP);
  kjChildAdd(orionldState.responseTree, typeNodeP);
  kjChildAdd(orionldState.responseTree, typeNameNodeP);
  kjChildAdd(orionldState.responseTree, entityCountNodeP);
  kjChildAdd(orionldState.responseTree, attributeDetailsNodeP);

  //
  // Now populate 'attributeDetailsNodeP' with the info from 'entityTypeV'
  //
  for (KjNode* dbEntityP = entityTypeV->value.firstChildP; dbEntityP != NULL; dbEntityP = dbEntityP->next)
  {
    // Loop over all attrs of entityP
    for (KjNode* dbAttrP = dbEntityP->value.firstChildP; dbAttrP != NULL; dbAttrP = dbAttrP->next)
    {
      // Attr already present in attributeDetailsNodeP?
      KjNode* outAttrP = kjAttrIdLookup(attributeDetailsNodeP, dbAttrP->name);

      if (outAttrP == NULL)
        outAttrCreate(attributeDetailsNodeP, dbAttrP->name, dbAttrP->value.s);
      else
        outAttrInfoAdd(outAttrP, dbAttrP->value.s);
    }
  }

  orionldState.httpStatusCode = 200;
  return true;
}
