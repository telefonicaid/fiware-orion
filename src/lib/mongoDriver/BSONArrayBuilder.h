#ifndef SRC_LIB_MONGODRIVER_BSONARRAYBUILDER_H_
#define SRC_LIB_MONGODRIVER_BSONARRAYBUILDER_H_

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

#include <string>

#include "common/limits.h"

#include "mongoDriver/BSONArray.h"
#include "mongoDriver/BSONObj.h"

namespace orion
{
/* ****************************************************************************
*
* BSONObjBuilder -
*/
class BSONArrayBuilder
{
 private:
  bson_t*      b;
  uint32_t     size;                      // current number of elements in array builder (starts at 0)
  char         buf[STRING_SIZE_FOR_INT];  // for internal use in append methods
  const char*  key;                       // for internal use in append methods
 public:
  // methods to be used by client code (without references to low-level driver code)
  BSONArrayBuilder();
  BSONArray arr(void);
  int arrSize(void) const;
  void append(const BSONObj& value);
  void append(const BSONArray& value);
  void append(const std::string& value);
  void append(const char* value);
  void append(double value);
  void append(bool value);
  void appendNull(void);
  void appendRegex(const std::string& value);
  BSONArrayBuilder& operator= (const BSONArrayBuilder& rhs);

  // methods to be used only by mongoDriver/ code (with references to low-level driver code)
  ~BSONArrayBuilder(void);
};
}



#endif  // SRC_LIB_MONGODRIVER_BSONARRAYBUILDER_H_
