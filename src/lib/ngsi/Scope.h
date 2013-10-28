#ifndef SCOPE_H
#define SCOPE_H

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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>

#include "ngsi/Request.h"
#include "common/Format.h"

/* ****************************************************************************
*
* Defined scopes so far
*/
//FIXME: SCOPE_TYPE_ASSOC is not very good as string token, we should talk with NEC
//to use a better one
#define SCOPE_TYPE_ASSOC           "Include Associations"
#define SCOPE_VALUE_ASSOC_SOURCE   "SOURCES"
#define SCOPE_VALUE_ASSOC_TARGET   "TARGETS"
#define SCOPE_VALUE_ASSOC_ALL      "ALL"

/* ****************************************************************************
*
* Scope -
*/
typedef struct Scope
{
  std::string  type;    // Mandatory
  std::string  value;   // Mandatory

  Scope();
  Scope(std::string _type, std::string _value);

  std::string render(Format format, std::string indent);
  std::string check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter);
  void        present(std::string indent, int ix);
  void        release(void);
} Scope;

#endif
