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

/* ****************************************************************************
*
* basePart, idPart -
*
* Helper functions for entitysQuery to split the attribute name string into part,
* e.g. "A1__ID1" into "A1" and "ID1"
*/
inline std::string basePart(std::string name)
{
  /* Search for "__" */
  std::size_t pos = name.find("__");
  if (pos == std::string::npos)
  {
    /* If not found, return just 'name' */
    return name;
  }

  /* If found, return substring */
  return name.substr(0, pos);

}

inline std::string idPart(std::string name)
{
  /* Search for "__" */
  std::size_t pos = name.find("__");
  if (pos == std::string::npos)
  {
    /* If not found, return just "" */
    return "";
  }

  /* If found, return substring */
  return name.substr(pos + 2, name.length());

}

/* ****************************************************************************
*
* dbDotEncode -
*
*/
extern std::string dbDotEncode(std::string fromString);

/* ****************************************************************************
*
* dbDotDecode -
*
*/
extern std::string dbDotDecode(std::string fromString);

#endif // SRC_LIB_MONGOBACKEND_DBFIELDENCODING_H_
