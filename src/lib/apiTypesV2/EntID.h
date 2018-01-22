#ifndef SRC_LIB_APITYPESV2_ENTID_H_
#define SRC_LIB_APITYPESV2_ENTID_H_

/*
*
* Copyright 2017 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Fermín Galán Márquez
*/
#include <string>


namespace ngsiv2
{
/* ****************************************************************************
*
* EntID -
*/
struct EntID
{
  std::string id;
  std::string idPattern;
  std::string type;
  std::string typePattern;
  std::string toJson();

  EntID(const std::string& idA, const std::string& idPatternA,
        const std::string& typeA, const std::string& typePatternA):
    id(idA),
    idPattern(idPatternA),
    type(typeA),
    typePattern(typePatternA)
  {}

  EntID()  {}

  bool operator==(const EntID& e)
  {
    return (id          == e.id)        &&
           (idPattern   == e.idPattern) &&
           (type        == e.type)      &&
           (typePattern == e.typePattern);
  }

  bool operator!=(const EntID& e)
  {
    return (id          != e.id)        ||
           (idPattern   != e.idPattern) ||
           (type        != e.type)      ||
           (typePattern != e.typePattern);
  }
};

}  // end namespace

#endif  // SRC_LIB_APITYPESV2_ENTID_H_
