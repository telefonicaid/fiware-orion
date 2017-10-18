#ifndef SRC_LIB_NGSI_THROTTLING_H_
#define SRC_LIB_NGSI_THROTTLING_H_

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
#include <stdint.h>
#include <string>

#include "ngsi/Request.h"



/* ****************************************************************************
*
* Throttling - 
*/
typedef struct Throttling
{
  std::string   string;
  int64_t       seconds;

  Throttling(): seconds(-1) {}

  void               set(const std::string& value);
  const std::string  get(void);
  bool               isEmpty(void);
  std::string        render(bool comma);

  std::string        check(void);

  int64_t            parse(void);
  void               present(const std::string& indent);
} Throttling;

#endif  // SRC_LIB_NGSI_THROTTLING_H_
