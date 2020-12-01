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
* Author: Ken Zangelin, Chandra Challagonda
*/
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/HttpStatusCode.h"                               // SccNotImplemented
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService
#include "orionld/temporal/temporalTenantInitialise.h"         // Own interface



// -----------------------------------------------------------------------------
//
// Global vars - FIXME: need a connection pool, or for now at least a semaphore for the connection?
//
PGconn*    oldPgDbConnection        = NULL;
PGconn*    oldPgDbTenantConnection  = NULL;
PGresult*  oldPgTenandDbResult      = NULL;



// ----------------------------------------------------------------------------
//
// TemporalPgDBConnectorOpen - function to close the Postgres database connection gracefully
//
// FIXME: protect the connection with a semaphore
//
bool TemporalPgDBConnectorClose(void)
{
  if (oldPgDbTenantConnection != NULL)
  {
    PQfinish(oldPgDbTenantConnection); // Closes the TenantDB connection
    oldPgDbTenantConnection = NULL;
  }

  if (oldPgDbConnection == NULL)
  {
    LM_E(("Error ... oldPgDbConnection == NULL"));
    return false;
  }

  PQfinish(oldPgDbConnection); //Closes connection and and also frees memory used by the PGconn* conn variable
  oldPgDbConnection = NULL;

  return true;
}



// ----------------------------------------------------------------------------
//
// TemporalPgDBConnectorOpen - function to open the Postgres database connection
//
bool TemporalPgDBConnectorOpen(void)
{
  char oldPgDbConnCheckSql[512];

  // FIXME: use CLI variables instead of these definitions (TEMPORAL_DB_USER, TEMPORAL_DB_PASSWORD, TEMPORAL_DB)
  snprintf(oldPgDbConnCheckSql, sizeof(oldPgDbConnCheckSql), "user=%s password=%s dbname=%s", TEMPORAL_DB_USER, TEMPORAL_DB_PASSWORD, TEMPORAL_DB);

  oldPgDbConnection = PQconnectdb(oldPgDbConnCheckSql);
  if (PQstatus(oldPgDbConnection) != CONNECTION_OK)
  {
    LM_E(("Database Error (error connecting to database: %s)", PQerrorMessage(oldPgDbConnection)));
    TemporalPgDBConnectorClose(); //close connection and cleanup
    return false;
  }

  return true;  // FIXME: return the connection handler
}


/*

// ----------------------------------------------------------------------------
//
// TemporalPgDBConnectorOpen - function to open the Postgres database connection
//
bool TemporalPgDBConnectorOpen(char* tenant)
{
  if (TemporalPgDBConnectorOpen() == true)  // oldPgDbConnection is set by TemporalPgDBConnectorOpen ...
  {
    char oldPgDbSqlSyntax[]= ";";
    char oldPgDbSqlCreateTDbSQL[] = "CREATE DATABASE ";  // FIXME: snprintf
    strcat(oldPgDbSqlCreateTDbSQL, tenant);
    strcat(oldPgDbSqlCreateTDbSQL, oldPgDbSqlSyntax);

    char oldPgTDbConnSQL[] = "user=postgres password=password dbname= ";    // FIXME: snprintf
    strcat (oldPgTDbConnSQL, tenant);

    LM_TMP(("Command to create database for Tenant %s\n", tenant));

    PGresult* oldPgTenandDbResult = PQexec(oldPgDbConnection, oldPgDbSqlCreateTDbSQL);
    LM_TMP(("Opening database connection for Tenant %s\n", tenant));

    oldPgDbTenantConnection = PQconnectdb(oldPgTDbConnSQL);
    if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK && PQstatus(oldPgDbTenantConnection) != CONNECTION_OK)
    {
      LM_E(("Connection to %s database is not achieved or created", tenant));
      LM_E(("Database Error (error connecting to postgres: %s)", PQerrorMessage(oldPgDbTenantConnection)));
      TemporalPgDBConnectorClose();
      return false;
    }
    PQclear(oldPgTenandDbResult);
	}
	else
	{
    LM_E(("Connection to PostGress database is not achieved or created", tenant));
    LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgDbConnection)));
    TemporalPgDBConnectorClose(); //close Tenant DB connection and cleanup
	}

	return true;
}

*/

