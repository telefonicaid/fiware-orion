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
#include "kjson/kjFree.h"                                        // kjFree
#include "kjson/kjLookup.h"                                      // kjLookup
#include "kalloc/kaBufferInit.h"                                 // kaBufferInit
#include "kjson/kjRender.h"                                      // kjRender
#include "kjson/kjBuilder.h"                                     // kjChildRemove .....
}

#include "logMsg/logMsg.h"                                       // LM_*
#include "logMsg/traceLevels.h"                                  // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/HttpStatusCode.h"                               // SccNotImplemented
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService
#include "orionld/types/OrionldGeoIndex.h"                       // OrionldGeoIndex
#include "orionld/db/dbConfiguration.h"                          // DB_DRIVER_MONGOC
#include "orionld/context/orionldCoreContext.h"                  // orionldCoreContext
#include "orionld/common/QNode.h"                                // QNode

#include "orionld/temporal/temporalCommon.h"                     // Temporal common


PGconn* oldPgDbConnection = NULL;
PGconn* oldPgDbTenantConnection = NULL;
PGresult* oldPgTenandDbResult = NULL;
//OrionldTemporalDbEntityTable* dbEntityTableLocal;
//OrionldTemporalDbAttributeTable* dbAttributeTableLocal;

// -----------------------------------------------------------------------------
//
// temporalOrionldCommonExtractTree - initialize the thread-local variables of temporalOrionldCommonState
// INSERT INTO entity_table(entity_id,entity_type,geo_property,created_at,modified_at, observed_at)
//      VALUES ("%s,%s,%s,%s");
//
OrionldTemporalDbAllTables*  singleTemporalEntityExtract()
{
    OrionldTemporalDbAllTables*          dbAllTablesLocal; // Chandra - TBI
    OrionldTemporalDbEntityTable*        dbEntityTableLocal;
    OrionldTemporalDbAttributeTable*     dbAttributeTableLocal;
    OrionldTemporalDbSubAttributeTable** dbSubAttributeTableLocal;

    int dbAllTablesSize = sizeof(OrionldTemporalDbAllTables);
    dbAllTablesLocal = (OrionldTemporalDbAllTables*) kaAlloc(&orionldState.kalloc, dbAllTablesSize);
    bzero(dbAllTablesLocal, dbAllTablesSize);

    int entityArrayTotalSize = sizeof(OrionldTemporalDbEntityTable);
    dbEntityTableLocal = (OrionldTemporalDbEntityTable*) kaAlloc(&orionldState.kalloc, entityArrayTotalSize);
    bzero(dbEntityTableLocal, entityArrayTotalSize);

    dbEntityTableLocal[0].entityId = orionldState.payloadIdNode->value.s;
    dbEntityTableLocal[0].entityType = orionldState.payloadTypeNode->value.s;

/*
    char buff [1024];  // Chandra-TBR
    kjRender(orionldState.kjsonP,orionldState.requestTree,buff,sizeof(buff));  // Chandra-TBR
    LM_TMP(("CCSR: The entire tree:     '%s'", buff));  // Chandra-TBR


    dbEntityTableLocal[0].entityId = orionldState.payloadIdNode->value.s;
    dbEntityTableLocal[0].entityType = orionldState.payloadTypeNode->value.s;

    int oldTemporalSQLFullBufferSize = 10 * 1024;
    char* oldTemporalSQLBuffer = kaAlloc(&orionldState.kalloc, oldTemporalSQLFullBufferSize);
    bzero(oldTemporalSQLBuffer, oldTemporalSQLFullBufferSize);
#if 0
    snprintf(oldTemporalSQLBuffer, oldTemporalSQLFullBufferSize, "INSERT INTO entity_table(entity_id,entity_type,geo_property,"
            "created_at,modified_at, observed_at) VALUES (%s, %s, NULL, %s, %s, NULL)",
            orionldState.payloadIdNode->value.s,
            orionldState.payloadTypeNode->value.s,
            "createdAt",
            "modifiedAt");
#else
    int oldTemporalSQLUsedBufferSize = 0;
    int oldTemporalSQLRemainingBufferSize = 10* 1024;
    const char* oldTemporalSQLInsertIntoTable = "INSERT INTO entity_table(entity_id,entity_type,geo_property,created_at,modified_at, observed_at) VALUES (";

    strncpy(oldTemporalSQLBuffer, oldTemporalSQLInsertIntoTable, oldTemporalSQLRemainingBufferSize);
    oldTemporalSQLUsedBufferSize = strlen(oldTemporalSQLInsertIntoTable);  // string length of "INSERT INTO entity_table(....."
    oldTemporalSQLRemainingBufferSize = oldTemporalSQLFullBufferSize - oldTemporalSQLUsedBufferSize;

    // char* entityId   = orionldState.payloadIdNode->value.s;
    // char* entityType = orionldState.payloadTypeNode->value.s;

    strncat(oldTemporalSQLBuffer,dbEntityTableLocal[0].entityId,oldTemporalSQLRemainingBufferSize);
    oldTemporalSQLUsedBufferSize += strlen(dbEntityTableLocal[0].entityId);

    strncat(oldTemporalSQLBuffer,", ",oldTemporalSQLFullBufferSize-oldTemporalSQLUsedBufferSize); // Chandra-TBD
    oldTemporalSQLUsedBufferSize += 2;

    strncat(oldTemporalSQLBuffer,dbEntityTableLocal[0].entityType,oldTemporalSQLRemainingBufferSize);
    oldTemporalSQLUsedBufferSize += strlen(dbEntityTableLocal[0].entityType);


    // Geo Property
    strncat(oldTemporalSQLBuffer,", NULL, ",oldTemporalSQLFullBufferSize-oldTemporalSQLUsedBufferSize);
    oldTemporalSQLUsedBufferSize += 8;

    // Created At
    dbEntityTableLocal[0].createdAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000; //same for modififedAt
    char entityCreateAtCharBuffer[64];
    snprintf(entityCreateAtCharBuffer, sizeof(entityCreateAtCharBuffer), "%.3f", dbEntityTableLocal[0].createdAt);
    strncat(oldTemporalSQLBuffer,entityCreateAtCharBuffer, oldTemporalSQLFullBufferSize - oldTemporalSQLUsedBufferSize);
    oldTemporalSQLUsedBufferSize += strlen(entityCreateAtCharBuffer);
    strncat(oldTemporalSQLBuffer,", ",oldTemporalSQLFullBufferSize-oldTemporalSQLUsedBufferSize); // Chandra-TBD
    oldTemporalSQLUsedBufferSize += 2;

    // Modified At
    strncat(oldTemporalSQLBuffer,entityCreateAtCharBuffer, oldTemporalSQLFullBufferSize-oldTemporalSQLUsedBufferSize);
    oldTemporalSQLUsedBufferSize += strlen(entityCreateAtCharBuffer);

    // To be removed
    strncat(oldTemporalSQLBuffer,", NULL)",oldTemporalSQLFullBufferSize-oldTemporalSQLUsedBufferSize); // Chandra-TBD
    oldTemporalSQLUsedBufferSize += 7;
#endif
*/

    int attributesNumbers = 0;
    for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
       attributesNumbers++;
    }

    int attribArrayTotalSize = attributesNumbers * sizeof(OrionldTemporalDbAttributeTable);
    dbAttributeTableLocal = (OrionldTemporalDbAttributeTable*) kaAlloc(&orionldState.kalloc, attribArrayTotalSize);
    bzero(dbAttributeTableLocal, attribArrayTotalSize);

    dbSubAttributeTableLocal = (OrionldTemporalDbSubAttributeTable**) kaAlloc(&orionldState.kalloc, (attributesNumbers * sizeof(OrionldTemporalDbSubAttributeTable*)) );
    bzero(dbSubAttributeTableLocal, (attributesNumbers * sizeof(OrionldTemporalDbSubAttributeTable*)));

    int attrIndex=0;
    for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
       dbAttributeTableLocal[attrIndex].entityId = dbEntityTableLocal[0].entityId;
       attrExtract (attrP, &dbAttributeTableLocal[attrIndex], dbSubAttributeTableLocal[attrIndex], attrIndex);
       attrIndex++;
    }


