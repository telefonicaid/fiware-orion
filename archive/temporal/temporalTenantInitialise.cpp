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
#include <postgresql/libpq-fe.h>                               // Postgres

#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/HttpStatusCode.h"                               // SccNotImplemented
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService
#include "orionld/temporal/temporal.h"                         // TEMPORAL_DB, TEMPORAL_DB_USER, TEMPORAL_DB_PASSWORD
#include "orionld/temporal/temporalTenantInitialise.h"         // Own interface



// -----------------------------------------------------------------------------
//
// Global vars - FIXME: need a connection pool
//
PGconn*    oldPgDbConnection        = NULL;
PGconn*    oldPgDbTenantConnection  = NULL;



// ----------------------------------------------------------------------------
//
// TemporalPgDBConnectorClose -
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



// -----------------------------------------------------------------------------
//
// pgTablesCreate -
//
bool pgTablesCreate(PGconn* pgConnectionP)
{
  const char* sqlV[] =
    {
      // "CREATE EXTENSION IF NOT EXISTS postgis",
      // "CREATE EXTENSION IF NOT EXISTS timescaledb",
      // "DROP TABLE attribute_sub_properties_table",      // For testing only
      // "DROP TABLE attributes_table",                    // For testing only
      // "DROP TABLE entity_table",                        // For testing only

      "CREATE TABLE IF NOT EXISTS entity_table ("
        "entity_id TEXT NOT NULL,"
        "entity_type TEXT,"
        "geo_property GEOMETRY,"
        "created_at TIMESTAMP,"
        "modified_at TIMESTAMP,"
        "observed_at TIMESTAMP,"
        "PRIMARY KEY (entity_id, modified_at))",

      // FIXME: The type 'attribute_value_type_enum' needs to be added at startup but only if it doesn't already exists
      // "CREATE TYPE IF NOT EXISTS attribute_value_type_enum as enum ('value_string'," "'value_number', 'value_boolean', 'value_relation'," "'value_object', 'value_datetime', 'value_geo')",

      "CREATE TABLE IF NOT EXISTS attributes_table("
        "entity_id TEXT NOT NULL,"
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
        "PRIMARY KEY (entity_id, id, modified_at))",

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
        "value_datetime TIMESTAMP)"
    };
  
  for (unsigned int ix = 0; ix < sizeof(sqlV) / sizeof(sqlV[0]); ix++)
  {
    PGresult* pgResult = PQexec(pgConnectionP, sqlV[ix]);

    if (PQresultStatus(pgResult) != PGRES_COMMAND_OK)
    {
      LM_E(("Database Error (Postgres DB command '%s': %s)", sqlV[ix], PQerrorMessage(pgConnectionP)));
      // No break here - let's continue to detect more errors ...
    }
    PQclear(pgResult);
  }

  return true;
}



// ----------------------------------------------------------------------------
//
// temporalTenantInitialise -
//
bool temporalTenantInitialise(const char* tenant)
{
  if (TemporalPgDBConnectorOpen() == false)
  {
    TemporalPgDBConnectorClose();  // close Postgres DB connection and cleanup
    return false;
  }

  LM_K(("Trying to create database for Tenant %s", tenant));

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

  PGresult* oldPgTenandDbResult = PQexec(oldPgDbConnection, oldPgDbSqlCreateTDbSQL);

  oldPgDbTenantConnection = PQconnectdb(oldTemporalSQLBuffer);
  if (PQresultStatus(oldPgTenandDbResult) != PGRES_COMMAND_OK && PQstatus(oldPgDbTenantConnection) != CONNECTION_OK)
  {
    LM_E(("Database Error (unable to connect to tenant '%s': %s", tenant, PQerrorMessage(oldPgDbTenantConnection)));
    TemporalPgDBConnectorClose(); //close Tenant DB connection and cleanup
    return false;
  }
  else if (PQstatus(oldPgDbTenantConnection) == CONNECTION_OK)
    pgTablesCreate(oldPgDbTenantConnection);

  return true;
}



// -----------------------------------------------------------------------------
//
// temporalExecSqlStatement -
//
bool temporalExecSqlStatement(char* sql)
{
  char oldTenantName[] = "orion_ld";

  temporalTenantInitialise(oldTenantName);  //  opening Tenant Db connection

  PGresult* pgResult = PQexec(oldPgDbTenantConnection, "BEGIN");
  if (PQresultStatus(pgResult) != PGRES_COMMAND_OK)
  {
    LM_E(("BEGIN command failed for inserting single Entity into DB %s", oldTenantName));
    PQclear(pgResult);
    TemporalPgDBConnectorClose();
    return false;
  }
  PQclear(pgResult);

	//  char* oldTemporalSQLFullBuffer = temporalCommonExtractTree();
	pgResult = PQexec(oldPgDbTenantConnection, sql);
	if (PQresultStatus(pgResult) != PGRES_COMMAND_OK)
  {
    LM_E(("command failed for inserting single Attribute into DB %s: '%s'", oldTenantName, sql));
    LM_E(("Reason %s", PQerrorMessage(oldPgDbTenantConnection)));
    PQclear(pgResult);
    TemporalPgDBConnectorClose();
    return false;
  }
  PQclear(pgResult);

	pgResult = PQexec(oldPgDbTenantConnection, "COMMIT");
  if (PQresultStatus(pgResult) != PGRES_COMMAND_OK)
  {
    LM_E(("COMMIT command failed for inserting single Sub Attribute into DB %s", oldTenantName));
    PQclear(pgResult);
    TemporalPgDBConnectorClose();
    return false;
  }

  PQclear(pgResult);
  TemporalPgDBConnectorClose();

  return true;
}



// -----------------------------------------------------------------------------
//
// temporalDbTableExists -
//
bool temporalDbTableExists(const char* dbName, const char* tableName)
{
  char*     sqlStm;
  PGconn*   conn = NULL;
  sqlStm = kaAlloc(&orionldState.kalloc, 512);
  bzero(sqlStm, 512);
  snprintf(sqlStm, 512, "SELECT entity_id FROM %s", tableName);

  // need to create a routine - Fix me PLEEEEEASE - start
  PGresult *res = PQexec(conn, dbName);

  if( PQresultStatus(res) == PGRES_FATAL_ERROR )
    {
      LM_K(("CCSR - Table does not EXIST - what can we do"));
      PQclear(res);
      return false;
    }

  PQclear(res);
  return true;
  // need to create a routine - Fix me PLEEEEEASE - end
}



// -----------------------------------------------------------------------------
//
// temporalDbExists -
//
bool temporalDbExists(const char* dbName)
{
  char*    sqlStm;
  sqlStm = kaAlloc(&orionldState.kalloc, 512);
  bzero(sqlStm, 512);
  snprintf(sqlStm, 512, "dbname = %s", dbName);

  // need to create a routine - Fix me PLEEEEEASE - start
  PGconn *conn = PQconnectdb(sqlStm);

  if( PQstatus(conn) != CONNECTION_OK )
  {
    LM_K(("CCSR - DATABASE does not EXIST - what can we do"));
    return false;
  }
  return true;
  // need to create a routine - Fix me PLEEEEEASE - end
}
