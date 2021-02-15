#ifndef SRC_LIB_MONGODRIVER_BSONOBJ_H_
#define SRC_LIB_MONGODRIVER_BSONOBJ_H_

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
#include <map>
#include <set>

#include "mongoDriver/BSONElement.h"

namespace orion
{
// Forward declaration
class BSONElement;

// FIXME OLD-DR (this applies to several classes but I'm including only here)
// According to http://mongoc.org/libbson/current/bson_t.html#performance-notes
// we could use bson_t instead of bson_t* to take advantage of stack for
// small BSON objects. However, not sure how bson_destroy() has teo be used
// in that case. Create an issue about this

/* ****************************************************************************
*
* BSONObj -
*/
class BSONObj
{
 private:
  bson_t*  b;

 public:
  // methods to be used by client code (without references to low-level driver code)
  BSONObj();
  BSONObj(const BSONObj& _bo);
  int getFieldNames(std::set<std::string>* fields) const;
  bool hasField(const std::string& field) const;
  int nFields(void) const;
  std::string toString(void) const;
  bool isEmpty(void);
  void toStringMap(std::map<std::string, std::string>* m);
  void toElementsVector(std::vector<BSONElement>* v);
  BSONObj& operator= (BSONObj rhs);

  // methods to be used only by mongoDriver/ code (with references to low-level driver code)
  ~BSONObj(void);
  bson_t* get(void) const;
  explicit BSONObj(const bson_t* _b);
};
}

#endif  // SRC_LIB_MONGODRIVER_BSONOBJ_H_