/*
    for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
    {
        dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeName = attrP->name;
        if (attrP->type != KjObject)
        {
            LM_W(("Teamporal - Bad Input - Key values not supported"));
            continue;
        }

        KjNode* attrTypeP  = kjLookup(attrP, "type");
        kjChildRemove (attrP,attrTypeP);
        dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType = attrTypeP->value.s;

         if (strcmp (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType,"Relationship") == 0)
        // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Relationship")
        {
            KjNode* attributeObject  = kjLookup(attrP, "object");
            kjChildRemove (attrP,attributeObject);

            // dbEntityTableLocal.attributeValueType  = kjLookup(attrP, "object");
            dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueRelation;
            dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString = attributeObject->value.s;
            LM_TMP(("CCSR:  Relationship : '%s'", dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString));

        }
        else if (strcmp (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType,"GeoProperty") == 0)
        // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "GeoProperty")
        {
            KjNode* valueP  = kjLookup(attrP, "value");  //Chandra-TBD
            kjChildRemove (attrP,valueP);
            // Chandra-TBI
        }
        else if (strcmp (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType,"Property") == 0)
        // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Property")
        {
            KjNode* valueP  = kjLookup(attrP, "value");  //Chandra-TBD
            kjChildRemove (attrP,valueP);

            if (valueP->type == KjFloat)
            {
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueNumber;
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueNumber = valueP->value.f;
            }
            else if (valueP->type == KjInt)
            {
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueNumber;
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueNumber = valueP->value.i;
            }
            else if (valueP->type == KjArray)
            {
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueArray;
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
                  kjRender(orionldState.kjsonP, valueP->value.firstChildP, dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString, 1024);
            }
            else if (valueP->type == KjObject)
            {
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueObject;
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
                  kjRender(orionldState.kjsonP, valueP->value.firstChildP, dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString, 1024);
            }
            else if (valueP->type == KjBoolean)
            {
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueBool;
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueNumber = valueP->value.b;

            }
            else if (valueP->type == KjString)
            {
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueString;
                  dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString = valueP->value.s;
            }
        }

        // Now we look the special sub attributes - unitCode, observacationspace, dataSetId, instanceid, location & operationSpace
        KjNode* nodeP  = kjLookup(attrP, "unitCode");
        if(nodeP != NULL)
        {
            kjChildRemove (attrP,nodeP);
            // Chandra-TBI
        }

        nodeP  = kjLookup(attrP, "observationSpace");
        if(nodeP != NULL)
        {
            kjChildRemove (attrP,nodeP);
            // Chandra-TBI
        }

        nodeP  = kjLookup(attrP, "datasetId");
        if(nodeP != NULL)
        {
            kjChildRemove (attrP,nodeP);
            // Chandra-TBI
        }

        nodeP  = kjLookup(attrP, "instanceid");
        if(nodeP != NULL)
        {
            kjChildRemove (attrP,nodeP);
            // Chandra-TBI
        }

        nodeP  = kjLookup(attrP, "location");
        if(nodeP != NULL)
        {
            kjChildRemove (attrP,nodeP);
            // Chandra-TBI
        }

        nodeP  = kjLookup(attrP, "operationSpace");
        if(nodeP != NULL)
        {
            kjChildRemove (attrP,nodeP);
            // Chandra-TBI
        }


        if (attrP->value.firstChildP != NULL)
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
                    subAttrTreat (subAttrP, &dbSubAttributeTableLocal[subAttrIx++]);
            }

        }

        dbAttributeTableLocal[oldTemporalTreeNodeLevel].createdAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;
        dbAttributeTableLocal[oldTemporalTreeNodeLevel].modifiedAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;

        KjNode* observedAtP = kjLookup(attrP, "observedAt");
        if (observedAtP != NULL)
        {
            dbAttributeTableLocal[oldTemporalTreeNodeLevel].observedAt = parse8601Time(observedAtP->value.s);
        }

        oldTemporalTreeNodeLevel++;
    }
*/
    dbAllTablesLocal->entityTableArray = dbEntityTableLocal;
    dbAllTablesLocal->attributeTableArray = dbAttributeTableLocal;
    dbAllTablesLocal->subAttributeTableArray = *dbSubAttributeTableLocal;

    return dbAllTablesLocal;
}


