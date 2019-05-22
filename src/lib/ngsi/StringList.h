#ifndef SRC_LIB_NGSI_STRINGLIST_H_
#define SRC_LIB_NGSI_STRINGLIST_H_

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

#include "ngsi/Metadata.h"
#include "ngsi/Request.h"



/* ****************************************************************************
*
* StringList -
*/
typedef struct StringList
{
  std::vector<std::string>  stringV;

  void         fill(const std::vector<std::string>& aVec);
  void         fill(const std::string& commaSeparatedList);
  std::string  toJsonV1(bool comma, const std::string& fieldName);
  std::string  toJson(void);
  std::string  toString(void);
  void         release(void);
  bool         lookup(const std::string& string) const;
  void         push_back(const std::string& string);
  void         push_back_if_absent(const std::string& string);
  unsigned int size(void) const;
  void         clone(const StringList& sList);

  std::string  check(void);

  std::string  operator[](unsigned int ix)  const
  {
    return stringV[ix];
  }

  std::string  get(unsigned int ix)  const
  {
    return stringV[ix];
  }

} StringList;

#endif  // SRC_LIB_NGSI_STRINGLIST_H_
