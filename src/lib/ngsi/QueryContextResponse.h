#ifndef SRC_LIB_NGSI_QUERYCONTEXTRESPONSE_H_
#define SRC_LIB_NGSI_QUERYCONTEXTRESPONSE_H_

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
#include <iostream>
#include <sstream>

#include "ngsi/ContextElementResponseVector.h"
#include "rest/OrionError.h"



/* ****************************************************************************
*
* QueryContextResponse -
*
* NOTE
* Both contextElementResponseV and statusCode are optional, but a response
* MUST contain either contextElementResponseV OR statusCode.
*/
typedef struct QueryContextResponse
{
  ContextElementResponseVector  contextElementResponseVector;  // Optional
  OrionError                    error;                         // Optional

  QueryContextResponse();
  ~QueryContextResponse();

  void                   release(void);
  void                   fill(const EntityVector& entities);
  ContextAttribute*      getAttr(const std::string& attrName, OrionError* oeP);
} QueryContextResponse;

#endif  // SRC_LIB_NGSI_QUERYCONTEXTRESPONSE_H_
