#ifndef SRC_LIB_NGSI_QUERYCONTEXTREQUEST_H_
#define SRC_LIB_NGSI_QUERYCONTEXTREQUEST_H_

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

#include "ngsi/Request.h"
#include "ngsi/StringList.h"
#include "ngsi/EntityIdVector.h"
#include "rest/EntityTypeInfo.h"

#include "apiTypesV2/SubscriptionExpression.h"



/* ****************************************************************************
*
* Forward types - instead of including in header file ...
*/
class BatchQuery;



/* ****************************************************************************
*
* QueryContextRequest - 
*/
typedef struct QueryContextRequest
{
  EntityIdVector    entityIdVector;  // Mandatory
  StringList        attributeList;   // Optional
  StringList        attrsList;       // Used by the NGSIv2 forwarding logic, to avoid over-querying attributes (see pruneContextElements)
  SubscriptionExpression expr;       // Optional

  StringList        metadataList;          // From URI param 'metadata'
  std::string       contextProvider;       // Not part of the payload - used internally only
  bool              legacyProviderFormat;  // Not part of the payload - used internally only

  QueryContextRequest();
  QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const std::string& attributeName, bool _legacyProviderFormat);
  QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const StringList&  attributeList, bool _legacyProviderFormat);

  std::string   toJsonV1(void);
  std::string   toJson(void);
  void          release(void);
  void          fill(const std::string&  entityId,
                     const std::string&  entityIdPattern,
                     const std::string&  entityType,
                     EntityTypeInfo      typeInfo);
  void          fill(BatchQuery* bqP);

} QueryContextRequest;

#endif  // SRC_LIB_NGSI_QUERYCONTEXTREQUEST_H_
