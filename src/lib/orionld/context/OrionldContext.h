#ifndef SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXT_H_
#define SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXT_H_

/*
*
* Copyright 2019 FIWARE Foundation e.V.
*
* This file is part of Orion-LD Context Broker.
*
* Orion-LD Context Broker is free software: you can redistribute it and/or
* modify it under the terms of the GNU Affero General Public License as
* published by the Free Software Foundation, either version 3 of the
* License, or (at your option) any later version.
*
* Orion-LD Context Broker is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
* General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with Orion-LD Context Broker. If not, see http://www.gnu.org/licenses/.
*
* For those usages not covered by this license please contact with
* orionld at fiware dot org
*
* Author: Ken Zangelin
*/
extern "C"
{
#include "khash/khash.h"                           // KHashTable
#include "kjson/KjNode.h"                          // KjNode
}


// -----------------------------------------------------------------------------
//
// OrionldContextHashTables -
//
typedef struct OrionldContextHashTables
{
  KHashTable*  nameHashTable;
  KHashTable*  valueHashTable;
} OrionldContextHashTables;



struct OrionldContext;
typedef struct OrionldContextArray
{
  int                     items;
  struct OrionldContext** vector;
} OrionldContextArray;



// -----------------------------------------------------------------------------
//
// OrionldContextValue
//
typedef union OrionldContextInfo
{
  OrionldContextHashTables  hash;
  OrionldContextArray       array;
} OrionldContextInfo;



// -----------------------------------------------------------------------------
//
// OrionldContextOrigin -
//
typedef enum OrionldContextOrigin
{
  OrionldContextUnknownOrigin,
  OrionldContextFromInline,
  OrionldContextDownloaded,
  OrionldContextFileCached,
  OrionldContextForNotifications,
  OrionldContextForForwarding,
  OrionldContextUserCreated
} OrionldContextOrigin;



// ----------------------------------------------------------------------------
//
// OrionldContext -
//
// The context is either an array of contexts or "the real thing" - a list of key-values in
// a hash-list
//
typedef struct OrionldContext
{
  char*                 url;
  char*                 id;         // For contexts that were created by the broker itself
  KjNode*               tree;
  bool                  keyValues;
  OrionldContextInfo    context;
  OrionldContextOrigin  origin;
} OrionldContext;

#endif  // SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXT_H_
