#ifndef SRC_LIB_UTILITY_ENTITYTYPESATTRIBUTESRESPONSE_H_
#define SRC_LIB_UTILITY_ENTITYTYPESATTRIBUTESRESPONSE_H_

/*
*
* Copyright 2014 Telefonica Investigacion y Desarrollo, S.A.U
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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>

#include "common/Format.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* EntityTypesAttributesResponse - 
*/
class EntityTypesAttributesResponse
{
public:
  std::vector<std::string> attributeV;

  std::string   render(Format format, const std::string& indent);
  std::string   check(RequestType requestType, Format format, const std::string& indent, const std::string& predetectedError, int counter);
  void          present(const std::string& indent);
  void          release(void);
  void          push_back(const std::string& item);
  unsigned int  size(void);
  std::string   operator[](unsigned int ix)       { if (ix < attributeV.size()) return attributeV[ix]; else return ""; }
};

#endif  // SRC_LIB_UTILITY_ENTITYTYPESATTRIBUTESRESPONSE_H_
