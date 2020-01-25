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
#include "kjson/KjNode.h"                                        // KjNode
}

#include "orionld/db/dbConfiguration.h"                          // Own interface



// -----------------------------------------------------------------------------
//
// Function pointers for the DB interface
//
DbEntityLookupFunction                    dbEntityLookup;
DbEntityLookupManyFunction                dbEntityLookupMany;
DbEntityUpdateFunction                    dbEntityUpdate;
DbEntityBatchDeleteFunction               dbEntityBatchDelete;
DbDataToKjTreeFunction                    dbDataToKjTree;
DbDataFromKjTreeFunction                  dbDataFromKjTree;
DbSubscriptionMatchEntityIdAndAttributes  dbSubscriptionMatchEntityIdAndAttributes;
DbEntityListLookupWithIdTypeCreDate       dbEntityListLookupWithIdTypeCreDate;  // FIXME: Name must change - what does it to really?
DbRegistrationLookup                      dbRegistrationLookup;

//
// FIXME
//   About DbQueryEntitiesAsKjTree:
//   First of all, a KjNode-tree is used as neutral format in all DB functions that need it.
//   So, KjTree should never be part of the name - not necessary
//
//   Then, we already have a function called dbEntityLookup, and another one called dbEntityLookupMany.
//   dbQueryEntitiesAsKjTree ... with "correct" nomenclatura would be called "dbEntitesLookup", which is the same as dbEntityLookupMany.
//
//   So, what does this function do, that dbEntityLookupMany does not?
//   The answer to that question should answer the question about "what is a good name for this function".
//
//   And, the name for the function should be like this:  dbEntityLookupXxxFunction
//   Xxx describing the particularies with the function.
//   We might have to rename dbEntityLookupMany - which would not be a problem.
//
//   A good name for dbQueryEntitiesAsKjTree would be dbEntityListLookupWithIdTypeCreDate
//
