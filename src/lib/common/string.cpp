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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "common/string.h"
#include "common/wsStrip.h"
#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"



/* ****************************************************************************
*
* DEFAULT_HTTP_PORT - 
*/
#define DEFAULT_HTTP_PORT  80



/* ****************************************************************************
*
* checkGroupIPv6 -
*/
static bool checkGroupIPv6(std::string in)
{
  // Can receive for example:
  // :, 2001:, db8:, 0DB8:

  if (in.empty())
  {
    return false;
  }

  if (in == ":")
  {
    return true;
  }

  if (in.length() > 5)
  {
    return false;
  }

  for (uint i = 0; i < in.length() - 1 ; i++)
  {
    if (isxdigit(in[i]) == false)
    {
      return false;
    }
  }

  return true;
}



/* ****************************************************************************
*
* isIPv6 -
*
* An IP v6 has between two and seven character ":"
*   o ::
*   o 2001:0db8:85a3:08d3:1319:8a2e:0370:7334
*
*/
bool isIPv6(const std::string& in)
{
  size_t      pos;
  std::string partip;
  std::string resu;
  std::string staux = in;
  int         cont  = 0;

  pos = staux.find(":");
  while (pos != std::string::npos)
  {
    cont++;
    partip = staux.substr(0, pos+1);
    resu  += partip;

    if (checkGroupIPv6(partip) == false)
    {
      return false;
    }

    partip = staux.substr(pos+1);
    staux  = partip;
    pos    = staux.find(":");
  }

  return ((cont > 1) && (cont < 8));
}



/* ****************************************************************************
*
* getIPv6Port -
*/
bool getIPv6Port(const std::string& in, std::string& outIp, std::string& outPort)
{
  size_t       pos;
  std::string  partip;
  std::string  res;
  std::string  staux = in;

  // Split IP and port
  pos = staux.find(":");
  while (pos != std::string::npos)
  {
    partip  = staux.substr(0, pos+1);
    res    += partip;
    partip  = staux.substr(pos+1);
    staux   = partip;
    pos     = staux.find(":");
  }

  if (res.empty())
  {
    return false;
  }

  outIp   = res.substr(0, res.length() - 1);
  outPort = staux;

  return isIPv6(res);
}



/* ****************************************************************************
*
* stringSplit - 
*/
int stringSplit(const std::string& in, char delimiter, std::vector<std::string>& outV)
{
  char* s          = strdup(in.c_str());
  char* toFree     = s;
  int   components = 1;
  char* start;

  // 1. Skip leading delimiters
  while (*s == delimiter)
  {
    ++s;
  }
  start = s;


  // 2. Empty string?
  if (*s == 0)
  {
    free(toFree);
    return 0;
  }


  // 3. replace all delimiter for ZERO
  while (*s != 0)
  {
    if (*s == delimiter)
    {
      *s = 0;
      ++components;
    }

    ++s;
  }


  // 4. pick up all components
  for (int ix = 0; ix < components; ix++)
  {
    outV.push_back(start);
    start = &start[strlen(start) + 1];
  }


  // 5. free s
  free(toFree);

  return components;
}



