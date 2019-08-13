/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
*
* This file is part of Orion Context Broker.
*
* Orion Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* iot_support at tid dot es
*
* Author: Jorge Pereira
*/
#include <string>                                              // std::string

extern "C"
{
#include "kjson/KjNode.h"                                      // KjNode
}

#include "rest/ConnectionInfo.h"                               // ConnectionInfo
#include "apiTypesV2/Registration.h"                           // Registration
#include "mongoBackend/MongoGlobal.h"                          // mongoIdentifier

#include "orionld/common/CHECK.h"                              // CHECKx()
#include "orionld/common/SCOMPARE.h"                           // SCOMPAREx
#include "orionld/common/orionldState.h"                       // orionldState
#include "orionld/common/orionldErrorResponse.h"               // orionldErrorResponseCreate
#include "orionld/common/urlCheck.h"                           // urlCheck
#include "orionld/common/urnCheck.h"                           // urnCheck
#include "orionld/context/orionldContextTreat.h"               // orionldContextTreat
#include "orionld/kjTree/kjTreeToEntIdVector.h"                // kjTreeToEntIdVector
#include "orionld/kjTree/kjTreeToTimeInterval.h"               // kjTreeToTimeInterval
#include "orionld/kjTree/kjTreeToStringList.h"                 // kjTreeToStringList



