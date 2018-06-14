#ifndef SRC_LIB_NGSI10_UPDATECONTEXTREQUEST_H_
#define SRC_LIB_NGSI10_UPDATECONTEXTREQUEST_H_

/*
*
* Copyright 2013 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "ngsi/ContextElementVector.h"
#include "ngsi/UpdateActionType.h"
#include "apiTypesV2/Entity.h"
#include "apiTypesV2/Entities.h"



/* ****************************************************************************
*
* Forward declarations
*/
struct UpdateContextElementRequest;
struct AppendContextElementRequest;
struct UpdateContextAttributeRequest;



/* ****************************************************************************
*
* UpdateContextRequest - 
*/
typedef struct UpdateContextRequest
{
  ContextElementVector    contextElementVector;  // Mandatory
  UpdateActionType        updateActionType;      // Mandatory

  std::string             contextProvider;       // Not part of the payload - used internally only

  UpdateContextRequest();
  UpdateContextRequest(const std::string& _contextProvider, EntityId* eP);

  std::string        render(ApiVersion apiVersion, bool asJsonObject);
  std::string        check(ApiVersion apiVersion, bool asJsonObject, const std::string& predetectedError);
  void               release(void);
  ContextAttribute*  attributeLookup(EntityId* eP, const std::string& attributeName);


  void         fill(const UpdateContextElementRequest* ucerP,
                    const std::string&                 entityId,
                    const std::string&                 entityType);

  void         fill(const AppendContextElementRequest* acerP,
                    const std::string&                 entityId,
                    const std::string&                 entityType);

  void         fill(const std::string& entityId,
                    const std::string& entityType,
                    const std::string& isPattern,
                    const std::string& attributeName,
                    const std::string& metaID,
                    const std::string& _updateActionType);

  void         fill(const UpdateContextAttributeRequest* ucarP,
                    const std::string&                   entityId,
                    const std::string&                   entityType,
                    const std::string&                   attributeName,
                    const std::string&                   metaID,
                    const std::string&                   _updateActionType);

  void         fill(const Entity* entP, const std::string& _updateActionType);
  void         fill(const std::string&   entityId,
                    ContextAttribute*    attributeP,
                    const std::string&   _updateActionType,
                    const std::string&   type = "");

  void         fill(Entities* entities, const std::string& _updateActionType);
} UpdateContextRequest;

#endif  // SRC_LIB_NGSI10_UPDATECONTEXTREQUEST_H_
