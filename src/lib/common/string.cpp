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
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <algorithm>  // find
#include <math.h>     // modf

#include "logMsg/logMsg.h"
#include "logMsg/traceLevels.h"

#include "common/string.h"
#include "common/wsStrip.h"
#include "common/limits.h"
#include "alarmMgr/alarmMgr.h"



/* ****************************************************************************
*
* DEFAULT_HTTP_PORT - 
* DEFAULT_HTTPS_PORT -
*/
#define DEFAULT_HTTP_PORT   80
#define DEFAULT_HTTPS_PORT  443



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
  std::string staux = in;
  int         cont  = 0;

  pos = staux.find(":");
  while (pos != std::string::npos)
  {
    cont++;
    partip = staux.substr(0, pos+1);

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
int stringSplit(const std::string& in, char delimiter, std::vector<std::string>& outV, bool unique)
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
    // If unique is true, we need to ensure the element hasn't been added previousy in order to add it
    if ((!unique) || (std::find(outV.begin(), outV.end(), std::string(start)) == outV.end()))
    {
      outV.push_back(start);
    }
    start = &start[strlen(start) + 1];
  }


  // 5. free s
  free(toFree);

  return components;
}



/* *****************************************************************************
*
* hostnameIsValid - check a hostname for validity
*
* See https://tools.ietf.org/html/rfc1034#section-3.1
*/
static bool hostnameIsValid(const char* hostname)
{
  if (strchr(hostname, ':') != NULL)  // hostname contains a ':' ?
  {
    //
    // Looks like a numerical IPv6 address.
    // That requires a different approach for validity check
    //
    // FIXME P4: Implement validity check of numerical IPv6 addresses.
    //
    return true;
  }

  int len = strlen(hostname);

  if (len > 253)  // Max length is 253 chars
  {
    return false;
  }

  if (*hostname == '.')  // Cannot start with a dot
  {
    return false;
  }

  if (hostname[len - 1] == '.')  // Cannot end in a dot
  {
    return false;
  }

  if (strstr(hostname, "..") != NULL)  // Cannot contain two consecutive dots
  {
    return false;
  }


  //
  // Now split hostname into labels: . label . label . label ... 
  //
  // We've already seen that no '..' exists and also that 'hostname' doesn't start nor end in a dot.
  // This means we have no empty labels, and that is good :-)
  //
  std::vector<std::string> labelV;
  
  stringSplit(hostname, '.', labelV);

  for (unsigned int ix = 0; ix < labelV.size(); ++ix)
  {
    char* label    = (char*) labelV[ix].c_str();
    int   labelLen = strlen(label);

    //
    // Maximum allowed length of a label is 63 characters (and min is 1)
    // [ that the label is not empty we have checked already. Let's do it again! :-) ]
    //
    if ((labelLen > 63) || (labelLen <= 0))
    {
      return false;
    }

    //
    // A label cannot start nor end with a hyphen
    //
    if ((*label == '-') || (label[labelLen - 1] == '-'))
    {
      return false;
    }

    //
    // Labels can only contain the characters [a-z], [A-Z], [0-9] and hyphen
    // Labels CAN start with [0-9] - this saves us, as NUMERICAL IPs pass the check as well ... :-)
    //
    while (*label != 0)
    {
      if      ((*label >= '0') && (*label <= '9'))  {}    // OK: 0-9
      else if ((*label >= 'a') && (*label <= 'z'))  {}    // OK: a-c
      else if ((*label >= 'A') && (*label <= 'Z'))  {}    // OK: A-C
      else if (*label == '-')                       {}    // OK: hyphen (not first nor last char)
      else                                                // NOT OK - forbidden char
      {
        return false;
      }

      ++label;
    }
  }

  return true;
}



