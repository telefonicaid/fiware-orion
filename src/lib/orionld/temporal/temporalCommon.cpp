/*
*
* Copyright 2020 FIWARE Foundation e.V.
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
* Author: Chandra Challagonda & Ken Zangelin
*/
#include <string.h>                                              // strlen

extern "C"
{
#include "kbase/kTime.h"                                         // kTimeGet
#include "kjson/kjBufferCreate.h"                                // kjBufferCreate
#include "kjson/KjNode.h"                                        // KjNode
#include "kjson/kjFree.h"                                        // kjFree
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaBufferInit.h"                                 // kaBufferInit
#include "kjson/kjRender.h"                                      // kjRender
#include "kjson/kjBuilder.h"                                     // kjChildRemove .....
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*
#include "orionld/common/uuidGenerate.h"                         // for uuidGenerate

#include "rest/ConnectionInfo.h"                                 // ConnectionInfo
#include "rest/HttpStatusCode.h"                                 // SccNotImplemented
#include "orionld/common/orionldState.h"                         // orionldState
#include "orionld/common/orionldErrorResponse.h"                 // orionldErrorResponseCreate
#include "orionld/common/QNode.h"                                // QNode
#include "orionld/common/orionldTenantCreate.h"                  // Own interface
#include "orionld/rest/OrionLdRestService.h"                     // OrionLdRestService
#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/db/dbConfiguration.h"                          // DB_DRIVER_MONGOC
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContext
#include "orionld/context/orionldContextItemExpand.h"            // orionldContextItemExpand
#include "orionld/temporal/temporalCommon.h"                     // Temporal common
#include "orionld/temporal/geoPropertyExtract.h"                 // geoPropertyExtract
#include "orionld/temporal/temporalTenantInitialise.h"           // Postgres db functions





// -----------------------------------------------------------------------------
//
// lmLogTree - FIXME: move to logMsg.h/cpp
//
void lmLogTree(const char* prefix, const char* what, KjNode* tree)
{
  char buf[4096];
  kjRender(orionldState.kjsonP, tree, buf, sizeof(buf));
  LM_TMP(("%s: %s: %s", prefix, what, buf));
}



// -----------------------------------------------------------------------------
//
// dbValueEnumString -
//
static const char* dbValueEnumString(OrionldTemporalAttributeValueTypeEnum valueType)
{
  switch (valueType)
  {
  case EnumValueString:    return "value_string";
  case EnumValueNumber:    return "value_number";
  case EnumValueBool:      return "value_boolean";
  case EnumValueArray:     return "value_array";
  case EnumValueRelation:  return "value_relation";
  case EnumValueObject:    return "value_object";
  case EnumValueDateTime:  return "value_datetime";
  default:                 return "Invalid attribute value type";
  }
}



// -----------------------------------------------------------------------------
//
// entityExtract -
//
void entityExtract
(
  OrionldTemporalDbAllTables*  allTab,
  KjNode*                      entityP,
  bool                         entityInArray,
  int                          entityIndex,
  int*                         attrIndexP,
  int*                         subAttrIndexP
)
{
  int    attrIndex    = *attrIndexP;
  int    subAttrIndex = *subAttrIndexP;
  double now          = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;  // FIXME: to orionldState

  if (entityInArray)
  {
    KjNode* idP   = kjLookup(entityP, "id");
    KjNode* typeP = kjLookup(entityP, "type");

    allTab->entityTableArray[entityIndex].entityId = idP->value.s;
    kjChildRemove(entityP, idP);

    if (typeP != NULL)
    {
      allTab->entityTableArray[entityIndex].entityType = typeP->value.s;
      kjChildRemove(entityP, typeP);
    }
    else
      allTab->entityTableArray[entityIndex].entityType = NULL;
  }
  else
  {
    allTab->entityTableArray[entityIndex].entityId   = orionldState.payloadIdNode->value.s;
    allTab->entityTableArray[entityIndex].entityType = (orionldState.payloadTypeNode != NULL)? orionldState.payloadTypeNode->value.s : NULL;
    allTab->entityTableArray[entityIndex].createdAt  = now;
    allTab->entityTableArray[entityIndex].modifiedAt = now;
  }

  LM_TMP (("CCSR : entityExtract func and allTab->entityTableArray[entityIndex].entityId %s", allTab->entityTableArray[entityIndex].entityId));

  allTab->entityTableArray[entityIndex].createdAt  = now;
  allTab->entityTableArray[entityIndex].modifiedAt = now;

  //
  // Count the total number of attributes and sub-attributes
  //
  int  attributesCount = 0;
  int  subAttrCount    = 0;
  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    attributesCount++;

    for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
    {
      subAttrCount++;
    }
  }

  for (KjNode* attrP = entityP->value.firstChildP; attrP != NULL; attrP = attrP->next)
  {
    allTab->attributeTableArray[attrIndex].entityId = allTab->entityTableArray[entityIndex].entityId;
    attrExtract(attrP, &allTab->attributeTableArray[attrIndex], allTab->subAttributeTableArray , attrIndex, &subAttrIndex);
    allTab->attributeTableArrayItems++;
    attrIndex++;
  }

  *attrIndexP    = attrIndex;
  *subAttrIndexP = subAttrIndex;
}