void  attrExtract(KjNode* attrP, OrionldTemporalDbAttributeTable* dbAttributeTableLocal, OrionldTemporalDbSubAttributeTable* dbSubAttributeTableLocal, int attrIndex)
{
   //int oldTemporalTreeNodeLevel = 0;
   //for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
   //{
    dbAttributeTableLocal->attributeName = attrP->name;
    if (attrP->type != KjObject)
    {
        LM_W(("Teamporal - Bad Input - Key values not supported"));
        return;
    }

    KjNode* attrTypeP  = kjLookup(attrP, "type");
    kjChildRemove (attrP,attrTypeP);
    dbAttributeTableLocal->attributeType = attrTypeP->value.s;

     if (strcmp (dbAttributeTableLocal->attributeType,"Relationship") == 0)
    // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Relationship")
    {
        KjNode* attributeObject  = kjLookup(attrP, "object");
        kjChildRemove (attrP,attributeObject);

        // dbEntityTableLocal.attributeValueType  = kjLookup(attrP, "object");
        dbAttributeTableLocal->attributeValueType  = EnumValueObject;
        dbAttributeTableLocal->valueString = attributeObject->value.s;
        LM_TMP(("CCSR:  Relationship : '%s'", dbAttributeTableLocal->valueString));

    }
    else if (strcmp (dbAttributeTableLocal->attributeType,"GeoProperty") == 0)
    // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "GeoProperty")
    {
        KjNode* valueP  = kjLookup(attrP, "value");  //Chandra-TBD
        kjChildRemove (attrP,valueP);
        // Chandra-TBI
    }
    else if (strcmp (dbAttributeTableLocal->attributeType,"Property") == 0)
    // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Property")
    {
        KjNode* valueP  = kjLookup(attrP, "value");  //Chandra-TBD
        kjChildRemove (attrP,valueP);

        if (valueP->type == KjFloat)
        {
              dbAttributeTableLocal->attributeValueType  = EnumValueNumber;
              dbAttributeTableLocal->valueNumber = valueP->value.f;
        }
        else if (valueP->type == KjInt)
        {
              dbAttributeTableLocal->attributeValueType  = EnumValueNumber;
              dbAttributeTableLocal->valueNumber = valueP->value.i;
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
        }
    }

    // Now we look the special sub attributes - unitCode, observacationspace, dataSetId, instanceid, location & operationSpace
    KjNode* nodeP  = kjLookup(attrP, "unitCode");
    if(nodeP != NULL)
    {
        kjChildRemove (attrP,nodeP);
        // Chandra-TBI
    }

    nodeP  = kjLookup(attrP, "observationSpace");
    if(nodeP != NULL)
    {
        kjChildRemove (attrP,nodeP);
        // Chandra-TBI
    }

    nodeP  = kjLookup(attrP, "datasetId");
    if(nodeP != NULL)
    {
        kjChildRemove (attrP,nodeP);
        // Chandra-TBI
    }

    nodeP  = kjLookup(attrP, "instanceid");
    if(nodeP != NULL)
    {
        kjChildRemove (attrP,nodeP);
        // Chandra-TBI
    }

    nodeP  = kjLookup(attrP, "location");
    if(nodeP != NULL)
    {
        kjChildRemove (attrP,nodeP);
        // Chandra-TBI
    }

    nodeP  = kjLookup(attrP, "operationSpace");
    if(nodeP != NULL)
    {
        kjChildRemove (attrP,nodeP);
        // Chandra-TBI
    }



    if (attrP->value.firstChildP != NULL)
    {
        dbAttributeTableLocal->subProperty = true;
        int subAttrs = 0;
        for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
        {
                subAttrs++;
        }

        int subAttribArrayTotalSize = subAttrs * sizeof(OrionldTemporalDbSubAttributeTable);
        *dbSubAttributeTableLocal[attrIndex] = (OrionldTemporalDbSubAttributeTable*) kaAlloc(&orionldState.kalloc, subAttribArrayTotalSize);
        bzero(dbSubAttributeTableLocal, subAttribArrayTotalSize);

        int subAttrIx=0;
        for (KjNode* subAttrP = attrP->value.firstChildP; subAttrP != NULL; subAttrP = subAttrP->next)
        {
            dbSubAttributeTableLocal[subAttrIx].attributeName = dbAttributeTableLocal->attributeName;
            attrSubAttrExtract (subAttrP, &dbSubAttributeTableLocal[attrIndex][subAttrIx]);
            subAttrIx++;
        }
    }

    dbAttributeTableLocal->createdAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;
    dbAttributeTableLocal->modifiedAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;

    KjNode* observedAtP = kjLookup(attrP, "observedAt");
    if (observedAtP != NULL)
    {
        if (observedAtP->type == KjString)
            dbAttributeTableLocal->observedAt = parse8601Time(observedAtP->value.s);
        else
            dbAttributeTableLocal->observedAt = observedAtP->value.f;
    }

     //oldTemporalTreeNodeLevel++;
     //}
}



