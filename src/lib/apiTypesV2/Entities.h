#ifndef SRC_LIB_APITYPESV2_ENTITIES_H_
#define SRC_LIB_APITYPESV2_ENTITIES_H_

/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
#include <map>

#include "apiTypesV2/EntityVector.h"
#include "rest/OrionError.h"



/* ****************************************************************************
*
* To avoid a problematic and not necessary include
*/
struct QueryContextResponse;



/* ****************************************************************************
*
* Entities - 
*/
class Entities
{
 public:
  EntityVector  vec;          // Mandatory

  Entities();
  ~Entities();

  std::string  toJson(RenderFormat                     renderFormat,
                      const std::vector<std::string>&  attrsFilter,
                      bool                             blacklist,
                      const std::vector<std::string>&  metadataFilter);

  std::string  check(RequestType requestType);
  void         release(void);
  void         fill(const QueryContextResponse& qcrs, OrionError* oeP);
  int          size(void) const { return vec.vec.size(); }
};

#endif  // SRC_LIB_APITYPESV2_ENTITIES_H_
