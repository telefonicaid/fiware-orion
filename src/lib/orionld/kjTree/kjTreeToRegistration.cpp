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
#include "orionld/kjTree/kjTreeToStringList.h"                 // kjTreeToStringList

// -----------------------------------------------------------------------------
//
// kjTreeToRegistration -
//
bool kjTreeToRegistration(ConnectionInfo* ciP, ngsiv2::Registration* regP, char** regIdPP)
{
  KjNode*                   kNodeP;
  char*                     nameP                     = NULL;
  char*                     descriptionP              = NULL;
  char*                     timeIntervalP             = NULL;
  char*                     informationP              = NULL;
  char*                     endpointP                 = NULL;
  bool                      entitiesPresent           = false;
  char*                     expiresP                  = NULL;
  KjNode*                   entArrayNodeP             = NULL; //Information::entities Array Node Pointer
  KjNode*                   eNodeP                    = NULL; //Entities Node Pointer


  // id
  char* registrationId;

  if (orionldState.payloadIdNode == NULL)
  {
    char randomId[32];

    mongoIdentifier(randomId);

    regP->id  = "urn:ngsi-ld:ContextSourceRegistration:";
    regP->id += randomId;

    registrationId = (char*) regP->id.c_str();

    LM_T(LmtServiceRoutine, ("jorge-log: orionldState.payloadIdNode == NULL"));
  }
  else
    registrationId = orionldState.payloadIdNode->value.s;

  if ((urlCheck(registrationId, NULL) == false) && (urnCheck(registrationId, NULL) == false))
  {
    LM_W(("Bad Input (Registration::id is not a URI)"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Registration::id is not a URI", registrationId, OrionldDetailsAttribute);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }

  regP->id  = registrationId;
  *regIdPP  = registrationId;


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

//FIXME payload is null
  /*  if (orionldState.payloadTypeNode == NULL)
  {
    LM_W(("Bad Input (Mandatory field missing: Registration::type)"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Mandatory field missing", "Registration::type", OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  }  */

// FIXME:
// Create function SCOMPARE25 for compare if type is ContextSourceRegistration
  /* if (!SCOMPARE13(orionldState.payloadTypeNode->value.s, 'R', 'e', 'g', 'i', 's', 't', 'r', 'a', 't', 'i', 'o', 'n', 0))
  {
    LM_W(("Bad Input (Registration type must have the value /Registration/)"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid value for Registration::type", orionldState.payloadTypeNode->value.s, OrionldDetailsString);
    ciP->httpStatusCode = SccBadRequest;
    return false;
  } 

 */

  //
  // Loop over the tree
  //
  for (kNodeP = orionldState.requestTree->value.firstChildP; kNodeP != NULL; kNodeP = kNodeP->next)
  {   
    if (SCOMPARE3(kNodeP->name, 'i', 'd', 0))
    {
      //FiXME
       //do nothing, solve problem, the id must be in the orionldState.payloadIdNode

    }
    else if (SCOMPARE5(kNodeP->name, 't', 'y', 'p', 'e', 0))
    {
       //FiXME
       //do nothing, solve problem, the type must be in the orionldState.payloadTypeNode

    }
    else if (SCOMPARE5(kNodeP->name, 'n', 'a', 'm', 'e', 0))
    {
      DUPLICATE_CHECK(nameP, "Registration::name", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Registration::name");
      regP->name = nameP;
    }
    else if (SCOMPARE12(kNodeP->name, 'd', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(descriptionP, "Registration::description", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Registration::description");

      regP->description         = descriptionP;
      regP->descriptionProvided = true;
    }
    else if(SCOMPARE12(kNodeP->name, 'i', 'n', 'f', 'o', 'r', 'm', 'a', 't', 'i', 'o', 'n', 0))
    {
      DUPLICATE_CHECK(informationP, "Regisratrion::information", kNodeP->value.s);
      ARRAY_CHECK(kNodeP, "Registration::information");
      EMPTY_ARRAY_CHECK(kNodeP, "Registration::information");

      for(entArrayNodeP = kNodeP->value.firstChildP; entArrayNodeP != NULL; entArrayNodeP = entArrayNodeP->next)
      {
        for(eNodeP = entArrayNodeP->value.firstChildP; eNodeP != NULL; eNodeP = eNodeP->next)
        {
          if (SCOMPARE9(eNodeP->name, 'e', 'n', 't', 'i', 't', 'i', 'e', 's', 0))
          {           
            entitiesPresent = kjTreeToEntIdVector(ciP, eNodeP, &regP->dataProvided.entities);

            if (!entitiesPresent)
            {
              LM_E(("kjTreeToEntIdVector failed"));
              return false;  // orionldErrorResponseCreate is invoked by kjTreeToEntIdVector
            }
          }
          else if(SCOMPARE11(eNodeP->name, 'p', 'r', 'o', 'p', 'e', 'r', 't', 'i', 'e', 's', 0))
          {
            KjNode* propP; //Entities Node Pointer
            for(propP = eNodeP->value.firstChildP; propP!= NULL; propP = propP->next)
            {
               char*    propNameP         = NULL;
               DUPLICATE_CHECK(propNameP, "PropertyInfo::name",propP->value.s);
               STRING_CHECK(propP, "PropertyInfo::name");

               std::string property = propNameP;

               regP->dataProvided.attributes.push_back(property);
            }      
          }
          else if(SCOMPARE14(eNodeP->name, 'r', 'e', 'l', 'a', 't', 'i', 'o', 'n', 's', 'h', 'i', 'p', 's', 0))
          {
            //encontrar uma forma de salvar essa informação
          }
          else
          {
            orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid field for Registration::information::entities", eNodeP->name, OrionldDetailsString);
            return false;
          }
        }
      }

    }

    else if (SCOMPARE19(kNodeP->name, 'o', 'b', 's', 'e', 'r', 'v', 't', 'i','o','n', 'I', 'n', 't', 'e', 'r', 'v', 'a', 'l', 0))
    {
      DUPLICATE_CHECK(timeIntervalP, "Registration::observationInterval", kNodeP->value.s);
      INTEGER_CHECK(kNodeP, "Registration::observationInterval");
      // FIXME: Implement this ???
    }
     else if (SCOMPARE19(kNodeP->name, 'm', 'a', 'n', 'a', 'g', 'e', 'm', 'e','n','t', 'I', 'n', 't', 'e', 'r', 'v', 'a', 'l', 0))
    {
      DUPLICATE_CHECK(timeIntervalP, "Registration::managementInterval", kNodeP->value.s);
      INTEGER_CHECK(kNodeP, "Registration::managementInterval");
      // FIXME: Implement this ???
    }
    else if (SCOMPARE8(kNodeP->name, 'e', 'x', 'p', 'i', 'r', 'e', 's', 0))
    {
      DUPLICATE_CHECK(expiresP, "Registration::expires", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Registration::expires");
      DATETIME_CHECK(expiresP, "Registration::expires");

      regP->expires = parse8601Time(expiresP);
    }
    else if (SCOMPARE10(kNodeP->name, 'c', 'r', 'e', 'a', 't', 'e', 'd', 'A', 't', 0))
    {
      // Ignored - read-only
    }
    else if (SCOMPARE9(kNodeP->name, 'e', 'n', 'd', 'p', 'o', 'i', 'n', 't', 0))
    {
      DUPLICATE_CHECK(endpointP, "Registration::endpoint", kNodeP->value.s);
      STRING_CHECK(kNodeP, "Registration::endpoint");
  
      regP->provider.http.url = endpointP;
    }
    else if (SCOMPARE9(kNodeP->name, 'l', 'o', 'c', 'a', 't', 'i', 'o', 'n', 0))
    {
      // Ignored - read-only
    }
    else if (SCOMPARE11(kNodeP->name, 'm', 'o', 'd', 'i', 'f', 'i', 'e', 'd', 'A', 't', 0))
    {
      // Ignored - read-only
    }
    else
    {
      orionldErrorResponseCreate(ciP, OrionldBadRequestData, "Invalid field for Registration", kNodeP->name, OrionldDetailsString);
      return false;
    }
  }


  if ((entitiesPresent == false))
  {
    LM_E(("At least one 'entity' must be present"));
    orionldErrorResponseCreate(ciP, OrionldBadRequestData, "At least one 'entity' must be present", NULL, OrionldDetailsString);
    return false;
  }


  return true;
}
