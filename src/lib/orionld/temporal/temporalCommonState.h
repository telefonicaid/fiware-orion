#ifndef TEMPORAL_ORIONLD_COMMON_STATE_H_
#define TEMPORAL_ORIONLD_COMMON_STATE_H_

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
* Author: Chandra Challagonda
*/

#include <time.h>                                                // struct timespec

#include "orionld/db/dbDriver.h"                                 // database driver header
#include "orionld/db/dbConfiguration.h"                          // DB_DRIVER_MONGOC

extern "C"
{
#include "kjson/kjson.h"                                         // Kjson
#include "kjson/KjNode.h"                                        // KjNode
}

#include "common/globals.h"                                      // ApiVersion
#include "common/MimeType.h"                                     // MimeType
#include "rest/HttpStatusCode.h"                                 // HttpStatusCode
#include "rest/Verb.h"                                           // Verb
#include "orionld/common/QNode.h"                                // QNode
#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/types/OrionldGeoJsonType.h"                    // OrionldGeoJsonType
#include "orionld/types/OrionldPrefixCache.h"                    // OrionldPrefixCache
#include "orionld/common/OrionldResponseBuffer.h"                // OrionldResponseBuffer
#include "orionld/context/OrionldContext.h"                      // OrionldContext

// -----------------------------------------------------------------------------
//
// OrionldTemporalAttributeValueTypeEnum 
//
//
typedef enum OrionldTemporalAttributeValueTypeEnum
{
  ValueString = 0,   
  ValueNumber,  
  ValueBool,           
  ValueRelation,  
  ValueObject,  
  ValueGeo,
  ValueDateTime,          
} OrionldTemporalAttributeValueTypeEnum;

// -----------------------------------------------------------------------------
//
// OrionldTemporalDbConnectionState - Contains general Temporal variables
//
// 
//
typedef struct OrionldTemporalDbConnectionState
{
  bool oldPgDbConnectionFlag;
  bool oldPgDbTenantConnectionFlag;
} OrionldTemporalDbConnectionState;

// -----------------------------------------------------------------------------
//
// OrionldTemporalCommonStateGeneral - Contains general Temporal variables
//
// 
//
typedef struct OrionldTemporalCommonStateGeneral
{
  bool oldPgDbConnectionFlag;
  bool oldPgDbTenantConnectionFlag;
} OrionldTemporalCommonStateGeneral;

// -----------------------------------------------------------------------------
//
// OrionldTemporalCommonStateGeneral - Contains entity contents
//
// 
//
typedef struct OrionldTemporalDbEntityTable
{
  char*           entityId; 
  char*           entityType;
  struct timespec createdAt;
  struct timespec modifiedAt;
  struct timespec observedAt;
  float           geoProperty[];  // Chandra-TBD
} OrionldTemporalDbEntityTable;

// -----------------------------------------------------------------------------
//
// OrionldTemporalDbAttributeTable - Contains Attribute contents
//
// 
//
typedef struct OrionldTemporalDbAttributeTable
{
  char*                                     entityId; 
  char*                                     attributeId;
  char*                                     attributeName; 
  OrionldTemporalAttributeValueTypeEnum     attributeValueType; 
  bool                                      subProperty; 
  char*                                     unitCode; 
  char*                                     dataSetId;
  char*                                     valueString; 
  bool                                      valueBoolean; 
  double                                    valueNumber;
  char*                                     value_relation;
  char*                                     valueObject; 
  struct timespec                           valueDatetime;
  struct timespec                           createdAt;
  struct timespec                           modifiedAt;
  struct timespec                           observedAt;
  float                                     geoProperty[];  // Chandra-TBD
} OrionldTemporalDbAttributeTable;

// -----------------------------------------------------------------------------
//
// OrionldTemporalDbSubAttributeTable - Contains sub Attribute contents
//
// 
//
typedef struct OrionldTemporalDbSubAttributeTable
{
  char*                                     entityId; 
  char*                                     attributeId;
  char*                                     subAtrributeId; 
  OrionldTemporalAttributeValueTypeEnum     attributeValueType; 
  bool                                      subProperty; 
  char*                                     unitCode; 
  char*                                     dataSetId;
  char*                                     valueString; 
  bool                                      valueBoolean; 
  double                                    valueNumber;
  char*                                     value_relation;
  char*                                     valueObject; 
  struct timespec                           valueDatetime;
  struct timespec                           createdAt;
  struct timespec                           modifiedAt;
  struct timespec                           observedAt;
  float                                     geoProperty[];  // Chandra-TBD
} OrionldTemporalDbSubAttributeTable;



// -----------------------------------------------------------------------------
//
// temporalOrionldCommonStateInit - initialize the thread-local variables of temporalOrionldCommonState
//
extern void temporalOrionldCommonStateInit(void);



// -----------------------------------------------------------------------------
//
// temporalOrionldCommonStateRelease - releasing the thread-local variables of temporalOrionldCommonState
//
extern void temporalOrionldCommonStateRelease(void);


#endif  // TEMPORAL_ORIONLD_COMMON_STATE_H_
