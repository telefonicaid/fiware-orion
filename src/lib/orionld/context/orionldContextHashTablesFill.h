#ifndef SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTHASHTABLESFILL_H_
#define SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTHASHTABLESFILL_H_

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

#include "orionld/common/OrionldProblemDetails.h"                // OrionldProblemDetails, orionldProblemDetailsFill
#include "orionld/context/OrionldContext.h"                      // OrionldContext



// -----------------------------------------------------------------------------
//
// orionldContextHashTablesFill -
//
// Fill both the name hashtable and the value hashtable.
//
// The values in the key-value list can be either a string or an object
//
// To support prefix expansion, eventual prefixes in the values must be expanded.
// If not expanded, it's too slow to find longnames in the process of finding aliases.
//
// For example, we have a context with the following items:
//
// @context: {
//   "prefix": "http://a.b.c/mySet/",
//   "P1": "prefix:P1"
// }
//
// Now, an attribute named 'P1' will get the longname "http://a.b.c/mySet/P1".
//
// In a GET process, that's what we get from the database (http://a.b.c/mySet/P1), and we now want to look in the current @context to
// see if we can find a shortname (i.e. a key-value whose value is "http://a.b.c/mySet/P1").
//
// If "P1" == "prefix:P1", we won't find "http://a.b.c/mySet/P1" ...
//
// It could be found, bt parting "prefix:P1" and looking up "prefix", concatenate "http://a.b.c/mySet/" and "P1" and get to
// "http://a.b.c/mySet/P1". However, this would be really really slow.
//
// Instead, we expand the values of the context key-values (if the value uses a prefix).
// The problem is that when we treat the first item in a context, that context isn't created yet.
// So, we will need to do this in two passes:
//
// 1. Create the context without expanding prefixes - create only the name-table
// 2. Go over the context again and expand prefixes - create the value-table
//
// The value-table MUST be created in the second pass as its key is the value, and as the value changes in pass II, no other choice.
// Remember that the key (in this case the value is used as key) is what decides the slot in the hash array.
//
extern bool orionldContextHashTablesFill(OrionldContext* contextP, KjNode* keyValueTree, OrionldProblemDetails* pdP);

#endif  // SRC_LIB_ORIONLD_CONTEXT_ORIONLDCONTEXTHASHTABLESFILL_H_
