#ifndef SRC_LIB_NGSI_UPDATECONTEXTREQUEST_H_
#define SRC_LIB_NGSI_UPDATECONTEXTREQUEST_H_

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

#include "apiTypesV2/EntityVector.h"
#include "orionTypes/UpdateActionType.h"
#include "apiTypesV2/Entity.h"



/* ****************************************************************************
*
* UpdateContextRequest - 
*/
typedef struct UpdateContextRequest
{
  EntityVector            entityVector;          // Mandatory
  ActionType              updateActionType;      // Mandatory

  std::string             contextProvider;       // Not part of the payload - used internally only
  bool                    legacyProviderFormat;  // Not part of the payload - used internally only

  UpdateContextRequest();
  UpdateContextRequest(const std::string& _contextProvider, bool _providerFormat, Entity* eP);

  std::string        toJsonV1(void);
  std::string        toJson(void);
  void               release(void);
  ContextAttribute*  attributeLookup(Entity* eP, const std::string& attributeName);

  void         fill(const std::string& entityId,
                    const std::string& entityIdPattern,
                    const std::string& entityType,
                    const std::string& attributeName,
                    ActionType         _updateActionType);

  void         fill(const Entity* entP, ActionType _updateActionType);
  void         fill(const std::string&   entityId,
                    ContextAttribute*    attributeP,
                    ActionType           _updateActionType,
                    const std::string&   type = "");

  void         fill(EntityVector* entities, ActionType _updateActionType);
} UpdateContextRequest;

#endif  // SRC_LIB_NGSI_UPDATECONTEXTREQUEST_H_
