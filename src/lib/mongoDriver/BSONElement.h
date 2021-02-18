#ifndef SRC_LIB_MONGODRIVER_BSONELEMENT_H_
#define SRC_LIB_MONGODRIVER_BSONELEMENT_H_

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
#include <vector>

#include "mongoDriver/BSONObj.h"
#include "mongoDriver/BSONDate.h"
#include "mongoDriver/BSONTypes.h"

namespace orion
{
// Forward declaration
class BSONObj;

/* ****************************************************************************
*
* BSONElement -
*/
class BSONElement
{
 private:
  std::string   field;
  bson_value_t  bv;

 public:
  // methods to be used by client code (without references to low-level driver code)
  BSONElement();
  BSONType type(void) const;
  bool isNull(void) const;
  std::string OID(void) const;
  std::string String(void) const;
  bool Bool(void) const;
  double Number(void) const;
  std::vector<BSONElement> Array(void) const;
  BSONObj embeddedObject(void) const;
  BSONDate date(void) const;
  std::string fieldName(void) const;
  bool eoo(void) const;

  // methods to be used only by mongoDriver/ code (with references to low-level driver code)
  BSONElement(const std::string& _field, const bson_value_t* _bv);
};
}

#endif  // SRC_LIB_MONGODRIVER_BSONELEMENT_H_
