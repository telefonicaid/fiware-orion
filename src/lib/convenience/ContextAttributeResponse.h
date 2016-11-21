#ifndef SRC_LIB_CONVENIENCE_CONTEXTATTRIBUTERESPONSE_H_
#define SRC_LIB_CONVENIENCE_CONTEXTATTRIBUTERESPONSE_H_

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

#include "ngsi/ContextAttributeVector.h"
#include "ngsi/StatusCode.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* Forward declarations
*/
struct QueryContextResponse;



/* ****************************************************************************
*
* ContextAttributeResponse - 
*/
typedef struct ContextAttributeResponse
{
  ContextAttributeVector     contextAttributeVector;     // Mandatory
  StatusCode                 statusCode;                 // Mandatory

  std::string render(const std::string&  apiVersion,
                     bool                asJsonObject,
                     RequestType         request,
                     const std::string&  indent);
  void        present(std::string indent);
  void        release(void);
  std::string check(const std::string&  apiVersion,
                    bool                asJsonObject,
                    RequestType         requestType,
                    std::string         indent,
                    std::string         predetectedError,
                    int                 counter);
  void        fill(ContextAttributeVector* _cavP, const StatusCode& _statusCode);
  void        fill(QueryContextResponse*  qcrP,
                   const std::string&     entityId,
                   const std::string&     entityType,
                   const std::string&     attributeName,
                   const std::string&     metaID);
} ContextAttributeResponse;

#endif  // SRC_LIB_CONVENIENCE_CONTEXTATTRIBUTERESPONSE_H_
