#ifndef ORION_ERROR_H
#define ORION_ERROR_H

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

#include "common/globals.h"

#include "rest/OrionError.h"
#include "rest/HttpStatusCode.h"



/* ****************************************************************************
*
* OrionError - 
*/
typedef struct OrionError
{
public:
  HttpStatusCode  code;
  std::string     error;
  std::string     description;
  bool            filled;

  OrionError();
  OrionError(HttpStatusCode _code, const std::string& _description = "", const std::string& _error = "");

  std::string  setSCAndRender(HttpStatusCode* scP);
  std::string  toJson(void);
  void         fill(HttpStatusCode _code, const std::string& _description,  const std::string& _error = "");
  void         fill(const OrionError& oe);
  void         fill(OrionError* oeP);
  void         fillOrAppend(HttpStatusCode _code, const std::string& fullDetails, const std::string& appendDetail, const std::string& _error);

} OrionError;

#endif