// -----------------------------------------------------------------------------
//
// temporalEntityExtract -
//
OrionldTemporalDbAllTables* temporalEntityExtract(void)
{
  OrionldTemporalDbAllTables*  dbAllTablesLocal; // Chandra - TBI

  dbAllTablesLocal = (OrionldTemporalDbAllTables*) kaAlloc(&orionldState.kalloc, sizeof(OrionldTemporalDbAllTables));
  bzero(dbAllTablesLocal, sizeof(OrionldTemporalDbAllTables));

  int attribArrayTotalSize = 100;  // Chandra - TBI
  LM_K(("CCSR: at func temporalEntityExtract dbAllTables->entityTableArrayItems pointer - 000.1 %i", dbAllTablesLocal->attributeTableArray));

  dbAllTablesLocal->attributeTableArray = (OrionldTemporalDbAttributeTable*)
    kaAlloc(&orionldState.kalloc, (attribArrayTotalSize * sizeof(OrionldTemporalDbAttributeTable)));
  LM_K(("CCSR: at func temporalEntityExtract dbAllTables->entityTableArrayItems pointer - 000.2 %i", dbAllTablesLocal->attributeTableArray));

  bzero(dbAllTablesLocal->attributeTableArray, attribArrayTotalSize);
  LM_K(("CCSR: at func temporalEntityExtract dbAllTables->entityTableArrayItems pointer %i", dbAllTablesLocal->attributeTableArray));

  // int subAttribArrayTotalSize = subAttrCount * sizeof(OrionldTemporalDbSubAttributeTable);
  int subAttribArrayTotalSize = 100; // Chandra - TBI
  dbAllTablesLocal->subAttributeTableArray =
    (OrionldTemporalDbSubAttributeTable*) kaAlloc(&orionldState.kalloc, (subAttribArrayTotalSize * sizeof(OrionldTemporalDbSubAttributeTable)));
  bzero(dbAllTablesLocal->subAttributeTableArray, (subAttribArrayTotalSize * sizeof(OrionldTemporalDbSubAttributeTable)));
  LM_K(("CCSR: at func temporalEntityExtract dbAllTables->subAttributeTableArray pointer %i", dbAllTablesLocal->subAttributeTableArray));


  dbAllTablesLocal->entityTableArrayItems = 0;
  dbAllTablesLocal->attributeTableArrayItems = 0;
  dbAllTablesLocal->subAttributeTableArrayItems = 0;

  //  OrionldTemporalDbEntityTable*        dbEntityTableLocal;
  //  OrionldTemporalDbAttributeTable*     dbAttributeTableLocal;
  //  OrionldTemporalDbSubAttributeTable*  dbSubAttributeTableLocal;

  lmLogTree("CCSR", "orionldState.requestTree", orionldState.requestTree);

  int entityIndex  = 0;
  int attrIndex    = 0;
  int subAttrIndex = 0;

  // dbAllTablesLocal->attributeTableArray = dbAttributeTableLocal;
  // dbAllTablesLocal->subAttributeTableArray = dbSubAttributeTableLocal;

  //  orionldState.requestTree->type == KjArray;
  if (orionldState.requestTree->type == KjArray)
  {
    // int entityCount = 0;
    for (KjNode* entityP = orionldState.requestTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      LM_K(("CCSR : at func & first FOR loop temporalEntityExtract entityP %i", entityP));
      dbAllTablesLocal->entityTableArrayItems++;
    }

    LM_K(("CCSR: at func temporalEntityExtract Number of Entities after count %i", dbAllTablesLocal->entityTableArrayItems));

    dbAllTablesLocal->entityTableArray =
      (OrionldTemporalDbEntityTable*) kaAlloc(&orionldState.kalloc, (dbAllTablesLocal->entityTableArrayItems * sizeof(OrionldTemporalDbEntityTable)) );
    bzero(dbAllTablesLocal->entityTableArray, (dbAllTablesLocal->entityTableArrayItems * sizeof(OrionldTemporalDbEntityTable)));
    LM_K(("CCSR: at func temporalEntityExtract dbAllTables->entityTableArrayItems pointer %i", dbAllTablesLocal->entityTableArrayItems));
    LM_K(("CCSR: at func temporalEntityExtract dbAllTables->entityTableArray pointer %i", dbAllTablesLocal->entityTableArray));
    LM_K(("CCSR: at func temporalEntityExtract dbAllTables pointer %i", dbAllTablesLocal));

    // dbAllTablesLocal->entityTableArray = dbEntityTableLocal;

    for (KjNode* entityP = orionldState.requestTree->value.firstChildP; entityP != NULL; entityP = entityP->next)
    {
      LM_TMP(("CCSR : at func & second FOR loop temporalEntityExtract entityP %i", entityP));
      entityExtract (dbAllTablesLocal, entityP, true, entityIndex, &attrIndex, &subAttrIndex );
      // dbAllTablesLocal->entityTableArrayItems++;

      LM_TMP(("CCSR: After callig entityExtract - Array and attributeValueType %i, entityId %s, attribute %s",
              dbAllTablesLocal->attributeTableArray[5].attributeValueType, dbAllTablesLocal->attributeTableArray[5].entityId,
              dbAllTablesLocal->attributeTableArray[5].attributeName));

      entityIndex++;
    }
  }
  else
  {
    dbAllTablesLocal->entityTableArray = (OrionldTemporalDbEntityTable*) kaAlloc(&orionldState.kalloc, sizeof(OrionldTemporalDbEntityTable));
    bzero(dbAllTablesLocal->entityTableArray, sizeof(OrionldTemporalDbEntityTable));

    // dbAllTablesLocal->entityTableArray = dbEntityTableLocal;

    entityExtract(dbAllTablesLocal, orionldState.requestTree, false, entityIndex, &attrIndex, &subAttrIndex );
    dbAllTablesLocal->entityTableArrayItems++;
  }

  return dbAllTablesLocal;
}