/* ****************************************************************************
*
* parseUrl - parse a URL and return its pieces
*
* Breaks a URL into pieces. It returns false if the string passed as first
* argument is not a valid URL. Otherwise, it returns true.
*
* PARAMETERS
*   - url
*   - host
*   - port
*   - path
*   - protocol
*
* RETURN VALUE
*   parseUrl returns TRUE on successful operation, FALSE otherwise
*
* NOTE
*   About the components in a URL: according to https://tools.ietf.org/html/rfc3986#section-3,
*   the scheme component is mandatory, i.e. the 'http://' or 'https://' must be present,
*   otherwise the URL is invalid. 
*/
bool parseUrl(const std::string& url, std::string& host, int& port, std::string& path, std::string& protocol)
{
  /* Sanity check */
  if (url.empty())
  {
    return false;
  }


  /* http://some.host.com/my/path
   *      ^^             ^  ^
   *      ||             |  |
   * -----  ------------- -- ----
   *   0          2       3    4  position in urlTokens vector
   *   1  23             4  5     components
   */


  /* First: split by the first '/' to get host:ip and path */
  std::vector<std::string>  urlTokens;
  int                       components = stringSplit(url, '/', urlTokens);  

  //
  // Ensuring the scheme is present
  // Note that although mqtt:// is not officially supported it is used de facto (see for instance https://github.com/mqtt/mqtt.org/wiki/URI-Scheme)
  //
  if ((urlTokens.size() == 0) || ((urlTokens[0] != "https:") && (urlTokens[0] != "http:") && (urlTokens[0] != "mqtt:")))
  {
    return false;
  }
  protocol = urlTokens[0];  // needed by the caller

  //
  // Ensuring the host is present
  //
  if ((urlTokens.size() < 3) || (urlTokens[2].empty()))
  {
    return false;
  }

  if ((components < 3) || (components == 3 && urlTokens[2].empty()))
  {
    return false;
  }

  path = "";
  //
  // Note that components could be 3, in which case we don't enter the for-loop. This is
  // the case of URL without '/' like "http://www.google.com"
  //
  for (int ix = 3; ix < components; ++ix)
  {
    path += "/" + urlTokens[ix];
  }

  if (path.empty())
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
      /* Sanity check (corresponding to http://xxxx:/path) */
      if (hostTokens[1].empty())
      {
        return false;
      }

      port = atoi(hostTokens[1].c_str());
    }
    else  // port not given - using default ports
    {
      port = urlTokens[0] == "https:" ? DEFAULT_HTTPS_PORT : DEFAULT_HTTP_PORT;
    }
  }

  //
  // Is 'port' a valid number?
  // MAX_PORT is the maximum number that a port can have.
  // Negative port numbers cannot exist and 0 is reserved.
  //
  if ((port > MAX_PORT) || (port <= 0))
  {
    return false;
  }

  if (hostnameIsValid(host.c_str()) == false)
  {
    return false;
  }

  return true;
}