void  attrSubAttrExtract(KjNode* subAttrP, OrionldTemporalDbSubAttributeTable* dbSubAttributeTableLocal)
{
    //for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
    //{
     dbSubAttributeTableLocal->subAttributeName = subAttrP->name;
     if (subAttrP->type != KjObject)
     {
         LM_W(("Temporal - Bad Input - Key values not supported"));
         return;
     }

     KjNode* attrTypeP  = kjLookup(subAttrP, "type");
     kjChildRemove (subAttrP,attrTypeP);
     dbSubAttributeTableLocal->subAttributeType = attrTypeP->value.s;

      if (strcmp (dbSubAttributeTableLocal->subAttributeType,"Relationship") == 0)
     // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Relationship")
     {
         KjNode* attributeObject  = kjLookup(attrP, "object");
         kjChildRemove (attrP,attributeObject);

         // dbEntityTableLocal.attributeValueType  = kjLookup(attrP, "object");
         dbSubAttributeTableLocal->subAttributeValueType  = EnumValueObject;
         dbSubAttributeTableLocal->subAttributeValueString = attributeObject->value.s;
         LM_TMP(("CCSR:  Relationship : '%s'", dbSubAttributeTableLocal->subAttributeValueString));

     }
     else if (strcmp (dbSubAttributeTableLocal->subAttributeType,"GeoProperty") == 0)
     // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "GeoProperty")
     {
         KjNode* valueP  = kjLookup(attrP, "value");  //Chandra-TBD
         kjChildRemove (attrP,valueP);
         // Chandra-TBI
     }
     else if (strcmp (dbSubAttributeTableLocal->subAttributeType,"Property") == 0)
     // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Property")
     {
         KjNode* valueP  = kjLookup(attrP, "value");  //Chandra-TBD
         kjChildRemove (attrP,valueP);

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
       KjNode* nodeP  = kjLookup(attrP, "unitCode");
       if(nodeP != NULL)
       {
           kjChildRemove (attrP,nodeP);
           // Chandra-TBI
       }

       nodeP  = kjLookup(attrP, "observationSpace");
       if(nodeP != NULL)
       {
           kjChildRemove (attrP,nodeP);
           // Chandra-TBI
       }

       nodeP  = kjLookup(attrP, "datasetId");
       if(nodeP != NULL)
       {
           kjChildRemove (attrP,nodeP);
           // Chandra-TBI
       }

       nodeP  = kjLookup(attrP, "instanceid");
       if(nodeP != NULL)
       {
           kjChildRemove (attrP,nodeP);
           // Chandra-TBI
       }

       nodeP  = kjLookup(attrP, "location");
       if(nodeP != NULL)
       {
           kjChildRemove (attrP,nodeP);
           // Chandra-TBI
       }

       nodeP  = kjLookup(attrP, "operationSpace");
       if(nodeP != NULL)
       {
           kjChildRemove (attrP,nodeP);
           // Chandra-TBI
       }

       /*if (attrP->value.firstChildP != NULL)  //Chandra-TBCKZ (Can we safely assume that there are sub attributes to sub attributes?)
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

       }*/

       dbSubAttributeTableLocal->createdAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;
       dbSubAttributeTableLocal->modifiedAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;

       KjNode* observedAtP = kjLookup(attrP, "observedAt");
       if (observedAtP != NULL)
       {
          if (observedAtP->type == KjString)
             dbSubAttributeTableLocal->observedAt = parse8601Time(observedAtP->value.s);
          else
             dbSubAttributeTableLocal->observedAt = observedAtP->value.f;
       }
    // }
}




// ----------------------------------------------------------------------------
//
// TemporalPgDBConnectorOpen(PGconn* conn) - function to close the Postgres database connection gracefully
//
// ----------------------------------------------------------------------------
bool TemporalPgDBConnectorClose()
{
    if(oldPgDbTenantConnection != NULL)
    {
          PQfinish(oldPgDbTenantConnection); // Closes the TenantDB connection
    }


    if(oldPgDbConnection != NULL)
    {
    	PQfinish(oldPgDbConnection); //Closes connection and and also frees memory used by the PGconn* conn variable
    }
    else
    {
    	return false;
    }

    return true;
}

// ----------------------------------------------------------------------------
//
// bool TemporalPgDBConnectorOpen(char *tenantName) - function to open the Postgres database connection
//
// ----------------------------------------------------------------------------
bool TemporalPgDBConnectorOpen()
{
    char oldPgDbConnCheckSql[] = "user=postgres password=orion dbname=postgres"; //Need to be changed to environment variables CHANDRA-TBD
    oldPgDbConnection = PQconnectdb(oldPgDbConnCheckSql);
    if (PQstatus(oldPgDbConnection) == CONNECTION_BAD)
    {
        LM_E(("Connection to Postgress database is not achieved"));
        LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgDbConnection)));
        TemporalPgDBConnectorClose(); //close connection and cleanup
        return false;
    }
    else if (PQstatus(oldPgDbConnection) == CONNECTION_OK)
    {
        //puts("CONNECTION_OK");
        LM_K(("Connection is ok with the Postgres database\n"));
        return true; //Return the connection handler
    }
    return false;
}


