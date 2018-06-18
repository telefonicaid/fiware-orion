#ifndef SRC_LIB_NGSI_CONDITIONVALUELIST_H_
#define SRC_LIB_NGSI_CONDITIONVALUELIST_H_

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
* ConditionValueList -
*/
typedef struct ConditionValueList
{
  std::vector<std::string>  vec;

  std::string  render(bool comma);
  void         release(void);
  void         push_back(const std::string& attributeName);
  unsigned int size(void);
  void         fill(ConditionValueList& cvlP);

  std::string  check(void);

  std::string operator[] (unsigned int ix) const;


} ConditionValueList;

#endif  // SRC_LIB_NGSI_CONDITIONVALUELIST_H_
