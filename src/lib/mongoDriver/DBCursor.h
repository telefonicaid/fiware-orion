#ifndef SRC_LIB_MONGODRIVER_DBCURSOR_H_
#define SRC_LIB_MONGODRIVER_DBCURSOR_H_

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

#include <mongoc/mongoc.h>
#include <string>

#include "mongoDriver/BSONObj.h"

#define ON_NEXT_NO_ERROR         0
#define ON_NEXT_MANAGED_ERROR    1
#define ON_NEXT_UNMANAGED_ERROR  2

namespace orion
{
/* ****************************************************************************
*
* DBCursor -
*/
class DBCursor
{
 private:
  mongoc_cursor_t* c;

 public:
  // methods to be used by client code (without references to low-level driver code)
  DBCursor();
  bool more(void);
  bool next(BSONObj* nextDoc, int* errTypeP = NULL, std::string* err = NULL);
  bool isNull(void);

  // methods to be used only by mongoDriver/ code (with references to low-level driver code)
  void set(mongoc_cursor_t* _c);
  ~DBCursor();
};
}

#endif  // SRC_LIB_MONGODRIVER_DBCURSOR_H_