// ----------------------------------------------------------------------------
//
// bool TemporalPgDBConnectorOpen(char *tenantName) - function to open the Postgres database connection
//
// ----------------------------------------------------------------------------
bool TemporalPgDBConnectorOpen(char *tenantName)
{
    LM_K(("Trying to open connection to Postgres database for new tenat database creation %s\n", tenantName));

    //  oldPgDbConnection = TemporalDBConnectorOpen();
    if(TemporalPgDBConnectorOpen() != false)
    {
        LM_K(("Trying to create database for Tenant %s\n", tenantName));

        char oldPgDbSqlSyntax[]= ";";
        char oldPgDbSqlCreateTDbSQL[] = "CREATE DATABASE ";
        strcat (oldPgDbSqlCreateTDbSQL, tenantName);
        strcat (oldPgDbSqlCreateTDbSQL, oldPgDbSqlSyntax);
        char oldPgTDbConnSQL[] = "user=postgres password=orion dbname= ";
        strcat (oldPgTDbConnSQL, tenantName);

        LM_K(("Command to create database for Tenant %s\n", tenantName));

        PGresult* oldPgTenandDbResult = PQexec(oldPgDbConnection, oldPgDbSqlCreateTDbSQL);
        LM_K(("Opening database connection for Tenant %s\n", tenantName));

        oldPgDbTenantConnection = PQconnectdb(oldPgTDbConnSQL);
        if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK && PQstatus(oldPgDbTenantConnection) != CONNECTION_OK)
        {
                LM_E(("Connection to %s database is not achieved or created", tenantName));
                LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgDbTenantConnection)));
                TemporalPgDBConnectorClose(); //close Tenant DB connection and cleanup
                return false;
        }
        PQclear(oldPgTenandDbResult);
	}
	else
	{
		 LM_E(("Connection to PostGress database is not achieved or created", tenantName));
                 LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgDbConnection)));
                 TemporalPgDBConnectorClose(); //close Tenant DB connection and cleanup
	}
	return true;
}


