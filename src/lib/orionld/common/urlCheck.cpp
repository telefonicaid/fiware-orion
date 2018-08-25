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
#include <stdlib.h>                               // atoi

#include "logMsg/logMsg.h"                        // LM_*

#include "orionld/common/SCOMPARE.h"              // SCOMPAREx
#include "orionld/common/urlCheck.h"              // Own interface



// ----------------------------------------------------------------------------
//
// urlCheck -
//
// 1. Make sure protocol is either 'http' or 'https'
// 3. Make sure "//" comes after ':'
// 4. Copy all uptil (not including) the next '/' to 'ip'
// 5. Make sure the ip is a correct IP address
// 6. Optionally, a :<port number> - if so, make sure it is a number
// 7. Make sure the rest is a valid path
//
bool urlCheck(char* url, char** detailsPP)
{
  //
  // 1. A URL for ngsi-ld MUST start with http:// or https://
  //
  // So:
  // 1.1: Make sure the first four chars are 'http' - eat them
  // 1.2: if char 5 == 's', eat it
  // 1.3: Make sure next three chars are ://
  //
  if (!SCOMPARE4(url, 'h', 't', 't', 'p'))
  {
    *detailsPP = (char*) "protocol doesn't start with 'http' nor 'https'";
    return false;
  }

  char* urlP  = &url[4];  // Skipping the first four chars, that are already checked

  if (*urlP == 's')
    ++urlP;

  if (!SCOMPARE3(urlP, ':', '/', '/'))
  {
    *detailsPP = (char*) "protocol doesn't start with 'http://' nor 'https://'";
    return false;
  }

  
  //
  // 3. Getting the IP address
  //
  char ip[128];
  int  ipSize = sizeof(ip);

  int toIx = 0;
  while((*urlP != 0) && (*urlP != '/'))
  {
    if (toIx < ipSize - 1)
      ip[toIx] = *urlP;
    else
    {
      *detailsPP = (char*) "assumed IP address too long";
      ip[toIx] = 0;
      return false;
    }

    ++toIx;
    ++urlP;
  }

  if (*urlP == 0)
  {
    *detailsPP = (char*) "URL parse error - no slash found after IP address";
    return false;
  }
  ip[toIx] = 0;

  // FIXME: Check that 'ip' is a valid IP address


  //
  // 4. Optionally, a :<port number>
  //
  if (*urlP == ':')  // It's a port number
  {
    char portNumberString[6];
    int  portNumberStringSize = sizeof(portNumberString);

    toIx = 0;

    ++urlP;  // Step over the ':'

    while ((*urlP != 0) && (*urlP != '/'))
    {
      if (toIx >= portNumberStringSize - 1)  // One char left for nuling the string
      {
        *detailsPP = (char*) "port number too big";
      }

      portNumberString[toIx++] = *urlP;
      ++urlP;
    }
    if (*urlP == 0)
    {
      *detailsPP = (char*) "URL parse error - no slash found after port number";
      return false;
    }
    portNumberString[toIx] = 0;

    for (char* portCharP = portNumberString; *portCharP != 0; ++portCharP)
    {
      if ((*portCharP < '0') || (*portCharP > '9'))
      {
        *detailsPP = (char*) "URL parse error - invalid port number";
        return false;
      }
    }
    
  }

  //
  // 5. Make *urlPathPP point to the rest
  //
  if (*urlP != '/')
  {
    *detailsPP = (char*) "URL parse error - no slash found to start the URL PATH";
    return false;
  }

  // FIXME: check that urlP is a valid URL PATH

  return true;
}
