#ifndef SRC_LIB_NGSI_SCOPE_H_
#define SRC_LIB_NGSI_SCOPE_H_

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
#include "orionTypes/areas.h"
#include "rest/StringFilter.h"



/* ****************************************************************************
*
* Defined scopes so far
*/
#define SCOPE_OPERATOR_NOT           "Not"

#define SCOPE_FILTER                 "FIWARE" "::" "Filter"
#define SCOPE_FILTER_EXISTENCE       SCOPE_FILTER "::" "Existence"
#define SCOPE_FILTER_NOT_EXISTENCE   SCOPE_FILTER "::" SCOPE_OPERATOR_NOT "::" "Existence"

#define SCOPE_TYPE_SIMPLE_QUERY      "FIWARE::StringQuery"
#define SCOPE_TYPE_SIMPLE_QUERY_MD   "FIWARE::StringQuery::Metadata"

#define SCOPE_TYPE_LOCATION          FIWARE_LOCATION

#define SCOPE_VALUE_ENTITY_TYPE      "entity::type"



/* ****************************************************************************
*
* Scope -
*/
typedef struct Scope
{
  std::string  type;     // Mandatory
  std::string  value;    // Mandatory

  std::string  oper;     // Optional - used for filters

  orion::AreaType     areaType;
  orion::Circle       circle;
  orion::Polygon      polygon;
  orion::Point        point;
  orion::Line         line;
  orion::Box          box;
  orion::Georel       georel;
  StringFilter*       stringFilterP;
  StringFilter*       mdStringFilterP;

  Scope();
  Scope(const std::string& _type, const std::string& _value,  const std::string& _oper = "");

  int          fill(ApiVersion          apiVersion,
                    const std::string&  geometry,
                    const std::string&  coords,
                    const std::string&  georelString,
                    std::string*        errorString);

  std::string  toJsonV1(bool notLastInVector);
  void         release(void);

  std::string  check(void);
  void         areaTypeSet(const std::string& areaTypeString);
} Scope;

#endif  // SRC_LIB_NGSI_SCOPE_H_