// ----------------------------------------------------------------------------
//
// temporalInitialiseTenant -
//
bool temporalInitialiseTenant(char *tenantName)
{
    LM_K(("Trying to open connection to Postgres database for new tenat database creation %s\n", tenantName));

    //  oldPgDbConnection = TemporalDBConnectorOpen();
    if(TemporalPgDBConnectorOpen() != false)
    {
        LM_K(("Trying to create database for Tenant %s\n", tenantName));

    		int oldPgDbSqlCreateTDbSQLBufferSize = 1024;
    		int oldPgDbSqlCreateTDbSQLUsedBufferSize = 0;
      	char* oldPgDbSqlCreateTDbSQL = kaAlloc(&orionldState.kalloc, oldPgDbSqlCreateTDbSQLBufferSize);

        // char oldPgDbSqlSyntax[]= ";";
        // char oldPgDbSqlCreateTDbSQL[] = "CREATE DATABASE ";
        // strcat (oldPgDbSqlCreateTDbSQL, tenantName);
        // strcat (oldPgDbSqlCreateTDbSQL, oldPgDbSqlSyntax);
        // char oldPgTDbConnSQL[] = "user=postgres password=orion dbname= ";
        // strcat (oldPgTDbConnSQL, tenantName);


    		strncpy(oldPgDbSqlCreateTDbSQL, "CREATE DATABASE ", oldPgDbSqlCreateTDbSQLBufferSize);
    		oldPgDbSqlCreateTDbSQLUsedBufferSize += 16;
    		strncat(oldPgDbSqlCreateTDbSQL, tenantName, oldPgDbSqlCreateTDbSQLBufferSize - oldPgDbSqlCreateTDbSQLUsedBufferSize);
    		oldPgDbSqlCreateTDbSQLUsedBufferSize += sizeof(tenantName);
        strncpy(oldPgDbSqlCreateTDbSQL, ";", oldPgDbSqlCreateTDbSQLBufferSize - oldPgDbSqlCreateTDbSQLUsedBufferSize);
        oldPgDbSqlCreateTDbSQLUsedBufferSize += 1;

    		int oldPgTDbConnSQLBufferSize = 1024;
    		int oldPgTDbConnSQLUsedBufferSize = 0;
    		char oldPgTDbConnSQLUser[] = "postgres"; // Chandra-TBD
    		char oldPgTDbConnSQLPasswd[] = "orion"; // Chandra-TBD
    		char* oldTemporalSQLBuffer = kaAlloc(&orionldState.kalloc, oldPgTDbConnSQLBufferSize);

        strncpy(oldTemporalSQLBuffer, "user=", oldPgTDbConnSQLBufferSize);
        oldPgTDbConnSQLUsedBufferSize += 5;
    		strncat(oldTemporalSQLBuffer, oldPgTDbConnSQLUser, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    		oldPgTDbConnSQLUsedBufferSize += sizeof(oldPgTDbConnSQLUser);
        strncat(oldTemporalSQLBuffer, " ", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
        oldPgTDbConnSQLUsedBufferSize += 1;
        strncat(oldTemporalSQLBuffer, "password=", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
        oldPgTDbConnSQLUsedBufferSize += 9;
        strncat(oldTemporalSQLBuffer, oldPgTDbConnSQLPasswd, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
        oldPgTDbConnSQLUsedBufferSize += sizeof(oldPgTDbConnSQLPasswd);
        strncat(oldTemporalSQLBuffer, " ", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
        oldPgTDbConnSQLUsedBufferSize += 1;
        strncat(oldTemporalSQLBuffer, "dbname=", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
        oldPgTDbConnSQLUsedBufferSize += 7;
        strncat(oldTemporalSQLBuffer, tenantName, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
        oldPgTDbConnSQLUsedBufferSize += sizeof(tenantName);


        LM_K(("Command to create database for Tenant %s\n", tenantName));

        PGresult* oldPgTenandDbResult = PQexec(oldPgDbConnection, oldPgDbSqlCreateTDbSQL);
        LM_K(("Opening database connection for Tenant %s\n", tenantName));

        oldPgDbTenantConnection = PQconnectdb(oldTemporalSQLBuffer);
        if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK && PQstatus(oldPgDbTenantConnection) != CONNECTION_OK)
        {
            LM_E(("Connection to %s database is not achieved or created", tenantName));
            LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgDbTenantConnection)));
            TemporalPgDBConnectorClose(); //close Tenant DB connection and cleanup
            return false;
        }
        else if (PQstatus(oldPgDbTenantConnection) == CONNECTION_OK)
        {
            LM_K(("Connection is ok with the %s database\n", tenantName));
            LM_K(("Now crreating the tables for the teanant %s \n", tenantName));
            const char *oldPgDbCreateTenantTables[9][250] =
            {
                "CREATE EXTENSION IF NOT EXISTS postgis",

                //  "CREATE EXTENSION IF NOT EXISTS timescaledb",

                "drop table attribute_sub_properties_table",

                "drop table attributes_table",

                "drop type attribute_value_type_enum",

                "drop table entity_table",

                "CREATE TABLE IF NOT EXISTS entity_table (entity_id TEXT NOT NULL,entity_type TEXT, geo_property GEOMETRY,created_at TIMESTAMP,modified_at TIMESTAMP, observed_at TIMESTAMP,PRIMARY KEY (entity_id))",

                "create type attribute_value_type_enum as enum ('value_string', 'value_number', 'value_boolean', 'value_relation', 'value_object', 'value_datetime', 'value_geo')",

                "CREATE TABLE IF NOT EXISTS attributes_table (entity_id TEXT NOT NULL REFERENCES entity_table(entity_id),id TEXT NOT NULL, name TEXT,"
                        "value_type attribute_value_type_enum, sub_property BOOL, unit_code TEXT, data_set_id TEXT,"
                        "instance_id bigint GENERATED BY DEFAULT AS IDENTITY(START WITH 1 INCREMENT BY 1), value_string TEXT, value_boolean BOOL, value_number float8,"
                        "value_relation TEXT,value_object TEXT, value_datetime TIMESTAMP,geo_property GEOMETRY,created_at TIMESTAMP NOT NULL,modified_at TIMESTAMP NOT NULL,"
                        "observed_at TIMESTAMP NOT NULL,PRIMARY KEY (entity_id,id,observed_at,created_at,modified_at))",

                //  "SELECT create_hypertable('attributes_table', 'modified_at')",

                "CREATE TABLE IF NOT EXISTS attribute_sub_properties_table (entity_id TEXT NOT NULL,attribute_id TEXT NOT NULL,attribute_instance_id bigint, id TEXT NOT NULL,"
                        "value_type attribute_value_type_enum,value_string TEXT, value_boolean BOOL, value_number float8, value_relation TEXT,name TEXT,geo_property GEOMETRY,"
                        "unit_code TEXT, value_object TEXT, value_datetime TIMESTAMP,instance_id bigint GENERATED BY DEFAULT AS IDENTITY(START WITH 1 INCREMENT BY 1),PRIMARY KEY (instance_id))"
            };
            PQclear(oldPgTenandDbResult);

            for(int oldPgDbNumObj = 0; oldPgDbNumObj < 11; oldPgDbNumObj++)
            {
                oldPgTenandDbResult = PQexec(oldPgDbTenantConnection, *oldPgDbCreateTenantTables[oldPgDbNumObj]);

                if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK)
                {
                        LM_K(("Postgres DB command failed for database for Tenant %s%s\n", tenantName,oldPgDbCreateTenantTables[oldPgDbNumObj]));
                        break;
                }
                PQclear(oldPgTenandDbResult);
            }
        }
    }
    else
    {
            TemporalPgDBConnectorClose(); //close Postgres DB connection and cleanup
            return false;
    }
    return true;
}

// -----------------------------------------------------------------------------
//
// temporalExecSqlStatement
//
//
bool temporalExecSqlStatement(char* oldTemporalSQLBuffer)
{
        char oldTenantName[] = "orionld";

        TemporalPgDBConnectorOpen(oldTenantName);  //  opening Tenant Db connection

        oldPgTenandDbResult = PQexec(oldPgDbTenantConnection, "BEGIN");
        if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK)
        {
                LM_E(("BEGIN command failed for inserting single Entity into DB %s\n",oldTenantName));
                PQclear(oldPgTenandDbResult);
                TemporalPgDBConnectorClose();
                return false;
        }
        PQclear(oldPgTenandDbResult);

	//  char* oldTemporalSQLFullBuffer = temporalCommonExtractTree();
	oldPgTenandDbResult = PQexec(oldPgDbTenantConnection, oldTemporalSQLBuffer);
	if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK)
  	{
        	LM_E(("i%s command failed for inserting single Entity into DB %s\n",oldTemporalSQLBuffer, oldTenantName));
        	PQclear(oldPgTenandDbResult);
        	TemporalPgDBConnectorClose();
        	return false;
  	}
  	PQclear(oldPgTenandDbResult);


	oldPgTenandDbResult = PQexec(oldPgDbTenantConnection, "COMMIT");
  	if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK)
  	{
        	LM_E(("COMMIT command failed for inserting single Entity into DB %s\n",oldTenantName));
        	PQclear(oldPgTenandDbResult);
        	TemporalPgDBConnectorClose();
        	return false;
  	}

  	PQclear(oldPgTenandDbResult);
  	TemporalPgDBConnectorClose();
  	return true;
}


