#ifndef ERROR_CODE_H
#define ERROR_CODE_H

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
#include <iostream>
#include <sstream>

#include "common/Format.h"
#include "ngsi/Request.h"
#include "ngsi/StatusCode.h"



/* ****************************************************************************
*
* NO_ERROR_CODE - 
*/
#define NO_ERROR_CODE 0



/* ****************************************************************************
*
* ErrorCode - 
*/
typedef struct ErrorCode
{
  int          code;             // Mandatory
  std::string  reasonPhrase;     // Mandatory
  std::string  details;          // Optional

  ErrorCode();
  ErrorCode(int _code, std::string _reasonPhrase, std::string _details);

  std::string  render(Format format, std::string indent);
  std::string  check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter);
  void         fill(int _code, std::string _reasonPhrase, std::string _details = "");
  void         fill(StatusCode* scP);
  void         fill(ErrorCode* scP);    // FIXME P3: having StatusCode and ErrorCode actually being the same type lead to this dirty duplication
  void         present(std::string indent);
  void         release(void);
} ErrorCode;

#endif
