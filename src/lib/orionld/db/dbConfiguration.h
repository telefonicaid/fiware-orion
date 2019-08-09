#ifndef SRC_LIB_ORIONLD_DB_DBCONFIGURATION_H_
#define SRC_LIB_ORIONLD_DB_DBCONFIGURATION_H_

/*
*
* Copyright 2019 Telefonica Investigacion y Desarrollo, S.A.U
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
* Author: Ken Zangelin
*/

extern "C"
{
#include "kjson/KjNode.h"                                        // KjNode
}



// -----------------------------------------------------------------------------
//
// DB_DRIVER_MONGO_CPP_LEGACY - Use the mongo C++ Legacy driver
//
#define DB_DRIVER_MONGO_CPP_LEGACY 1



// -----------------------------------------------------------------------------
//
// DB_DRIVER_MONGOC - Use the "newest" mongo C driver
//
// #define DB_DRIVER_MONGOC



// -----------------------------------------------------------------------------
//
// Function pointers for the DB interface
//
typedef KjNode* (*DbEntityLookupFunction)(const char* entityId);
typedef bool    (*DbEntityUpdateFunction)(char* entityId, KjNode* requestTree);
typedef KjNode* (*DbDataToKjTreeFunction)(void* dbData, char** titleP, char** detailsP);
typedef void    (*DbDataFromKjTreeFunction)(KjNode* nodeP, void* dbDataP);

extern DbEntityLookupFunction   dbEntityLookup;
extern DbEntityUpdateFunction   dbEntityUpdate;
extern DbDataToKjTreeFunction   dbDataToKjTree;
extern DbDataFromKjTreeFunction dbDataFromKjTree;

#endif  // SRC_LIB_ORIONLD_DB_DBCONFIGURATION_H_