// ----------------------------------------------------------------------------
//
// PGconn* TemporalConstructUpdateSQLStatement(OrionldTemporalDbAllTables* dbAllTablesLocal) - function to buil update SQL statement
//
// ----------------------------------------------------------------------------
bool TemporalConstructInsertSQLStatement(OrionldTemporalDbAllTables* dbAllTablesLocal)
{
    //if (strcmp (tableName,"Entity") == 0);
    //int temporalSQLStatementLengthBuffer = sizeof(dbAllTablesLocal->dbEntityTableLocal);
    //char* updateEntityTableSQLStatement = temporalSQLStatementLengthBuffer * 1024;  // Not smart Chandra-TBI
    //int dbEntityTable = sizeof(dbAllTablesLocal.entityTableArray);
    int dbEntityTable = sizeof(dbAllTablesLocal->entityTableArray);
    int dbAttribTable = sizeof(dbAllTablesLocal->attributeTableArray);
    int dbSubAttribTable = sizeof(dbAllTablesLocal->subAttributeTableArray);


    for (int dbEntityLoop=0; dbEntityLoop < dbEntityTable; dbEntityLoop++)
    {
        int dbEntityBufferSize = 10 * 1024;
        char* dbEntityStrBuffer = kaAlloc(&orionldState.kalloc, dbEntityBufferSize);
        bzero(dbEntityStrBuffer, dbEntityBufferSize);

        snprintf(dbEntityStrBuffer, dbEntityBufferSize, "INSERT INTO entity_table(entity_id,entity_type,geo_property,"
                "created_at,modified_at, observed_at) VALUES (%s, %s, NULL, %f, %f, NULL)",
                dbAllTablesLocal->entityTableArray[dbEntityLoop].entityId,
                dbAllTablesLocal->entityTableArray[dbEntityLoop].entityType,
                dbAllTablesLocal->entityTableArray[dbEntityLoop].createdAt,
                dbAllTablesLocal->entityTableArray[dbEntityLoop].modifiedAt);
        //
        // Some traces just to see how the KjNode tree works
        //
        LM_TMP(("CCSR: dbEntityStrBuffer:     '%s'", dbEntityStrBuffer));
        LM_TMP(("CCSR:"));
    }

    for (int dbAttribLoop=0; dbAttribLoop < dbAttribTable; dbAttribLoop++)
    {
        int dbAttribBufferSize = 10 * 1024;
        char* dbAttribStrBuffer = kaAlloc(&orionldState.kalloc, dbAttribBufferSize);
        bzero(dbAttribStrBuffer, dbAttribBufferSize);

        int allValuesSize = 2048;
        char* allValues = kaAlloc(&orionldState.kalloc,allValuesSize);

        allValuesRender (&dbAllTablesLocal->attributeTableArray[dbAttribLoop], allValues, allValuesSize);

            //Chandra-TBI
        snprintf(dbAttribStrBuffer, dbAttribBufferSize, "INSERT INTO attributes_table(entity_id,id,value_type,"
            "sub_property,unit_code, data_set_id,value_string, value_boolean, value_number, value_relation,"
            "value_object, value_datetime, geo_property, observed_at, created_at, modified_at) "
                " VALUES (%s, %s, %s, %s, %s, %s, %s)",
                dbAllTablesLocal->attributeTableArray[dbAttribLoop].entityId,
                dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeName,
                dbAllTablesLocal->attributeTableArray[dbAttribLoop].attributeValueType,
                (dbAllTablesLocal->attributeTableArray[dbAttribLoop].subProperty==true)? "true" : "false",
                allValues,
                dbAllTablesLocal->attributeTableArray[dbAttribLoop].createdAt,
                dbAllTablesLocal->attributeTableArray[dbAttribLoop].modifiedAt);
    }

    for (int dbSubAttribLoop=0; dbSubAttribLoop < dbSubAttribTable; dbSubAttribLoop++)
    {
        int dbSubAttribBufferSize = 10 * 1024;
        char* dbSubAttribStrBuffer = kaAlloc(&orionldState.kalloc, dbSubAttribBufferSize);
        bzero(dbSubAttribStrBuffer, dbSubAttribBufferSize);
        //Chandra-TBI
    }

    return true;
}

