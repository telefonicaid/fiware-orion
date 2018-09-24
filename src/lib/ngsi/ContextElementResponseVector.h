#ifndef SRC_LIB_NGSI_CONTEXTELEMENTRESPONSEVECTOR_H_
#define SRC_LIB_NGSI_CONTEXTELEMENTRESPONSEVECTOR_H_

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

#include "ngsi/ContextElementResponse.h"
#include "common/RenderFormat.h"



/* ****************************************************************************
*
* ContextElementResponseVector -
*/
typedef struct ContextElementResponseVector
{
  std::vector<ContextElementResponse*>  vec;

  std::string              render(bool         asJsonObject,
                                  RequestType  requestType,
                                  bool         comma               = false,
                                  bool         omitAttributeValues = false);

  std::string              toJson(RenderFormat                     renderFormat,
                                  const std::vector<std::string>&  metadataFilter);
  void                     push_back(ContextElementResponse* item);
  unsigned int             size(void) const;
  ContextElementResponse*  lookup(EntityId* eP, HttpStatusCode code = SccNone);
  void                     release();
  void                     fill(ContextElementResponseVector& cerV);
  ContextElementResponse*  operator[] (unsigned int ix) const;
  

  std::string              check(ApiVersion          apiVersion,
                                 RequestType         requestType,
                                 const std::string&  predetectedError,
                                 int                 counter);
} ContextElementResponseVector;

#endif  // SRC_LIB_NGSI_CONTEXTELEMENTRESPONSEVECTOR_H_
