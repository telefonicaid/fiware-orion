#ifndef SRC_LIB_MONGOBACKEND_DBFIELDENCODING_H_
#define SRC_LIB_MONGOBACKEND_DBFIELDENCODING_H_

/*
*
* Copyright 2015 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán
*/
#include <string>

#include "common/globals.h"



/* ****************************************************************************
*
* ESC decoded/encoded
*/
#define ESCAPE_1_DECODED  '.'
#define ESCAPE_1_ENCODED  '='


#define ESCAPE_2_DECODED  '$'
#define ESCAPE_2_ENCODED  '>'

// From https://docs.mongodb.com/manual/reference/limits/#mongodb-limit-Restrictions-on-Field-Names
//
// "Until support is added in the query language, the use of $ and . in field names is not recommended
// and is not supported by the official MongoDB drivers.
//
// Note that both = and > are forbidden chars in CB (see https://fiware-orion.readthedocs.io/en/master/user/forbidden_characters/index.html)
// so they are safe, as they cannot appear in any part of the CB API


/* ****************************************************************************
*
* dbEncode -
*/
inline std::string dbEncode(const std::string& _s)
{
  std::string s(_s);   // replace cannot be used in const std::string&

  std::replace(s.begin(), s.end(), ESCAPE_1_DECODED, ESCAPE_1_ENCODED);
  std::replace(s.begin(), s.end(), ESCAPE_2_DECODED, ESCAPE_2_ENCODED);

  return s;
}



/* ****************************************************************************
*
* dbDecode -
*/
inline std::string dbDecode(const std::string& _s)
{
  std::string s(_s);   // replace cannot be used in const std::string&

  std::replace(s.begin(), s.end(), ESCAPE_1_ENCODED, ESCAPE_1_DECODED);
  std::replace(s.begin(), s.end(), ESCAPE_2_ENCODED, ESCAPE_2_DECODED);

  return s;
}

#endif  // SRC_LIB_MONGOBACKEND_DBFIELDENCODING_H_
