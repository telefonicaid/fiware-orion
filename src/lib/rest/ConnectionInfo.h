#ifndef CONNECTION_INFO_H
#define CONNECTION_INFO_H

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
#include <time.h>
#include <sys/time.h>

#include <string>
#include <vector>
#include <map>

#include "ngsi/Request.h"
#include "parse/CompoundValueNode.h"

#include "rest/mhd.h"



/* ****************************************************************************
*
* Forward declarations
*/
struct ParseData;
struct RestService;



/* ****************************************************************************
*
* ConnectionInfo -
*/
class ConnectionInfo
{
public:
  ConnectionInfo();
  ConnectionInfo(MHD_Connection* _connection);
  ~ConnectionInfo();

  int                        urlComponents;
  std::vector<std::string>   urlCompV;
  RestService*               restServiceP;
  std::vector<std::string>   servicePathV;
  std::string                answer;
  std::vector<std::string>   uriParamTypes;
};



/* ****************************************************************************
*
* uriParamTypesParse - parse the URI param 'type' into uriParamTypes vector
*/
extern void uriParamTypesParse(ConnectionInfo* ciP, const char* value);

#endif
