/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "common/string.h"

#include "rest/ConnectionInfo.h"
#include "rest/RestService.h"
#include "rest/restServiceLookup.h"



/* ****************************************************************************
*
* restServiceLookup -
*/
RestService* restServiceLookup(ConnectionInfo* ciP, bool* badVerbP)
{
  RestService*              serviceV = restServiceGet(ciP->verb);
  std::vector<std::string>  compV;
  int                       components;
  int                       serviceIx = 0;

  if (serviceV == NULL)
  {
    *badVerbP = true;
    return NULL;  // Error taken care of later
  }


  // Split URI PATH into components
  components = stringSplit(ciP->url.c_str(), '/', compV);

  while (serviceV[serviceIx].request != InvalidRequest)
  {
    RestService* serviceP = &serviceV[serviceIx];

    if (serviceP->components != components)
    {
      ++serviceIx;
      continue;
    }

    bool match = true;
    for (int compNo = 0; compNo < components; ++compNo)
    {
      const char* component = serviceP->compV[compNo].c_str();

      if ((component[0] == '*') && (component[1] == 0))
      {
        continue;
      }

      if (ciP->apiVersion == V1)
      {
        if (strcasecmp(component, compV[compNo].c_str()) != 0)
        {
          match = false;
          break;
        }
      }
      else
      {
        if (strcmp(component, compV[compNo].c_str()) != 0)
        {
          match = false;
          break;
        }
      }
    }

    if (match == true)
    {
      break;
    }

    ++serviceIx;
  }

  return &serviceV[serviceIx];
}