// -----------------------------------------------------------------------------
//
// attrExtract -
//
void attrExtract
(
  KjNode*                             attrP,
  OrionldTemporalDbAttributeTable*    dbAttributeTableLocal,
  OrionldTemporalDbSubAttributeTable* dbSubAttributeTableLocal,
  int                                 attrIndex,
  int*                                subAttrIndexP
)
{
  double now = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;  // FIXME: to orionldState

  LM_TMP(("CCSR - In attrExtract function "));
  int subAttrIx = *subAttrIndexP;

  dbAttributeTableLocal->attributeName = attrP->name;
  LM_TMP(("CCSR - In attrExtract function attributename %s ", attrP->name));

  if (attrP->type != KjObject)
  {
    LM_W(("Temporal - Bad Input - Key values not supported"));
    return;
  }

  //  adding instance id to map to sub attributes
  dbAttributeTableLocal->instanceId = kaAlloc(&orionldState.kalloc, 64);
  uuidGenerate(dbAttributeTableLocal->instanceId);

  KjNode* observedAtP = kjLookup(attrP, "observedAt");
  if (observedAtP != NULL)
  {
    // if (observedAtP->type == KjString)
    //    dbAttributeTableLocal->observedAt = parse8601Time(observedAtP->value.s);
    // else
    dbAttributeTableLocal->observedAt = 49;
    LM_TMP(("CCSR - Temporal - attrP Found observedAt %s",observedAtP->value.s));
    kjChildRemove(attrP,observedAtP);
  }

  KjNode* nodeP  = kjLookup(attrP, "unitCode");
  if (nodeP != NULL)
  {
    LM_TMP(("CCSR - attrP Found unitCode "));
    kjChildRemove (attrP,nodeP);
    // Chandra-TBI
  }

  nodeP  = kjLookup(attrP, "location");
  if (nodeP != NULL)
  {
    LM_TMP(("CCSR - attrP Found Location "));
    kjChildRemove (attrP,nodeP);
    geoPropertyExtract(nodeP, attrP->name, dbAttributeTableLocal->instanceId, dbAttributeTableLocal->entityId);
  }

  nodeP  = kjLookup(attrP, "operationSpace");
  if (nodeP != NULL)
  {
    LM_TMP(("CCSR - attrP Found operationSpace "));
    kjChildRemove (attrP,nodeP);
    // Chandra-TBI
  }

  nodeP  = kjLookup(attrP, "observationSpace");
  if (nodeP != NULL)
  {
    LM_TMP(("CCSR - attrP Found observationSpace "));
    kjChildRemove (attrP,nodeP);
    // Chandra-TBI
  }

  nodeP  = kjLookup(attrP, "datasetId");
  if (nodeP != NULL)
  {
    LM_TMP(("CCSR - attrP Found datasetId "));
    kjChildRemove (attrP,nodeP);
    // Chandra-TBI
  }

  // nodeP  = kjLookup(attrP, "instanceid");
  // if (nodeP != NULL)
  // {
  //    kjChildRemove (attrP,nodeP);
  //    // Chandra-TBI
  // }

  KjNode* attrTypeP  = kjLookup(attrP, "type");
  if (attrTypeP != NULL)
  {
    LM_TMP(("CCSR - attrTypeP Found type  "));
    kjChildRemove (attrP,attrTypeP);
  }

  LM_TMP(("CCSR:  after kjChildRemove (attrP,attrTypeP); : '%i'", attrTypeP));
  dbAttributeTableLocal->attributeType = attrTypeP->value.s;

  if (strcmp(dbAttributeTableLocal->attributeType,"Relationship") == 0)
  {
    KjNode* attributeObject = kjLookup(attrP, "object");

    if (attributeObject != NULL)
      kjChildRemove (attrP,attributeObject);

    // dbEntityTableLocal.attributeValueType  = kjLookup(attrP, "object");
    dbAttributeTableLocal->attributeValueType  = EnumValueRelation;
    dbAttributeTableLocal->valueString = attributeObject->value.s;
    LM_TMP(("CCSR:  Relationship : '%s'", dbAttributeTableLocal->valueString));
  }
  else if (strcmp(dbAttributeTableLocal->attributeType, "GeoProperty") == 0)
  {
#if 0
    // Chandra-TBD
    KjNode* valueP  = kjLookup(attrP, "value");

    dbAttributeTableLocal->geoPropertyType = ...
#endif
    LM_TMP(("CCSR:  Found GeoProperty : "));
  }
  else if (strcmp(dbAttributeTableLocal->attributeType, "Property") == 0)
  {
    KjNode* valueP  = kjLookup(attrP, "value");

    if (valueP != NULL)
      kjChildRemove (attrP,valueP);

    if (valueP->type == KjFloat)
    {
      dbAttributeTableLocal->attributeValueType  = EnumValueNumber;
      dbAttributeTableLocal->valueNumber = valueP->value.f;
      LM_TMP(("CCSR:  attribute value number  : %f", dbAttributeTableLocal->valueNumber));
    }
    else if (valueP->type == KjInt)
    {
      dbAttributeTableLocal->attributeValueType  = EnumValueNumber;
      dbAttributeTableLocal->valueNumber = valueP->value.i;
      LM_TMP(("CCSR:  attribute value number  : %i", dbAttributeTableLocal->valueNumber));
    }
    else if (valueP->type == KjArray)
    {
      dbAttributeTableLocal->attributeValueType  = EnumValueArray;
      dbAttributeTableLocal->valueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
      kjRender(orionldState.kjsonP, valueP->value.firstChildP, dbAttributeTableLocal->valueString, 1024);
    }
    else if (valueP->type == KjObject)
    {
      dbAttributeTableLocal->attributeValueType  = EnumValueObject;
      dbAttributeTableLocal->valueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
      kjRender(orionldState.kjsonP, valueP->value.firstChildP, dbAttributeTableLocal->valueString, 1024);
    }
    else if (valueP->type == KjBoolean)
    {
      dbAttributeTableLocal->attributeValueType  = EnumValueBool;
      dbAttributeTableLocal->valueNumber = valueP->value.b;
    }
    else if (valueP->type == KjString)
    {
      dbAttributeTableLocal->attributeValueType  = EnumValueString;
      dbAttributeTableLocal->valueString = valueP->value.s;
      LM_TMP(("CCSR:  attribute value string  : %s", dbAttributeTableLocal->valueString));
    }

    LM_TMP(("CCSR:  Attribute Value type : %d", dbAttributeTableLocal->attributeValueType));
  }

  // Now we look the special sub attributes - unitCode, observacationspace, dataSetId, instanceid, location & operationSpace

  if (attrP->value.firstChildP != NULL)
  {
    LM_TMP(("CCSR:  Tring to extract subattribute "));

    dbAttributeTableLocal->subProperty = true;

    for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
    {
      dbSubAttributeTableLocal[subAttrIx].attributeName = dbAttributeTableLocal->attributeName;
      dbSubAttributeTableLocal[subAttrIx].attrInstanceId = dbAttributeTableLocal->instanceId;
      attrSubAttrExtract (subAttrP, &dbSubAttributeTableLocal[subAttrIx]);
      subAttrIx++;
    }
    *subAttrIndexP = subAttrIx;
  }

  dbAttributeTableLocal->createdAt  = now;
  dbAttributeTableLocal->modifiedAt = now;
  // oldTemporalTreeNodeLevel++;
}



