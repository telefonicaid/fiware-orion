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
#include "orionld/temporal/temporalOpenDBConnection.h"         // for opening and closing DB connections
#include "orionld/temporal/temporalInitialiseTenant.h"         // Own interface



// ----------------------------------------------------------------------------
//
// temporalInitialiseTenant -
//
PGconn* temporalInitialiseTenant(char *tenantName)
{
	LM_K(("Trying to open connection to Postgres database for new tenat database creation %s\n", tenantName));

	PGconn* oldPgConnPDb = TemporalDBConnectorOpen();
	if(oldPgConnPDb != NULL)
	{
		LM_K(("Trying to create database for Tenant %s\n", tenantName));
		char oldPgDbSqlSyntax[]= ";";
		char oldPgDbSqlCreateTDbSQL[] = "CREATE DATABASE ";
		strcat (oldPgDbSqlCreateTDbSQL, tenantName);
		strcat (oldPgDbSqlCreateTDbSQL, oldPgDbSqlSyntax);
		char oldPgTDbConnSQL[] = "user=postgres password=orion dbname= ";
		strcat (oldPgTDbConnSQL, tenantName);
		LM_K(("Command to create database for Tenant %s\n", tenantName));
		PGresult* oldPgTDbRes = PQexec(oldPgConnPDb, oldPgDbSqlCreateTDbSQL);
		LM_K(("Opening database connection for Tenant %s\n", tenantName));
		PGconn* oldPgConnTDb = PQconnectdb(oldPgTDbConnSQL);
		if (PQresultStatus(oldPgTDbRes) != PGRES_COMMAND_OK && PQstatus(oldPgConnTDb) != CONNECTION_OK)
    		{
       		 	LM_E(("Connection to %s database is not achieved or created", tenantName));
        		LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgConnTDb)));
        		TemporalDBConnectorClose(oldPgConnTDb); //close Tenant DB connection and cleanup
			TemporalDBConnectorClose(oldPgConnPDb); //close Postgres DB connection and cleanup
        		return NULL;
    		}
    		else if (PQstatus(oldPgConnTDb) == CONNECTION_OK)
    		{
        		LM_K(("Connection is ok with the %s database\n", tenantName));
			LM_K(("Now crreating the tables for the teanant %s \n", tenantName));
			const char *oldPgDbCreateTenantTables[2][150] = {"CREATE EXTENSION IF NOT EXISTS postgis","CREATE TABLE IF NOT EXISTS entity_table (entity_id TEXT NOT NULL,entity_type TEXT,createdAt TIMESTAMP,modifiedAt TIMESTAMP,PRIMARY KEY (entity_id))"};

			for(int oldPgDbNumObj = 0; oldPgDbNumObj < 2; oldPgDbNumObj++)
        		{
           			PGresult* oldPgRes = PQexec(oldPgConnTDb, *oldPgDbCreateTenantTables[oldPgDbNumObj]);

            			if (PQresultStatus(oldPgRes) != PGRES_COMMAND_OK)
            			{
					LM_K(("Postgres DB command failed for database for Tenant %s%s\n", tenantName,oldPgDbCreateTenantTables[oldPgDbNumObj]));
                			break;
           			}
           		 	PQclear(oldPgRes);
        		}
				
        		//return oldPgConnTDb; //Return the connection handler
    		}
	}
	else  
	{
		TemporalDBConnectorClose(oldPgConnPDb); //close Postgres DB connection and cleanup
		return NULL;
	}

  	LM_E(("Not Implemented"));
  	orionldState.httpStatusCode  = SccNotImplemented;
  	orionldState.noLinkHeader    = true;  // We don't want the Link header for non-implemented requests
  	orionldErrorResponseCreate(OrionldBadRequestData, "Not Implemented", orionldState.serviceP->url);

  	return NULL;
}

