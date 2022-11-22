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
* Author: Ken Zangelin
*/
#include <string.h>                                             // strcmp, strcspn

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
#include "kjson/kjLookup.h"                                     // kjLookup
#include "kjson/kjBuilder.h"                                    // kjChildAdd, ...
}

#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldError.h"                        // orionldError
#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/context/OrionldContext.h"                     // OrionldContext
#include "orionld/context/orionldAttributeExpand.h"             // orionldAttributeExpand
#include "orionld/forwarding/FwdOperation.h"                    // fwdOperationFromString, FwdNone
#include "orionld/regCache/regCacheItemContextCheck.h"          // regCacheItemContextCheck
#include "orionld/payloadCheck/PCHECK.h"                        // PCHECK_*
#include "orionld/payloadCheck/pcheckInformation.h"             // pcheckInformation
#include "orionld/payloadCheck/pcheckTimeInterval.h"            // pcheckTimeInterval
#include "orionld/payloadCheck/pcheckGeoPropertyValue.h"        // pcheckGeoPropertyValue
#include "orionld/payloadCheck/pcheckRegistration.h"            // Own interface



// -----------------------------------------------------------------------------
//
// pCheckRegistrationMode - FIXME: Own module
//
bool pCheckRegistrationMode(const char* mode)
{
  if ((strcmp(mode, "inclusive") != 0) &&
      (strcmp(mode, "exclusive") != 0) &&
      (strcmp(mode, "redirect")  != 0) &&
      (strcmp(mode, "auxiliary") != 0))
  {
    orionldError(OrionldBadRequestData, "Invalid Registration Mode", mode, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckRegistrationOperations - FIXME: Own module
//
bool pCheckRegistrationOperations(KjNode* operationsP)
{
  for (KjNode* fwdOpP = operationsP->value.firstChildP; fwdOpP != NULL; fwdOpP = fwdOpP->next)
  {
    if (fwdOpP->type != KjString)
    {
      orionldError(OrionldBadRequestData, "Invalid JSON type for Registration::operations array item (not a JSON String)", kjValueType(fwdOpP->type), 400);
      return false;
    }

    if (fwdOpP->value.s[0] == 0)
    {
      orionldError(OrionldBadRequestData, "Empty String in Registration::operations array", "", 400);
      return false;
    }

    if (fwdOperationFromString(fwdOpP->value.s) == FwdNone)
    {
      if (fwdOperationAliasFromString(fwdOpP->value.s) == FwdNone)
      {
        orionldError(OrionldBadRequestData, "Invalid value for Registration::operations array item", fwdOpP->value.s, 400);
        return false;
      }
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjLookupInKvList - FIXME: Own module in kjTree library
//
KjNode* kjLookupInKvList(KjNode* firstSiblingP, const char* name)
{
  for (KjNode* siblingP = firstSiblingP->next; siblingP != NULL; siblingP = siblingP->next)
  {
    KjNode* keyP = kjLookup(siblingP, "key");

    if ((keyP != NULL) && (keyP->type == KjString))
    {
      if (strcmp(keyP->value.s, name) == 0)
        return siblingP;
    }
  }

  return NULL;
}



// -----------------------------------------------------------------------------
//
// pCheckKeyValueArray - FIXME: Own module
//
// Keys with special treatment:
// * jsonldContext - needs to be downloaded, parsed etc, to be accepted.
// * Content-Type  - must be a valid mime type
// * Accept        - must be a valid Accept value (parse it and make sure)
//
bool pCheckKeyValueArray(KjNode* csiP, OrionldContext** fwdContextPP)
{
  for (KjNode* kvObjectP = csiP->value.firstChildP; kvObjectP != NULL; kvObjectP = kvObjectP->next)
  {
    KjNode* keyP   = NULL;
    KjNode* valueP = NULL;

    PCHECK_OBJECT(kvObjectP, 0, NULL, "Registration::contextSourceInfo[X]", 400);
    PCHECK_OBJECT_EMPTY(kvObjectP, 0, NULL, "Registration::contextSourceInfo[X]", 400);

    for (KjNode* kvItemP = kvObjectP->value.firstChildP; kvItemP != NULL; kvItemP = kvItemP->next)
    {
      if (strcmp(kvItemP->name, "key") == 0)
      {
        PCHECK_DUPLICATE(keyP, kvItemP, 0, "Duplicated field in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING(keyP, 0, "Non-String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING_EMPTY(keyP, 0, "Empty String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
      }
      else if (strcmp(kvItemP->name, "value") == 0)
      {
        PCHECK_DUPLICATE(valueP, kvItemP, 0, "Duplicated field in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING(valueP, 0, "Non-String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        PCHECK_STRING_EMPTY(valueP, 0, "Empty String item in Registration::contextSourceInfo[X]", kvItemP->name, 400);
      }
      else
      {
        orionldError(OrionldBadRequestData, "Unrecognized field in Registration::contextSourceInfo[X]", kvItemP->name, 400);
        return false;
      }
    }

    if (keyP == NULL)
    {
      orionldError(OrionldBadRequestData, "Missing Mandatory Field in Registration::contextSourceInfo[X]", "key", 400);
      return false;
    }

    if (valueP == NULL)
    {
      orionldError(OrionldBadRequestData, "Missing Mandatory Field in Registration::contextSourceInfo[X]", "value", 400);
      return false;
    }

    //
    // Check for duplicates
    //
    if ((kvObjectP->next != NULL) && (kjLookupInKvList(kvObjectP, keyP->value.s) != NULL))
    {
      orionldError(OrionldBadRequestData, "Duplicated Key in Registration::contextSourceInfo", keyP->value.s, 400);
      return false;
    }

    // Special keys
    if (strcmp(keyP->value.s, "jsonldContext") == 0)
    {
      // If an @context is given for the registration, make sure it's valid
      if (regCacheItemContextCheck(NULL, valueP->value.s, fwdContextPP) == false)
      {
        LM_W(("Unable to add Registration @context '%s' for an item in the reg-cache", valueP->value.s));
        return 0;
      }
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckRegistrationManagement - FIXME: Own module
//
bool pCheckRegistrationManagement(KjNode* rmP)
{
  KjNode* localOnlyP     = NULL;
  KjNode* timeoutP       = NULL;
  KjNode* cooldownP      = NULL;

  for (KjNode* rmItemP = rmP->value.firstChildP; rmItemP != NULL; rmItemP = rmItemP->next)
  {
    if (strcmp(rmItemP->name, "localOnly") == 0)
    {
      PCHECK_DUPLICATE(localOnlyP, rmItemP, 0, NULL, "Registration::management::localOnly", 400);
      PCHECK_BOOL(localOnlyP, 0, NULL, "Registration::management::localOnly", 400);
    }
    else if (strcmp(rmItemP->name, "timeout") == 0)
    {
      PCHECK_DUPLICATE(timeoutP, rmItemP, 0, NULL, "Registration::management::timeout", 400);
      PCHECK_NUMBER(timeoutP, 0, NULL, "Registration::management::timeout", 400);

      if (timeoutP->type == KjFloat)
      {
        if (timeoutP->value.f <= 0.001)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::timeout", "Must be Greater Than 0", 400);
          return false;
        }
      }
      else if (timeoutP->type == KjInt)
      {
        if (timeoutP->value.i <= 0)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::timeout", "Must be Greater Than 0", 400);
          return false;
        }
      }
    }
    else if (strcmp(rmItemP->name, "cooldown") == 0)
    {
      PCHECK_DUPLICATE(cooldownP, rmItemP, 0, NULL, "Registration::management::cooldown", 400);
      PCHECK_NUMBER(cooldownP, 0, NULL, "Registration::management::cooldown", 400);

      if (cooldownP->type == KjFloat)
      {
        if (cooldownP->value.f <= 0.001)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::cooldown", "Must be Greater Than 0", 400);
          return false;
        }
      }
      else if (cooldownP->type == KjInt)
      {
        if (cooldownP->value.i <= 0)
        {
          orionldError(OrionldBadRequestData, "Non-supported value for Registration::management::cooldown", "Must be Greater Than 0", 400);
          return false;
        }
      }
    }
    else if (strcmp(rmItemP->name, "cacheDuration") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", "Registration::management::cacheDuration", 501);
      return false;
    }
    else
    {
      orionldError(OrionldBadRequestData, "Unrecognized field in Registration::management", rmItemP->name, 400);
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckTenant - FIXME: Own module
//
bool pCheckTenant(const char* tenantName)
{
  if (strlen(tenantName) > 16)
  {
    orionldError(OrionldBadRequestData, "Invalid tenant name (too long)", tenantName, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pCheckScope - FIXME: Own module
//
bool pCheckScope(const char* scope)
{
  size_t len = strlen(scope);

  if (len > 80)
  {
    orionldError(OrionldBadRequestData, "Invalid scope (too long)", scope, 400);
    return false;
  }

  if (strcspn(scope, " :.\\\"'<>|") != len)
  {
    orionldError(OrionldBadRequestData, "Invalid scope (contains forbidden characters)", scope, 400);
    return false;
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// pcheckRegistration -
//
// This function makes sure that the incoming payload is OK
// - all types are as they should
// - the values with special needs are correct (if DataTikme for example)
// - no field is duplicated
// - etc
//
// However, there is ONE thing that cannot be checked here, as we still haven't queries the database
// for the original registration - the one to be patched.
// The problem is with registration properties.
// A registration can have any property added to it, and with any name.
// In this function we can check that the property isn't duplicated but, as this is a PATCH, the
// property to be patched MUST ALREADY EXIST in the original registration, and this cannot be checked until
// AFTER we extract the the original registration from the database.
//
// SO, what we do here is to separate the regiustration properties from the rest of the fields and keep thjat
// as a KjNode tree, until we are able to perform this check.
// The output parameter 'propertyTreeP' is used for this purpose.
//
// FIXME:
// - output param for the bitmask of "operations"
// - output param for "mode"
// - ...
//
bool pcheckRegistration(KjNode* registrationP, bool idCanBePresent, bool creation, KjNode**  propertyTreeP, OrionldContext** contextPP)
{
  KjNode*  idP                   = NULL;  // Optional but already extracted by orionldMhdConnectionInit
  KjNode*  typeP                 = NULL;  // Mandatory but already extracted by orionldMhdConnectionInit
  KjNode*  nameP                 = NULL;
  KjNode*  descriptionP          = NULL;
  KjNode*  informationP          = NULL;
  KjNode*  observationIntervalP  = NULL;
  KjNode*  managementIntervalP   = NULL;
  KjNode*  locationP             = NULL;
  KjNode*  observationSpaceP     = NULL;
  KjNode*  operationSpaceP       = NULL;
  KjNode*  expiresP              = NULL;
  KjNode*  endpointP             = NULL;
  KjNode*  tenantP               = NULL;
  KjNode*  contextSourceInfoP    = NULL;
  KjNode*  scopeP                = NULL;
  KjNode*  modeP                 = NULL;
  KjNode*  operationsP           = NULL;
  KjNode*  managementP           = NULL;
  KjNode*  propertyTree          = kjObject(orionldState.kjsonP, "properties");  // Temp storage for properties
  int64_t  dateTime;

  if (registrationP->type != KjObject)
  {
    orionldError(OrionldBadRequestData, "Invalid Registration", "The payload data for updating a registration must be a JSON Object", 400);
    return false;
  }

  //
  // Loop over the entire registration (incoming payload data) and check all is OK
  //
  // To be able to easy and fast check that no registration properties are duplicated, the registration properties are
  // removed from the tree and added to 'propertyTree'.
  // Later, these registration properties must be put back!
  //
  // It's quicker to implement this like that, as there's no need to go over the entire path tree and also not necessary to look for >1 matches.
  // When Property P1 is about to be added to 'propertyTree' for a second time, the duplication will be noticed and the error triggered.
  //
  KjNode* nodeP = registrationP->value.firstChildP;
  KjNode* next;

  while (nodeP != NULL)
  {
    next = nodeP->next;

    if ((strcmp(nodeP->name, "id") == 0) || (strcmp(nodeP->name, "@id") == 0))
    {
      if (idCanBePresent == false)
      {
        orionldError(OrionldBadRequestData, "Invalid field for Registration Update", nodeP->name, 400);
        return false;
      }

      DUPLICATE_CHECK(idP, "id", nodeP);
      STRING_CHECK(nodeP, "id");
      URI_CHECK(nodeP->value.s, "id", true);
    }
    else if (strcmp(nodeP->name, "type") == 0 || strcmp(nodeP->name, "@type") == 0)
    {
      DUPLICATE_CHECK(typeP, "type", nodeP);
      STRING_CHECK(nodeP, "type");

      if (strcmp(nodeP->value.s, "ContextSourceRegistration") != 0)
      {
        orionldError(OrionldBadRequestData, "Invalid value for type", nodeP->value.s, 400);
        return false;
      }
    }
    else if ((strcmp(nodeP->name, "registrationName") == 0) || (strcmp(nodeP->name, "name") == 0))
    {
      nodeP->name = (char*) "registrationName";
      DUPLICATE_CHECK(nameP, "registrationName", nodeP);
      STRING_CHECK(nodeP, "registrationName");
      EMPTY_STRING_CHECK(nodeP, "registrationName");
    }
    else if (strcmp(nodeP->name, "description") == 0)
    {
      DUPLICATE_CHECK(descriptionP, "description", nodeP);
      STRING_CHECK(nodeP, "description");
      EMPTY_STRING_CHECK(nodeP, "description");
    }
    else if (strcmp(nodeP->name, "information") == 0)
    {
      DUPLICATE_CHECK(informationP, "information", nodeP);
      ARRAY_CHECK(nodeP, "information");
      EMPTY_ARRAY_CHECK(nodeP, "information");

      if (pcheckInformation(nodeP) == false)
        return false;
    }
    else if (strcmp(nodeP->name, "tenant") == 0)
    {
      DUPLICATE_CHECK(tenantP, "tenant", nodeP);
      STRING_CHECK(nodeP, "tenant");
      EMPTY_STRING_CHECK(nodeP, "tenant");

      if (pCheckTenant(nodeP->value.s) == false)
      {
        // pCheckTenant calls orionldError
        return false;
      }
    }
    else if (strcmp(nodeP->name, "observationInterval") == 0)
    {
      DUPLICATE_CHECK(observationIntervalP, "observationInterval", nodeP);
      OBJECT_CHECK(nodeP, "observationInterval");
      EMPTY_OBJECT_CHECK(nodeP, "observationInterval");
      if (pcheckTimeInterval(nodeP, "observationInterval") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "managementInterval") == 0)
    {
      DUPLICATE_CHECK(managementIntervalP, nodeP->name, nodeP);
      OBJECT_CHECK(nodeP, nodeP->name);
      EMPTY_OBJECT_CHECK(nodeP, nodeP->name);
      if (pcheckTimeInterval(nodeP, nodeP->name) == false)
        return false;
    }
    else if (strcmp(nodeP->name, "location") == 0)
    {
      DUPLICATE_CHECK(locationP, nodeP->name, nodeP);
      OBJECT_CHECK(nodeP, nodeP->name);
      EMPTY_OBJECT_CHECK(nodeP, nodeP->name);

      if (pcheckGeoPropertyValue(locationP, NULL, NULL, nodeP->name) == false)
      {
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "observationSpace") == 0)
    {
      DUPLICATE_CHECK(observationSpaceP, nodeP->name, nodeP);
      OBJECT_CHECK(nodeP, nodeP->name);
      EMPTY_OBJECT_CHECK(nodeP, nodeP->name);
      if (pcheckGeoPropertyValue(observationSpaceP, NULL, NULL, nodeP->name) == false)
      {
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "operationSpace") == 0)
    {
      DUPLICATE_CHECK(operationSpaceP, nodeP->name, nodeP);
      OBJECT_CHECK(nodeP, nodeP->name);
      EMPTY_OBJECT_CHECK(nodeP, nodeP->name);
      if (pcheckGeoPropertyValue(operationSpaceP, NULL, NULL, nodeP->name) == false)
        return false;
    }
    else if ((strcmp(nodeP->name, "expiresAt") == 0) || (strcmp(nodeP->name, "expires") == 0))
    {
      DUPLICATE_CHECK(expiresP, nodeP->name, nodeP);
      STRING_CHECK(nodeP, nodeP->name);
      EMPTY_STRING_CHECK(nodeP, nodeP->name);
      DATETIME_CHECK(expiresP->value.s, dateTime, nodeP->name);
      nodeP->name = (char*) "expiresAt";
    }
    else if (strcmp(nodeP->name, "endpoint") == 0)
    {
      DUPLICATE_CHECK(endpointP, "endpoint", nodeP);
      STRING_CHECK(nodeP, "endpoint");
      URI_CHECK(nodeP->value.s, nodeP->name, true);
    }
    else if (strcmp(nodeP->name, "contextSourceInfo") == 0)
    {
      DUPLICATE_CHECK(contextSourceInfoP, "contextSourceInfo", nodeP);
      ARRAY_CHECK(contextSourceInfoP, "contextSourceInfo");
      EMPTY_ARRAY_CHECK(contextSourceInfoP, "contextSourceInfo");
      if (pCheckKeyValueArray(contextSourceInfoP, contextPP) == false)
        return false;
    }
    else if (strcmp(nodeP->name, "scope") == 0)
    {
      DUPLICATE_CHECK(scopeP, "scope", nodeP);
      PCHECK_STRING_OR_ARRAY(scopeP, 0, NULL, "Invalid Scope", 400);

      if (scopeP->type == KjString)
      {
        PCHECK_STRING_EMPTY(scopeP, 0, NULL, "Invalid Scope", 400);
        if (pCheckScope(scopeP->value.s) == false)
          return false;
      }
      else
      {
        PCHECK_ARRAY_EMPTY(scopeP, 0, NULL, "Invalid Scope", 400);
        for (KjNode* scopeItemP = scopeP->value.firstChildP; scopeItemP != NULL; scopeItemP = scopeItemP->next)
        {
          PCHECK_STRING(scopeItemP, 0, NULL, "Invalid Scope", 400);
          PCHECK_STRING_EMPTY(scopeItemP, 0, NULL, "Invalid Scope", 400);
          if (pCheckScope(scopeItemP->value.s) == false)
            return false;
        }
      }
    }
    else if (strcmp(nodeP->name, "status") == 0)
    {
      orionldError(OrionldBadRequestData, "Read-only field", "Registration::status", 400);
      return false;
    }
    else if (strcmp(nodeP->name, "timesSent") == 0)
    {
      orionldError(OrionldBadRequestData, "Read-only field", "Registration::timesSent", 400);
      return false;
    }
    else if (strcmp(nodeP->name, "timesFailed") == 0)
    {
      orionldError(OrionldBadRequestData, "Read-only field", "Registration::timesFailed", 400);
      return false;
    }
    else if (strcmp(nodeP->name, "lastSuccess") == 0)
    {
      orionldError(OrionldBadRequestData, "Read-only field", "Registration::lastSuccess", 400);
      return false;
    }
    else if (strcmp(nodeP->name, "lastFailure") == 0)
    {
      orionldError(OrionldBadRequestData, "Read-only field", "Registration::lastFailure", 400);
      return false;
    }
    else if (strcmp(nodeP->name, "mode") == 0)
    {
      DUPLICATE_CHECK(modeP, "mode", nodeP);
      PCHECK_STRING(modeP, 0, NULL, "mode", 400);
      PCHECK_STRING_EMPTY(modeP, 0, NULL, "mode", 400);
      if (pCheckRegistrationMode(modeP->value.s) == false)
        return false;
    }
    else if (strcmp(nodeP->name, "operations") == 0)
    {
      DUPLICATE_CHECK(operationsP, "operations", nodeP);
      PCHECK_ARRAY(operationsP, 0, NULL, "operations", 400);
      PCHECK_ARRAY_EMPTY(operationsP, 0, NULL, "operations", 400);
      if (pCheckRegistrationOperations(operationsP) == false)
        return false;
    }
    else if (strcmp(nodeP->name, "refreshRate") == 0)
    {
      orionldError(OrionldOperationNotSupported, "Not Implemented", "Registration::refreshRate", 501);
      return false;
    }
    else if (strcmp(nodeP->name, "management") == 0)
    {
      DUPLICATE_CHECK(managementP, "management", nodeP);
      PCHECK_OBJECT(managementP, 0, NULL, "management", 400);
      PCHECK_OBJECT_EMPTY(managementP, 0, NULL, "management", 400);
      if (pCheckRegistrationManagement(managementP) == false)
        return false;
    }
    else  // It's a Property
    {
      kjChildRemove(registrationP, nodeP);

      //
      // Add the Property to 'propertyTree'
      //
      // But, before adding, look it up.
      // If already there, then we have a case of duplicate property and that is an error
      //
      nodeP->name = orionldAttributeExpand(orionldState.contextP, nodeP->name, true, NULL);
      if (kjLookup(propertyTree, nodeP->name) != NULL)
      {
        orionldError(OrionldBadRequestData, "Duplicate Property in Registration", nodeP->name, 400);
        return false;
      }

      kjChildAdd(propertyTree, nodeP);
    }

    nodeP = next;
  }

  if (creation == true)
  {
    if (informationP == NULL)
    {
      orionldError(OrionldBadRequestData, "Missing mandatory field for Registration", "information", 400);
      return false;
    }

    if (endpointP == NULL)
    {
      orionldError(OrionldBadRequestData, "Missing mandatory field for Registration", "endpoint", 400);
      return false;
    }
  }

  *propertyTreeP = propertyTree;  // These properties will be checked later, after getting the reg from DB

  return true;
}
