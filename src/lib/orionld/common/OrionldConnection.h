#ifndef SRC_LIB_ORIONLD_COMMON_ORIONLDCONNECTION_H_
#define SRC_LIB_ORIONLD_COMMON_ORIONLDCONNECTION_H_

/*
*
* Copyright 2018 Telefonica Investigacion y Desarrollo, S.A.U
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
#include "kjson/kjson.h"                                       // Kjson
}
#include "common/globals.h"                                    // ApiVersion
#include "orionld/context/OrionldContext.h"                    // OrionldContext



// -----------------------------------------------------------------------------
//
// OrionldConnectionState - the state of the connection
//
// This struct contains all the state of a connection, like the Kjson pointer, the pointer to
// the RestService of the request or the urlPath of the request or ...
// Basically EVERYTHING that is a 'characteristics' for the connection.
// These fields/variables will be set once, initially, when the request arrived and after that will only be read.
// It makes very little sense to send these variables to each and every function where they are to be used.
// Much easier and faster to simply store them in a thread global struct.
//
typedef struct OrionldConnectionState
{
  Kjson*           kjsonP;
  OrionldContext*  contextP;
  ApiVersion       apiVersion;
} OrionldConnection;



// -----------------------------------------------------------------------------
//
// orionldState - 
//
extern __thread OrionldConnectionState orionldState;

#endif  // SRC_LIB_ORIONLD_COMMON_ORIONLDCONNECTION_H_
