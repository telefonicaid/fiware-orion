#ifndef SRC_LIB_MONGODRIVER_CONNECTIONOPERATIONS_H_
#define SRC_LIB_MONGODRIVER_CONNECTIONOPERATIONS_H_

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

#include "mongoDriver/DBConnection.h"
#include "mongoDriver/DBCursor.h"
#include "mongoDriver/BSONObj.h"

namespace orion

{
/* ****************************************************************************
*
* orion::collectionQuery -
*/
extern bool collectionQuery
(
  DBConnection        connection,
  const std::string&  col,
  const BSONObj&      q,
  DBCursor*           cursor,
  std::string*        err
);



/* ****************************************************************************
*
* orion::collectionRangedQuery -
*/
extern bool collectionRangedQuery
(
  DBConnection        connection,
  const std::string&  col,
  const BSONObj&      q,
  const BSONObj&      sort,  // FIXME: this change can be propagated independtly to master
  int                 limit,
  int                 offset,
  DBCursor*           cursor,
  long long*          count,
  std::string*        err
);



/* ****************************************************************************
*
* orion::collectionCount -
*/
extern bool collectionCount
(
  const std::string&     col,
  const BSONObj&  q,
  unsigned long long*    c,
  std::string*           err
);



/* ****************************************************************************
*
* orion::collectionFindOne -
*/
extern bool collectionFindOne
(
  const std::string&     col,
  const BSONObj&  q,
  BSONObj*        doc,
  std::string*           err
);



/* ****************************************************************************
*
* orion::collectionInsert -
*/
extern bool collectionInsert
(
  const std::string&     col,
  const BSONObj&  doc,
  std::string*           err
);



/* ****************************************************************************
*
* orion::collectionUpdate -
*/
extern bool collectionUpdate
(
  const std::string&     col,
  const BSONObj&  q,
  const BSONObj&  doc,
  bool                   upsert,
  std::string*           err
);



/* ****************************************************************************
*
* orion::collectionRemove -
*/
extern bool collectionRemove
(
  const std::string&     col,
  const BSONObj&  q,
  std::string*           err
);


#if 0
/* ****************************************************************************
*
* orion::collectionCreateIndex -
*/
extern bool collectionCreateIndex
(
  const std::string&     col,
  const BSONObj&  indexes,
  const bool&            isTTL,
  std::string*           err
);
#endif


/* ****************************************************************************
*
* orion::runCollectionCommand -
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
* orion::runCollectionCommand -
*/
extern bool runCollectionCommand
(
  DBConnection        connection,
  const std::string&  col,
  const BSONObj&      command,
  BSONObj*            result,
  std::string*        err
);


#if 0
/* ****************************************************************************
*
* setWriteConcern -
*/
extern bool setWriteConcern
(
  DBConnection*        connection,
  const mongo::WriteConcern&  wc,
  std::string*                err
);



/* ****************************************************************************
*
* getWriteConcern -
*/
extern bool getWriteConcern
(
  DBConnection*  connection,
  mongo::WriteConcern*  wc,
  std::string*          err
);



/* ****************************************************************************
*
* connectionAuth -
*/
extern bool connectionAuth
(
  DBConnection*  connection,
  const std::string&    db,
  const std::string&    user,
  const std::string&    password,
  const std::string&    mechanism,
  std::string*          err
);
#endif
}

#endif  // SRC_LIB_MONGODRIVER_CONNECTIONOPERATIONS_H_
