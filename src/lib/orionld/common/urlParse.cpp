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
#include "orionld/common/urlParse.h"              // Own interface



// ----------------------------------------------------------------------------
//
// urlParse -
//
// 1. Find ':', copy left-hand-side to 'protocol'
// 2. Make sure "//" comes after ':'
// 3. Copy all uptil (not including) the next '/' to 'ip'
// 4. Optionally, a :<port number>
// 5. Make *urlPathPP point to the rest
//
// NOTE
//   The called MUST make sure 'protocol' and 'ip' have enough room
//
bool urlParse
(
  const char*  url,
  char*        protocol,
  int          protocolSize,
  char*        ip,
  int          ipSize,
  uint16_t*    portP,
  char**       urlPathPP,
  char**       detailsPP
)
{
  int urlIx  = 0;
  int toIx   = 0;

  //
  // 1. Find ':', copy left-hand-side to 'protocol'
  //
  while ((url[urlIx] != 0) && (url[urlIx] != ':'))
  {
    if (toIx < protocolSize - 1)
      protocol[toIx] = url[urlIx];
    else
    {
      LM_W(("Bad Input (not enough room in protocol char vector: url='%s')", url));
      *detailsPP = (char*) "Not a URI";
      protocol[toIx] = 0;
      return false;
    }

    ++toIx;
    ++urlIx;
  }

  if (url[urlIx] != ':')
  {
    *detailsPP = (char*) "URL parse error - no colon found - cannot determine the protocol";
    return false;
  }

  protocol[toIx] = 0;


  //
  // 2. Make sure "//" comes after ':'
  //
  if ((url[urlIx + 1] != '/') || (url[urlIx + 2] != '/'))
  {
    *detailsPP = (char*) "URL parse error - no :// found after protocol";
    return false;
  }
  urlIx += 3;  // Step over ://


  //
  // 3. Getting the IP address
  //
  toIx = 0;
  while ((url[urlIx] != 0) && (url[urlIx] != '/'))
  {
    if (toIx < ipSize - 1)
      ip[toIx] = url[urlIx];
    else
    {
      *detailsPP = (char*) "not enough room in ip char vector";
      ip[toIx] = 0;
      return false;
    }

    ++toIx;
    ++urlIx;
  }

  if (url[urlIx] == 0)
  {
    *detailsPP = (char*) "URL parse error - no slash found after IP address";
    return false;
  }
  ip[toIx] = 0;


  //
  // 4. Optionally, a :<port number>
  //
  if (url[urlIx] == ':')  // It's a port number
  {
    char portNumberString[6];

    toIx = 0;

    urlIx += 1;  // Step over the ':'

    while ((url[urlIx] != 0) && (url[urlIx] != '/'))
    {
      portNumberString[toIx++] = url[urlIx++];
    }
    if (url[urlIx] == 0)
    {
      *detailsPP = (char*) "URL parse error - no slash found after port number";
      return false;
    }
    portNumberString[toIx] = 0;
    *portP = atoi(portNumberString);
  }

  //
  // 5. Make *urlPathPP point to the rest
  //
  if (url[urlIx] == '/')
  {
    *urlPathPP = (char*) &url[urlIx];
  }
  else
  {
    *detailsPP = (char*) "URL parse error - no slash found to start the URL PATH";
    return false;
  }

  return true;
}
