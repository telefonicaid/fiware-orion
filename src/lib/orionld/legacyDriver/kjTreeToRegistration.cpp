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
* Author: Jorge Pereira amd Ken Zangelin
*/
#include <string>                                              // std::string

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
#include "kjson/kjBuilder.h"                                   // kjObject, kjChildRemove, kjChildAdd
#include "kjson/kjClone.h"                                     // kjClone
#include "kalloc/kaAlloc.h"                                    // kaAlloc
}

#include "apiTypesV2/Registration.h"                           // Registration

#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldError.h"                       // orionldError
#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/uuidGenerate.h"                       // uuidGenerate
#include "orionld/context/orionldContextItemExpand.h"          // orionldContextItemExpand
#include "orionld/context/orionldAttributeExpand.h"            // orionldAttributeExpand
#include "orionld/payloadCheck/PCHECK.h"                       // PCHECK_*
#include "orionld/kjTree/kjTreeToTimeInterval.h"               // kjTreeToTimeInterval
#include "orionld/kjTree/kjTreeToGeoLocation.h"                // kjTreeToGeoLocation
#include "orionld/legacyDriver/kjTreeToStringList.h"           // kjTreeToStringList
#include "orionld/legacyDriver/kjTreeToEntIdVector.h"          // kjTreeToEntIdVector
#include "orionld/legacyDriver/kjTreeToRegistration.h"         // Own interface



// -----------------------------------------------------------------------------
//
// kjTreeToRegistrationInformation -
//
// FIXME: move to its own module
//
static bool kjTreeToRegistrationInformation(KjNode* regInfoNodeP, ngsiv2::Registration* regP)
{
  //
  // For now, the information vector can only have ONE item.
  //
  // FIXME: To support more than one information vector item, we need to modify the data model of Orion.
  //        When we do that, we can no longer use Orion forwarding but will need to imnplement our own -
  //        which is not such a bad thing as Orions forwarding has major flaws
  //
  int items = 0;
  for (KjNode* informationItemP = regInfoNodeP->value.firstChildP; informationItemP != NULL; informationItemP = informationItemP->next)
    ++items;

  if (items == 0)
  {
    orionldError(OrionldBadRequestData, "Empty 'information' in Registration", NULL, 400);
    return false;
  }
  else if (items > 1)
  {
    orionldError(OrionldOperationNotSupported, "More than one item in Registration::information vector", "Not Implemented", 501);
    return false;
  }

  for (KjNode* informationItemP = regInfoNodeP->value.firstChildP; informationItemP != NULL; informationItemP = informationItemP->next)
  {
    //
    // FIXME: create a kjTree function for this:
    //   kjTreeToInformationNode(informationItemP);
    //
    KjNode* entitiesP      = NULL;
    KjNode* propertiesP    = NULL;
    KjNode* relationshipsP = NULL;

    for (KjNode* infoNodeP = informationItemP->value.firstChildP; infoNodeP != NULL; infoNodeP = infoNodeP->next)
    {
      if (strcmp(infoNodeP->name, "entities") == 0)
      {
        DUPLICATE_CHECK(entitiesP, "Registration::information::entities", infoNodeP);
        if (infoNodeP->value.firstChildP == NULL)
        {
          orionldError(OrionldBadRequestData, "Empty Array", "information::entities", 400);
          return false;
        }

        if (kjTreeToEntIdVector(infoNodeP, &regP->dataProvided.entities) == false)
          return false;  // orionldError is invoked by kjTreeToEntIdVector
      }
      else if ((strcmp(infoNodeP->name, "properties") == 0) || (strcmp(infoNodeP->name, "propertyNames") == 0))
      {
        DUPLICATE_CHECK(propertiesP, "Registration::information::propertyNames", infoNodeP);
        if (infoNodeP->value.firstChildP == NULL)
        {
          orionldError(OrionldBadRequestData, "Empty Array", "information::propertyNames", 400);
          return false;
        }

        for (KjNode* propP = infoNodeP->value.firstChildP; propP != NULL; propP = propP->next)
        {
          STRING_CHECK(propP, "PropertyInfo::name");

          propP->value.s = orionldAttributeExpand(orionldState.contextP, propP->value.s, true, NULL);
          regP->dataProvided.propertyV.push_back(propP->value.s);
        }
      }
      else if ((strcmp(infoNodeP->name, "relationships") == 0) || (strcmp(infoNodeP->name, "relationshipNames") == 0))
      {
        DUPLICATE_CHECK(relationshipsP, "Registration::information::relationshipNames", infoNodeP);

        if (infoNodeP->value.firstChildP == NULL)
        {
          orionldError(OrionldBadRequestData, "Empty Array", "information::relationshipNames", 400);
          return false;
        }

        for (KjNode* relP = infoNodeP->value.firstChildP; relP != NULL; relP = relP->next)
        {
          STRING_CHECK(relP, "RelationInfo::name");

          relP->value.s = orionldAttributeExpand(orionldState.contextP, relP->value.s, true, NULL);
          regP->dataProvided.relationshipV.push_back(relP->value.s);
        }
      }
      else
      {
        orionldError(OrionldBadRequestData, "Unknown field inside Registration::information", infoNodeP->name, 400);
        return false;
      }
    }

    if ((entitiesP == NULL) && (propertiesP == NULL) && (relationshipsP == NULL))
    {
      orionldError(OrionldBadRequestData, "Empty Registration::information item", NULL, 400);
      return false;
    }
  }

  return true;
}