// ----------------------------------------------------------------------------
//
// temporalTenantInitialise -
//
bool temporalTenantInitialise(const char* tenant)
{
  LM_K(("Trying to open connection to Postgres database for new tenat database creation %s\n", tenant));

  //  oldPgDbConnection = TemporalDBConnectorOpen();
  if (TemporalPgDBConnectorOpen() != false)
  {
    LM_K(("Trying to create database for Tenant %s\n", tenant));

    int   oldPgDbSqlCreateTDbSQLBufferSize     = 1024;
    int   oldPgDbSqlCreateTDbSQLUsedBufferSize = 0;
    char* oldPgDbSqlCreateTDbSQL               = kaAlloc(&orionldState.kalloc, oldPgDbSqlCreateTDbSQLBufferSize);


    //
    // FIXME: This entire bunch of strcpy/strcat needs to be change to use snprintf
    //

    strncpy(oldPgDbSqlCreateTDbSQL, "CREATE DATABASE ", oldPgDbSqlCreateTDbSQLBufferSize);
    oldPgDbSqlCreateTDbSQLUsedBufferSize += 16;
    strncat(oldPgDbSqlCreateTDbSQL, tenant, oldPgDbSqlCreateTDbSQLBufferSize - oldPgDbSqlCreateTDbSQLUsedBufferSize);
    oldPgDbSqlCreateTDbSQLUsedBufferSize += strlen(tenant);
    strncpy(oldPgDbSqlCreateTDbSQL, ";", oldPgDbSqlCreateTDbSQLBufferSize - oldPgDbSqlCreateTDbSQLUsedBufferSize);
    oldPgDbSqlCreateTDbSQLUsedBufferSize += 1;

    int oldPgTDbConnSQLBufferSize     = 1024;
    int oldPgTDbConnSQLUsedBufferSize = 0;
    char oldPgTDbConnSQLUser[]        = TEMPORAL_DB_USER; // Chandra-TBD
    char oldPgTDbConnSQLPasswd[]      = TEMPORAL_DB_PASSWORD; // Chandra-TBD
    char* oldTemporalSQLBuffer        = kaAlloc(&orionldState.kalloc, oldPgTDbConnSQLBufferSize);

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
    strncat(oldTemporalSQLBuffer, tenant, oldPgTDbConnSQLBufferSize - oldPgTDbConnSQLUsedBufferSize);
    oldPgTDbConnSQLUsedBufferSize += strlen(tenant);

    LM_K(("Command to create database for Tenant %s\n", tenant));

    PGresult* oldPgTenandDbResult = PQexec(oldPgDbConnection, oldPgDbSqlCreateTDbSQL);
    LM_K(("Opening database connection for Tenant %s\n", tenant));

    oldPgDbTenantConnection = PQconnectdb(oldTemporalSQLBuffer);
    if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK && PQstatus(oldPgDbTenantConnection) != CONNECTION_OK)
    {
      LM_E(("Database Error (unable  to connect to tenant '%s': %s", tenant, PQerrorMessage(oldPgDbTenantConnection)));
      TemporalPgDBConnectorClose(); //close Tenant DB connection and cleanup
      return false;
    }
    else if (PQstatus(oldPgDbTenantConnection) == CONNECTION_OK)
    {
      LM_K(("Connection is ok with the %s database\n", tenant));
      LM_K(("Now crreating the tables for the teanant %s \n", tenant));
      const char* oldPgDbCreateTenantTables[] =
        {
          // "CREATE EXTENSION IF NOT EXISTS postgis",
          //  "CREATE EXTENSION IF NOT EXISTS timescaledb",

          "CREATE TABLE IF NOT EXISTS entity_table ("
            "entity_id TEXT NOT NULL,"
            "entity_type TEXT,"
            "geo_property GEOMETRY,"
            "created_at TIMESTAMP,"
            "modified_at TIMESTAMP,"
            "observed_at TIMESTAMP,"
            "PRIMARY KEY (entity_id))",

          // FIXME: The type 'attribute_value_type_enum' needs to be added at startup but only if it doesn't already exists
          // "CREATE TYPE IF NOT EXISTS attribute_value_type_enum as enum ('value_string'," "'value_number', 'value_boolean', 'value_relation'," "'value_object', 'value_datetime', 'value_geo')",

          "CREATE TABLE IF NOT EXISTS attributes_table("
            "entity_id TEXT NOT NULL REFERENCES entity_table(entity_id),"
            "id TEXT NOT NULL,"
            "name TEXT,"
            "value_type attribute_value_type_enum,"
            "sub_property BOOL,"
            "unit_code TEXT,"
            "data_set_id TEXT,"
            "instance_id TEXT NOT NULL,"
            "value_string TEXT,"  
            "value_boolean BOOL,"
            "value_number float8,"
            "value_relation TEXT,"
            "value_object TEXT,"
            "value_datetime TIMESTAMP,"
            "geo_property GEOMETRY,"
            "created_at TIMESTAMP NOT NULL,"
            "modified_at TIMESTAMP NOT NULL,"
            "observed_at TIMESTAMP NOT NULL,"
            "PRIMARY KEY (entity_id,id,observed_at,created_at,modified_at))",

          //  "SELECT create_hypertable('attributes_table', 'modified_at')",

          "CREATE TABLE IF NOT EXISTS attribute_sub_properties_table("
            "entity_id TEXT NOT NULL,"
            "attribute_id TEXT NOT NULL,"
            "attribute_instance_id TEXT NOT NULL,"
            "id TEXT NOT NULL,"
            "value_type attribute_value_type_enum,"
            "value_string TEXT,"
            "value_boolean BOOL,"
            "value_number float8, "
            "value_relation TEXT,"
            "name TEXT,geo_property GEOMETRY,"
            "unit_code TEXT,"
            "value_object TEXT,"
            "value_datetime TIMESTAMP,"
            // 
            "instance_id bigint GENERATED BY DEFAULT AS IDENTITY"
            "(START WITH 1 INCREMENT BY 1),PRIMARY KEY (instance_id))"
        };
      PQclear(oldPgTenandDbResult);

      for (unsigned int oldPgDbNumObj = 0; oldPgDbNumObj < sizeof(oldPgDbCreateTenantTables) / sizeof(oldPgDbCreateTenantTables[0]); oldPgDbNumObj++)
      {
        oldPgTenandDbResult = PQexec(oldPgDbTenantConnection, oldPgDbCreateTenantTables[oldPgDbNumObj]);

        if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK)
        {
          LM_E(("Database Error(Postgres DB command failed for database for Tenant '%s'  (%s): %s)", tenant, oldPgDbCreateTenantTables[oldPgDbNumObj], PQerrorMessage(oldPgDbTenantConnection)));
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
// temporalExecSqlStatement -
//
bool temporalExecSqlStatement(char* oldTemporalSQLBuffer)
{
  char oldTenantName[] = "orion_ld";

  temporalTenantInitialise(oldTenantName);  //  opening Tenant Db connection

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
    LM_E(("command failed for inserting single Attribute into DB %s: '%s'", oldTenantName, oldTemporalSQLBuffer));
    LM_E(("Reason %s", PQerrorMessage(oldPgDbTenantConnection)));
    PQclear(oldPgTenandDbResult);
    TemporalPgDBConnectorClose();
    return false;
  }
  PQclear(oldPgTenandDbResult);

	oldPgTenandDbResult = PQexec(oldPgDbTenantConnection, "COMMIT");
  if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK)
  {
    LM_E(("COMMIT command failed for inserting single Sub Attribute into DB %s\n",oldTenantName));
    PQclear(oldPgTenandDbResult);
    TemporalPgDBConnectorClose();
    return false;
  }

  PQclear(oldPgTenandDbResult);
  TemporalPgDBConnectorClose();
  return true;
}
