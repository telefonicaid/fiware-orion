#ifndef SRC_LIB_MONGODRIVER_BSONDATE_H_
#define SRC_LIB_MONGODRIVER_BSONDATE_H_

/*
*
* Copyright 2020 Telefonica Investigacion y Desarrollo, S.A.U
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

#include <stdint.h>  // CentOS 7 needs this explicit declaration (and doesn't hurts in other OS)
#include <vector>
#include <string>

namespace orion
{
/* ****************************************************************************
*
* BSONDate -
*/
class BSONDate
{
 private:
  unsigned long long date;

 public:
  // methods to be used by client code (without references to low-level driver code)
  explicit BSONDate(unsigned long long m);
  bool equal(unsigned long long m);

  // methods to be used only by mongoDriver/ code (with references to low-level driver code)
  int64_t get(void) const;
};
}

#endif  // SRC_LIB_MONGODRIVER_BSONDATE_H_