// -----------------------------------------------------------------------------
//
// kjTreeToRegistration -
//
bool kjTreeToRegistration(ngsiv2::Registration* regP, char** regIdPP)
{
  KjNode*  kNodeP;
  KjNode*  nameP                     = NULL;
  KjNode*  descriptionP              = NULL;
  KjNode*  informationP              = NULL;
  KjNode*  observationIntervalP      = NULL;
  KjNode*  managementIntervalP       = NULL;
  KjNode*  locationP                 = NULL;
  KjNode*  endpointP                 = NULL;
  KjNode*  observationSpaceP         = NULL;
  KjNode*  operationSpaceP           = NULL;
  KjNode*  expiresP                  = NULL;
  KjNode*  scopeP                    = NULL;
  KjNode*  modeP                     = NULL;
  KjNode*  operationsP               = NULL;
  KjNode*  refreshRateP              = NULL;
  KjNode*  managementP               = NULL;
  KjNode*  tenantP                   = NULL;

  if (orionldState.payloadIdNode == NULL)
  {
    char registrationId[100];
    uuidGenerate(registrationId, sizeof(registrationId), "urn:ngsi-ld:ContextSourceRegistration:");
    regP->id = registrationId;
  }
  else
  {
    char* uri = orionldState.payloadIdNode->value.s;
    PCHECK_URI(uri, true, 0, "Registration::id is not a URI", uri, 400);
    regP->id = uri;
  }

  *regIdPP  = (char*) regP->id.c_str();

  //
  // type
  //
  // NOTE
  //   The spec of ngsi-ld states that the field "type" is MANDATORY and MUST be set to "ContextSourceRegistration".
  //   A bit funny in my opinion.
  //   However, here we make sure that the spec is followed, but we add nothing to the database.
  //   When rendering (serializing) subscriptions for GET /registration, the field
  //     "type": "ContextSourceRegistration"
  //   is added to the response payload.
  //
  if (orionldState.payloadTypeNode == NULL)
  {
    LM_W(("Bad Input (Mandatory field missing: Registration::type)"));
    orionldError(OrionldBadRequestData, "Mandatory field missing", "Registration::type", 400);
    return false;
  }

  if (strcmp(orionldState.payloadTypeNode->value.s, "ContextSourceRegistration") != 0)
  {
    LM_W(("Bad Input (Registration type must have the value /Registration/)"));
    orionldError(OrionldBadRequestData,
                 "Registration::type must have a value of /ContextSourceRegistration/",
                 orionldState.payloadTypeNode->value.s,
                 400);
    return false;
  }


  //
  // Loop over the tree
  //
  KjNode* next;

  kNodeP = orionldState.requestTree->value.firstChildP;
  while (kNodeP != NULL)
  {
    next = kNodeP->next;

    if ((strcmp(kNodeP->name, "name") == 0) || (strcmp(kNodeP->name, "registrationName") == 0))
    {
      DUPLICATE_CHECK(nameP, "Registration::name", kNodeP);
      STRING_CHECK(kNodeP, "Registration::name");
      regP->name = nameP->value.s;
    }
    else if (strcmp(kNodeP->name, "description") == 0)
    {
      DUPLICATE_CHECK(descriptionP, "Registration::description", kNodeP);
      STRING_CHECK(kNodeP, "Registration::description");

      regP->description         = descriptionP->value.s;
      regP->descriptionProvided = true;
    }
    else if (strcmp(kNodeP->name, "information") == 0)
    {
      DUPLICATE_CHECK(informationP, "Registration::information", kNodeP);
      ARRAY_CHECK(kNodeP, "Registration::information");
      EMPTY_ARRAY_CHECK(kNodeP, "Registration::information");

      if (kjTreeToRegistrationInformation(kNodeP, regP) == false)
        return false;
    }
    else if (strcmp(kNodeP->name, "observationInterval") == 0)
    {
      DUPLICATE_CHECK(observationIntervalP, "Registration::observationInterval", kNodeP);
      OBJECT_CHECK(kNodeP, "Registration::observationInterval");
      kjTreeToTimeInterval(kNodeP, &regP->observationInterval);
    }
    else if (strcmp(kNodeP->name, "managementInterval") == 0)
    {
      DUPLICATE_CHECK(managementIntervalP, "Registration::managementInterval", kNodeP);
      OBJECT_CHECK(kNodeP, "Registration::managementInterval");
      kjTreeToTimeInterval(kNodeP, &regP->managementInterval);
    }
    else if (strcmp(kNodeP->name, "location") == 0)
    {
      DUPLICATE_CHECK(locationP, "Registration::location", kNodeP);
      OBJECT_CHECK(locationP, "Registration::location");
      kjTreeToGeoLocation(kNodeP, &regP->location);
    }
    else if (strcmp(kNodeP->name, "observationSpace") == 0)
    {
      DUPLICATE_CHECK(observationSpaceP, "Registration::observationSpace", kNodeP);
      OBJECT_CHECK(observationSpaceP, "Registration::observationSpace");
      kjTreeToGeoLocation(kNodeP, &regP->observationSpace);
    }
    else if (strcmp(kNodeP->name, "operationSpace") == 0)
    {
      DUPLICATE_CHECK(operationSpaceP, "Registration::operationSpace", kNodeP);
      OBJECT_CHECK(operationSpaceP, "Registration::operationSpace");
      kjTreeToGeoLocation(kNodeP, &regP->operationSpace);
    }
    else if ((strcmp(kNodeP->name, "expires") == 0) || (strcmp(kNodeP->name, "expiresAt") == 0))
    {
      DUPLICATE_CHECK(expiresP, "Registration::expiresAt", kNodeP);
      STRING_CHECK(kNodeP, "Registration::expiresAt");
      DATETIME_CHECK(expiresP->value.s, regP->expires, "Registration::expiresAt");
      PCHECK_EXPIRESAT_IN_FUTURE(0, "Invalid Registration", "/expiresAt/ in the past", 400, regP->expires, orionldState.requestTime);
    }
    else if (strcmp(kNodeP->name, "endpoint") == 0)
    {
      DUPLICATE_CHECK(endpointP, "Registration::endpoint", kNodeP);
      STRING_CHECK(kNodeP, "Registration::endpoint");

      regP->provider.http.url = endpointP->value.s;
    }
    else if (strcmp(kNodeP->name, "scope") == 0)
    {
      PCHECK_DUPLICATE(scopeP, kNodeP, 0, NULL, "Registration::scope", 400);
      PCHECK_STRING(scopeP, 0, NULL, "Registration::scope", 400);
      PCHECK_STRING_EMPTY(scopeP, 0, NULL, "Registration::scope", 400);
      regP->scope = kjClone(orionldState.kjsonP, scopeP);
    }
    else if (strcmp(kNodeP->name, "mode") == 0)
    {
      PCHECK_DUPLICATE(modeP, kNodeP, 0, NULL, "Registration::mode", 400);
      PCHECK_STRING(modeP, 0, NULL, "Registration::mode", 400);
      PCHECK_STRING_EMPTY(modeP, 0, NULL, "Registration::mode", 400);

      regP->mode = registrationMode(modeP->value.s);
      if (regP->mode == RegModeNone)
      {
        orionldError(OrionldBadRequestData, "invalid value for Registration::mode", modeP->value.s, 400);
        return false;
      }
    }
    else if (strcmp(kNodeP->name, "operations") == 0)
    {
      PCHECK_DUPLICATE(operationsP, kNodeP, 0, NULL, "Registration::operations", 400);
      PCHECK_ARRAY(operationsP, 0, NULL, "Registration::operations", 400);
      PCHECK_ARRAY_EMPTY(operationsP, 0, NULL, "Registration::operations", 400);
      PCHECK_ARRAY_OF_STRING(operationsP, 0, NULL, "Registration::operations", 400);
      regP->operations = kjClone(orionldState.kjsonP, operationsP);
    }
    else if (strcmp(kNodeP->name, "refreshRate") == 0)
    {
      PCHECK_DUPLICATE(refreshRateP, kNodeP, 0, NULL, "Registration::refreshRate", 400);
      PCHECK_STRING(refreshRateP, 0, NULL, "Registration::refreshRate", 400);
      // regP->refreshRate = iso8601Duration(refreshRate->value.s);
      regP->refreshRate = 7.3;
    }
    else if (strcmp(kNodeP->name, "management") == 0)
    {
      PCHECK_DUPLICATE(managementP, kNodeP, 0, NULL, "Registration::management", 400);
      PCHECK_OBJECT(managementP, 0, NULL, "Registration::management", 400);
      PCHECK_OBJECT_EMPTY(managementP, 0, NULL, "Registration::management", 400);
      regP->management = kjClone(orionldState.kjsonP, managementP);
    }
    else if (strcmp(kNodeP->name, "createdAt") == 0)
    {
      // Ignored - read-only
    }
    else if (strcmp(kNodeP->name, "modifiedAt") == 0)
    {
      // Ignored - read-only
    }
    else if (strcmp(kNodeP->name, "tenant") == 0)
    {
      PCHECK_DUPLICATE(tenantP, kNodeP, 0, NULL, "Registration::tenant", 400);
      PCHECK_STRING(tenantP, 0, NULL, "Registration::tenant", 400);
      if (strlen(tenantP->value.s) > sizeof(regP->tenant) - 1)
      {
        orionldError(OrionldBadRequestData, "String field too long", "Registration::tenant", 400);
        return false;
      }
      strncpy(regP->tenant, tenantP->value.s, sizeof(regP->tenant) - 1);
    }
    else
    {
      // "property-name": value - See <Csource Property Name> in ETSI Spec

      if (regP->properties == NULL)
        regP->properties = kjObject(orionldState.kjsonP, "properties");

      // Here, where I remove 'kNodeP' from its tree, the current loop wiould end if I used kNodeP->next (which is set to NULL
      // when kNodeP is removed
      // Instead I must save the kNodeP->next pointer (in the variable 'next') and use that variable instead.
      //
      // See kNodeP = next; at the end of the loop
      //
      kjChildRemove(orionldState.requestTree, kNodeP);
      kjChildAdd(regP->properties, kNodeP);

      //
      // Expand the name of the property
      //
      kNodeP->name = orionldAttributeExpand(orionldState.contextP, kNodeP->name, true, NULL);
    }

    kNodeP = next;
  }

  return true;
}
