#ifndef SRC_LIB_ORIONLD_DB_DBMODELFROMAPIENTITY_H_
#define SRC_LIB_ORIONLD_DB_DBMODELFROMAPIENTITY_H_

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
#include "kjson/KjNode.h"                                        // KjNode
}



// -----------------------------------------------------------------------------
//
// dbModelFromApiEntity - modify the request tree to match the db model
//
// Modifications:
//
//   * Entity Level (dbModelEntity)
//     * "id" can't be modified    (make sure it's removed and partial error reported by pCheckEntity)
//     * "type" can't be modified  (make sure it's removed and partial error reported by pCheckEntity)
//     * "scope" doesn't exist yet (make sure it's removed and partial error reported by pCheckEntity)
//     * "modDate" is set with orionldState.requestTime
//     * "attrs" member is created and added
//     * It's an attribute - move to "attrs" and call "attributeToDbModel" on it.
//     * if Array - datasetId - error for now (partial 501)
//     * if != Object - error (as pCheckAttribute makes sure the request tree is normalized)
//     * move the attribute there + call "Level 1 Function" (orionldDbModelAttribute)
//
//   * Attribute Level (dbModelAttribute)
//     * If Array, recursive call for each member (set the name to the sttribute name)
//     * "datasetId" present - call orionldDbModelAttributeDatasetId
//     * "type" can't be modified
//     * "value"/"object"/"languageMap" changes name to "value" and RHS stays as is
//     * "observedAt" is made an Object with single member "value"
//     * "unitCode" is made an Object with single member "value"
//     * "md" is created and added
//
//   * Sub-Attribute Level (dbModelSubAttribute)
//
extern bool dbModelFromApiEntity(KjNode* entityP, KjNode* dbAttrsP, KjNode* dbAttrNamesP, bool creation);

#endif  // SRC_LIB_ORIONLD_DB_DBMODELFROMAPIENTITY_H_