// -----------------------------------------------------------------------------
//
// kjTreeToRegistration -
//
bool kjTreeToRegistration(ConnectionInfo* ciP, ngsiv2::Registration* regP, char** regIdPP)
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
  bool     entitiesPresent           = false;
  KjNode*  expiresP                  = NULL;

  if (orionldState.payloadIdNode == NULL)
  {
    char randomId[32];

    LM_T(LmtServiceRoutine, ("jorge-log: orionldState.payloadIdNode == NULL"));

    mongoIdentifier(randomId);

    regP->id  = "urn:ngsi-ld:ContextSourceRegistration:";
    regP->id += randomId;
  }
  else
    regP->id = orionldState.payloadIdNode->value.s;

  if ((urlCheck((char*) regP->id.c_str(), NULL) == false) && (urnCheck((char*) regP->id.c_str(), NULL) == false))
  {
    LM_W(("Bad Input (Registration::id is not a URI)"));
    orionldErrorResponseCreate(OrionldBadRequestData, "Registration::id is not a URI", regP->id.c_str(), OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
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
    orionldErrorResponseCreate(OrionldBadRequestData, "Mandatory field missing", "Registration::type", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  if (strcmp(orionldState.payloadTypeNode->value.s, "ContextSourceRegistration") != 0)
  {
    LM_W(("Bad Input (Registration type must have the value /Registration/)"));
    orionldErrorResponseCreate(OrionldBadRequestData,
                               "Registration::type must have a value of /ContextSourceRegistration/",
                               orionldState.payloadTypeNode->value.s,
                               OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  //
  // FIXME: add type to Registration
  //        To be fixed when we decide to break the data model of Orion APIv2
  // regP->type = (char*) "ContextSourceRegistration";
  //


  //
  // Loop over the tree
  //
  for (kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {
    if (SCOMPARE5(kNodeP->name, 'n', 'a', 'm', 'e', 0))
    {
      DUPLICATE_CHECK(nameP, "Registration::name", kNodeP);
      STRING_CHECK(kNodeP, "Registration::name");
      regP->name = nameP->value.s;
    }
    else if (SCOMPARE12(kNodeP->name, 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(descriptionP, "Registration::description", kNodeP);
      STRING_CHECK(kNodeP, "Registration::description");

      regP->description         = descriptionP->value.s;
      regP->descriptionProvided = true;
    }
    else if (SCOMPARE12(kNodeP->name, 'i', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(informationP, "Registatrion::information", kNodeP);
      ARRAY_CHECK(kNodeP, "Registration::information");
      EMPTY_ARRAY_CHECK(kNodeP, "Registration::information");

      for (KjNode* informationItemP = kNodeP->value.firstChildP; informationItemP != NULL; informationItemP = informationItemP->next)
      {
        //
        // FIXME: create a kjTree function for this:
        //   kjTreeToInformationNode(informationItemP);
        //
        for (KjNode* eNodeP = informationItemP->value.firstChildP; eNodeP != NULL; eNodeP = eNodeP->next)
        {
          if (SCOMPARE9(eNodeP->name, 'e', 'n', 't', 'i', 't', 'i', 'e', 's', 0))
          {
            if (kjTreeToEntIdVector(ciP, eNodeP, &regP->dataProvided.entities) == false)
            {
              LM_E(("kjTreeToEntIdVector failed"));
              return false;  // orionldErrorResponseCreate is invoked by kjTreeToEntIdVector
            }
            entitiesPresent = true;
          }
          else if (SCOMPARE11(eNodeP->name, 'p', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's', 0))
          {
            for (KjNode* propP = eNodeP->value.firstChildP; propP != NULL; propP = propP->next)
            {
               STRING_CHECK(propP, "PropertyInfo::name");
               regP->dataProvided.propertyV.push_back(propP->value.s);
            }
          }
          else if (SCOMPARE14(eNodeP->name, 'r', 'e', 'l', 'a', 't', 'i', 'o', 'n', 's', 'h', 'i', 'p', 's', 0))
          {
            //
            // FIXME: We mix Properties and Relationships for now
            //        To be fixed when we decide to break the data model of Orion APIv2
            //
            for (KjNode* relP = eNodeP->value.firstChildP; relP != NULL; relP = relP->next)
            {
               STRING_CHECK(relP, "RelationInfo::name");
               regP->dataProvided.relationshipV.push_back(relP->value.s);
            }
          }
          else
          {
            orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration::information::entities", eNodeP->name, OrionldDetailsString);
            return false;
          }
        }
      }
    }
    else if (SCOMPARE20(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'I', 'n', 't', 'e', 'r', 'v', 'a', 'l', 0))
    {
      DUPLICATE_CHECK(observationIntervalP, "Registration::observationInterval", kNodeP);
      OBJECT_CHECK(kNodeP, "Registration::observationInterval");
      kjTreeToTimeInterval(ciP, kNodeP, &regP->observationInterval);
    }
    else if (SCOMPARE19(kNodeP->name, 'm', 'a', 'n', 'a', 'g', 'e', 'm', 'e', 'n', 't', 'I', 'n', 't', 'e', 'r', 'v', 'a', 'l', 0))
    {
      DUPLICATE_CHECK(managementIntervalP, "Registration::managementInterval", kNodeP);
      OBJECT_CHECK(kNodeP, "Registration::managementInterval");
      kjTreeToTimeInterval(ciP, kNodeP, &regP->managementInterval);
    }
    else if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(locationP, "Registration::location", kNodeP);
      OBJECT_CHECK(locationP, "Registration::location");
      //
      // FIXME: kjTreeToGeoLocation(kNodeP, &regP->location);
      //        To be fixed when we decide to break the data model of Orion APIv2
    }
    else if (SCOMPARE17(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(observationSpaceP, "Registration::observationSpace", kNodeP);
      OBJECT_CHECK(observationSpaceP, "Registration::observationSpace");
      //
      // FIXME: kjTreeToGeoLocation(kNodeP, &regP->observationSpace);
      //        To be fixed when we decide to break the data model of Orion APIv2
    }
    else if (SCOMPARE15(kNodeP->name, 'o', 'p', 'e', 'r', 'a', 't', 'i', 'o', 'n', 'S', 'p', 'a', 'c', 'e', 0))
    {
      DUPLICATE_CHECK(operationSpaceP, "Registration::operationSpace", kNodeP);
      OBJECT_CHECK(operationSpaceP, "Registration::operationSpace");
      //
      // FIXME: kjTreeToGeoLocation(kNodeP, &regP->operationSpace);
      //        To be fixed when we decide to break the data model of Orion APIv2
    }
    else if (SCOMPARE8(kNodeP->name, 'e', 'x', 'p', 'i', 'r', 'e', 's', 0))
    {
      DUPLICATE_CHECK(expiresP, "Registration::expires", kNodeP);
      STRING_CHECK(kNodeP, "Registration::expires");
      DATETIME_CHECK(expiresP->value.s, "Registration::expires");

      regP->expires = parse8601Time(expiresP->value.s);
    }
    else if (SCOMPARE9(kNodeP->name, 'e', 'n', 'd', 'p', 'o', 'i', 'n', 't', 0))
    {
      DUPLICATE_CHECK(endpointP, "Registration::endpoint", kNodeP);
      STRING_CHECK(kNodeP, "Registration::endpoint");

      regP->provider.http.url = endpointP->value.s;
    }
    else if (SCOMPARE10(kNodeP->name, 'c', 'r', 'e', 'a', 't', 'e', 'd', 'A', 't', 0))
    {
      // Ignored - read-only
    }
    else if (SCOMPARE11(kNodeP->name, 'm', 'o', 'd', 'i', 'f', 'i', 'e', 'd', 'A', 't', 0))
    {
      // Ignored - read-only
    }
    else
    {
      //
      // FIXME: Add to Property vector - To be fixed when we decide to break the data model of Orion APIv2
      //
      orionldErrorResponseCreate(OrionldBadRequestData, "Invalid field for Registration - ngsi-ld still follows Orion APIv2 data model", kNodeP->name, OrionldDetailsString);
      return false;
    }
  }


  if (entitiesPresent == false)
  {
    LM_E(("At least one 'entity' must be present"));
    orionldErrorResponseCreate(OrionldBadRequestData, "At least one 'entity' must be present", NULL, OrionldDetailsString);
    return false;
  }


  return true;
}