// -----------------------------------------------------------------------------
//
// attrSubAttrExtract -
//
void attrSubAttrExtract(KjNode* subAttrP, OrionldTemporalDbSubAttributeTable* dbSubAttributeTableLocal)
{
  double now = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;  // FIXME: to orionldState

  dbSubAttributeTableLocal->subAttributeName = subAttrP->name;
  if (subAttrP->type != KjObject)
  {
    LM_W(("Temporal - Bad Input - Key values not supported"));
    return;
  }

  KjNode* attrTypeP  = kjLookup(subAttrP, "type");
  kjChildRemove (subAttrP,attrTypeP);
  dbSubAttributeTableLocal->subAttributeType = attrTypeP->value.s;

  if (strcmp(dbSubAttributeTableLocal->subAttributeType, "Relationship") == 0)
  {
    KjNode* attributeObject  = kjLookup(subAttrP, "object");

    kjChildRemove(subAttrP,attributeObject);

    dbSubAttributeTableLocal->subAttributeValueType   = EnumValueRelation;
    dbSubAttributeTableLocal->subAttributeValueString = attributeObject->value.s;
    LM_TMP(("CCSR:  Relationship : '%s'", dbSubAttributeTableLocal->subAttributeValueString));
  }
  else if (strcmp(dbSubAttributeTableLocal->subAttributeType, "GeoProperty") == 0)
  {
    KjNode* valueP = kjLookup(subAttrP, "value");  //Chandra-TBD
    kjChildRemove(subAttrP, valueP);
    LM_TMP(("CCSR:  Found GeoProperty : "));         // Chandra-TBI
  }
  else if (strcmp(dbSubAttributeTableLocal->subAttributeType, "Property") == 0)
  {
    KjNode* valueP  = kjLookup(subAttrP, "value");  //Chandra-TBD
    kjChildRemove(subAttrP,valueP);

    if (valueP->type == KjFloat)
    {
      dbSubAttributeTableLocal->subAttributeValueType  = EnumValueNumber;
      dbSubAttributeTableLocal->subAttributeValueNumber = valueP->value.f;
    }
    else if (valueP->type == KjInt)
    {
      dbSubAttributeTableLocal->subAttributeValueType  = EnumValueNumber;
      dbSubAttributeTableLocal->subAttributeValueNumber = valueP->value.i;
    }
    else if (valueP->type == KjArray)
    {
      dbSubAttributeTableLocal->subAttributeValueType  = EnumValueArray;
      dbSubAttributeTableLocal->subAttributeValueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
      kjRender(orionldState.kjsonP, valueP->value.firstChildP, dbSubAttributeTableLocal->subAttributeValueString, 1024);
    }
    else if (valueP->type == KjObject)
    {
      dbSubAttributeTableLocal->subAttributeValueType  = EnumValueObject;
      dbSubAttributeTableLocal->subAttributeValueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
      kjRender(orionldState.kjsonP, valueP->value.firstChildP, dbSubAttributeTableLocal->subAttributeValueString, 1024);
    }
    else if (valueP->type == KjBoolean)
    {
      dbSubAttributeTableLocal->subAttributeValueType  = EnumValueBool;
      dbSubAttributeTableLocal->subAttributeValueNumber = valueP->value.b;
    }
    else if (valueP->type == KjString)
    {
      dbSubAttributeTableLocal->subAttributeValueType  = EnumValueString;
      dbSubAttributeTableLocal->subAttributeValueString = valueP->value.s;
    }
  }

  // Now we look the special sub attributes - unitCode, observacationspace, dataSetId, instanceid, location & operationSpace
  KjNode* nodeP  = kjLookup(subAttrP, "unitCode");
  if (nodeP != NULL)
  {
    kjChildRemove (subAttrP,nodeP);
    // Chandra-TBI
  }

  nodeP = kjLookup(subAttrP, "observationSpace");
  if (nodeP != NULL)
  {
    kjChildRemove (subAttrP,nodeP);
    // Chandra-TBI
  }

  nodeP = kjLookup(subAttrP, "datasetId");
  if (nodeP != NULL)
  {
    kjChildRemove (subAttrP,nodeP);
    // Chandra-TBI
  }

  nodeP = kjLookup(subAttrP, "instanceid");
  if (nodeP != NULL)
  {
    kjChildRemove (subAttrP,nodeP);
    // Chandra-TBI
  }

  nodeP = kjLookup(subAttrP, "location");
  if (nodeP != NULL)
  {
    kjChildRemove (subAttrP,nodeP);
    // Chandra-TBI
  }

  nodeP = kjLookup(subAttrP, "operationSpace");
  if (nodeP != NULL)
  {
    kjChildRemove (subAttrP,nodeP);
    // Chandra-TBI
  }

#if 0
  if (attrP->value.firstChildP != NULL)  //Chandra-TBCKZ (Can we safely assume that there are sub attributes to sub attributes?)
  {
    dbAttributeTableLocal[oldTemporalTreeNodeLevel].subProperty = true;
    int subAttrs = 0;
    for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
    {
      subAttrs++;
    }

    int subAttribArrayTotalSize = subAttrs * sizeof(OrionldTemporalDbSubAttributeTable);
    dbSubAttributeTableLocal = (OrionldTemporalDbSubAttributeTable*) kaAlloc(&orionldState.kalloc, subAttribArrayTotalSize);
    bzero(dbSubAttributeTableLocal, subAttribArrayTotalSize);

    int subAttrIx=0;
    for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
    {
      subAttrExtract (subAttrP, &dbSubAttributeTableLocal[subAttrIx++]);
    }
  }
#endif

  dbSubAttributeTableLocal->createdAt   = now;
  dbSubAttributeTableLocal->modifiedAt  = now;

  KjNode* observedAtP = kjLookup(subAttrP, "observedAt");
  if (observedAtP != NULL)
    dbSubAttributeTableLocal->observedAt = observedAtP->value.f;
  else
    dbSubAttributeTableLocal->observedAt = 0;
}



