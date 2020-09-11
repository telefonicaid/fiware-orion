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
#include "orionld/temporal/temporalPostEntity.h"               // Own interface
#include "orionld/temporal/temporalOpenDBConnection.h"	       // Temporal Include
#include "orionld/temporal/temporalOpenTenantDBConnection.h"   // Temporal Include


// ----------------------------------------------------------------------------
//
// temporalPostEntity -
//
bool temporalPostEntity(ConnectionInfo* ciP)
{
  char oldTenantName[] = "tbd-chandra";
  
  PGconn* oldTenantPgConn = TemporalTenantDBConnectorOpen(oldTenantName);
  
  PGresult *oldTenantDBEntityRes = PQexec(oldTenantPgConn, "BEGIN"); 
  if (PQresultStatus(oldTenantDBEntityRes) != PGRES_COMMAND_OK) 
  {

        LM_E(("BEGIN command failed for inserting single Entity into DB %s\n",oldTenantName));
        PQclear(oldTenantDBEntityRes);
        TemporalTenantDBConnectorClose(oldTenantPgConn);
	return false;
  }
  PQclear(oldTenantDBEntityRes);  

  char oldPGTenantPostEntityCloseBraces[] = ")";

  char oldPGTenantPostEntity_entity_id[] = "tbd-chandra";
  char oldPGTenantPostEntity_entity_type[] = "tbd-chandra";
  char oldPGTenantPostEntity_geo_property[] = "tbd-chandra";
  char oldPGTenantPostEntity_created_at[] = "tbd-chandra";
  char oldPGTenantPostEntity_modified_at[] = "tbd-chandra";
  char oldPGTenantPostEntity_observed_at[] = "tbd-chandra";

  char oldPGTenantPostEntity[] = "INSERT INTO entity_table(entity_id,entity_type,geo_property,created_at,modified_at, observed_at) VALUES (";

  strcat (oldPGTenantPostEntity, oldPGTenantPostEntity_entity_id);
  strcat (oldPGTenantPostEntity, oldPGTenantPostEntity_entity_type);
  strcat (oldPGTenantPostEntity, oldPGTenantPostEntity_geo_property);
  strcat (oldPGTenantPostEntity, oldPGTenantPostEntity_created_at);
  strcat (oldPGTenantPostEntity, oldPGTenantPostEntity_modified_at);
  strcat (oldPGTenantPostEntity, oldPGTenantPostEntity_observed_at); 
  strcat (oldPGTenantPostEntity, oldPGTenantPostEntityCloseBraces);

  oldTenantDBEntityRes = PQexec(oldTenantPgConn, oldPGTenantPostEntity);    

  if (PQresultStatus(oldTenantDBEntityRes) != PGRES_COMMAND_OK)
  {

        LM_E(("INSERT command failed for inserting single Entity into DB %s\n",oldTenantName));
        PQclear(oldTenantDBEntityRes);
        TemporalTenantDBConnectorClose(oldTenantPgConn);
	return false;
  }
  PQclear(oldTenantDBEntityRes);

  oldTenantDBEntityRes = PQexec(oldTenantPgConn, "COMMIT");
  if (PQresultStatus(oldTenantDBEntityRes) != PGRES_COMMAND_OK)
  {

        LM_E(("COMMIT command failed for inserting single Entity into DB %s\n",oldTenantName));
        PQclear(oldTenantDBEntityRes);
        TemporalTenantDBConnectorClose(oldTenantPgConn);
	return false;
  }
  PQclear(oldTenantDBEntityRes);
  TemporalTenantDBConnectorClose(oldTenantPgConn);
  return true;


  LM_E(("Not Implemented"));
  orionldState.httpStatusCode  = SccNotImplemented;
  orionldState.noLinkHeader    = true;  // We don't want the Link header for non-implemented requests
  orionldErrorResponseCreate(OrionldBadRequestData, "Not Implemented", orionldState.serviceP->url);

  return false;
}
