#ifndef SRC_LIB_ORIONLD_DB_DBMODELFROMAPIATTRIBUTE_H_
#define SRC_LIB_ORIONLD_DB_DBMODELFROMAPIATTRIBUTE_H_

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
extern bool dbModelFromApiAttribute(KjNode* attrP, KjNode* dbAttrsP, KjNode* attrAddedV, KjNode* attrRemovedV);

#endif  // SRC_LIB_ORIONLD_DB_DBMODELFROMAPIATTRIBUTE_H_
