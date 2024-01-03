/*
*
* Copyright 2022 FIWARE Foundation e.V.
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
* Author: Ken Zangelin
*/
#include "logMsg/logMsg.h"                                     // LM_*

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjLookup.h"                                    // kjLookup
#include "kjson/kjBuilder.h"                                   // kjString, kjFloat, ...
}

#include "orionld/common/orionldState.h"                       // orionldState, coreContextUrl
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/http/httpHeaderLocationAdd.h"                // httpHeaderLocationAdd
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_OBJECT
#include "orionld/payloadCheck/pcheckRegistration.h"           // pcheckRegistration
#include "orionld/payloadCheck/pCheckUri.h"                    // pcheckUri
#include "orionld/legacyDriver/legacyPostRegistrations.h"      // legacyPostRegistrations
#include "orionld/mongoc/mongocRegistrationExists.h"           // mongocRegistrationExists
#include "orionld/mongoc/mongocRegistrationInsert.h"           // mongocRegistrationInsert
#include "orionld/regCache/regCacheItemAdd.h"                  // regCacheItemAdd
#include "orionld/dbModel/dbModelFromApiRegistration.h"        // dbModelFromApiRegistration
#include "orionld/serviceRoutines/orionldPostRegistrations.h"  // Own interface



// -----------------------------------------------------------------------------
//
// pCheckRegistrationType -
//
bool pCheckRegistrationType(KjNode* regTypeP)
{
  if (regTypeP == NULL)
  {
    orionldError(OrionldBadRequestData, "Invalid Registration", "Mandatory field 'type' is missing", 400);
    return false;
  }
  else if (regTypeP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid Registration", "The 'type' field must be a JSON string", 400);
    return false;
  }
  else if (regTypeP->value.s[0] == 0)
  {
    orionldError(OrionldBadRequestData, "Invalid Registration", "The 'type' field cannot be an empty string", 400);
    return false;
  }
  else if (strcmp(regTypeP->value.s, "ContextSourceRegistration") != 0)
  {
    orionldError(OrionldBadRequestData, "Invalid Registration", "The 'type' field must have the value 'ContextSourceRegistration'", 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckRegistrationId -
//
bool pCheckRegistrationId(KjNode* regIdP)
{
  if (regIdP->type != KjString)
  {
    orionldError(OrionldBadRequestData, "Invalid Registration", "The 'id' field must be a JSON string", 400);
    return false;
  }
  else if (regIdP->value.s[0] == 0)
  {
    orionldError(OrionldBadRequestData, "Invalid Registration", "The 'id' field cannot be an empty string", 400);
    return false;
  }
  else if (pCheckUri(regIdP->value.s, "Registration::id", true) == false)
    return false;

  return true;
}



extern void dbModelFromApiTimeInterval(KjNode* intervalP);
// -----------------------------------------------------------------------------
//
// apiModelToCacheRegistration -
//
void apiModelToCacheRegistration(KjNode* apiRegistrationP)
{
  KjNode* observationIntervalP  = kjLookup(apiRegistrationP, "observationInterval");
  KjNode* managementIntervalP   = kjLookup(apiRegistrationP, "managementInterval");

  if (observationIntervalP != NULL)
    dbModelFromApiTimeInterval(observationIntervalP);
  if (managementIntervalP != NULL)
    dbModelFromApiTimeInterval(managementIntervalP);

  // createdAt+modifiedAt to KjFloat
}



// ----------------------------------------------------------------------------
//
// orionldPostRegistrations -
//
bool orionldPostRegistrations(void)
{
  if ((experimental == false) || (orionldState.in.legacy != NULL))  // If Legacy header - use old implementation
    return legacyPostRegistrations();

  PCHECK_OBJECT(orionldState.requestTree, 0, NULL, "the payload data for a Registration must be a JSON Object", 400);

  //
  // If createdAt and/or modifiedAt is/are present is the incoming payload body, they need to be ignored
  //
  KjNode* createdAtP  = kjLookup(orionldState.requestTree, "createdAt");
  KjNode* modifiedAtP = kjLookup(orionldState.requestTree, "modifiedAt");

  if (createdAtP != NULL)
    kjChildRemove(orionldState.requestTree, createdAtP);
  if (modifiedAtP != NULL)
    kjChildRemove(orionldState.requestTree, modifiedAtP);

  KjNode*  regP            = orionldState.requestTree;
  KjNode*  regIdP          = orionldState.payloadIdNode;
  KjNode*  regTypeP        = orionldState.payloadTypeNode;
  KjNode*  propertyTree    = NULL;  // Will be set by pcheckRegistration

  if (pCheckRegistrationType(regTypeP) == false)
    LM_RE(false, ("pCheckRegistrationType failed"));

  char  registrationIdV[100];
  char* registrationId = registrationIdV;

  if (regIdP == NULL)
  {
    // Invent a registration id
    uuidGenerate(registrationIdV, sizeof(registrationIdV), "urn:ngsi-ld:ContextSourceRegistration:");
    regIdP = kjString(orionldState.kjsonP, "id", registrationId);
  }
  else
  {
    if (pCheckRegistrationId(regIdP) == false)
      LM_RE(false, ("pCheckRegistrationId failed"));

    registrationId = regIdP->value.s;
  }

  OrionldContext* fwdContextP = NULL;
  bool            b           = pcheckRegistration(NULL, regP, NULL, false, true, &propertyTree, &fwdContextP);

  if (b == false)
    LM_RE(false, ("pCheckRegistration FAILED"));

  // Make sure it doesn't exist already
  bool found;

  if (mongocRegistrationExists(registrationId, &found) == false)
  {
    LM_E(("mongocRegistrationExists FAILED"));
    return false;
  }

  if (found == true)
  {
    orionldError(OrionldAlreadyExists, "Registration already exists", registrationId, 409);
    return false;
  }

  //
  // Add the registrationId as the first item to the payload -for the DB (the type we don't store in the database)
  // Add also createdAt and modifiedAt
  //
  if (createdAtP == NULL)
    createdAtP = kjFloat(orionldState.kjsonP, "createdAt",  orionldState.requestTime);
  if (modifiedAtP == NULL)
    modifiedAtP = kjFloat(orionldState.kjsonP, "modifiedAt", orionldState.requestTime);

  if ((createdAtP == NULL) || (modifiedAtP == NULL))
  {
    orionldError(OrionldInternalError, "Out of Memory", "allocating system timestamps", 500);
    return false;
  }

  // Put 'id', 'createdAt', and 'modifiedAt' first in the registration
  regIdP->next      = createdAtP;
  createdAtP->next  = modifiedAtP;
  modifiedAtP->next = orionldState.requestTree->value.firstChildP;

  orionldState.requestTree->value.firstChildP = regIdP;

  // Add the property tree at the end
  kjChildAdd(orionldState.requestTree, propertyTree);

  apiModelToCacheRegistration(orionldState.requestTree);

  regCacheItemAdd(orionldState.tenantP->regCache, registrationId, orionldState.requestTree, false, fwdContextP);  // Clones the registration

  // This is where "id" is changed to "_id"
  dbModelFromApiRegistration(orionldState.requestTree, NULL);

  if (mongocRegistrationInsert(orionldState.requestTree, regIdP->value.s) == false)
    return false;

  orionldState.httpStatusCode = 201;
  httpHeaderLocationAdd("/ngsi-ld/v1/csourceRegistrations/", regIdP->value.s, orionldState.tenantP->tenant);

  return true;
}
