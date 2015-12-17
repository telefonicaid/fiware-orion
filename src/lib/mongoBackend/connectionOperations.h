#ifndef SRC_LIB_MONGOBACKEND_CONNECTIONOPERATIONS_H_
#define SRC_LIB_MONGOBACKEND_CONNECTIONOPERATIONS_H_

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

#include "mongo/client/dbclient.h"

using namespace mongo;
using std::auto_ptr;

/* ****************************************************************************
*
* collectionQuery -
*
*/
extern bool collectionQuery
(
  DBClientBase*                   connection,
  const std::string&              col,
  const BSONObj&                  q,
  std::auto_ptr<DBClientCursor>*  cursor,
  std::string*                    err
);

/* ****************************************************************************
*
* collectionRangedQuery -
*
*/
extern bool collectionRangedQuery
(
  DBClientBase*                   connection,
  const std::string&              col,
  const Query&                    q,
  int                             limit,
  int                             offset,
  std::auto_ptr<DBClientCursor>*  cursor,
  long long*                      count,
  std::string*                    err
);

/* ****************************************************************************
*
* collectionCount -
*
*/
extern bool collectionCount
(
  const std::string&   col,
  const BSONObj&       q,
  unsigned long long*  c,
  std::string*         err
);

/* ****************************************************************************
*
* collectionFindOne -
*
*/
extern bool collectionFindOne
(
  const std::string&  col,
  const BSONObj&      q,
  BSONObj*            doc,
  std::string*        err
);

/* ****************************************************************************
*
* collectionInsert -
*
*/
extern bool collectionInsert
(
  const std::string&  col,
  const BSONObj&      doc,
  std::string*        err
);

/* ****************************************************************************
*
* collectionUpdate -
*
*/
extern bool collectionUpdate
(
  const std::string&  col,
  const BSONObj&      q,
  const BSONObj&      doc,
  bool                upsert,
  std::string*        err
);

/* ****************************************************************************
*
* collectionRemove -
*
*/
extern bool collectionRemove
(
  const std::string&  col,
  const BSONObj&      q,
  std::string*        err
);

/* ****************************************************************************
*
* collectionCreateIndex -
*
*/
extern bool collectionCreateIndex
(
  const std::string&  col,
  const BSONObj&      indexes,
  std::string*        err
);

/* ****************************************************************************
*
* runCollectionCommand -
*
*/
extern bool runCollectionCommand
(
  const std::string&  col,
  const BSONObj&      command,
  BSONObj*            result,
  std::string*        err
);

/* ****************************************************************************
*
* runCollectionCommand -
*
*/
extern bool runCollectionCommand
(
  DBClientBase*       connection,
  const std::string&  col,
  const BSONObj&      command,
  BSONObj*            result,
  std::string*        err
);

/* ****************************************************************************
*
* setWriteConcern -
*
*/
extern bool setWriteConcern
(
  DBClientBase*        connection,
  const WriteConcern&  wc,
  std::string*         err
);

/* ****************************************************************************
*
* getWriteConcern -
*
*/
extern bool getWriteConcern
(
  DBClientBase*  connection,
  WriteConcern*  wc,
  std::string*   err
);

/* ****************************************************************************
*
* connectionAuth -
*
*/
extern bool connectionAuth
(
  DBClientBase*       connection,
  const std::string&  db,
  const std::string&  user,
  const std::string&  password,
  std::string*        err
);

#endif // SRC_LIB_MONGOBACKEND_CONNECTIONOPERATIONS_H_
