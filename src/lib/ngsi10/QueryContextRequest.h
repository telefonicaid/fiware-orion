#ifndef SRC_LIB_NGSI10_QUERYCONTEXTREQUEST_H_
#define SRC_LIB_NGSI10_QUERYCONTEXTREQUEST_H_

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
#include "ngsi/Restriction.h"
#include "rest/EntityTypeInfo.h"





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
  Restriction       restriction;     // Optional

  int               restrictions;
  StringList        metadataList;     // From URI param 'metadata'
  std::string       contextProvider;  // Not part of the payload - used internally only
  ProviderFormat    providerFormat;   // Not part of the payload - used internally only

  QueryContextRequest();
  QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const std::string& attributeName, ProviderFormat _providerFormat);
  QueryContextRequest(const std::string& _contextProvider, EntityId* eP, const StringList&  attributeList, ProviderFormat _providerFormat);

  std::string   toJsonV1(void);
  std::string   toJson(void);
  std::string   check(ApiVersion apiVersion, bool asJsonObject, const std::string& predetectedError);
  void          release(void);
  void          fill(const std::string& entityId, const std::string& entityType, const std::string& attributeName);
  void          fill(const std::string&  entityId,
                     const std::string&  entityType,
                     const std::string&  isPattern,
                     EntityTypeInfo      typeInfo,
                     const std::string&  attributeName);
  void          fill(BatchQuery* bqP);

} QueryContextRequest;

#endif  // SRC_LIB_NGSI10_QUERYCONTEXTREQUEST_H_