// -----------------------------------------------------------------------------
//
// numberToDate -
//
static bool numberToDate(double timestamp, char* date, int dateLen)
{
  struct tm  tm;
  time_t     fromEpoch = timestamp;

  gmtime_r(&fromEpoch, &tm);
  strftime(date, dateLen, "%Y-%m-%dT%H:%M:%S", &tm);

  return true;
}



// ----------------------------------------------------------------------------
//
// PGconn* TemporalConstructUpdateSQLStatement(OrionldTemporalDbAllTables* dbAllTablesLocal) - function to buil update SQL statement
//
bool TemporalConstructInsertSQLStatement(OrionldTemporalDbAllTables* dbAllTablesLocal, bool entityUpdateFlag)
{
  // if (strcmp (tableName,"Entity") == 0);
  // int temporalSQLStatementLengthBuffer = sizeof(dbAllTablesLocal->dbEntityTableLocal);
  // char* updateEntityTableSQLStatement = temporalSQLStatementLengthBuffer * 1024;  // Not smart Chandra-TBI
  // int dbEntityTable = sizeof(dbAllTablesLocal.entityTableArray);
  // int dbEntityTable = dbAllTablesLocal->entityTableArrayItems;
  // int dbAttribTable = dbAllTablesLocal->attributeTableArrayItems;
  // int dbSubAttribTable = dbAllTablesLocal->subAttributeTableArrayItems;


  int dbEntityBufferSize = 10 * 1024;
  char* dbEntityStrBuffer = kaAlloc(&orionldState.kalloc, dbEntityBufferSize);
  bzero(dbEntityStrBuffer, dbEntityBufferSize);

  LM_TMP(("CCSR: step1 TemporalConstructInsertSQLStatement:  entity count   %i", dbAllTablesLocal->entityTableArrayItems));

  for (int dbEntityLoop=0; dbEntityLoop < dbAllTablesLocal->entityTableArrayItems; dbEntityLoop++)
  {
    LM_TMP(("CCSR: step1.1 TemporalConstructInsertSQLStatement: %s", dbAllTablesLocal->entityTableArray[dbEntityLoop].entityType));
    char* expandedEntityType = orionldContextItemExpand(orionldState.contextP,
                                                        dbAllTablesLocal->entityTableArray[dbEntityLoop].entityType,
                                                        true,
                                                        NULL);

    LM_TMP(("CCSR: step2 TemporalConstructInsertSQLStatement: "));

    char entCreatedAt[64];
    char entModifiedAt[64];

    numberToDate(dbAllTablesLocal->entityTableArray[dbEntityLoop].createdAt, entCreatedAt, sizeof(entCreatedAt));
    numberToDate(dbAllTablesLocal->entityTableArray[dbEntityLoop].modifiedAt, entModifiedAt, sizeof(entModifiedAt));

    LM_TMP(("CCSR: step3 TemporalConstructInsertSQLStatement: "));

    if (entityUpdateFlag)
    {
      snprintf(dbEntityStrBuffer, dbEntityBufferSize, "UPDATE entity_table "
               "SET modified_at = '%s' WHERE entity_id = '%s'",
               // dbAllTablesLocal->entityTableArray[dbEntityLoop].createdAt,
               // dbAllTablesLocal->entityTableArray[dbEntityLoop].modifiedAt,
               entModifiedAt,
               dbAllTablesLocal->entityTableArray[dbEntityLoop].entityId);

      LM_TMP(("CCSR: step4 TemporalConstructInsertSQLStatement:"));
    }
    else
    {
      snprintf(dbEntityStrBuffer, dbEntityBufferSize, "INSERT INTO entity_table(entity_id,entity_type,geo_property,"
               "created_at,modified_at, observed_at) VALUES ('%s', '%s', NULL, '%s', '%s', NULL)",
               dbAllTablesLocal->entityTableArray[dbEntityLoop].entityId,
               expandedEntityType,
               // dbAllTablesLocal->entityTableArray[dbEntityLoop].createdAt,
               // dbAllTablesLocal->entityTableArray[dbEntityLoop].modifiedAt
               entCreatedAt,
               entModifiedAt);

      LM_TMP(("CCSR: step5 TemporalConstructInsertSQLStatement: "));
    }

    LM_TMP(("CCSR: dbEntityStrBuffer: '%s'", dbEntityStrBuffer));

    if (!temporalExecSqlStatement(dbEntityStrBuffer))
      return false;
  }

  //  temporalExecSqlStatement (dbEntityStrBuffer);  //Chandra - hack TBR

  for (int dbAttribLoop = 0; dbAttribLoop < dbAllTablesLocal->attributeTableArrayItems; dbAttribLoop++)
  {
    int   dbAttribBufferSize = 10 * 1024;
    char* dbAttribStrBuffer  = kaAlloc(&orionldState.kalloc, dbAttribBufferSize);
    bzero(dbAttribStrBuffer, dbAttribBufferSize);

    int   allValuesSize = 2048;
    char* allValues     = kaAlloc(&orionldState.kalloc,allValuesSize);

    LM_TMP(("CCSR: TemporalConstructInsertSQLStatement dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeName :  %s",
            dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeName));

    allValuesRenderAttr(&dbAllTablesLocal->attributeTableArray[dbAttribLoop], allValues, allValuesSize);

    // Chandra-TBI

    char* uuidBuffer = kaAlloc(&orionldState.kalloc, 64);
    uuidGenerate(uuidBuffer);

    char attrCreatedAt[64];
    char attrModifiedAt[64];
    char attrObservedAt[64];

    numberToDate(dbAllTablesLocal->attributeTableArray[dbAttribLoop].createdAt,  attrCreatedAt,  sizeof(attrCreatedAt));
    numberToDate(dbAllTablesLocal->attributeTableArray[dbAttribLoop].modifiedAt, attrModifiedAt, sizeof(attrModifiedAt));
    numberToDate(dbAllTablesLocal->attributeTableArray[dbAttribLoop].observedAt, attrObservedAt, sizeof(attrObservedAt));

    char* expandedAttrType = orionldContextItemExpand(orionldState.contextP,
                                                      dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeType,
                                                      true,
                                                      NULL);

    LM_TMP (("CCSR - Printing attributeName %s", dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeName));

    //"type,", Fix me - put type back Chandra TBC
    snprintf(dbAttribStrBuffer, dbAttribBufferSize, "INSERT INTO attributes_table("
             "entity_id,"
             "id,"
             "name,"
             "value_type,"
             "sub_property,"
             "instance_id,"
             "unit_code,"
             "data_set_id,"
             "value_string,"
             "value_boolean,"
             "value_number,"
             "value_relation,"
             "value_object,"
             "value_datetime,"
             "geo_property,"
             "created_at,"
             "modified_at,"
             "observed_at) "
             "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', %s, '%s', '%s', '%s')",
             dbAllTablesLocal->attributeTableArray[dbAttribLoop].entityId,
             dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeName,
             expandedAttrType,
             dbValueEnumString(dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeValueType),  //Chandra-TBD
             (dbAllTablesLocal->attributeTableArray[dbAttribLoop].subProperty==true)? "true" : "false",
             uuidBuffer, allValues, attrCreatedAt, attrModifiedAt, attrObservedAt);

    LM_TMP (("CCSR - Printing SQL attribute %s", dbAttribStrBuffer));
    if (!temporalExecSqlStatement (dbAttribStrBuffer))
      return false;

    // FIXME: The attribute loop should end here, right?
    //        Cause otherwise, we are doing the sub-attr loop once per attribute 
    
    for (int dbSubAttribLoop = 0; dbSubAttribLoop < dbAllTablesLocal->subAttributeTableArrayItems; dbSubAttribLoop++)
    {
      int   dbSubAttribBufferSize  = 10 * 1024;
      char* dbSubAttribStrBuffer   = kaAlloc(&orionldState.kalloc, dbSubAttribBufferSize);

      bzero(dbSubAttribStrBuffer, dbSubAttribBufferSize);
      // Chandra-TBI
      int   allValuesSizeSubAttr   = 2048;
      char* allValuesSubAttr       = kaAlloc(&orionldState.kalloc,allValuesSizeSubAttr);

      allValuesRenderSubAttr(&dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop], allValuesSubAttr, allValuesSizeSubAttr);

      char subAttrCreatedAt[64];
      char subAttrModifiedAt[64];
      char subAttrObeservedAt[64];

      numberToDate(dbAllTablesLocal->attributeTableArray[dbSubAttribLoop].createdAt,     subAttrCreatedAt,   sizeof(subAttrCreatedAt));
      numberToDate(dbAllTablesLocal->attributeTableArray[dbSubAttribLoop].modifiedAt,    subAttrModifiedAt,  sizeof(subAttrModifiedAt));
      numberToDate(dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop].observedAt, subAttrObeservedAt, sizeof(subAttrObeservedAt));

      snprintf(dbSubAttribStrBuffer, dbSubAttribBufferSize, "INSERT INTO attribute_sub_properties_table(entity_id,"
               "attribute_id, id, type, value_type, unit_code, data_set_id, value_string, value_boolean, value_number,"
               "value_relation,value_object, value_datetime, geo_property, observed_at, created_at, modified_at)"
               "VALUES ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s')",
               dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop].entityId,
               dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop].attributeName,
               dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop].subAttributeName,
               dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop].subAttributeType,
               dbValueEnumString(dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop].subAttributeValueType),  //Chandra-TBD
               // (dbAllTablesLocal->subAttributeTableArray[dbSubAttribLoop].subProperty==true)? "true" : "false",
               uuidBuffer,
               allValuesSubAttr,
               subAttrCreatedAt,
               subAttrModifiedAt,
               subAttrObeservedAt);
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// allValuesRenderAttr -
//
void allValuesRenderAttr(OrionldTemporalDbAttributeTable* attrLocalP, char* allValues, int allValuesSize)
{
  LM_TMP(("Temporal allValuesRenderAttr - attrLocalP->attributeValueType %i , %s",attrLocalP->attributeValueType, attrLocalP->entityId));

  char attributeValue[512];

  switch (attrLocalP->attributeValueType)
  {
  case EnumValueString:
    snprintf(attributeValue, sizeof(attributeValue), "'%s', NULL, NULL, NULL, NULL", attrLocalP->valueString);
    break;

  case EnumValueBool:
    snprintf(attributeValue, sizeof(attributeValue), "NULL, '%s', NULL, NULL, NULL", (attrLocalP->valueBool == true)? "true" : "false");
    break;

  case EnumValueNumber:
    snprintf(attributeValue, sizeof(attributeValue), "NULL, NULL, %lld, NULL, NULL", attrLocalP->valueNumber);
    break;

  case EnumValueRelation:  // same object
    snprintf(attributeValue, sizeof(attributeValue), "NULL, NULL, NULL, '%s', NULL", attrLocalP->valueString);
    break;

  case EnumValueArray:  // same object
    snprintf(attributeValue, sizeof(attributeValue), "NULL, NULL, NULL, '%s', NULL", attrLocalP->valueString);
    break;

  case EnumValueObject:
    snprintf(attributeValue, sizeof(attributeValue), "NULL, NULL, NULL, '%s', NULL", attrLocalP->valueString);
    break;

  case EnumValueDateTime:
    char atrrValueDataTime[64];
    numberToDate (attrLocalP->valueDatetime, atrrValueDataTime, sizeof(atrrValueDataTime));
    snprintf(attributeValue, sizeof(attributeValue), "NULL, NULL, NULL, NULL, '%s'", atrrValueDataTime);
    break;

  default:
    LM_W(("Error - Invalid attribute value type: %d", attrLocalP->attributeValueType));
    return;
  }

  int   unitCodeValuesSize = 128;
  char* unitCodeValue      = kaAlloc(&orionldState.kalloc, unitCodeValuesSize);
  bzero(unitCodeValue, unitCodeValuesSize);

  if (attrLocalP->unitCode == NULL)
    snprintf(unitCodeValue, unitCodeValuesSize, "NULL");
  else
    snprintf(unitCodeValue, unitCodeValuesSize, "%s", attrLocalP->unitCode);

  int   dataSetIdSize  = 128;
  char* dataSetIdValue = kaAlloc(&orionldState.kalloc, dataSetIdSize);
  bzero(dataSetIdValue, dataSetIdSize);

  if (attrLocalP->dataSetId == NULL)
    snprintf(dataSetIdValue, dataSetIdSize, "NULL");
  else
    snprintf(dataSetIdValue, dataSetIdSize, "%s", attrLocalP->dataSetId);

  int   geoPropertySize  = 512;
  char* geoPropertyValue = kaAlloc(&orionldState.kalloc, geoPropertySize);
  bzero(geoPropertyValue, geoPropertySize);

  if (attrLocalP->geoProperty == NULL)
    snprintf(geoPropertyValue, geoPropertySize, "NULL");
  else    // Chandra-TBI
    snprintf(geoPropertyValue, geoPropertySize, "%s", "NULL");

  int   observedAtSize  = 512;
  char* observedAtValue = kaAlloc(&orionldState.kalloc, observedAtSize);
  bzero(observedAtValue, observedAtSize);

  if (attrLocalP->observedAt == 0)  // Chandra - Need to be initialize the value
    snprintf(observedAtValue, observedAtSize, "NULL");
  else
    snprintf(observedAtValue, observedAtSize, "%f", attrLocalP->observedAt);

  // char* uuidBuffer = kaAlloc(&orionldState.kalloc, 64);
  // uuidGenerate(uuidBuffer);

  snprintf(allValues, allValuesSize, "%s, %s, %s, %s, %s",
           unitCodeValue, dataSetIdValue, attributeValue, geoPropertyValue, observedAtValue);

  LM_TMP(("Printing all Values in the end in attribute extract %s", allValues));
}



