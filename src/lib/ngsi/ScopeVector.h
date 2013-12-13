#ifndef SCOPE_VECTOR_H
#define SCOPE_VECTOR_H

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
#include <vector>

#include "ngsi/Scope.h"



/* ****************************************************************************
*
* ScopeVector - 
*/
typedef struct ScopeVector
{
  std::vector<Scope*>  vec;

  std::string  render(Format format, std::string indent, bool comma);
  std::string  check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter);
  void         present(std::string indent);
  void         push_back(Scope* item);
  unsigned int size(void);
  Scope*       get(int ix);
  void         release();
} ScopeVector;

#endif
