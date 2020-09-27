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
OrionldTemporalDbAllTables  temporalCommonExtractFullAttributeTable()
{
        OrionldTemporalDbAllTables dbAllTablesLocal;
        OrionldTemporalDbEntityTable* dbEntityTableLocal;
        OrionldTemporalDbAttributeTable* dbAttributeTableLocal;
        OrionldTemporalDbSubAttributeTable* dbSubAttributeTableLocal;

        char buff [1024];  // Chandra-TBR
        kjRender(orionldState.kjsonP,orionldState.requestTree,buff,sizeof(buff));  // Chandra-TBR
        LM_TMP(("CCSR: The entire tree:     '%s'", buff));  // Chandra-TBR

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

        int entityArrayTotalSize = sizeof(OrionldTemporalDbAttributeTable);
        dbEntityTableLocal = (OrionldTemporalDbEntityTable*) kaAlloc(&orionldState.kalloc, entityArrayTotalSize);


        dbEntityTableLocal[0].entityId = orionldState.payloadIdNode->value.s;
        dbEntityTableLocal[0].entityType = orionldState.payloadTypeNode->value.s;

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
        //
        // Some traces just to see how the KjNode tree works
        //
        LM_TMP(("CCSR: oldTemporalSQLBuffer:     '%s'", oldTemporalSQLBuffer));
        LM_TMP(("CCSR:"));

        int oldTemporalTreeNodeLevel = 0;

        int attributesNumbers = 0;
        for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
        {
                attributesNumbers++;
        }

        int attribArrayTotalSize = attributesNumbers * sizeof(OrionldTemporalDbAttributeTable);
        dbAttributeTableLocal = (OrionldTemporalDbAttributeTable*) kaAlloc(&orionldState.kalloc, attribArrayTotalSize);
        bzero(dbAttributeTableLocal, attribArrayTotalSize);


        for (KjNode* attrP = orionldState.requestTree->value.firstChildP; attrP != NULL; attrP = attrP->next)
        {
                dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeName = attrP->name;
                if (attrP->type != KjObject)
                {
                        LM_W(("Teamporal - Bad Input - Key values not supported"));
                        continue;
                }

                KjNode* attrTypeP  = kjLookup(attrP, "type");
                dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType = attrTypeP->value.s;

                 if (strcmp (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType,"Relationship") == 0)
                // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Relationship")
                {
                        KjNode* attributeObject  = kjLookup(attrP, "object");
                        if(attributeObject != NULL)
                        {
                              // dbEntityTableLocal.attributeValueType  = kjLookup(attrP, "object");
                              dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeValueType  = EnumValueRelation;
                              dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString = attributeObject->value.s;
                              LM_TMP(("CCSR:  Relationship : '%s'", dbAttributeTableLocal[oldTemporalTreeNodeLevel].valueString));
                        }
                }
                else if (strcmp (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType,"GeoProperty") == 0)
                // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "GeoProperty")
                {
                        // Chandra-TBI
                }
                else if (strcmp (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType,"Property") == 0)
                // if (dbAttributeTableLocal[oldTemporalTreeNodeLevel].attributeType == "Property")
                {
                        KjNode* valueP  = kjLookup(attrP, "value");  //Chandra-TBD
                        if (valueP != NULL)
                        {
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
                        else // if (valueP->type == "p")  //Chandra - TBCKZ
                        {
                                dbAttributeTableLocal[oldTemporalTreeNodeLevel].subProperty = true;
                                int subAttributeTreeNodeLevel = 0;
                                int subAtrributeNumbers = 0;
                                for (KjNode* subAttrP = valueP->value.firstChildP; subAttrP != NULL; subAttrP = attrP->next)
                                // for (valueP->value.firstChildP; attrP != NULL; attrP = attrP->next)  // Chandra-TBCKZ
                                {
                                        subAtrributeNumbers++;
                                }

                                int subAttribArrayTotalSize = subAtrributeNumbers * sizeof(OrionldTemporalDbSubAttributeTable);
                                dbSubAttributeTableLocal = (OrionldTemporalDbSubAttributeTable*) kaAlloc(&orionldState.kalloc, subAttribArrayTotalSize);
                                bzero(dbSubAttributeTableLocal, subAttribArrayTotalSize);
                                for (KjNode* subAttrP = valueP->value.firstChildP; subAttrP != NULL; subAttrP = attrP->next)
                                {
                                        if (attrP->type != KjObject)
                                        {
                                                LM_W(("Teamporal - Bad Input - Key values not supported"));
                                                continue;
                                        }

                                        KjNode* attrTypeP  = kjLookup(attrP, "type");
                                        dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeType = attrTypeP->value.s;

                                        if (strcmp (dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeType,"Relationship") == 0)
                                        // if (dbSubAttributeTableLocal[subAttributeTreeNodeLevel].attributeType == "Relationship")
                                        {
                                                KjNode* attributeObject  = kjLookup(attrP, "object");
                                                // dbEntityTableLocal.attributeValueType  = kjLookup(attrP, "object");
                                                dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueType  = EnumValueRelation;
                                                dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueString = attributeObject->value.s;
                                                LM_TMP(("CCSR:  Relationship : '%s'", dbSubAttributeTableLocal[oldTemporalTreeNodeLevel].subAttributeValueString));
                                        }

                                        if (strcmp (dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeType,"Property") == 0)
                                        // if (dbSubAttributeTableLocal[subAttributeTreeNodeLevel].attributeType == "Property")
                                        {

                                                KjNode* subAttrValueP  = kjLookup(subAttrP, "value");  //Chandra-TBD
                                                if (valueP->type == KjFloat)
                                                {
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueType  = EnumValueNumber;
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueBoolean = subAttrValueP->value.f;
                                                }
                                                else if (valueP->type == KjInt)
                                                {
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueType  = EnumValueNumber;
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueNumber = subAttrValueP->value.i;
                                                }
                                                else if (valueP->type == KjArray)
                                                {
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueType  = EnumValueArray;
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
                                                      kjRender(orionldState.kjsonP, subAttrValueP->value.firstChildP, dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueString, 1024);
                                                }
                                                else if (valueP->type == KjObject)
                                                {
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueType  = EnumValueObject;
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueString = kaAlloc(&orionldState.kalloc, 1024); //Chandra-TBD Not smart
                                                      kjRender(orionldState.kjsonP, subAttrValueP->value.firstChildP, dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueString, 1024);
                                                }
                                                else if (valueP->type == KjBoolean)
                                                {
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueType  = EnumValueBool;
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueBoolean = subAttrValueP->value.b;

                                                }
                                                else if (valueP->type == KjString)
                                                {
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueType  = EnumValueString;
                                                      dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeValueString = subAttrValueP->value.s;
                                                }
                                        }
                                        if (strcmp (dbSubAttributeTableLocal[subAttributeTreeNodeLevel].subAttributeType,"GeoProperty") == 0)
                                        // if (dbSubAttributeTableLocal[subAttributeTreeNodeLevel].attributeType == "GeoProperty")
                                        {
                                                // Chandra-TBI
                                        }

                                        dbSubAttributeTableLocal[subAttributeTreeNodeLevel].createdAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;
                                        dbSubAttributeTableLocal[subAttributeTreeNodeLevel].modifiedAt = orionldState.timestamp.tv_sec + ((double) orionldState.timestamp.tv_nsec) / 1000000000;

                                        KjNode* subAttrObservedAtP = kjLookup(attrP, "observedAt");
                                        if (subAttrObservedAtP != NULL)
                                        {
                                                dbSubAttributeTableLocal[subAttributeTreeNodeLevel].observedAt = parse8601Time(subAttrObservedAtP->value.s);
                                        }

                                        subAttributeTreeNodeLevel++;
                                }
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

        dbAllTablesLocal.entityTableArray = dbEntityTableLocal;
        dbAllTablesLocal.attributeTableArray = dbAttributeTableLocal;
        dbAllTablesLocal.subAttributeTableArray = dbSubAttributeTableLocal;

        return dbAllTablesLocal;
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
bool TemporalConstructUpdateSQLStatement(OrionldTemporalDbAllTables* dbAllTablesLocal)
{
    //if (strcmp (tableName,"Entity") == 0);
    //int temporalSQLStatementLengthBuffer = sizeof(dbAllTablesLocal->dbEntityTableLocal);
    //char* updateEntityTableSQLStatement = temporalSQLStatementLengthBuffer * 1024;  // Not smart Chandra-TBI
    return false;
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