/* ****************************************************************************
*
* validUrl - check validity of a URL
*/
bool validUrl(const std::string& url)
{
  std::string  host;
  int          port;
  std::string  path;
  std::string  protocol;

  return parseUrl(url, host, port, path, protocol);
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
  double newLatitude;
  double newLongitude;

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

  if ((str2double(number1, &newLatitude) == false) || (newLatitude > 90) || (newLatitude < -90))
  {
    std::string details = std::string("bad latitude value in coordinate string '") + initial + "'";
    alarmMgr.badInput(clientIp, details, s);

    free(initial);
    return false;
  }

  if ((str2double(number2, &newLongitude) == false) || (newLongitude > 180) || (newLongitude < -180))
  {
    std::string details = std::string("bad longitude value in coordinate string '") + initial + "'";
    alarmMgr.badInput(clientIp, details, s);

    free(initial);
    return false;
  }

  latitude  = newLatitude;
  longitude = newLongitude;

  free(initial);
  return true;
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
* atoUL -
*/
unsigned long atoUL(const char* string, std::string* errorMsg)
{
  // Initially I tryed with stroul() but it doesn't worked... so I'm using strol() plus
  // explicity checking for positive number
  char *ptr;
  long value = strtol (string, &ptr, 0);
  if (string == ptr)
  {
    *errorMsg = "parsing error";
  }
  if (value < 0)
  {
    *errorMsg = "negative number";
  }
  return (unsigned long) value;
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
      strncat(to, newString, toLen - strlen(to));
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



/* ****************************************************************************
*
* servicePathCheck - check one single component of the service-path
*
* NOTE
* This function is used for register/notify only. '#' is not an accepted character,
* as it would be for discoveries/subscriptions
*/
std::string servicePathCheck(const char* servicePath)
{
  if (servicePath  == NULL)      return "No Service Path";
  if (*servicePath == 0)         return "OK";   // Special case, default service path

  //
  // A service-path contains only alphanumeric characters, plus underscore
  //
  while (*servicePath != 0)
  {
    if ((*servicePath >= 'A') && (*servicePath <= 'Z'))
      ;
    else if ((*servicePath >= 'a') && (*servicePath <= 'z'))
      ;
    else if ((*servicePath >= '0') && (*servicePath <= '9'))
      ;
    else if (*servicePath == '_')
      ;
    else if (*servicePath == '/')
      ;
    else
    {
      std::string details = std::string("Invalid character '") + *servicePath + "' in Service-Path";
      alarmMgr.badInput(clientIp, details, servicePath);

      return "Bad Character in Service-Path";
    }

    ++servicePath;
  }

  return "OK";
}



/* ****************************************************************************
*
* str2double - convert string to double, and return false if no valid double is found
*
* From the manual page of strtod:
*   double strtod(const char *nptr, char **endptr);
*
*   If endptr is not NULL, a pointer to the character after the last character
*   used in the conversion is stored in the location referenced by endptr.
*
*   If no conversion is performed, zero is returned and the value of nptr is stored 
*   in the location referenced by endptr.
*
* Now, this is just perfect. It is just what we need to assure that a string is
* in fact 'expressing' a double.
* All we need to do is to check that endptr only contains whitespace (or nothing at all)
* after the call to 'strtod'. And also, that a covervsion has actually been performed.
* If so, the string sent to isDouble is indeed a valid 'double'.
*
* More from the man page:
*   If the correct value would cause overflow, plus or minus HUGE_VAL (HUGE_VALF, HUGE_VALL)
*   is returned (according to the sign of the value), and ERANGE is stored in errno.
*   If the correct value would cause underflow, zero is returned and ERANGE is stored in errno.
*
* So, we will need to check errno for ERANGE also.
*
* This in turn will make a 'HUGE_VAL double' 12e999999 (is that enough? :-)) be treated as an error.
* It IS a double, but this function will say it is not, as it actually is not a valid double for
* this computer as the computer cannot represent it as the C builtin type 'double' ... OK!
*/
bool str2double(const char* s, double* dP)
{
  char*   rest = NULL;
  double  d;

  errno = 0;
  d = strtod(s, &rest);

  if ((rest == NULL) || (errno == ERANGE))
  {
    return false;
  }

  // If all WS after the last char used in the conversion, then all is OK
  while (isspace(*rest))
  {
    ++rest;
  }

  if (*rest != 0)
  {
    return false;
  }

  if (dP != NULL)
  {
    *dP = d;
  }

  return true;
}



/* ****************************************************************************
*
* double2string
*
*/
std::string double2string(double f)
{
  char  buf[STRING_SIZE_FOR_DOUBLE];
  int bufSize = sizeof(buf);

  long long  intPart   = (long long) f;
  double     diff      = f - intPart;
  bool       isInteger = false;

  // abs value for 'diff'
  diff = (diff < 0)? -diff : diff;

  if (diff > 0.9999999998)
  {
    intPart += 1;
    isInteger = true;
  }
  else if (diff < 0.000000001)  // it is considered an integer
  {
    isInteger = true;
  }

  if (isInteger)
  {
    snprintf(buf, bufSize, "%lld", intPart);
  }
  else
  {
    snprintf(buf, bufSize, "%.9f", f);

    // Clear out any unwanted trailing zeroes
    char* last = &buf[strlen(buf) - 1];

    while ((*last == '0') && (last != buf))
    {
      *last = 0;
      --last;
    }
  }

  return std::string(buf);
}



/*****************************************************************************
*
* isodate2str -
*
*/
std::string isodate2str(double timestamp)
{
  // 80 bytes is enough to store any ISO8601 string safely
  // We use gmtime() to get UTC strings, otherwise we would use localtime()
  // Date pattern: 1970-04-26T17:46:40.000Z

  char    buffer[80];

  //
  // About the added microsecond in  **int millis = (micros + 1) / 1000;**
  //
  // Floating point numbers are often a problem for a CPU, giving funny rounding errors.
  // We've seen that e.g. the number 12.60 is stored as 12.599999999 and as here we cut
  // the number it ends up as 12.59, which is no good ...
  //
  // Two choices here, either call some rounding function, or use a trick.
  // The trick is to add a microsecond (we're only interested in milliseconds) and then cut.
  //
  // Assuming we have nine decimals: 12.599999999, adding a microsecond to this gives 12.600000999:
  //
  //  12.599999999 + 0.000001 == 12.600000999
  //
  // After that we cut it at three decimals and end up with 12.600.
  //
  // If instead the rounding error would go on the upper side, i.e. 12.60 would be 12.6000000001, adding that microsecond
  // doesn't change anything. All still work just fine.
  //
  // Better example: what if we wish to store 12.599?
  // A typical rounding error on the "upper side" would then be 12.599000001, and again, adding that microsecond after
  // cutting doesn't change a thing.
  //
  //
  // So, the "trick algorithm" is as follows:
  //
  // 1. Get the integer part of "double timestamp" - this cuts all decimals, no rounding is done,
  //    and store it in the variable 'seconds'
  // 2. Store the decimals in 'ms', by extracting the integer part ('seconds') from 'timestamp'
  // 3. Convert the decimals into microseconds, by multiplicating with one million (1000000) - in the variable 'micros'.
  // 4. Add a microsecond and then divide by one thousand - cutting all decimals
  //
  time_t  seconds   = (time_t) timestamp;
  double  ms        = timestamp - (double) seconds;
  int     micros    = ms * 1000000;
  int     millis    = (micros + 1) / 1000;   // (timestamp - seconds) * 1000 gives rounding errors ...

  // Unused, but needed to fullfill gmtime_r() signature
  struct tm  tmP;

  strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", gmtime_r(&seconds, &tmP));

  char* eob = &buffer[strlen(buffer)];
  sprintf(eob, ".%03dZ", millis);
  return std::string(buffer);
}



/* ****************************************************************************
*
* toLowercase - convert string to lowercase
*
* This function modifies the actual string.
* See also the function 'strToLower', that converts one string to another, with all uppercase
* changed to lowercase.
*
* This approach is faster as no copy is done.
*/
void toLowercase(char* s)
{
  int toLowerOffset = 'a' - 'A';

  while (*s != 0)
  {
    if ((*s >= 'A') && (*s <= 'Z'))
    {
      *s += toLowerOffset;
    }

    ++s;
  }
}



/* ****************************************************************************
*
* offuscatePassword -
*/
std::string offuscatePassword(const std::string& uri, const std::string& pwd)
{
  if ((pwd.empty()) || (uri.find(pwd) ==  std::string::npos))
  {
    return uri;
  }

  std::string s(uri);  // replace cannot be called in const std::string&
  s.replace(uri.find(pwd), pwd.length(), "******");
  return s;
}



/* ****************************************************************************
*
* regComp -
*
* Wrapper or standard regcomp() from regex.h, including some advanced features
* (e.g. error logging)
*/
bool regComp(regex_t* re, const char* pattern, int flags)
{
  int r = regcomp(re, pattern, flags);
  if (r == 0)
  {
    return true;
  }

  // Log error in corresponding tracelevel (see example at
  // https://www.ibm.com/docs/en/i/7.1?topic=functions-regerror-return-error-message-regular-expression)
  char    buffer[100];
  regerror(r, re, buffer, 100);
  LM_T(LmtRegexError, ("regcomp() failed for pattern '%s': %s", pattern, buffer));

  return false;
}
