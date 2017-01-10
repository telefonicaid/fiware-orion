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
#include "ngsi/ContextElement.h"
#include "ngsi/StatusCode.h"
#include "ngsi/AttributeList.h"

#include "mongo/client/dbclient.h"


/* ****************************************************************************
*
* Forward declarations
*/
struct QueryContextResponse;



/* ****************************************************************************
*
* ContextElementResponse -
*/
typedef struct ContextElementResponse
{
  ContextElement   contextElement;             // Mandatory
  StatusCode       statusCode;                 // Mandatory

  bool             prune;                      // operational attribute used internally by the queryContext logic for not deleting entities that were
                                               // without attributes in the Orion DB

  ContextElementResponse();
  ContextElementResponse(EntityId* eP, ContextAttribute* aP);
  ContextElementResponse(ContextElementResponse* cerP);
  ContextElementResponse(const mongo::BSONObj&  entityDoc,
                         const AttributeList&   attrL,
                         bool                   includeEmpty = true,
                         ApiVersion             apiVersion   = V1);
  ContextElementResponse(ContextElement* ceP, bool useDefaultType = false);

  std::string  render(ApiVersion   apiVersion,
                      bool         asJsonObject,
                      RequestType  requestType,
                      bool         comma               = false,
                      bool         omitAttributeValues = false);
  std::string  toJson(RenderFormat                     renderFormat,
                      const std::vector<std::string>&  attrsFilter,
                      const std::vector<std::string>&  metadataFilter,
                      bool blacklist = false);
  void         present(const std::string& indent, int ix);
  void         release(void);

  std::string  check(ApiVersion          apiVersion,
                     RequestType         requestType,
                     const std::string&  predetectedError,
                     int                 counter);

  void                     fill(struct QueryContextResponse*  qcrP,
                                const std::string&            entityId = "",
                                const std::string&            entityType = "");
  void                     fill(ContextElementResponse* cerP);
  ContextElementResponse*  clone(void);
} ContextElementResponse;

#endif  // SRC_LIB_NGSI_CONTEXTELEMENTRESPONSE_H_
