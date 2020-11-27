#ifndef TEMPORAL_COMMON_H_
#define TEMPORAL_COMMON_H_

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

#include "orionld/db/dbDriver.h"                                 // database driver header
#include "orionld/db/dbConfiguration.h"                          // DB_DRIVER_MONGOC

extern "C"
{
#include "kjson/kjson.h"                                         // Kjson
#include "kjson/KjNode.h"                                        // KjNode
}

#include <postgresql/libpq-fe.h>                                 // For Postgres

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
enum OrionldTemporalAttributeValueTypeEnum
{
  EnumValueString = 0,
  EnumValueNumber,
  EnumValueBool,
  EnumValueObject,
  EnumValueArray,
  EnumValueDateTime
};


// -----------------------------------------------------------------------------
//
// OrionldTemporalDbConnectionState - Contains general Temporal variables
//
//
//
extern PGconn* oldPgDbConnection;
extern PGconn* oldPgDbTenantConnection;
extern PGresult* oldPgTenandDbResult;

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
  double          createdAt;
  double          modifiedAt;
  double          observedAt;
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
  char*                                     attributeName;
  char*                                     attributeType;  // Chandra to change this to Enum - Property, Geo-property & Relationship kjType
  OrionldTemporalAttributeValueTypeEnum     attributeValueType;
  bool                                      subProperty;
  char*                                     unitCode;
  char*                                     dataSetId;
  char*                                     valueString;
  long long int                             valueNumber;
  bool                                      valueBool;
  char*                                     valueArray;
  char*                                     valueObject;
  double                                    valueDatetime;
  double                                    createdAt;
  double                                    modifiedAt;
  double                                    observedAt;
  char*                                     instanceId;
  double                                    geoProperty[];  // Chandra-TBD
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
  char*                                     attributeName;
  char*                                     subAttributeName;
  char*                                     subAttributeType;  // Chandra to change this to Enum - Property, Geo-property & Relationship kjType
  OrionldTemporalAttributeValueTypeEnum     subAttributeValueType;
  char*                                     subAttributeUnitCode;
  char*                                     subAttributeDataSetId;
  char*                                     subAttributeValueString;
  bool                                      subAttributeValueBoolean;
  char*                                     subAttributeValueArray;
  long long int                             subAttributeValueNumber;
  char*                                     subAttributeValueRelation;
  char*                                     subAttributeValueObject;
  double                                    subAttributeValueDatetime;
  double                                    createdAt;
  double                                    modifiedAt;
  double                                    observedAt;
  char*                                     attrInstanceId;
  float                                     subAttributeGeoProperty[];  // Chandra-TBD
} OrionldTemporalDbSubAttributeTable;


// -----------------------------------------------------------------------------
//
// OrionldTemporalDbAllTables - Contains all the PostGress Db Table contents
//
//
//

typedef struct OrionldTemporalDbAllTables
{
  OrionldTemporalDbEntityTable*         entityTableArray;
  int entityTableArrayItems;
  OrionldTemporalDbAttributeTable*      attributeTableArray;
  int attributeTableArrayItems;
  OrionldTemporalDbSubAttributeTable*   subAttributeTableArray;
  int subAttributeTableArrayItems;
} OrionldTemporalDbAllTables;

// ----------------------------------------------------------------------------
//
// bool TemporalPgDBConnectorOpen() - function to open the Postgres database connection
//
// ----------------------------------------------------------------------------
extern bool TemporalPgDBConnectorOpen();

// ----------------------------------------------------------------------------
//
// bool TemporalPgDBConnectorOpen() - function to close the Postgres database connection gracefully
//
// ----------------------------------------------------------------------------
extern bool TemporalPgDBConnectorClose();

// -----------------------------------------------------------------------------
//
// temporalOrionldCommonBuildInsertEntity - initialize the thread-local variables of temporalOrionldCommonState
//
OrionldTemporalDbAllTables*  attrSubattrExtract();

// -----------------------------------------------------------------------------
//
// temporalOrionldCommonBuildInsertEntity - initialize the thread-local variables of temporalOrionldCommonState
//
OrionldTemporalDbAllTables*  singleTemporalEntityExtract();

// -----------------------------------------------------------------------------
//
// temporalExecSqlStatement
//
//
extern bool TemporalConstructInsertSQLStatement(OrionldTemporalDbAllTables* dbAllTablesLocal, bool entityUpdateFlag);

// ----------------------------------------------------------------------------
//
// temporalTenantInitialise -
//
extern bool temporalTenantInitialise(char *tenantName);


// ----------------------------------------------------------------------------
//
// PGconn* TemporalPgTenantDBConnectorOpen(ichar* tenantName) - function to open the Postgres database connection
//
// ----------------------------------------------------------------------------
extern bool TemporalPgTenantDBConnectorOpen(char* tenantName);


void  attrSubAttrExtract(KjNode* subAttrP, OrionldTemporalDbSubAttributeTable* dbSubAttributeTableLocal);


//void  attrExtract(KjNode* attrP, OrionldTemporalDbAttributeTable* dbAttributeTableLocal, int attrIndex);
//void  attrExtract(KjNode* attrP, OrionldTemporalDbAttributeTable* dbAttributeTableLocal, OrionldTemporalDbSubAttributeTable** dbSubAttributeTableLocal, int attrIndex);
void attrExtract(KjNode* attrP, OrionldTemporalDbAttributeTable* dbAttributeTableLocal, OrionldTemporalDbSubAttributeTable* dbSubAttributeTableLocal, int attrIndex, int* subAttrIndex);

void allValuesRenderAttr (OrionldTemporalDbAttributeTable* attrLocalP, char* allValues, int allValuesSize);


void allValuesRenderSubAttr (OrionldTemporalDbSubAttributeTable* attrLocalP, char* allValues, int allValuesSize);

void entityExtract (OrionldTemporalDbAllTables* allTab, KjNode* entityP, bool arrayFlag, int entityIndex);


#endif  // TEMPORAL_COMMON_H_
