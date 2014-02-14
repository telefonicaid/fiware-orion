#ifndef DURATION_H
#define DURATION_H

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

#include "common/Format.h"
#include "ngsi/Request.h"

#define DEFAULT_DURATION "PT24H"


/* ****************************************************************************
*
* Duration - 
*/
class Duration
{
public:
  std::string   string;
  long long     seconds; 

private:
  bool          used;
  bool          valid;

public:
  Duration();
  void          set(std::string value);
  std::string   get(void);
  bool          isEmpty(void);
  std::string   render(Format format, std::string indent, bool comma = true);
  std::string   check(RequestType requestType, Format format, std::string indent, std::string predetectedError, int counter);
  long long     parse(void);
  void          present(std::string indent);
  void          release(void);
};

#endif
