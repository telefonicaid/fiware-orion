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
* Author: Chandra Challagonda
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

#include "orionld/temporal/temporalOpenDBConnection.h" 		// For opening DB connection
#include "orionld/temporal/temporalOpenTenantDBConnection.h" 	// For opneing Tenant DB Connection   
#include "orionld/temporal/temporalCommonState.h"                // common Include
#include "orionld/temporal/temporalExecSqlStatement.h"	         // Own Interface

// -----------------------------------------------------------------------------
//
// temporalExecSqlStatement 
//
//
bool temporalExecSqlStatement(char* oldTemporalSQLBuffer)
{
	char oldTenantName[] = "tbd-chandra";

  	oldPgDbTenantConnection = TemporalTenantDBConnectorOpen(oldTenantName);

  	PGresult *oldTenantDBEntityRes = PQexec(oldPgDbTenantConnection, "BEGIN");
  	if (PQresultStatus(oldTenantDBEntityRes) != PGRES_COMMAND_OK)
  	{
        	LM_E(("BEGIN command failed for inserting single Entity into DB %s\n",oldTenantName));
        	PQclear(oldTenantDBEntityRes);
        	TemporalTenantDBConnectorClose(oldPgDbTenantConnection);
        	return false;
  	}

  	PQclear(oldTenantDBEntityRes);

	return false;

}