/* ****************************************************************************
*
* parseUrl -
*
* Breaks an URL into pieces. It returns false if the string passed as first
* argument is not a valid URL. Otherwise, it returns true.
*
*/
bool parseUrl(const std::string& url, std::string& host, int& port, std::string& path, std::string& protocol)
{
  /* Sanity check */
  if (url == "")
  {
    return false;
  }

  /* First: split by the first '/' to get host:ip and path */
  std::vector<std::string>  urlTokens;
  int                       components = stringSplit(url, '/', urlTokens);

  protocol = urlTokens[0];

  /* http://some.host.com/my/path
   *      ^^             ^  ^
   *      ||             |  |
   * -----  ------------- -- ----
   *   0          2       3    4  position in urlTokens vector
   *   1  23             4  5     coponentes
   */

  if ((components < 3) || (components == 3 && urlTokens[2].length() == 0))
  {
    return false;
  }

  path = "";
  //
  // Note that components could be 3, in which case we don't enter in the for. This is
  // the case of URL without '/' like eg. "http://www.google.com"
  //
  for (int ix = 3; ix < components; ++ix)
  {
    path += "/" + urlTokens[ix];
  }

  if (path == "")
  {
    /* Minimum path is always "/" */
    path = "/";
  }

  /* Second: split third token for host and port */

  std::string  auxIp;
  std::string  auxPort;

  // First we check if it is IPv6
  if (getIPv6Port(urlTokens[2], auxIp, auxPort))
  {
    // IPv6
    host = auxIp;
    port = atoi(auxPort.c_str());
  }
  else
  {
    // IPv4
    std::vector<std::string>  hostTokens;
    components = stringSplit(urlTokens[2], ':', hostTokens);

    /* some.host.com:8080
     *              ^
     *              |
     * ------------- ----
     *   0             1  position in urlTokens vector
     * 1            2     components
     */

    /* Sanity check */
    if (components > 2)
    {
      return false;
    }

    host = hostTokens[0];

    if (components == 2)
    {
      port = atoi(hostTokens[1].c_str());
    }
    else
    {
      port = DEFAULT_HTTP_PORT;
    }
  }

  return true;
}



/* ****************************************************************************
*
* i2s - integer to string
*/
char* i2s(int i, char* placeholder, int placeholderSize)
{
  snprintf(placeholder, placeholderSize, "%d", i);
  return placeholder;
}


/* ****************************************************************************
*
* parsedUptime
*/
std::string parsedUptime(int uptime)
{
  char  s[50];
  int   seconds;
  int   minutes;
  int   hours;
  int   days;

  minutes = uptime / 60;
  seconds = uptime % 60;

  hours   = minutes / 60;
  minutes = minutes % 60;

  days    = hours / 24;
  hours   = hours % 24;

  snprintf(s, sizeof(s), "%d d, %d h, %d m, %d s", days, hours, minutes, seconds);
  return std::string(s);
}



/* ****************************************************************************
*
* onlyWs - 
*/
bool onlyWs(const char* s)
{
  if (*s == 0)
  {
    return true;
  }

  while (*s != 0)
  {
    if ((*s != ' ') && (*s != '\t') && (*s != '\n'))
    {
      return false;
    }

    ++s;
  }

  return true;
}



/* ****************************************************************************
*
* string2coords - 
*/
bool string2coords(const std::string& s, double& latitude, double& longitude)
{
  char*  initial = strdup(s.c_str());
  char*  cP      = initial;
  char*  comma;
  char*  number1;
  char*  number2;
  bool   ret = true;

  cP    = wsStrip(cP);

  comma = strchr(cP, ',');
  if (comma == NULL)
  {
    free(initial);
    return false;
  }
  *comma = 0;
  ++comma;

  number1 = cP;
  number2 = comma;

  number1 = wsStrip(number1);
  number2 = wsStrip(number2);

  std::string  err;
  double       oldLatitude  = latitude;
  double       oldLongitude = longitude;

  latitude                  = atoF(number1, &err);

  if (err.length() > 0)
  {
    latitude = oldLatitude;
    LM_W(("Bad Input (bad latitude value in coordinate string '%s')", initial));
    ret = false;
  }
  else
  {
    longitude = atoF(number2, &err);

    if (err.length() > 0)
    {
      /* Rollback latitude */
      latitude = oldLatitude;
      longitude = oldLongitude;
      LM_W(("Bad Input (bad longitude value in coordinate string '%s')", initial));
      ret = false;
    }
  }

  if ((latitude > 90) || (latitude < -90))
  {
    LM_W(("Bad Input (bad value for latitude '%s')", initial));
    ret = false;
  }
  else if ((longitude > 180) || (longitude < -180))
  {
    LM_W(("Bad Input (bad value for longitude '%s')", initial));
    ret = false;
  }

  free(initial);
  return ret;
}



/* ****************************************************************************
*
* coords2string - 
*/
void coords2string(std::string* s, double latitude, double longitude, int decimals)
{
  char buf[256];
  char format[32];

  snprintf(format, sizeof(format), "%%.%df, %%.%df", decimals, decimals);
  snprintf(buf,    sizeof(buf),    format,           latitude, longitude);

  *s = buf;
}



