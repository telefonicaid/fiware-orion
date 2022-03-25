#ifndef SRC_LIB_ORIONLD_TYPES_ORIONLDALTERATION_H_
#define SRC_LIB_ORIONLD_TYPES_ORIONLDALTERATION_H_

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
#include "kjson/KjNode.h"                           // KjNode
}



// -----------------------------------------------------------------------------
//
// CachedSubscription - can't include subCache.h as subCache.h includes this header ...
//
struct CachedSubscription;



// -----------------------------------------------------------------------------
//
// OrionldAlterationType -
//
typedef enum OrionldAlterationType
{
  EntityCreated = 1,
  EntityDeleted,
  EntityModified,  // Any of the Attribute alterations imply EntityModified
  AttributeAdded,
  AttributeDeleted,
  AttributeValueChanged,
  AttributeMetadataChanged,
  AttributeModifiedAtChanged,
  OrionldAlterationTypes = AttributeModifiedAtChanged
} OrionldAlterationType;



// -----------------------------------------------------------------------------
//
// OrionldAttributeAlteration -
//
typedef struct OrionldAttributeAlteration
{
  OrionldAlterationType  alterationType;
  char*                  attrName;
  char*                  attrNameEq;
} OrionldAttributeAlteration;



// -----------------------------------------------------------------------------
//
// OrionldAlteration -
//
typedef struct OrionldAlteration
{
  char*                        entityId;
  char*                        entityType;
  KjNode*                      entityP;              // New entity, after the update
  KjNode*                      patchTree;            // Patch Info, to merge with 'dbEntityP' to create 'entityP'
  KjNode*                      dbEntityP;            // Entity in DB before the modification
  int                          alteredAttributes;
  OrionldAttributeAlteration*  alteredAttributeV;
  struct OrionldAlteration*    next;
} OrionldAlteration;



// -----------------------------------------------------------------------------
//
// OrionldAlterationMatch
//
typedef struct OrionldAlterationMatch
{
  OrionldAlteration*              altP;
  OrionldAttributeAlteration*     altAttrP;
  CachedSubscription*             subP;
  struct OrionldAlterationMatch*  next;
} OrionldAlterationMatch;



// -----------------------------------------------------------------------------
//
// orionldAlterationType -
//
extern const char* orionldAlterationType(OrionldAlterationType altType);

#endif  // SRC_LIB_ORIONLD_TYPES_ORIONLDALTERATION_H_
