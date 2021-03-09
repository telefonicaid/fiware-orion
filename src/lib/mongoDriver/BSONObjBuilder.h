#ifndef SRC_LIB_MONGODRIVER_BSONOBJBUILDER_H_
#define SRC_LIB_MONGODRIVER_BSONOBJBUILDER_H_

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

#include <bson/bson.h>
#include <string>

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/OID.h"
#include "mongoDriver/BSONArray.h"
#include "mongoDriver/BSONDate.h"

namespace orion
{
/* ****************************************************************************
*
* BSONObjBuilder -
*/
class BSONObjBuilder
{
 private:
  bson_t*  b;

 public:
  // methods to be used by client code (without references to low-level driver code)
  BSONObjBuilder();
  void append(const std::string& key, const std::string& value);
  void append(const std::string& key, const char* value);
  void append(const std::string& key, int value);
  void append(const std::string& key, long long value);
  void append(const std::string& key, double value);
  void append(const std::string& key, bool value);
  void append(const std::string& key, const orion::OID& value);
  void append(const std::string& key, const BSONObj& value);
  void append(const std::string& key, const BSONArray& value);
  void appendCode(const std::string& key, const std::string& value);
  void appendRegex(const std::string& key, const std::string& value);
  void appendDate(const std::string& key, const BSONDate& value);
  void appendNull(const std::string& key);
  void appendElements(orion::BSONObj b);
  int nFields(void) const;
  BSONObj obj(void);
  BSONObjBuilder& operator= (const BSONObjBuilder& rhs);

  // methods to be used only by mongoDriver/ code (with references to low-level driver code)
  ~BSONObjBuilder(void);
};
}



#endif  // SRC_LIB_MONGODRIVER_BSONOBJBUILDER_H_