/* ****************************************************************************
*
* versionParse -
*/
bool versionParse(const std::string& version, int& mayor, int& minor, std::string& bugFix)
{
  char*  copy = strdup(version.c_str());
  char*  s    = wsStrip(copy);
  char*  dotP;


  //
  // mayor number
  //
  dotP = strchr(s, '.');
  if (dotP == NULL)
  {
    free(copy);
    return false;
  }

  *dotP = 0;
  ++dotP;

  s = wsStrip(s);
  mayor = atoi(s);
  if (strspn(s, "0123456789") != strlen(s))
  {
    free(copy);
    return false;
  }
  s = dotP;


  //
  // minor number
  // If no dot is found, no bugFix 'version' is present.
  // Just zero the 'bugFix' and keep the remaining string in minor.
  //
  bool bugFixEmpty = false;

  dotP = strchr(s, '.');
  if (dotP != NULL)
  {
    *dotP = 0;
    ++dotP;
  }
  else
  {
    bugFix = "";
    bugFixEmpty = true;
  }

  s = wsStrip(s);
  minor = atoi(s);
  if (strspn(s, "0123456789") != strlen(s))
  {
    free(copy);
    return false;
  }

  if (bugFixEmpty == true)
  {
    free(copy);
    return true;
  }

  s = dotP;



  //
  // bugfix
  //
  s = wsStrip(s);
  bugFix = s;

  free(copy);
  return true;
}



/* ****************************************************************************
*
* atoF - 
*/
double atoF(const char* string, std::string* errorMsg)
{
  char* cP = (char*) string;
  int   noOf;

  *errorMsg = "";

  if (string[0] == 0)
  {
    *errorMsg = "empty string";
    return 0.0;
  }

  if ((*cP == '-') || (*cP == '+'))
  {
    ++cP;

    if (!isdigit(*cP) && (*cP != '.'))
      // the check on '.' is to allow e.g. '-.7' and '+.7'
    {
      *errorMsg = "non-digit after unary minus/plus";
      return 0.0;
    }
  }

  // Number of dots
  noOf = 0;
  char* tmp = cP;
  while (*tmp != 0)
  {
    if (*tmp == '.')
    {
      ++noOf;
      if (tmp[1] == 0)
      {
        *errorMsg = "last character in a double cannot be a dot";
        return 0.0;
      }
    }

    ++tmp;
  }

  if (noOf > 1)
  {
    *errorMsg = "more than one dot";
    return 0.0;
  }

  if (strspn(cP, ".0123456789") != strlen(cP))
  {
    *errorMsg = "invalid characters in string to convert";
    return 0.0;
  }

  return atof(string);
}



/* ****************************************************************************
*
* strToLower - 
*/
char* strToLower(char* to, const char* from, int toSize)
{
  int fromSize = strlen(from);

  if (toSize < fromSize + 1)
  {
    LM_E(("Runtime Error (cannot copy %d bytes into a buffer of %d bytes)", fromSize + 1, toSize));
    fromSize = toSize;
  }

  int ix;
  for (ix = 0; ix < fromSize; ix++)
  {
    if ((from[ix] >= 'A') && (from[ix] <= 'Z'))
    {
      to[ix] = from[ix] + ('a' - 'A');
    }
    else
    {
      to[ix] = from[ix];
    }
  }

  to[ix] = 0;

  return to;
}



/* ****************************************************************************
*
* strReplace - 
*/
void strReplace(char* to, int toLen, const char* from, const char* oldString, const char* newString)
{
  int toIx   = 0;
  int fromIx = 0;
  int oldLen = strlen(oldString);
  int newLen = strlen(newString);

  while (from[fromIx] != 0)
  {
    if (strncmp(&from[fromIx], oldString, oldLen) == 0)
    {
      snprintf(to, toLen, "%s", newString);
      toIx   += newLen;
      fromIx += oldLen;
    }
    else
    {
      to[toIx] = from[fromIx];
      toIx   += 1;
      fromIx += 1;
    }
  }

  to[toIx] = 0;
}
