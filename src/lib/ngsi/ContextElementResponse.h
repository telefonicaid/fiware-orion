#ifndef SRC_LIB_NGSI_CONTEXTELEMENTRESPONSE_H_
#define SRC_LIB_NGSI_CONTEXTELEMENTRESPONSE_H_

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

#include "common/RenderFormat.h"
#include "common/globals.h"
#include "rest/OrionError.h"
#include "ngsi/StringList.h"
#include "ngsi/ContextAttribute.h"
#include "apiTypesV2/Entity.h"

#include "mongoDriver/BSONObj.h"



/* ****************************************************************************
*
* ContextElementResponse -
*/
typedef struct ContextElementResponse
{
  Entity           entity;                     // Mandatory (represents a Context Element)
  OrionError       error;                 // Mandatory

  bool             prune;                      // operational attribute used internally by the queryContext logic for not deleting entities that were
                                               // without attributes in the Orion DB

  ContextElementResponse();
  ContextElementResponse(EntityId* eP, ContextAttribute* aP);
  ContextElementResponse(ContextElementResponse* cerP, bool cloneCompound = false);
  ContextElementResponse(const orion::BSONObj&  entityDoc,
                         const StringList&      attrL);
  ContextElementResponse(Entity* eP, bool useDefaultType = false);

  std::string  toJson(RenderFormat                         renderFormat,
                      const std::vector<std::string>&      attrsFilter,
                      bool                                 blacklist,
                      const std::vector<std::string>&      metadataFilter);

  void         applyUpdateOperators(void);

  void         release(void);

} ContextElementResponse;

#endif  // SRC_LIB_NGSI_CONTEXTELEMENTRESPONSE_H_
