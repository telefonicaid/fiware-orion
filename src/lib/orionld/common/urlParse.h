#ifndef SRC_LIB_ORIONLD_COMMON_URLPARSE_H_
#define SRC_LIB_ORIONLD_COMMON_URLPARSE_H_

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



// ----------------------------------------------------------------------------
//
// urlParse - move to common/urlParse.cpp
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
extern bool urlParse(char* url, char* protocol, int protocolSize, char* ip, int ipSize, short* portP, char** urlPathPP, char** detailsPP);

#endif  // SRC_LIB_ORIONLD_COMMON_URLPARSE_H_
