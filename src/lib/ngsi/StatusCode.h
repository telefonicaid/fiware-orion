#ifndef STATUS_CODE_H
#define STATUS_CODE_H

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
#include "rest/HttpStatusCode.h"



/* ****************************************************************************
*
* StatusCode - 
*/
typedef struct StatusCode
{
  HttpStatusCode  code;             // Mandatory
  std::string     reasonPhrase;     // Mandatory
  std::string     details;          // Optional

  std::string     tag;              // tag to be rendered

  StatusCode();
  StatusCode(std::string _tag);
  StatusCode(HttpStatusCode _code, std::string _details, std::string _tag = "statusCode");

  std::string  render(Format format, std::string indent, bool comma = false, bool showTag = true);
  std::string  check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter);
  void         fill(HttpStatusCode _code, std::string _details = "");
  void         fill(StatusCode* scP);
  void         present(std::string indent);
  void         release(void);
  void         tagSet(std::string _tag);
} StatusCode;

#endif
