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
#include <map>

#include "orionld/common/orionldState.h"                    // orionldState

#include "common/string.h"
#include "common/globals.h"
#include "rest/ConnectionInfo.h"



/* ****************************************************************************
*
* ConnectionInfo::ConnectionInfo - 
*/
ConnectionInfo::ConnectionInfo():
  restServiceP           (NULL)
{
}



/* ****************************************************************************
*
* ConnectionInfo::ConnectionInfo - 
*/
ConnectionInfo::ConnectionInfo(MHD_Connection* _connection):
  restServiceP           (NULL)
{
  orionldState.mhdConnection = _connection;

  if ((orionldState.verb != HTTP_POST)    &&
      (orionldState.verb != HTTP_PUT)     &&
      (orionldState.verb != HTTP_GET)     &&
      (orionldState.verb != HTTP_DELETE)  &&
      (orionldState.verb != HTTP_PATCH)   &&
      (orionldState.verb != HTTP_OPTIONS))
  {
    orionldState.badVerb = true;
    orionldState.verb    = HTTP_NOVERB;
  }
}



/* ****************************************************************************
*
* ConnectionInfo::~ConnectionInfo - 
*/
ConnectionInfo::~ConnectionInfo()
{
  servicePathV.clear();
}



/* ****************************************************************************
*
* uriParamTypesParse - parse the URI param 'type' into uriParamTypes vector
*/
void uriParamTypesParse(ConnectionInfo* ciP, const char* value)
{
  std::vector<std::string> vec;

  stringSplit(value, ',', vec);

  for (unsigned int ix = 0; ix < vec.size(); ++ix)
  {
    ciP->uriParamTypes.push_back(vec[ix]);
  }
}