void allValuesRender (OrionldTemporalDbAttributeTable* attrLocalP, char* allValues, int allValuesSize)
{
    switch (attrLocalP->attributeValueType)
    {
      case EnumValueString:
        snprintf(allValues, allValuesSize, "%s, NULL, NULL, NULL, NULL",attrLocalP->valueString);

        case EnumValueNumber:
          snprintf(allValues, allValuesSize, "NULL, %lld, NULL, NULL, NULL",attrLocalP->valueNumber);

        case EnumValueBool:
          snprintf(allValues, allValuesSize, "NULL, NULL, %s, NULL, NULL",(attrLocalP->valueBool==true)? "true" : "false");

        case EnumValueArray:
          snprintf(allValues, allValuesSize, "NULL, NULL, NULL, %s, NULL",attrLocalP->valueArray);

        case EnumValueObject:
          snprintf(allValues, allValuesSize, "NULL, NULL, NULL, NULL, %s",attrLocalP->valueObject);

        default:
          LM_W(("Teamporal - Bad Input - Key values not supported"));
          return;
    }

    int unitCodeValuesSize = 128;
    char* unitCodeValue = kaAlloc(&orionldState.kalloc, unitCodeValuesSize);
    if (attrLocalP->unitCode == NULL)
    {
        snprintf(unitCodeValue, unitCodeValuesSize, "NULL", attrLocalP->unitCode);
    }
    else
    {
        snprintf(unitCodeValue, unitCodeValuesSize, "%s", attrLocalP->unitCode);
    }

    int dataSetIdSize = 128;
    char* dataSetIdValue = kaAlloc(&orionldState.kalloc, dataSetIdSize);
    if (attrLocalP->dataSetId == NULL)
    {
        snprintf(dataSetIdValue, dataSetIdSize, "NULL", attrLocalP->dataSetId);
    }
    else
    {
        snprintf(dataSetIdValue, dataSetIdSize, "%s", attrLocalP->dataSetId);
    }

    int geoProprtySize = 512;
    char* geoProprtyValue = kaAlloc(&orionldState.kalloc, geoProprtySize);
    if (attrLocalP->geoProperty == NULL)
    {
        snprintf(geoProprtyValue, geoProprtySize, "NULL", attrLocalP->geoProperty);
    }
    else
    {
        // Chandra-TBI
        // snprintf(geoProprtyValue, geoProprtySize, "%f", attrLocalP->geoProperty);
    }

    int observedAtSize = 512;
    char* observedAtValue = kaAlloc(&orionldState.kalloc, observedAtSize);
    if (attrLocalP->observedAt == NULL)
    {
        snprintf(observedAtValue, observedAtSize, "NULL", attrLocalP->observedAt);
    }
    else
    {
        snprintf(observedAtValue, observedAtSize, "%f", attrLocalP->observedAt);
    }

    snprintf(allValues, allValuesSize, "%s, %s, %s, %s, %s",
        unitCodeValue, dataSetIdValue, allValues, geoProprtyValue, observedAtValue);
}


// ----------------------------------------------------------------------------
//
// PGconn* TemporalPgTenantDBConnectorOpen(char* tenantName) - function to open the Postgres database connection
//
// ----------------------------------------------------------------------------
bool TemporalPgTenantDBConnectorOpen(char* tenantName)
{
    int oldPgTDbConnSQLBufferSize = 1024;
    int oldPgTDbConnSQLUsedBufferSize = 0;
    char oldPgTDbConnSQLUser[] = "postgres"; // Chandra-TBD
    char oldPgTDbConnSQLPasswd[] = "orion"; // Chandra-TBD
    char* oldTemporalSQLBuffer = kaAlloc(&orionldState.kalloc, oldPgTDbConnSQLBufferSize);

    strncpy(oldTemporalSQLBuffer, "user=", oldPgTDbConnSQLBufferSize);
    oldPgTDbConnSQLUsedBufferSize += 5;
    strncat(oldTemporalSQLBuffer, oldPgTDbConnSQLUser, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += sizeof(oldPgTDbConnSQLUser);
    strncat(oldTemporalSQLBuffer, " ", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += 1;
    strncat(oldTemporalSQLBuffer, "password=", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += 9;
    strncat(oldTemporalSQLBuffer, oldPgTDbConnSQLPasswd, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += sizeof(oldPgTDbConnSQLPasswd);
    strncat(oldTemporalSQLBuffer, " ", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += 1;
    strncat(oldTemporalSQLBuffer, "dbname=", oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += 7;
    strncat(oldTemporalSQLBuffer, tenantName, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += sizeof(tenantName);

    oldPgDbTenantConnection = PQconnectdb(oldTemporalSQLBuffer);

    if (PQstatus(oldPgDbTenantConnection) == CONNECTION_BAD)
    {
        LM_E(("Connection to Tenant database is not achieved"));
        LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgDbTenantConnection)));
        TemporalPgDBConnectorClose(); //close connection and cleanup
        return false;
    }
    else if (PQstatus(oldPgDbConnection) == CONNECTION_OK)
    {
        //puts("CONNECTION_OK");
        LM_K(("Connection is ok with the Postgres database\n"));
        return true; //Return the connection handler
    }
    return false;
}
