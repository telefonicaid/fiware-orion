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
#include "orionld/temporal/temporalOpenDBConnection.h"         // Own interface



// ----------------------------------------------------------------------------
//
// temporalOpenDBConnection -
//
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//
// TemporalDBConnectorClose(PGconn* conn) - function to close the Postgres database connection gracefully
//
// ----------------------------------------------------------------------------
void TemporalDBConnectorClose(PGconn* conn)
{
    PQfinish(conn); //Closes connection and and also frees memory used by the PGconn* conn variable
}

// ----------------------------------------------------------------------------
//
// PGconn* TemporalDBConnectorOpen(char *tenantName) - function to open the Postgres database connection
//
// ----------------------------------------------------------------------------
PGconn* TemporalDBConnectorOpen()
{
    char oldPgDbConnCheckSql[] = "user=postgres password=orion dbname=postgres"; //Need to be changed to environment variables CHANDRA-TBD
    PGconn *oldPgConn = PQconnectdb(oldPgDbConnCheckSql);
    if (PQstatus(oldPgConn) == CONNECTION_BAD)
    {
	LM_E(("Connection to Postgress database is not achieved"));
        LM_E(("CONNECTION_BAD %s\n", PQerrorMessage(oldPgConn)));
	TemporalDBConnectorClose(oldPgConn); //close connection and cleanup
        return NULL;
    }
    else if (PQstatus(oldPgConn) == CONNECTION_OK)
    {
        //puts("CONNECTION_OK");
        LM_K(("Connection is ok with the Postgres database\n"));
        return oldPgConn; //Return the connection handler
    }
  
    LM_E(("Not Implemented"));
  
    orionldState.httpStatusCode  = SccNotImplemented;
    orionldState.noLinkHeader    = true;  // We don't want the Link header for non-implemented requests
    orionldErrorResponseCreate(OrionldBadRequestData, "Not Implemented", orionldState.serviceP->url);

    return NULL;
}
