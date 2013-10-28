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
#include <string.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <stdio.h>

#include "common/string.h"

#define DEFAULT_HTTP_PORT 80

/* ****************************************************************************
*
* stringSplit - 
*/
int stringSplit(std::string in, char delimiter, std::vector<std::string>& outV)
{
  char* s          = strdup(in.c_str());
  char* toFree     = s;
  char* start;
  int   components = 1;


  // 1. Skip leading delimiters
  while (*s == delimiter)
     ++s;
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
bool parseUrl(std::string url, std::string& host, int& port, std::string& path)
{

    /* Sanity check */
    if (url == "") {
        return false;
    }

    /* First: split by the first '/' to get host:ip and path */
    std::vector<std::string>  urlTokens;
    int                       components = stringSplit(url, '/', urlTokens);

    /* http://some.host.com/my/path
     *      ^^             ^  ^
     *      ||             |  |
     * -----  ------------- -- ----
     *   0          2       3    4  position in urlTokens vector
     *   1  23             4  5     coponentes
     */

    if ((components < 3) || (components == 3 && urlTokens[2].length() == 0)) {
        return false;
    }

    path = "";
    /* Note that components could be 3, in which case we don't enter in the for. This is
     * the case of URL without '/' like eg. "http://www.google.com" */
    for (int ix = 3; ix < components; ++ix ) {
        path += "/" + urlTokens[ix];
    }
    if (path == "") {
        /* Minimum path is always "/" */
        path = "/";
    }

    /* Second: split third token for host and port */
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
    if (components > 2) {
        return false;
    }

    host = hostTokens[0];

    if (components == 2) {
        port = atoi(hostTokens[1].c_str());
    }
    else {
        port = DEFAULT_HTTP_PORT;
    }

    return true;

}


/* ****************************************************************************
*
* i2s - integer to string
*/
char* i2s(int i, char* placeholder)
{
  sprintf(placeholder, "%d", i);
  return placeholder;
}

/* ****************************************************************************
*
* parsedUptime
*/
std::string parsedUptime(int uptime)
{

  char s[50];

  int seconds;
  int minutes;
  int hours;
  int days;

  minutes = uptime / 60;
  seconds = uptime % 60;

  hours = minutes / 60;
  minutes = minutes % 60;

  days = hours / 24;
  hours = hours % 24;

  sprintf(s, "%d d, %d h, %d m, %d s", days, hours, minutes, seconds);
  return std::string(s);
}