// -----------------------------------------------------------------------------
//
// allValuesRenderSubAttr -
//
void allValuesRenderSubAttr(OrionldTemporalDbSubAttributeTable* attrLocalP, char* allValues, int allValuesSize)
{
  char subAttributeValue[512];

  switch (attrLocalP->subAttributeValueType)
  {
  case EnumValueString:
    snprintf(subAttributeValue, sizeof(subAttributeValue), "'%s', NULL, NULL, NULL, NULL",attrLocalP->subAttributeValueString);
    break;

  case EnumValueNumber:
    snprintf(subAttributeValue, sizeof(subAttributeValue), "NULL, %lld, NULL, NULL, NULL",attrLocalP->subAttributeValueNumber);
    break;

  case EnumValueBool:
    snprintf(subAttributeValue, sizeof(subAttributeValue), "NULL, NULL, '%s', NULL, NULL",(attrLocalP->subAttributeValueBoolean==true)? "true" : "false");
    break;

  case EnumValueRelation:  // same object
    snprintf(subAttributeValue, sizeof(subAttributeValue), "NULL, NULL, NULL, '%s', NULL",attrLocalP->subAttributeValueString);
    break;

  case EnumValueArray:
    snprintf(subAttributeValue, sizeof(subAttributeValue), "NULL, NULL, NULL, '%s', NULL",attrLocalP->subAttributeValueString);
    break;

  case EnumValueObject:
    snprintf(subAttributeValue, sizeof(subAttributeValue), "NULL, NULL, NULL, '%s', NULL",attrLocalP->subAttributeValueString);
    break;

  case EnumValueDateTime:
    char subAtrrValueDataTime[64];
    numberToDate (attrLocalP->subAttributeValueDatetime, subAtrrValueDataTime, sizeof(subAtrrValueDataTime));
    snprintf(subAtrrValueDataTime, sizeof(subAtrrValueDataTime), "NULL, NULL, NULL, NULL, '%s'",subAtrrValueDataTime);
    break;

  default:
    LM_W(("Error - Invalid Sub attribute Value type %d", attrLocalP->subAttributeValueType));
    return;
  }

  int   unitCodeValuesSize = 128;
  char* unitCodeValue      = kaAlloc(&orionldState.kalloc, unitCodeValuesSize);

  bzero(unitCodeValue, unitCodeValuesSize);

  if (attrLocalP->subAttributeUnitCode == NULL)
    snprintf(unitCodeValue, unitCodeValuesSize, "NULL");
  else
    snprintf(unitCodeValue, unitCodeValuesSize, "%s", attrLocalP->subAttributeUnitCode);


  int   dataSetIdSize  = 128;
  char* dataSetIdValue = kaAlloc(&orionldState.kalloc, dataSetIdSize);
  bzero(dataSetIdValue, dataSetIdSize);

  if (attrLocalP->subAttributeDataSetId == NULL)
    snprintf(dataSetIdValue, dataSetIdSize, "NULL");
  else
    snprintf(dataSetIdValue, dataSetIdSize, "%s", attrLocalP->subAttributeDataSetId);


  int   geoPropertySize  = 512;
  char* geoPropertyValue = kaAlloc(&orionldState.kalloc, geoPropertySize);
  bzero(geoPropertyValue, geoPropertySize);

  if (attrLocalP->subAttributeGeoProperty == NULL)
    snprintf(geoPropertyValue, geoPropertySize, "NULL");
  else
  {
    // Chandra-TBI
    // snprintf(geoPropertyValue, geoPropertySize, "%f", attrLocalP->geoProperty);
  }

  int   observedAtSize  = 512;
  char* observedAtValue = kaAlloc(&orionldState.kalloc, observedAtSize);
  bzero(observedAtValue, observedAtSize);

  if (attrLocalP->observedAt == 0)  // Chandra - Need to be initialize the value
    snprintf(observedAtValue, observedAtSize, "NULL");
  else
    snprintf(observedAtValue, observedAtSize, "%f", attrLocalP->observedAt);

  char* uuidBuffer = kaAlloc(&orionldState.kalloc, 64);
  uuidGenerate(uuidBuffer);

  snprintf(allValues, allValuesSize, "%s, %s, %s, %s, %s",
           unitCodeValue, dataSetIdValue, allValues, geoPropertyValue, observedAtValue);
}



