/*
*
* Copyright 2018 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
#include <stdlib.h>                               // atoi

#include "logMsg/logMsg.h"                        // LM_*

#include "orionld/common/orionldState.h"          // orionldState
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

  LM_T(LmtAlt, ("Incoming url: '%s'", url));

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
  LM_T(LmtAlt, ("Got the protocol: '%s'", protocol));
  LM_T(LmtAlt, ("Rest: '%s'", &url[urlIx]));

  //
  // 2. Make sure "//" comes after ':'
  //
  if ((url[urlIx + 1] != '/') || (url[urlIx + 2] != '/'))
  {
    *detailsPP = (char*) "URL parse error - no :// found after protocol";
    return false;
  }
  urlIx += 3;  // Step over ://
  LM_T(LmtAlt, ("Rest: '%s'", url));


  //
  // 3. Getting the IP address
  //
  toIx = 0;
  while ((url[urlIx] != 0) && (url[urlIx] != '/') && (url[urlIx] != ':'))
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

  ip[toIx] = 0;
  LM_T(LmtAlt, ("Got the IP: '%s'", ip));
  LM_T(LmtAlt, ("Rest: '%s'", &url[urlIx]));

  if (url[urlIx] == 0)
  {
    LM_T(LmtAlt, ("Were done (url[%d] == 0) (url: '%s')", urlIx, url));
    return true;
  }

  //
  // 4. Optionally, a :<port number>
  //
  if (url[urlIx] == ':')  // It's a port number
  {
    LM_T(LmtAlt, ("There's a port number"));
    char portNumberString[6];

    toIx = 0;

    urlIx += 1;     // Step over the ':'

    while ((url[urlIx] != 0) && (url[urlIx] != '/'))
    {
      portNumberString[toIx++] = url[urlIx++];
    }

    portNumberString[toIx] = 0;
    *portP = atoi(portNumberString);

    LM_T(LmtAlt, ("Port: %d", *portP));
    if (url[urlIx] == 0)
    {
      LM_T(LmtAlt, ("We're done"));
      return true;
    }

    LM_T(LmtAlt, ("Rest: '%s'", &url[urlIx]));
  }

  //
  // 5. Make *urlPathPP point to the rest
  //
  if (url[urlIx] == '/')
  {
    *urlPathPP = (char*) &url[urlIx];
    LM_T(LmtAlt, ("Got an URL PATH: '%s'", *urlPathPP));
  }
  else
  {
    LM_T(LmtAlt, ("No URL PATH"));
    *detailsPP = (char*) "URL parse error - no slash found to start the URL PATH";
    LM_T(LmtAlt, ("Done, but with error ..."));
    return false;
  }

  LM_T(LmtAlt, ("Done"));
  return true;
}



// -----------------------------------------------------------------------------
//
// urlParse - extract protocol, ip, port and URL-PATH from a 'reference' string
//
// FIXME
//   Unify the two urlParse functions
//
bool urlParse(char* url, char** protocolP, char** ipP, unsigned short* portP, char** restP)
{
  char*            protocolEnd;
  char*            colon;
  char*            ip;
  char*            rest;

  LM_T(LmtAlt, ("URL:      '%s'", url));

  // Check for custom url, e.g. "${abc}" - only if NGSIv2
  if (orionldState.apiVersion != API_VERSION_NGSILD_V1)
  {
    if (strncmp(url, "${", 2) == 0)
    {
      int len = strlen(url);

      if (url[len - 1] == '}')
      {
        *protocolP = NULL;
        *ipP       = NULL;
        *portP     = 0;
        *restP     = NULL;
        return true;
      }
    }
  }


  //
  // URL: <protocol> "://" <ip> [:<port] [path]
  //
  protocolEnd = strstr(url, "://");
  if (protocolEnd != NULL)
  {
    *protocolEnd = 0;
    *protocolP   = url;
    ip           = &protocolEnd[3];
  }
  else
  {
    *protocolP = (char*) "none";
    ip = url;
  }

  colon = strchr(ip, ':');
  if (colon != NULL)
  {
    *colon = 0;
    *portP = atoi(&colon[1]);
    rest   = &colon[1];
  }
  else
  {
    if (strcmp(*protocolP, "https") == 0)
      *portP = 443;
    else
      *portP = 80;

    rest   = ip;
  }

  *ipP   = ip;

  rest = strchr(rest, '/');
  if (rest != NULL)
  {
    *rest  = 0;    // The removed '/' is put back in later (by httpsNotify(), notificationSend()
    *restP = &rest[1];
  }
  else
    *restP = NULL;

  LM_T(LmtAlt, ("Protocol: '%s'", *protocolP));
  LM_T(LmtAlt, ("Host:     '%s'", *ipP));
  LM_T(LmtAlt, ("Port:      %d", *portP));
  LM_T(LmtAlt, ("Path:     '%s'", *restP));

  return true;
}
