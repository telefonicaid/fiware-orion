#ifndef SRC_LIB_COMMON_STRING_H_
#define SRC_LIB_COMMON_STRING_H_

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
#include <sstream>
#include <iomanip>
#include <vector>

#include "common/limits.h"

// the same macro in parseArg library
#define FT(x) (x == true)? "true" : "false"

/* ****************************************************************************
*
* isIPv6 -
*/
extern bool isIPv6(const std::string& in);



/* ****************************************************************************
*
* stringSplit - 
*/
extern int stringSplit(const std::string& in, char delimiter, std::vector<std::string>& outV, bool unique = false);



/* ****************************************************************************
*
* parseUrl -
*/
extern bool parseUrl
(
  const std::string&  url,
  std::string&        host,
  int&                port,
  std::string&        path,
  std::string&        protocol
);



/* ****************************************************************************
*
* validUrl - check validity of a URL
*/
extern bool validUrl(const std::string& url);



/* ****************************************************************************
*
* i2s - integer to string
*/
extern char* i2s(int i, char* placeholder, int placeholderSize);



/* ****************************************************************************
*
* parsedUptime
*/
extern std::string parsedUptime(int uptime);



/* ****************************************************************************
*
* onlyWs - 
*/
extern bool onlyWs(const char* s);



/* ****************************************************************************
*
* string2coords - 
*/
extern bool string2coords(const std::string& s, double& latitude, double& longitude);



/* ****************************************************************************
*
* versionParse -
*/
bool versionParse
(
  const std::string&  version,
  int&                mayor,
  int&                minor,
  std::string&        extra
);



/* ****************************************************************************
*
* atoF - 
*/
extern double atoF(const char* string, std::string* errorMsg);




/* ****************************************************************************
*
* atoUL -
*/
extern unsigned long atoUL(const char* string, std::string* errorMsg);



/* ****************************************************************************
*
* strToLower - 
*/
extern char* strToLower(char* to, const char* from, int toSize);



/* ****************************************************************************
*
* strReplace - 
*/
extern void strReplace
(
  char*       to,
  int         toLen,
  const char* from,
  const char* newString,
  const char* oldString
);



/* ****************************************************************************
*
* servicePathCheck - 
*/
extern std::string servicePathCheck(const char* servicePath);



/* ****************************************************************************
*
* str2double - is the string a correct float (double)?
*/
extern bool str2double(const char* s, double* dP = NULL);


/*****************************************************************************
*
* double2string -
*
*/
extern std::string double2string(double f);



/*****************************************************************************
*
* isodate2str -
*
*/
extern std::string isodate2str(double timestamp);



/* ****************************************************************************
*
* toLowercase - convert string to lowercase
*/
extern void toLowercase(char* s);



/* ****************************************************************************
*
* offuscatePassword -
*/
extern std::string offuscatePassword(const std::string& uri, const std::string& pwd);

#endif  // SRC_LIB_COMMON_STRING_H_
