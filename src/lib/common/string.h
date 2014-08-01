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
* fermin at tid dot es
*
* Author: Ken Zangelin
*/
#include <string>
#include <vector>



/* ****************************************************************************
*
* isIPv6 -
*/
extern bool isIPv6(const std::string& in);



/* ****************************************************************************
*
* stringSplit - 
*/
extern int stringSplit(const std::string& in, char delimiter, std::vector<std::string>& outV);



/* ****************************************************************************
*
* parseUrl -
*
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
* coords2string - 
*/
extern void coords2string
(
  std::string*  s,
  double        latitude,
  double        longitude,
  int           decimals = 6
);



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

#endif  // SRC_LIB_COMMON_STRING_H_