// ----------------------------------------------------------------------------
//
// TemporalPgTenantDBConnectorOpen - open the Postgres database connection
//
bool TemporalPgTenantDBConnectorOpen(const char* tenant)
{
  int   oldPgTDbConnSQLBufferSize      = 1024;
  int   oldPgTDbConnSQLUsedBufferSize  = 0;
  char* oldTemporalSQLBuffer           = kaAlloc(&orionldState.kalloc, oldPgTDbConnSQLBufferSize);

  //
  // FIXME:
  // - snprintf instead of this mess of strcat
  // - user from cli option that defaults to TEMPORAL_DB_USER
  // - password from cli option that defaults to TEMPORAL_DB_PASSWORD
  //
  strncpy(oldTemporalSQLBuffer, "user=", oldPgTDbConnSQLBufferSize);
  oldPgTDbConnSQLUsedBufferSize += 5;
  strncat(oldTemporalSQLBuffer, TEMPORAL_DB_USER, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
  oldPgTDbConnSQLUsedBufferSize += strlen(TEMPORAL_DB_USER);
  strncat(oldTemporalSQLBuffer, " ", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
  oldPgTDbConnSQLUsedBufferSize += 1;
  strncat(oldTemporalSQLBuffer, "password=", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
  oldPgTDbConnSQLUsedBufferSize += 9;
  strncat(oldTemporalSQLBuffer, TEMPORAL_DB_PASSWORD, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
  oldPgTDbConnSQLUsedBufferSize += strlen(TEMPORAL_DB_PASSWORD);
  strncat(oldTemporalSQLBuffer, " ", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
  oldPgTDbConnSQLUsedBufferSize += 1;
  strncat(oldTemporalSQLBuffer, "dbname=", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
  oldPgTDbConnSQLUsedBufferSize += 7;
  strncat(oldTemporalSQLBuffer, tenant, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
  oldPgTDbConnSQLUsedBufferSize += strlen(tenant);

  oldPgDbTenantConnection = PQconnectdb(oldTemporalSQLBuffer);

  if (PQstatus(oldPgDbTenantConnection) != CONNECTION_OK)
  {
    LM_E(("Database Error (Postgres DB connection failed: %s)", PQerrorMessage(oldPgDbTenantConnection)));
    TemporalPgDBConnectorClose();
    return false;
  }

  LM_TMP(("TEMP: Connection is ok with the Postgres database"));
  return true;  // FIXME: return instead the connection handler
}



// -----------------------------------------------------------------------------
//
// temporalInit -
//
int temporalInit(void)
{
  LM_TMP(("TEMP: Calling temporalTenantInitialise"));
  temporalTenantInitialise("orion_ld");
  LM_TMP(("TEMP: After temporalTenantInitialise"));

  return 0;
}
