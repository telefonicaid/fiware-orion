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



/* ****************************************************************************
*
* dbDotEncode -
*/
inline std::string dbDotEncode(const std::string& _s)
{
  std::string s(_s);   // replace cannot be used in const std::string&
  std::replace(s.begin(), s.end(), ESCAPE_1_DECODED, ESCAPE_1_ENCODED);

  return s;
}



/* ****************************************************************************
*
* dbDotDecode -
*/
inline std::string dbDotDecode(const std::string& _s)
{
  std::string s(_s);   // replace cannot be used in const std::string&

  std::replace(s.begin(), s.end(), ESCAPE_1_ENCODED, ESCAPE_1_DECODED);

  return s;
}

#endif  // SRC_LIB_MONGOBACKEND_DBFIELDENCODING_H_
