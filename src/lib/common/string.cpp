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

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/wsStrip.h"

#define DEFAULT_HTTP_PORT 80

/* ****************************************************************************
*
* checkGroupIPv6 -
*/
bool checkGroupIPv6(std::string in)
{
  // Can receive for example:
  // :, 2001:, db8:, 0DB8:

  if (in.empty()) return false;

  if (in == ":") return true;

  if (in.length() > 5) return false;

  bool resu = true;
  for (uint i=0; i < in.length() - 1 ; i++) 
  {
     if (isxdigit(in[i]) == false)
     {
       resu = false;
       break;
     }
  }
  return resu;
}


/* ****************************************************************************
*
* isIPv6 -
*/
bool isIPv6(std::string in)
{
   // An IP v6 have between two and seven character ":"
   //  ::
   //  2001:0db8:85a3:08d3:1319:8a2e:0370:7334

   size_t pos;
   std::string partip;
   std::string resu;
   std::string staux = in;
   int cont = 0;

   pos = staux.find(":");
   while (pos != std::string::npos)
   {
      cont++;
      partip = staux.substr(0, pos+1);
      resu += partip;

      if (checkGroupIPv6(partip)== false)
         return false;

      partip = staux.substr(pos+1);
      staux = partip;

      pos = staux.find(":");
   }

   return ((cont > 1) && (cont < 8));
}

/* ****************************************************************************
*
* getIPv6Port -
*/
bool getIPv6Port(std::string in, std::string& outIp, std::string& outPort)
{
   size_t pos; 
   std::string partip;
   std::string resu;
   std::string staux = in; 
   
   // Split IP and port
   pos = staux.find(":");  
   while (pos != std::string::npos)
   {
      partip = staux.substr(0, pos+1);
      resu += partip;
      partip = staux.substr(pos+1);
      staux = partip;
      pos = staux.find(":");
   }


   if (resu.empty())
      return false;

   outIp = resu.substr(0, resu.length() -1 );
   outPort = staux;

   return isIPv6(resu);
}


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

    std::string  auxIp;
    std::string  auxPort;

    // First we check if it is IPv6
    if (getIPv6Port(urlTokens[2], auxIp, auxPort))  
    {
      // IPv6
      host = auxIp;
      port = atoi(auxPort.c_str());
      LM_VVV(("Parsed IPv6: '%s' and port: '%d'", host.c_str(), port));
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



/* ****************************************************************************
*
* string2coords - 
*/
bool string2coords(std::string s, double& latitude, double& longitude)
{
  char* initial = strdup(s.c_str());
  char* cP      = initial;
  char* comma;
  char* number1;
  char* number2;

  cP = wsStrip(cP);

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

  std::string err;
  double oldLatitude = latitude;
  double oldLongitude = longitude;
  latitude = atoF(number1, err);
  if (err.length() > 0) {
     latitude = oldLatitude; 
     return false;
  }
  else {
     longitude = atoF(number2, err);
     if (err.length() > 0) {
         /* Rollback latitude */
         latitude = oldLatitude;
         longitude = oldLongitude;
         return false;
     }
  }

  return true;
}



/* ****************************************************************************
*
* coords2string - 
*/
void coords2string(std::string& s, double latitude, double longitude, int decimals)
{
  char buf[256];
  char format[32];

  snprintf(format, sizeof(format), "%%.%df, %%.%df", decimals, decimals);
  snprintf(buf,    sizeof(buf),    format,           latitude, longitude);

  s = buf;
}

/* ****************************************************************************
*
* versionParse -
*/
bool versionParse(std::string version, int& mayor, int& minor, std::string& bugFix)
{
   char* copy = strdup(version.c_str());
   char* s    = wsStrip(copy);
   char* dotP;


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
double atoF(const char* string, std::string& errorMsg)
{
   char* cP = (char*) string;
   int   noOf;

   errorMsg = "";

   if (string[0] == 0)
   {
     errorMsg = "empty string";
     return 0.0;
   }

   if ((*cP == '-') || (*cP == '+'))
   {
      ++cP;

      if (!isdigit(*cP) && (*cP != '.'))
      // the check on '.' is to allow e.g. '-.7' and '+.7'
      {
         errorMsg = "non-digit after unary minus/plus";
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
            errorMsg = "last character in a double cannot be a dot";
            return 0.0;
         }
      }

      ++tmp;
   }

   if (noOf > 1)
   {
      errorMsg = "more than one dot";
      return 0.0;
   }

   if (strspn(cP, ".0123456789") != strlen(cP))
   {
      errorMsg = "invalid characters in string to convert";
      return 0.0;
   }

   return atof(string);
}
