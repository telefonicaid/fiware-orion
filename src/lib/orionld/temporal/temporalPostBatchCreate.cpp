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
* Author: Ken Zangelin, Chandra Challagonda
*/
#include "logMsg/logMsg.h"                                     // LM_*
#include "logMsg/traceLevels.h"                                // Lmt*

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "rest/HttpStatusCode.h"                               // SccNotImplemented
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/rest/OrionLdRestService.h"                   // OrionLdRestService
#include "orionld/temporal/temporalPostBatchCreate.h"          // Own interface



// ----------------------------------------------------------------------------
//
// temporalPostBatchCreate -
//
bool temporalPostBatchCreate(ConnectionInfo* ciP)
{
  char tenantName[] = "orion_ld"; // Chandra-TBD This needs to be changed
	if (oldPgDbConnection == NULL)
	{
		//if(!temporalTenanatValidate())
		//{
		//	LM_TMP(("CCSR: Tenant initialisation failed"));
		//}

		if(TemporalPgDBConnectorOpen() == true)
		{
			LM_TMP(("CCSR: connection to postgress db is open"));
			if(TemporalPgTenantDBConnectorOpen(tenantName) == true)
			{
				LM_TMP(("CCSR: connection to tenant db is open"));
			}
			else
			{
				LM_TMP(("CCSR: connection to tenant db is not successful with error '%s'", PQerrorMessage(oldPgDbConnection)));
				return false;
			}
		}
		else
		{
			LM_TMP(("CCSR: connection to postgres db is not successful with error '%s'", PQerrorMessage(oldPgDbConnection)));
			return false;
		}
	}

	//char* oldTemporalSQLFullBuffer = temporalCommonExtractTree();
	OrionldTemporalDbAllTables* dbAllTables = temporalEntityExtract();


	if(TemporalConstructInsertSQLStatement(dbAllTables, false) == true)
	{
		LM_TMP(("CCSR: temporalPostEntities -- Post Entities success to database:"));
		return true;
	}
	else
	{
		return false;
	}
	return false;

}
