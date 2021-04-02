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
#include <string.h>                                             // strcmp

extern "C"
{
#include "kjson/KjNode.h"                                       // KjNode
#include "kjson/kjLookup.h"                                     // kjLookup
#include "kjson/kjBuilder.h"                                    // kjChildAdd, ...
}

#include "orionld/common/CHECK.h"                               // STRING_CHECK, ...
#include "orionld/common/orionldState.h"                        // orionldState
#include "orionld/common/orionldErrorResponse.h"                // orionldErrorResponseCreate
#include "orionld/payloadCheck/pcheckInformation.h"             // pcheckInformation
#include "orionld/payloadCheck/pcheckTimeInterval.h"            // pcheckTimeInterval
#include "orionld/payloadCheck/pcheckGeoPropertyValue.h"        // pcheckGeoPropertyValue
#include "orionld/payloadCheck/pcheckRegistration.h"            // Own interface



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
bool pcheckRegistration(KjNode* registrationP, bool idCanBePresent, KjNode**  propertyTreeP)
{
  KjNode*  idP                   = NULL;
  KjNode*  typeP                 = NULL;
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
  KjNode*  propertyTree          = kjObject(orionldState.kjsonP, NULL);  // Temp storage for properties
  int64_t  dateTime;

  if (registrationP->type != KjObject)
  {
    orionldErrorResponseCreate(OrionldBadRequestData, "Invalid Registration", "The payload data for updating a registration must be a JSON Object");
    orionldState.httpStatusCode = SccBadRequest;
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
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration Update", "id");
        orionldState.httpStatusCode = SccBadRequest;
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
        orionldErrorResponseCreate(OrionldBadRequestData, "Invalid value for type", nodeP->value.s);
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "name") == 0)
    {
      DUPLICATE_CHECK(nameP, "name", nodeP);
      STRING_CHECK(nodeP, "name");
      EMPTY_STRING_CHECK(nodeP, "name");
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
      DUPLICATE_CHECK(managementIntervalP, "managementInterval", nodeP);
      OBJECT_CHECK(nodeP, "managementInterval");
      EMPTY_OBJECT_CHECK(nodeP, "managementInterval");
      if (pcheckTimeInterval(nodeP, "managementInterval") == false)
        return false;
    }
    else if (strcmp(nodeP->name, "location") == 0)
    {
      DUPLICATE_CHECK(locationP, "location", nodeP);
      OBJECT_CHECK(nodeP, "location");
      EMPTY_OBJECT_CHECK(nodeP, "location");

      if (pcheckGeoPropertyValue(locationP, NULL, NULL) == false)
      {
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "observationSpace") == 0)
    {
      DUPLICATE_CHECK(observationSpaceP, "observationSpace", nodeP);
      OBJECT_CHECK(nodeP, "observationSpace");
      EMPTY_OBJECT_CHECK(nodeP, "observationSpace");
      if (pcheckGeoPropertyValue(observationSpaceP, NULL, NULL) == false)
      {
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "operationSpace") == 0)
    {
      DUPLICATE_CHECK(operationSpaceP, "operationSpace", nodeP);
      OBJECT_CHECK(nodeP, "operationSpace");
      EMPTY_OBJECT_CHECK(nodeP, "operationSpace");
      if (pcheckGeoPropertyValue(operationSpaceP, NULL, NULL) == false)
      {
        orionldState.httpStatusCode = SccBadRequest;
        return false;
      }
    }
    else if (strcmp(nodeP->name, "expires") == 0)
    {
      DUPLICATE_CHECK(expiresP, "expires", nodeP);
      STRING_CHECK(nodeP, "expires");
      EMPTY_STRING_CHECK(nodeP, "expires");
      DATETIME_CHECK(expiresP->value.s, dateTime, "expires");
    }
    else if (strcmp(nodeP->name, "endpoint") == 0)
    {
      DUPLICATE_CHECK(endpointP, "endpoint", nodeP);
      STRING_CHECK(nodeP, "endpoint");
      URI_CHECK(nodeP->value.s, nodeP->name, true);
    }
    else
    {
      kjChildRemove(registrationP, nodeP);

      //
      // Add the Property to 'propertyTree'
      //
      // But, before adding, look it up.
      // If already there, then we have a case of duplicate property and that is an error
      //
      if (kjLookup(propertyTree, nodeP->name) != NULL)
      {
        orionldState.httpStatusCode = SccBadRequest;
        orionldErrorResponseCreate(OrionldBadRequestData, "Duplicate Property in Registration Update", nodeP->name);
        return false;
      }

      kjChildAdd(propertyTree, nodeP);
    }

    nodeP = next;
  }

  *propertyTreeP = propertyTree;  // These properties will be checked later, after getting the reg from DB

  return true;
}




